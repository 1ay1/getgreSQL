#pragma once

#include "http/router.hpp"

namespace getgresql::api {

using namespace http;

// GET /roles — list PostgreSQL roles
struct RolesHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /extensions — list installed extensions
struct ExtensionsHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /settings — server configuration viewer
struct SettingsHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /settings/search — htmx partial for searching settings
struct SettingsSearchHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /db/{db}/schema/{schema}/functions — list functions
struct FunctionListHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /db/{db}/schema/{schema}/function/{func} — function detail/source
struct FunctionDetailHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /db/{db}/schema/{schema}/sequences — list sequences
struct SequenceListHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /db/{db}/schema/{schema}/indexes — index usage analysis
struct IndexAnalysisHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /db/{db}/size — database size breakdown
struct DatabaseSizeHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /monitor/replication — replication slots viewer
struct ReplicationHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /monitor/tablestats — table statistics page
struct TableStatsHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /monitor/tablestats/data — htmx partial for table stats
struct TableStatsDataHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /monitor/terminate/{pid} — force terminate backend
struct TerminateHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /explain — EXPLAIN ANALYZE page
struct ExplainPageHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /explain/exec — run EXPLAIN
struct ExplainExecHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

} // namespace getgresql::api
