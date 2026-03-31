#pragma once

#include "core/static_string.hpp"
#include "core/type_list.hpp"
#include "http/context.hpp"
#include "http/request.hpp"
#include "http/response.hpp"

#include <boost/beast/http.hpp>
#include <functional>

namespace getgresql::http {

namespace beast = boost::beast;

// ─── HTTP methods as enum (matches Beast verbs) ─────────────────────

enum class Method {
    GET, POST, PUT, DELETE_, PATCH
};

constexpr auto to_beast_verb(Method m) -> beast::http::verb {
    switch (m) {
        case Method::GET:     return beast::http::verb::get;
        case Method::POST:    return beast::http::verb::post;
        case Method::PUT:     return beast::http::verb::put;
        case Method::DELETE_: return beast::http::verb::delete_;
        case Method::PATCH:   return beast::http::verb::patch;
    }
    return beast::http::verb::get; // unreachable
}

// ─── Route: a compile-time binding of method + path → handler ───────
// The handler is a type with a static `handle(Request&) -> Response`.

template <Method M, StaticString Path, typename Handler>
struct Route {
    static constexpr Method method = M;
    static constexpr auto path = Path;
    using handler_type = Handler;

    // Compile-time path info
    static constexpr bool has_params = Path.has_params();
    static constexpr bool is_catch_all = Path.is_catch_all();
    static constexpr std::size_t param_count = Path.param_count();
};

// ─── RouteTable: compile-time list of all routes ────────────────────
// Dispatch iterates the type list at compile time, generating an
// efficient if-else chain. No virtual calls, no hash maps.

template <typename... Routes>
struct RouteTable {
    using routes = meta::TypeList<Routes...>;
    static constexpr std::size_t count = sizeof...(Routes);

    // Dispatch a request to the matching handler
    static auto dispatch(Request& req, AppContext& ctx) -> Response {
        Response result = Response::not_found(req.path());
        bool matched = false;

        // Fold expression: try each route at compile time
        ((try_route<Routes>(req, ctx, result, matched)), ...);

        return result;
    }

private:
    template <typename R>
    static void try_route(Request& req, AppContext& ctx, Response& result, bool& matched) {
        if (matched) return;

        // Check HTTP method
        if (req.method() != to_beast_verb(R::method)) return;

        // Try to match the path
        auto match = match_path(R::path.sv(), req.path());
        if (!match.matched) return;

        // Set path parameters on request
        req.set_path_match(match);
        matched = true;

        // Call the handler
        result = R::handler_type::handle(req, ctx);
    }
};

// ─── Handler concept ────────────────────────────────────────────────
// Every handler must satisfy this concept.

template <typename H>
concept HandlerType = requires(Request& req, AppContext& ctx) {
    { H::handle(req, ctx) } -> std::same_as<Response>;
};

} // namespace getgresql::http
