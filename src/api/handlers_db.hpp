#pragma once

#include "http/router.hpp"

namespace getgresql::api {

using namespace http;

struct IndexHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// Shell-first dashboard sections (loaded in parallel by htmx)
struct DashboardHealthHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};
struct DashboardStatsHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};
struct DashboardContentHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};
struct DashboardActivityHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};
struct DashboardTopTablesHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /dashboard/fix — execute a health check remediation action
struct DashboardFixHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

struct DatabaseListHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

struct SchemaListHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

struct TableListHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

struct TableDetailHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

struct TableDataHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /db/{db}/schema/{schema}/table/{table}/columns — columns/indexes/constraints partial
struct TableColumnsHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /db/{db}/schema/{schema}/table/{table}/ddl — generate DDL
struct TableDDLHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /db/{db}/schema/{schema}/table/{table}/stats — column statistics
struct ColumnStatsHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /db/{db}/schema/{schema}/table/{table}/vacuum — vacuum table
struct VacuumHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /db/{db}/schema/{schema}/table/{table}/analyze — analyze table
struct AnalyzeHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /db/{db}/schema/{schema}/erd — ERD data as JSON
struct ERDDataHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /db/{db}/schema/{schema}/erd/page — ERD visualization page
struct ERDPageHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /db/{db}/schema/{schema}/table/{table}/browse — paginated data browser
struct TableBrowseHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /db/{db}/schema/{schema}/table/{table}/update-cell — inline cell edit
struct CellUpdateHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /db/{db}/schema/{schema}/table/{table}/delete-row — delete a row
struct RowDeleteHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /db/{db}/schema/{schema}/table/{table}/insert-row — insert new row
struct RowInsertHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /db/{db}/schema/{schema}/table/{table}/truncate — truncate table
struct TruncateTableHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /db/{db}/schema/{schema}/table/{table}/export — export as CSV
struct TableExportHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

} // namespace getgresql::api
