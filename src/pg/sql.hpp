#pragma once

// ─── SQL Building Utilities ──────────────────────────────────────────
// Compile-time safe SQL identifier quoting and validation.
// Prevents SQL injection in DDL statements (which can't use parameterized queries).

#include <string>
#include <string_view>
#include <algorithm>

namespace getgresql::pg::sql {

// Quote a SQL identifier: my_table → "my_table"
// Handles embedded double-quotes: my"table → "my""table"
inline auto quote_ident(std::string_view name) -> std::string {
    std::string out;
    out.reserve(name.size() + 4);
    out += '"';
    for (char c : name) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += '"';
    return out;
}

// Quote a SQL literal: it's → 'it''s'
inline auto quote_literal(std::string_view val) -> std::string {
    std::string out;
    out.reserve(val.size() + 4);
    out += '\'';
    for (char c : val) {
        if (c == '\'') out += "''";
        else out += c;
    }
    out += '\'';
    return out;
}

// Validate a SQL identifier (no quoting needed for safe names)
inline auto is_safe_ident(std::string_view name) -> bool {
    if (name.empty() || name.size() > 128) return false;
    if (!(name[0] == '_' || (name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z')))
        return false;
    return std::all_of(name.begin(), name.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '_';
    });
}

// Qualified name: "schema"."table"
inline auto qualified(std::string_view schema, std::string_view name) -> std::string {
    return quote_ident(schema) + "." + quote_ident(name);
}

} // namespace getgresql::pg::sql
