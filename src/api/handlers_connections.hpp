#pragma once

#include "config/connections.hpp"
#include "http/router.hpp"
#include "ssr/engine.hpp"

#include <vector>

namespace getgresql::api {

using namespace http;

// GET /connections — connection manager page
struct ConnectionsPageHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
    // Shared renderer for the connection list (used by save/delete handlers)
    static auto render_connection_list(ssr::Html& h,
        const std::vector<config::SavedConnection>& conns,
        std::string_view current_connstr) -> void;
};

// POST /connections/save — save a new connection
struct ConnectionSaveHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /connections/delete — delete a saved connection
struct ConnectionDeleteHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /connections/switch — switch to a different connection
struct ConnectionSwitchHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /connections/test — test a connection string without switching
struct ConnectionTestHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /api/connection-info — JSON with current DB name (for toolbar)
struct ConnectionInfoHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

} // namespace getgresql::api
