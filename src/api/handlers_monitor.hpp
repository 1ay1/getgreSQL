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

// GET /monitor/slow — slow queries
struct SlowQueriesHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /monitor/blocking — blocking chains
struct BlockingHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /monitor/wal — WAL statistics
struct WALStatsHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /monitor/vacuum-progress — vacuum progress
struct VacuumProgressHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /monitor/health — health checks
struct HealthCheckHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /monitor/databases — per-database detailed stats
struct DatabaseStatsHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /monitor/bloat — table bloat detection
struct BloatHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

} // namespace getgresql::api
