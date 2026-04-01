#pragma once

#include "api/handlers_admin.hpp"
#include "api/handlers_db.hpp"
#include "api/handlers_monitor.hpp"
#include "api/handlers_query.hpp"
#include "api/handlers_schema.hpp"
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
    // Dashboard (shell-first: parallel section loading)
    Route<GET, "/",                                              IndexHandler>,
    Route<GET, "/dashboard/health",                              DashboardHealthHandler>,
    Route<GET, "/dashboard/stats",                               DashboardStatsHandler>,
    Route<GET, "/dashboard/content",                             DashboardContentHandler>,
    Route<GET, "/dashboard/activity",                            DashboardActivityHandler>,
    Route<GET, "/dashboard/top-tables",                          DashboardTopTablesHandler>,

    // Database browsing
    Route<GET, "/databases",                                     DatabaseListHandler>,
    Route<GET, "/db/{db}/schemas",                               SchemaListHandler>,
    Route<GET, "/db/{db}/schema/{schema}/tables",                TableListHandler>,
    Route<GET, "/db/{db}/schema/{schema}/table/{table}",         TableDetailHandler>,
    Route<GET, "/db/{db}/schema/{schema}/table/{table}/columns",  TableColumnsHandler>,
    Route<GET, "/db/{db}/schema/{schema}/table/{table}/data",    TableDataHandler>,

    // Query editor
    Route<GET,  "/query",                                        QueryPageHandler>,
    Route<POST, "/query/exec",                                   QueryExecHandler>,

    // Table tools
    Route<GET,  "/db/{db}/schema/{schema}/table/{table}/ddl",         TableDDLHandler>,
    Route<GET,  "/db/{db}/schema/{schema}/table/{table}/stats",       ColumnStatsHandler>,
    Route<GET,  "/db/{db}/schema/{schema}/table/{table}/browse",      TableBrowseHandler>,
    Route<GET,  "/db/{db}/schema/{schema}/table/{table}/export",      TableExportHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/vacuum",      VacuumHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/analyze",     AnalyzeHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/update-cell", CellUpdateHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/delete-row",  RowDeleteHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/insert-row",  RowInsertHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/truncate",    TruncateTableHandler>,

    // ERD
    Route<GET, "/db/{db}/schema/{schema}/erd",                     ERDDataHandler>,
    Route<GET, "/db/{db}/schema/{schema}/erd/page",                ERDPageHandler>,

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
    Route<GET,  "/monitor/health",                                  HealthCheckHandler>,
    Route<GET,  "/monitor/slow",                                    SlowQueriesHandler>,
    Route<GET,  "/monitor/blocking",                                BlockingHandler>,
    Route<GET,  "/monitor/wal",                                     WALStatsHandler>,
    Route<GET,  "/monitor/vacuum-progress",                         VacuumProgressHandler>,
    Route<GET,  "/monitor/databases",                               DatabaseStatsHandler>,
    Route<GET,  "/monitor/bloat",                                   BloatHandler>,

    // Schema management (CREATE/ALTER/DROP)
    Route<GET,  "/db/{db}/schema/{schema}/create-table",              CreateTablePageHandler>,
    Route<POST, "/db/{db}/schema/{schema}/create-table/exec",         CreateTableExecHandler>,
    Route<GET,  "/db/{db}/schema/{schema}/table/{table}/alter",       AlterTablePageHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/add-column",  AddColumnHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/drop-column", DropColumnHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/add-index",   AddIndexHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/drop-index",  DropIndexHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/rename",      RenameTableHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/drop",        DropTableHandler>,
    Route<GET,  "/db/{db}/schema/{schema}/table/{table}/import",      ImportPageHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/import/exec", ImportExecHandler>,
    Route<POST, "/db/{db}/schema/{schema}/table/{table}/exact-count", ExactCountHandler>,

    // Schema diff
    Route<GET,  "/schema-diff",                                     SchemaDiffPageHandler>,
    Route<POST, "/schema-diff/exec",                                SchemaDiffExecHandler>,

    // API
    Route<GET,  "/api/pg-types",                                    PgTypesHandler>,

    // Query explain (used by editor.js)
    Route<POST, "/query/explain",                                   ExplainExecHandler>,

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
