#pragma once

#include "http/router.hpp"

namespace getgresql::api {

using namespace http;

// GET /monitor — full monitoring dashboard
struct MonitorPageHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /monitor/stats — htmx partial for auto-refreshing stats
struct MonitorStatsHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /monitor/activity — htmx partial for active queries
struct MonitorActivityHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /monitor/locks — lock viewer page
struct MonitorLocksHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /monitor/cancel/{pid} — cancel a running query
struct CancelQueryHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

} // namespace getgresql::api
