#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace getgresql {

// ─── Strong type wrapper ────────────────────────────────────────────
// Prevents mixing up strings that represent different domains.
// Port{8080} and DatabaseName{"mydb"} are incompatible types.

template <typename Tag, typename T = std::string>
struct Strong {
    T value;

    explicit constexpr Strong(T v) : value(std::move(v)) {}

    constexpr auto operator==(const Strong& other) const -> bool = default;
    constexpr auto operator<=>(const Strong& other) const = default;

    constexpr auto get() const noexcept -> const T& { return value; }
    constexpr auto get() noexcept -> T& { return value; }

    // Implicit conversion to underlying for read-only contexts
    constexpr operator const T&() const noexcept { return value; }
};

// ─── Domain types ───────────────────────────────────────────────────

struct DatabaseNameTag {};
struct SchemaNameTag {};
struct TableNameTag {};
struct ColumnNameTag {};
struct ConnStringTag {};
struct PortTag {};
struct HostTag {};

using DatabaseName = Strong<DatabaseNameTag>;
using SchemaName   = Strong<SchemaNameTag>;
using TableName    = Strong<TableNameTag>;
using ColumnName   = Strong<ColumnNameTag>;
using ConnString   = Strong<ConnStringTag>;
using Port         = Strong<PortTag, std::uint16_t>;
using Host         = Strong<HostTag>;

// ─── OID wrapper (PostgreSQL object identifiers) ────────────────────

struct OidTag {};
using Oid = Strong<OidTag, std::uint32_t>;

} // namespace getgresql
