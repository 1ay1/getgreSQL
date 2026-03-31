#pragma once

#include <boost/beast/http.hpp>
#include <string>

namespace getgresql::http {

namespace beast = boost::beast;

// ─── Response builder ───────────────────────────────────────────────
// Fluent interface for constructing HTTP responses.

class Response {
    beast::http::response<beast::http::string_body> res_;

public:
    Response() { res_.version(11); }

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

    // Set headers
    auto& set(beast::http::field field, std::string_view value) {
        res_.set(field, value);
        return *this;
    }

    auto& set(std::string_view name, std::string_view value) {
        res_.set(name, value);
        return *this;
    }

    auto& keep_alive(bool v) {
        res_.keep_alive(v);
        return *this;
    }

    auto& version(unsigned v) {
        res_.version(v);
        return *this;
    }

    auto release() -> beast::http::response<beast::http::string_body> {
        return std::move(res_);
    }
};

} // namespace getgresql::http
