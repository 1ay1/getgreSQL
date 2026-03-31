#pragma once

#include "api/handlers_db.hpp"
#include "api/handlers_monitor.hpp"
#include "api/handlers_query.hpp"
#include "http/router.hpp"

namespace getgresql::api {

using namespace http;
using enum Method;

// ─── Static asset handler ───────────────────────────────────────────

struct StaticHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// ─── The compile-time route table ───────────────────────────────────
// Every route in the application is defined here as a type.
// Adding a route = adding a line. The compiler enforces that
// every handler satisfies the HandlerType concept.

using AppRoutes = RouteTable<
    // Dashboard
    Route<GET, "/",                                              IndexHandler>,

    // Database browsing
    Route<GET, "/databases",                                     DatabaseListHandler>,
    Route<GET, "/db/{db}/schemas",                               SchemaListHandler>,
    Route<GET, "/db/{db}/schema/{schema}/tables",                TableListHandler>,
    Route<GET, "/db/{db}/schema/{schema}/table/{table}",         TableDetailHandler>,
    Route<GET, "/db/{db}/schema/{schema}/table/{table}/data",    TableDataHandler>,

    // Query editor
    Route<GET,  "/query",                                        QueryPageHandler>,
    Route<POST, "/query/exec",                                   QueryExecHandler>,

    // Monitoring
    Route<GET,  "/monitor",                                      MonitorPageHandler>,
    Route<GET,  "/monitor/stats",                                MonitorStatsHandler>,
    Route<GET,  "/monitor/activity",                             MonitorActivityHandler>,
    Route<GET,  "/monitor/locks",                                MonitorLocksHandler>,
    Route<POST, "/monitor/cancel/{pid}",                         CancelQueryHandler>,

    // Static assets (catch-all)
    Route<GET, "/assets/{path...}",                              StaticHandler>
>;

} // namespace getgresql::api
