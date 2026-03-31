#include "http/server.hpp"

#include <boost/asio/strand.hpp>
#include <iostream>
#include <print>
#include <thread>
#include <vector>

namespace getgresql::http {

Server::Server(AppContext ctx, DispatchFn dispatch, unsigned threads)
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

void Server::run() {
    std::println("getgreSQL listening on http://{}:{}",
                 ctx_.config.bind_address, ctx_.config.port);

    do_accept();

    // Run the I/O context on multiple threads
    std::vector<std::jthread> workers;
    workers.reserve(threads_ - 1);
    for (unsigned i = 0; i < threads_ - 1; ++i) {
        workers.emplace_back([this] { ioc_.run(); });
    }
    ioc_.run();  // main thread also runs
}

void Server::stop() {
    ioc_.stop();
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](beast::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                // Handle in a detached strand to allow concurrency
                std::jthread([this, s = std::move(socket)]() mutable {
                    handle_session(std::move(s));
                }).detach();
            }
            do_accept();  // accept next connection
        }
    );
}

void Server::handle_session(asio::ip::tcp::socket socket) {
    beast::flat_buffer buffer;
    beast::error_code ec;

    for (;;) {
        // Read a request
        beast::http::request<beast::http::string_body> req;
        beast::http::read(socket, buffer, req, ec);

        if (ec == beast::http::error::end_of_stream) break;
        if (ec) {
            std::println(stderr, "read error: {}", ec.message());
            return;
        }

        // Wrap and dispatch
        Request wrapped_req(std::move(req));
        auto response = dispatch_(wrapped_req, ctx_);

        // Apply connection info
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

} // namespace getgresql::http
