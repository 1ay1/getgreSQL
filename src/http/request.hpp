#pragma once

#include "core/static_string.hpp"

#include <boost/beast/http.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

namespace getgresql::http {

namespace beast = boost::beast;

// ─── Typed request wrapper ──────────────────────────────────────────

class Request {
    beast::http::request<beast::http::string_body> req_;
    PathMatch path_match_;  // populated by router after matching
    std::unordered_map<std::string, std::string> query_params_;

    void parse_query_string() {
        auto target = std::string_view(req_.target());
        auto qpos = target.find('?');
        if (qpos == std::string_view::npos) return;

        auto qs = target.substr(qpos + 1);
        while (!qs.empty()) {
            auto amp = qs.find('&');
            auto pair = qs.substr(0, amp);
            auto eq = pair.find('=');
            if (eq != std::string_view::npos) {
                query_params_[std::string(pair.substr(0, eq))] =
                    std::string(pair.substr(eq + 1));
            }
            if (amp == std::string_view::npos) break;
            qs = qs.substr(amp + 1);
        }
    }

public:
    explicit Request(beast::http::request<beast::http::string_body> req)
        : req_(std::move(req)) {
        parse_query_string();
    }

    void set_path_match(PathMatch m) { path_match_ = m; }

    // Path (without query string)
    auto path() const -> std::string_view {
        auto target = std::string_view(req_.target());
        auto qpos = target.find('?');
        return qpos == std::string_view::npos ? target : target.substr(0, qpos);
    }

    auto method() const -> beast::http::verb { return req_.method(); }
    auto method_string() const -> std::string_view { return req_.method_string(); }
    auto body() const -> std::string_view { return req_.body(); }
    auto target() const -> std::string_view { return req_.target(); }

    // Path parameters from route matching: /db/{name} → param("name")
    auto param(std::string_view name) const -> std::string_view {
        return path_match_.get(name);
    }

    // Query string parameters: ?page=2 → query("page")
    auto query(const std::string& name) const -> std::string_view {
        auto it = query_params_.find(name);
        return it != query_params_.end() ? std::string_view(it->second) : std::string_view{};
    }

    auto header(std::string_view name) const -> std::string_view {
        return req_[boost::beast::http::string_to_field(std::string(name))];
    }

    auto is_htmx() const -> bool {
        return !req_["HX-Request"].empty();
    }

    auto htmx_target() const -> std::string_view {
        return req_["HX-Target"];
    }

    auto version() const { return req_.version(); }
    auto keep_alive() const { return req_.keep_alive(); }
};

} // namespace getgresql::http
