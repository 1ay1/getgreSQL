#pragma once

#include <boost/beast/http.hpp>
#include <cassert>
#include <string>

namespace getgresql::http {

namespace beast = boost::beast;

// ─── Response ────────────────────────────────────────────────────────
// Type-safe HTTP response. Bad states are unrepresentable:
//   - No public default constructor → must use a factory (html/json/etc.)
//   - Move-only → no accidental copies
//   - release() asserts not already consumed
//
// Every factory produces a complete, valid HTTP response.

class Response {
    beast::http::response<beast::http::string_body> res_;
    bool released_ = false;

    // Private: only factories can construct. Prevents empty/invalid responses.
    Response() { res_.version(11); }

public:
    // Move-only: prevent accidental copies of response bodies
    Response(Response&&) = default;
    Response& operator=(Response&&) = default;
    Response(const Response&) = delete;
    Response& operator=(const Response&) = delete;

    static auto html(std::string body, unsigned status = 200) -> Response {
        Response r;
        r.res_.result(static_cast<beast::http::status>(status));
        r.res_.set(beast::http::field::content_type, "text/html; charset=utf-8");
        r.res_.body() = std::move(body);
        r.res_.prepare_payload();
        return r;
    }

    static auto json(std::string body, unsigned status = 200) -> Response {
        Response r;
        r.res_.result(static_cast<beast::http::status>(status));
        r.res_.set(beast::http::field::content_type, "application/json");
        r.res_.body() = std::move(body);
        r.res_.prepare_payload();
        return r;
    }

    static auto text(std::string body, unsigned status = 200) -> Response {
        Response r;
        r.res_.result(static_cast<beast::http::status>(status));
        r.res_.set(beast::http::field::content_type, "text/plain; charset=utf-8");
        r.res_.body() = std::move(body);
        r.res_.prepare_payload();
        return r;
    }

    static auto redirect(std::string_view location) -> Response {
        Response r;
        r.res_.result(beast::http::status::found);
        r.res_.set(beast::http::field::location, location);
        r.res_.prepare_payload();
        return r;
    }

    static auto not_found(std::string_view path = "") -> Response {
        return html(std::string("<h1>404 Not Found</h1><p>") +
                     std::string(path) + "</p>", 404);
    }

    static auto error(std::string_view message, unsigned status = 500) -> Response {
        return html(std::string("<h1>Error</h1><pre>") +
                     std::string(message) + "</pre>", status);
    }

    static auto css(std::string body) -> Response {
        Response r;
        r.res_.result(beast::http::status::ok);
        r.res_.set(beast::http::field::content_type, "text/css");
        r.res_.set(beast::http::field::cache_control, "public, max-age=86400");
        r.res_.body() = std::move(body);
        r.res_.prepare_payload();
        return r;
    }

    static auto js(std::string body) -> Response {
        Response r;
        r.res_.result(beast::http::status::ok);
        r.res_.set(beast::http::field::content_type, "application/javascript");
        r.res_.set(beast::http::field::cache_control, "public, max-age=86400");
        r.res_.body() = std::move(body);
        r.res_.prepare_payload();
        return r;
    }

    // Set headers — asserts response hasn't been consumed
    auto& set(beast::http::field field, std::string_view value) {
        assert(!released_ && "Response modified after release()");
        res_.set(field, value);
        return *this;
    }

    auto& set(std::string_view name, std::string_view value) {
        assert(!released_ && "Response modified after release()");
        res_.set(name, value);
        return *this;
    }

    auto& keep_alive(bool v) {
        assert(!released_ && "Response modified after release()");
        res_.keep_alive(v);
        return *this;
    }

    auto& version(unsigned v) {
        assert(!released_ && "Response modified after release()");
        res_.version(v);
        return *this;
    }

    // Consume the response — can only be called once
    [[nodiscard]] auto release() -> beast::http::response<beast::http::string_body> {
        assert(!released_ && "Response::release() called twice");
        released_ = true;
        return std::move(res_);
    }
};

} // namespace getgresql::http
