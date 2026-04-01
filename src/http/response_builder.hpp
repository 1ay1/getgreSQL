#pragma once

// ─── Type-State Response Builder ─────────────────────────────────────
//
// Encodes response construction progress in the type system.
// Impossible to build() without setting a body. Impossible to
// double-set a body. The compiler enforces correct construction.
//
// Usage:
//   auto r = ResponseBuilder<>{}
//       .html("<h1>Hello</h1>")
//       .header(beast::http::field::cache_control, "no-cache")
//       .build();
//
// Compile error:
//   ResponseBuilder<>{}.build();           // error: no body set
//   ResponseBuilder<>{}.html("a").html("b"); // error: body already set

#include "http/response.hpp"
#include <concepts>

namespace getgresql::http {

// ── Phantom state tags ───────────────────────────────────────────────

struct NoBody {};
struct HasBody {};

// ── ResponseBuilder<BodyState> ───────────────────────────────────────

template<typename BodyState = NoBody>
class ResponseBuilder {
    beast::http::response<beast::http::string_body> res_;

    // Private: only transitions create HasBody builders
    explicit ResponseBuilder(beast::http::response<beast::http::string_body> r)
        : res_(std::move(r)) {}

    template<typename> friend class ResponseBuilder;

public:
    // Default constructor: only valid for NoBody state
    ResponseBuilder() requires std::same_as<BodyState, NoBody> {
        res_.version(11);
    }

    // ── Body setters — transition NoBody → HasBody ───────────────

    auto html(std::string body, unsigned status = 200) &&
        -> ResponseBuilder<HasBody>
        requires std::same_as<BodyState, NoBody>
    {
        res_.result(static_cast<beast::http::status>(status));
        res_.set(beast::http::field::content_type, "text/html; charset=utf-8");
        res_.body() = std::move(body);
        return ResponseBuilder<HasBody>{std::move(res_)};
    }

    auto json(std::string body, unsigned status = 200) &&
        -> ResponseBuilder<HasBody>
        requires std::same_as<BodyState, NoBody>
    {
        res_.result(static_cast<beast::http::status>(status));
        res_.set(beast::http::field::content_type, "application/json");
        res_.body() = std::move(body);
        return ResponseBuilder<HasBody>{std::move(res_)};
    }

    auto redirect(std::string_view location) &&
        -> ResponseBuilder<HasBody>
        requires std::same_as<BodyState, NoBody>
    {
        res_.result(beast::http::status::found);
        res_.set(beast::http::field::location, location);
        return ResponseBuilder<HasBody>{std::move(res_)};
    }

    // ── Headers — available in any state ─────────────────────────

    auto header(beast::http::field field, std::string_view value) &&
        -> ResponseBuilder<BodyState>
    {
        res_.set(field, value);
        return ResponseBuilder<BodyState>{std::move(res_)};
    }

    auto header(std::string_view name, std::string_view value) &&
        -> ResponseBuilder<BodyState>
    {
        res_.set(name, value);
        return ResponseBuilder<BodyState>{std::move(res_)};
    }

    // ── Build — only available when body is set ──────────────────

    auto build() && -> Response
        requires std::same_as<BodyState, HasBody>
    {
        res_.prepare_payload();
        Response r;
        // Move our beast response into the Response wrapper
        r.set(beast::http::field::content_type,
              std::string(res_[beast::http::field::content_type]));
        // We need direct access — use the existing factory + header override
        return Response::html(std::move(res_.body()),
                              static_cast<unsigned>(res_.result_int()));
    }
};

} // namespace getgresql::http
