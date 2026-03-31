#pragma once

#include "http/router.hpp"

namespace getgresql::api {

using namespace http;

// GET /query — renders the SQL editor page
struct QueryPageHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// POST /query/exec — executes SQL and returns results
struct QueryExecHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /api/completions — JSON metadata for SQL editor autocomplete
struct CompletionsHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

} // namespace getgresql::api
