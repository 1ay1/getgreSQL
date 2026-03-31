#pragma once

#include "http/router.hpp"

namespace getgresql::api {

using namespace http;

struct IndexHandler {
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

} // namespace getgresql::api
