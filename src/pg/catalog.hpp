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

struct RoleInfo {
    std::string name;
    bool is_superuser;
    bool can_login;
    bool can_create_db;
    bool can_create_role;
    int connection_limit;
    std::string valid_until;  // empty if no expiry
    std::string member_of;   // comma-separated list of parent roles
};

struct ExtensionInfo {
    std::string name;
    std::string version;
    std::string schema;
    std::string description;
};

struct FunctionInfo {
    std::string name;
    std::string arguments;
    std::string return_type;
    std::string language;
    std::string volatility;   // "VOLATILE", "STABLE", "IMMUTABLE"
    bool is_trigger;
};

struct SequenceInfo {
    std::string name;
    std::string data_type;
    long long current_value;
    long long start_value;
    long long increment;
    long long min_value;
    long long max_value;
    bool cycle;
};

struct IndexUsageInfo {
    std::string schema;
    std::string table;
    std::string index_name;
    long long index_size_bytes;
    std::string index_size;
    long long idx_scan;
    long long idx_tup_read;
    long long idx_tup_fetch;
};

struct SizeBreakdown {
    std::string schema;
    long long size_bytes;
    std::string size;
    int table_count;
    int index_count;
};

struct FunctionSource {
    std::string name;
    std::string language;
    std::string source;
    std::string definition;  // full CREATE OR REPLACE
};

// ─── Completion metadata for SQL editor autocomplete ────────────────

struct CompletionColumn {
    std::string name;
    std::string type;
};

struct CompletionTable {
    std::string schema;
    std::string name;
    std::string type;  // "table", "view"
    std::vector<CompletionColumn> columns;
};

struct CompletionData {
    std::vector<std::string> schemas;
    std::vector<CompletionTable> tables;
};

// ─── Catalog queries ────────────────────────────────────────────────

auto list_databases(const Connection& conn) -> Result<std::vector<DatabaseInfo>>;
auto list_schemas(const Connection& conn, std::string_view database = "") -> Result<std::vector<SchemaInfo>>;
auto list_tables(const Connection& conn, std::string_view schema = "public") -> Result<std::vector<TableInfo>>;
auto describe_columns(const Connection& conn, std::string_view schema, std::string_view table) -> Result<std::vector<ColumnDetail>>;
auto list_indexes(const Connection& conn, std::string_view schema, std::string_view table) -> Result<std::vector<IndexInfo>>;
auto list_constraints(const Connection& conn, std::string_view schema, std::string_view table) -> Result<std::vector<ConstraintInfo>>;
auto list_foreign_keys(const Connection& conn, std::string_view schema, std::string_view table) -> Result<std::vector<ForeignKeyInfo>>;

auto list_roles(const Connection& conn) -> Result<std::vector<RoleInfo>>;
auto list_extensions(const Connection& conn) -> Result<std::vector<ExtensionInfo>>;
auto list_functions(const Connection& conn, std::string_view schema = "public") -> Result<std::vector<FunctionInfo>>;
auto list_sequences(const Connection& conn, std::string_view schema = "public") -> Result<std::vector<SequenceInfo>>;
auto index_usage_stats(const Connection& conn, std::string_view schema = "public") -> Result<std::vector<IndexUsageInfo>>;
auto database_size_breakdown(const Connection& conn) -> Result<std::vector<SizeBreakdown>>;
auto get_function_source(const Connection& conn, std::string_view schema, std::string_view name, std::string_view args = "") -> Result<FunctionSource>;

// Completion metadata for SQL editor autocomplete
auto completion_metadata(const Connection& conn) -> Result<CompletionData>;

// Table row count (exact, via COUNT(*))
auto table_row_count(const Connection& conn, std::string_view schema, std::string_view table) -> Result<long long>;

// Preview rows (SELECT * LIMIT n)
auto preview_rows(const Connection& conn, std::string_view schema, std::string_view table, int limit = 100) -> Result<PgResult>;

} // namespace getgresql::pg
