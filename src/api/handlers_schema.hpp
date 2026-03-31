#pragma once

#include "http/router.hpp"

namespace getgresql::api {

using namespace http;

// ─── CREATE TABLE ────────────────────────────────────────────────────
struct CreateTablePageHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};
struct CreateTableExecHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// ─── ALTER TABLE ─────────────────────────────────────────────────────
struct AlterTablePageHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};
struct AddColumnHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};
struct DropColumnHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};
struct AddIndexHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};
struct DropIndexHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};
struct RenameTableHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// ─── DROP TABLE ──────────────────────────────────────────────────────
struct DropTableHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// ─── IMPORT DATA ─────────────────────────────────────────────────────
struct ImportPageHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};
struct ImportExecHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// ─── SCHEMA DIFF ─────────────────────────────────────────────────────
struct SchemaDiffPageHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};
struct SchemaDiffExecHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// ─── EXACT ROW COUNT ─────────────────────────────────────────────────
struct ExactCountHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// ─── PG TYPES (JSON) ─────────────────────────────────────────────────
struct PgTypesHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

} // namespace getgresql::api
