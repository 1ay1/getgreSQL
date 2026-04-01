#pragma once

#include "http/router.hpp"

namespace getgresql::api {

using namespace http;

// ── SSR DataView Operations ─────────────────────────────────────────
// All cell editing, sorting, filtering is server-rendered via htmx.
// No client-side JS needed for data manipulation.

// GET /dv/edit-cell — returns an inline <input> form for cell editing
struct DvEditCellHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /dv/save-cell — saves cell value (works for both table browse and query results)
struct DvSaveCellHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /dv/set-null — sets a cell to NULL
struct DvSetNullHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /dv/explain-cell — returns lineage info for a cell value
struct DvExplainCellHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

} // namespace getgresql::api
