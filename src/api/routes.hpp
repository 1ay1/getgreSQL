#pragma once

#include "api/handlers_admin.hpp"
#include "api/handlers_db.hpp"
#include "api/handlers_monitor.hpp"
#include "api/handlers_query.hpp"
#include "api/handlers_tree.hpp"
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

    // Database browsing (extended)
    Route<GET, "/db/{db}/schema/{schema}/functions",                FunctionListHandler>,
    Route<GET, "/db/{db}/schema/{schema}/function/{func}",          FunctionDetailHandler>,
    Route<GET, "/db/{db}/schema/{schema}/sequences",                SequenceListHandler>,
    Route<GET, "/db/{db}/schema/{schema}/indexes",                  IndexAnalysisHandler>,
    Route<GET, "/db/{db}/size",                                     DatabaseSizeHandler>,

    // Admin
    Route<GET, "/roles",                                            RolesHandler>,
    Route<GET, "/extensions",                                       ExtensionsHandler>,
    Route<GET, "/settings",                                         SettingsHandler>,
    Route<GET, "/settings/search",                                  SettingsSearchHandler>,

    // Monitoring
    Route<GET,  "/monitor",                                      MonitorPageHandler>,
    Route<GET,  "/monitor/stats",                                MonitorStatsHandler>,
    Route<GET,  "/monitor/activity",                             MonitorActivityHandler>,
    Route<GET,  "/monitor/locks",                                MonitorLocksHandler>,
    Route<POST, "/monitor/cancel/{pid}",                         CancelQueryHandler>,
    Route<GET,  "/monitor/replication",                             ReplicationHandler>,
    Route<GET,  "/monitor/tablestats",                              TableStatsHandler>,
    Route<GET,  "/monitor/tablestats/data",                         TableStatsDataHandler>,
    Route<POST, "/monitor/terminate/{pid}",                         TerminateHandler>,

    // EXPLAIN
    Route<GET,  "/explain",                                         ExplainPageHandler>,
    Route<POST, "/explain/exec",                                    ExplainExecHandler>,

    // API (JSON)
    Route<GET,  "/api/completions",                                 CompletionsHandler>,

    // Object explorer tree (htmx partials)
    Route<GET, "/tree",                                             TreeRootHandler>,
    Route<GET, "/tree/databases",                                   TreeDatabasesHandler>,
    Route<GET, "/tree/db/{db}",                                     TreeSchemasHandler>,
    Route<GET, "/tree/db/{db}/schema/{schema}",                     TreeSchemaChildrenHandler>,
    Route<GET, "/tree/db/{db}/schema/{schema}/tables",              TreeTablesHandler>,
    Route<GET, "/tree/db/{db}/schema/{schema}/functions",           TreeFunctionsHandler>,
    Route<GET, "/tree/db/{db}/schema/{schema}/sequences",           TreeSequencesHandler>,

    // Static assets (catch-all — must be last)
    Route<GET, "/assets/{path...}",                              StaticHandler>
>;

} // namespace getgresql::api
