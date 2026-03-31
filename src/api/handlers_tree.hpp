#pragma once

#include "http/router.hpp"

namespace getgresql::api {

using namespace http;

// GET /tree — root tree nodes for object explorer
struct TreeRootHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /tree/databases — database list as tree nodes
struct TreeDatabasesHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /tree/db/{db} — schemas under a database
struct TreeSchemasHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /tree/db/{db}/schema/{schema} — tables/functions/sequences under schema
struct TreeSchemaChildrenHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /tree/db/{db}/schema/{schema}/tables — table list as tree nodes
struct TreeTablesHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /tree/db/{db}/schema/{schema}/functions — function list as tree nodes
struct TreeFunctionsHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

// GET /tree/db/{db}/schema/{schema}/sequences — sequence list as tree nodes
struct TreeSequencesHandler {
    static auto handle(Request& req, AppContext& ctx) -> Response;
};

} // namespace getgresql::api
