#pragma once

#include "http/context.hpp"
#include "http/request.hpp"
#include "http/response.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <functional>

namespace getgresql::http {

namespace asio = boost::asio;
namespace beast = boost::beast;

// ─── Handler function type ──────────────────────────────────────────

using DispatchFn = std::function<Response(Request&, AppContext&)>;

// ─── HTTP Server ────────────────────────────────────────────────────

class Server {
    asio::io_context ioc_;
    asio::ip::tcp::acceptor acceptor_;
    AppContext ctx_;
    DispatchFn dispatch_;
    unsigned threads_;

public:
    Server(AppContext ctx, DispatchFn dispatch, unsigned threads = 4);

    void run();
    void stop();

private:
    void do_accept();
    void handle_session(asio::ip::tcp::socket socket);
};

} // namespace getgresql::http
