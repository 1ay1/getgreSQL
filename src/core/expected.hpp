#pragma once

#include <expected>
#include <format>
#include <string>
#include <variant>

namespace getgresql {

// ─── Error types ────────────────────────────────────────────────────
// Each subsystem has a concrete error type. All are collected into
// AppError so that std::expected<T, AppError> is the universal Result.

struct PgConnectionError {
    std::string message;
    auto what() const noexcept -> const std::string& { return message; }
};

struct PgQueryError {
    std::string message;
    std::string sql;  // the offending query (truncated)
    auto what() const noexcept -> const std::string& { return message; }
};

struct HttpError {
    int status_code;
    std::string message;
    auto what() const noexcept -> const std::string& { return message; }
};

struct AuthError {
    std::string message;
    auto what() const noexcept -> const std::string& { return message; }
};

struct ConfigError {
    std::string message;
    auto what() const noexcept -> const std::string& { return message; }
};

struct PoolError {
    std::string message;
    auto what() const noexcept -> const std::string& { return message; }
};

// ─── Unified error variant ──────────────────────────────────────────

using AppError = std::variant<
    PgConnectionError,
    PgQueryError,
    HttpError,
    AuthError,
    ConfigError,
    PoolError
>;

// Extract the human-readable message from any error variant
inline auto error_message(const AppError& err) -> std::string {
    return std::visit([](const auto& e) -> std::string { return e.what(); }, err);
}

// ─── Result alias ───────────────────────────────────────────────────
// Every fallible operation returns Result<T>. Monadic chaining via
// C++23's and_then / transform / or_else.

template <typename T>
using Result = std::expected<T, AppError>;

// Convenience: wrap a specific error into unexpected<AppError>
template <typename E>
auto make_error(E&& err) -> std::unexpected<AppError> {
    return std::unexpected<AppError>(AppError{std::forward<E>(err)});
}

} // namespace getgresql
