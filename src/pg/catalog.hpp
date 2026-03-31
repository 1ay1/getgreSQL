#pragma once

#include "core/expected.hpp"
#include "pg/connection.hpp"

#include <string>
#include <vector>

namespace getgresql::pg {

// ─── Catalog types: schema introspection results ────────────────────

struct DatabaseInfo {
    std::string name;
    std::string owner;
    std::string encoding;
    std::string size;       // human-readable e.g. "42 MB"
    long long size_bytes;
};

struct SchemaInfo {
    std::string name;
    std::string owner;
};

struct TableInfo {
    std::string schema;
    std::string name;
    std::string type;       // "table", "view", "materialized view"
    long long row_estimate;
    std::string size;
    long long size_bytes;
};

struct ColumnDetail {
    std::string name;
    std::string type;
    bool nullable;
    std::string default_value;
    bool is_primary_key;
    int ordinal;
};

struct IndexInfo {
    std::string name;
    std::string definition;
    bool is_unique;
    bool is_primary;
    std::string size;
};

struct ConstraintInfo {
    std::string name;
    std::string type;       // "PRIMARY KEY", "FOREIGN KEY", "UNIQUE", "CHECK"
    std::string definition;
};

struct ForeignKeyInfo {
    std::string name;
    std::string source_columns;
    std::string target_table;
    std::string target_columns;
};

// ─── Catalog queries ────────────────────────────────────────────────

auto list_databases(const Connection& conn) -> Result<std::vector<DatabaseInfo>>;
auto list_schemas(const Connection& conn, std::string_view database = "") -> Result<std::vector<SchemaInfo>>;
auto list_tables(const Connection& conn, std::string_view schema = "public") -> Result<std::vector<TableInfo>>;
auto describe_columns(const Connection& conn, std::string_view schema, std::string_view table) -> Result<std::vector<ColumnDetail>>;
auto list_indexes(const Connection& conn, std::string_view schema, std::string_view table) -> Result<std::vector<IndexInfo>>;
auto list_constraints(const Connection& conn, std::string_view schema, std::string_view table) -> Result<std::vector<ConstraintInfo>>;
auto list_foreign_keys(const Connection& conn, std::string_view schema, std::string_view table) -> Result<std::vector<ForeignKeyInfo>>;

// Table row count (exact, via COUNT(*))
auto table_row_count(const Connection& conn, std::string_view schema, std::string_view table) -> Result<long long>;

// Preview rows (SELECT * LIMIT n)
auto preview_rows(const Connection& conn, std::string_view schema, std::string_view table, int limit = 100) -> Result<PgResult>;

} // namespace getgresql::pg
