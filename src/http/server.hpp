#pragma once

#include "http/context.hpp"
#include "http/request.hpp"
#include "http/response.hpp"

#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast.hpp>
#include <concepts>
#include <iostream>
#include <print>
#include <thread>
#include <vector>

namespace getgresql::http {

namespace asio = boost::asio;
namespace beast = boost::beast;

// ─── Dispatch concept ───────────────────────────────────────────────
// The dispatch function must accept (Request&, AppContext&) → Response.
// Templating the server on this eliminates std::function type-erasure.

template <typename F>
concept Dispatchable = requires(F f, Request& req, AppContext& ctx) {
    { f(req, ctx) } -> std::same_as<Response>;
};

// ─── HTTP Server ────────────────────────────────────────────────────
// Templated on the dispatch callable — zero-overhead, no type erasure.
// The compile-time route table flows through without indirection.

template <Dispatchable DispatchFn>
class Server {
    asio::io_context ioc_;
    asio::ip::tcp::acceptor acceptor_;
    AppContext ctx_;
    DispatchFn dispatch_;
    unsigned threads_;

public:
    Server(AppContext ctx, DispatchFn dispatch, unsigned threads = 4)
        : ioc_(static_cast<int>(threads))
        , acceptor_(ioc_)
        , ctx_(ctx)
        , dispatch_(std::move(dispatch))
        , threads_(threads)
    {
        auto const address = asio::ip::make_address(ctx_.config.bind_address);
        auto const port = ctx_.config.port;

        asio::ip::tcp::endpoint endpoint{address, port};
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(asio::socket_base::max_listen_connections);
    }

    void run() {
        std::println("getgreSQL listening on http://{}:{}",
                     ctx_.config.bind_address, ctx_.config.port);

        do_accept();

        std::vector<std::jthread> workers;
        workers.reserve(threads_ - 1);
        for (unsigned i = 0; i < threads_ - 1; ++i) {
            workers.emplace_back([this] { ioc_.run(); });
        }
        ioc_.run();
    }

    void stop() { ioc_.stop(); }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](beast::error_code ec, asio::ip::tcp::socket socket) {
                if (!ec) {
                    std::jthread([this, s = std::move(socket)]() mutable {
                        handle_session(std::move(s));
                    }).detach();
                }
                do_accept();
            }
        );
    }

    void handle_session(asio::ip::tcp::socket socket) {
        beast::flat_buffer buffer;
        beast::error_code ec;

        for (;;) {
            beast::http::request<beast::http::string_body> req;
            beast::http::read(socket, buffer, req, ec);

            if (ec == beast::http::error::end_of_stream) break;
            if (ec) {
                std::println(stderr, "read error: {}", ec.message());
                return;
            }

            Request wrapped_req(std::move(req));
            auto response = dispatch_(wrapped_req, ctx_);

            auto beast_response = response
                .keep_alive(wrapped_req.keep_alive())
                .version(wrapped_req.version())
                .release();

            beast::http::write(socket, beast_response, ec);

            if (ec) {
                std::println(stderr, "write error: {}", ec.message());
                return;
            }

            if (!beast_response.keep_alive()) break;
        }

        socket.shutdown(asio::ip::tcp::socket::shutdown_send, ec);
    }
};

} // namespace getgresql::http
