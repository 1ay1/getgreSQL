#pragma once

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace getgresql {

// ─── Compile-time string: the foundation of type-safe routes and SQL ────
// A StaticString<N> carries its content in the type system. Two routes with
// different paths are different types — the compiler catches typos and
// duplicates before any code runs.

template <std::size_t N>
struct StaticString {
    char data[N]{};
    static constexpr std::size_t length = N - 1;

    constexpr StaticString() = default;

    constexpr StaticString(const char (&str)[N]) {
        std::copy_n(str, N, data);
    }

    constexpr operator std::string_view() const noexcept {
        return {data, length};
    }

    constexpr auto sv() const noexcept -> std::string_view {
        return {data, length};
    }

    constexpr auto size() const noexcept -> std::size_t { return length; }
    constexpr auto begin() const noexcept -> const char* { return data; }
    constexpr auto end() const noexcept -> const char* { return data + length; }

    constexpr auto operator[](std::size_t i) const noexcept -> char {
        return data[i];
    }

    constexpr auto operator==(const StaticString& other) const noexcept -> bool {
        return sv() == other.sv();
    }

    template <std::size_t M>
    constexpr auto operator==(const StaticString<M>& other) const noexcept -> bool {
        return sv() == other.sv();
    }

    // Compile-time concatenation: StaticString<3>{"ab"} + StaticString<3>{"cd"} = StaticString<5>{"abcd"}
    template <std::size_t M>
    constexpr auto operator+(const StaticString<M>& other) const noexcept -> StaticString<N + M - 1> {
        StaticString<N + M - 1> result;
        std::copy_n(data, length, result.data);
        std::copy_n(other.data, M, result.data + length);
        return result;
    }

    // Compile-time: does this path contain a parameter segment like {name}?
    constexpr auto has_params() const noexcept -> bool {
        for (std::size_t i = 0; i < length; ++i) {
            if (data[i] == '{') return true;
        }
        return false;
    }

    // Compile-time: count the number of path segments
    constexpr auto segment_count() const noexcept -> std::size_t {
        if (length == 0) return 0;
        std::size_t count = 0;
        for (std::size_t i = 0; i < length; ++i) {
            if (data[i] == '/' && i + 1 < length) ++count;
        }
        return count == 0 && length > 0 ? 1 : count;
    }

    // Compile-time: count parameter segments {name}
    constexpr auto param_count() const noexcept -> std::size_t {
        std::size_t count = 0;
        for (std::size_t i = 0; i + 1 < length; ++i) {
            if (data[i] == '{') ++count;
        }
        return count;
    }

    // Compile-time: check if this is a wildcard/catch-all route (ends with {path...})
    constexpr auto is_catch_all() const noexcept -> bool {
        if (length < 8) return false; // minimum: "{x...}"
        // Look for "...}" at the end
        return data[length - 1] == '}' &&
               data[length - 2] == '.' &&
               data[length - 3] == '.' &&
               data[length - 4] == '.';
    }
};

// CTAD
template <std::size_t N>
StaticString(const char (&)[N]) -> StaticString<N>;

// Compile-time string literal operator
namespace literals {
    template <StaticString S>
    constexpr auto operator""_ss() noexcept {
        return S;
    }
}

// ─── Compile-time path matching ─────────────────────────────────────
// Given a pattern like "/db/{name}/tables" and a runtime path like
// "/db/mydb/tables", extract "mydb" into the parameter map.

struct PathMatch {
    bool matched = false;
    // Up to 8 captured path parameters (name -> value)
    struct Param {
        std::string_view name;
        std::string_view value;
    };
    Param params[8]{};
    std::size_t param_count = 0;

    constexpr auto get(std::string_view name) const noexcept -> std::string_view {
        for (std::size_t i = 0; i < param_count; ++i) {
            if (params[i].name == name) return params[i].value;
        }
        return {};
    }
};

// Runtime path matching against a compile-time pattern
inline auto match_path(std::string_view pattern, std::string_view path) noexcept -> PathMatch {
    PathMatch result;

    std::size_t pi = 0; // pattern index
    std::size_t ri = 0; // request path index

    while (pi < pattern.size() && ri < path.size()) {
        if (pattern[pi] == '{') {
            // Extract parameter name
            auto close = pattern.find('}', pi);
            if (close == std::string_view::npos) return result;

            auto param_name = pattern.substr(pi + 1, close - pi - 1);

            // Check for catch-all {path...}
            if (param_name.ends_with("...")) {
                param_name = param_name.substr(0, param_name.size() - 3);
                result.params[result.param_count++] = {param_name, path.substr(ri)};
                result.matched = true;
                return result;
            }

            // Find end of this path segment
            auto seg_end = path.find('/', ri);
            if (seg_end == std::string_view::npos) seg_end = path.size();

            result.params[result.param_count++] = {param_name, path.substr(ri, seg_end - ri)};

            pi = close + 1;
            ri = seg_end;
        } else {
            if (pattern[pi] != path[ri]) return result;
            ++pi;
            ++ri;
        }
    }

    result.matched = (pi == pattern.size() && ri == path.size());
    return result;
}

} // namespace getgresql
