#pragma once

// ─── getgreSQL SQL DSL — Type-Safe Compile-Time Query Building ──────
//
// Three layers, each adding safety:
//
//   1. sql::lit("SELECT 1")
//      → Static SQL literal. The string is baked into the binary.
//        Type: SqlExpr<Static>. Cannot contain user input.
//
//   2. sql::query("SELECT * FROM {t} WHERE id = {id}", sql::ident(schema, table), sql::param(42))
//      → Parameterized query. Identifiers are quoted, values use $1/$2.
//        Type: SqlExpr<Parameterized>. SQL injection impossible.
//
//   3. sql::unsafe(user_string)
//      → Explicitly marks raw user SQL (for EXPLAIN, query editor).
//        Type: SqlExpr<Unsafe>. Auditable via grep for "sql::unsafe".
//
// The phantom types (Static, Parameterized, Unsafe) flow through the
// connection's exec methods:
//
//   conn.exec(sql::lit("SELECT 1"));           // always safe
//   conn.exec(query);                           // uses PQexecParams
//   conn.exec(sql::unsafe(user_sql));           // compiles, but auditable
//
// Builder pattern for complex queries:
//
//   auto q = sql::select("s.schemaname", "s.relname", "pg_size_pretty(pg_total_relation_size(...))")
//            .from("pg_stat_user_tables s")
//            .where("s.schemaname = ", sql::param(schema))
//            .order_by("pg_total_relation_size(...) DESC")
//            .limit(sql::param(10));

#include "pg/sql.hpp"
#include <array>
#include <concepts>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace getgresql::pg::sql {

// ─── Phantom safety tags ────────────────────────────────────────────

struct Static {};        // Compile-time literal — no user input possible
struct Parameterized {}; // Uses $1/$2 placeholders — injection impossible
struct Unsafe {};        // Raw user SQL — explicitly marked, auditable

// ─── Safety concepts ────────────────────────────────────────────────

template<typename T>
concept SqlSafety = std::same_as<T, Static> ||
                    std::same_as<T, Parameterized> ||
                    std::same_as<T, Unsafe>;

template<typename T>
concept SafeSql = std::same_as<T, Static> || std::same_as<T, Parameterized>;

// ─── SqlExpr<Safety> — a query with compile-time safety tracking ────

// Forward declarations for friend access
class QueryBuilder;
template<SqlSafety Safety> class SqlExpr;
inline auto lit(std::string_view sql) -> SqlExpr<Static>;
inline auto unsafe(std::string_view sql) -> SqlExpr<Unsafe>;
inline auto unsafe(std::string sql) -> SqlExpr<Unsafe>;

template<SqlSafety Safety>
class SqlExpr {
    std::string sql_;
    std::vector<std::string> params_;  // only used for Parameterized

    // Private tag — only friends can construct SqlExprs
    struct Key {};
    template<SqlSafety> friend class SqlExpr;
    friend class QueryBuilder;
    friend auto lit(std::string_view) -> SqlExpr<Static>;
    friend auto unsafe(std::string_view) -> SqlExpr<Unsafe>;
    friend auto unsafe(std::string) -> SqlExpr<Unsafe>;

    SqlExpr(Key, std::string sql) : sql_(std::move(sql)) {}
    SqlExpr(Key, std::string sql, std::vector<std::string> params)
        : sql_(std::move(sql)), params_(std::move(params)) {}

public:
    SqlExpr(SqlExpr&&) = default;
    SqlExpr& operator=(SqlExpr&&) = default;
    SqlExpr(const SqlExpr&) = default;
    SqlExpr& operator=(const SqlExpr&) = default;

    [[nodiscard]] auto sql() const -> const std::string& { return sql_; }
    [[nodiscard]] auto view() const -> std::string_view { return sql_; }
    [[nodiscard]] auto params() const -> const std::vector<std::string>& { return params_; }
    [[nodiscard]] auto param_count() const -> int { return static_cast<int>(params_.size()); }

    // C-string array for PQexecParams
    [[nodiscard]] auto param_ptrs() const -> std::vector<const char*> {
        std::vector<const char*> ptrs;
        ptrs.reserve(params_.size());
        for (auto& p : params_) ptrs.push_back(p.c_str());
        return ptrs;
    }
};

// ─── Factory: static SQL literals ───────────────────────────────────
// The sql is a compile-time string. No user input can enter.

inline auto lit(std::string_view sql) -> SqlExpr<Static> {
    return SqlExpr<Static>(SqlExpr<Static>::Key{}, std::string(sql));
}

// ─── Factory: explicitly unsafe SQL (user input) ────────────────────
// Auditable: grep for "sql::unsafe" to find all trust boundaries.

inline auto unsafe(std::string_view sql) -> SqlExpr<Unsafe> {
    return SqlExpr<Unsafe>(SqlExpr<Unsafe>::Key{}, std::string(sql));
}

inline auto unsafe(std::string sql) -> SqlExpr<Unsafe> {
    return SqlExpr<Unsafe>(SqlExpr<Unsafe>::Key{}, std::move(sql));
}

// ─── Param: a value placeholder ─────────────────────────────────────
// Holds the value and tracks its position ($1, $2, ...).

struct ParamValue {
    std::string value;
};

inline auto param(std::string_view v) -> ParamValue { return {std::string(v)}; }
inline auto param(const std::string& v) -> ParamValue { return {v}; }
inline auto param(const char* v) -> ParamValue { return {v}; }
inline auto param(int v) -> ParamValue { return {std::to_string(v)}; }
inline auto param(long v) -> ParamValue { return {std::to_string(v)}; }
inline auto param(long long v) -> ParamValue { return {std::to_string(v)}; }
inline auto param(double v) -> ParamValue { return {std::to_string(v)}; }
inline auto param(bool v) -> ParamValue { return {v ? "true" : "false"}; }

// ─── Ident: a safely-quoted SQL identifier ──────────────────────────
// Always double-quote quoted. "schema"."table" format.

struct IdentValue {
    std::string quoted;
};

inline auto ident(std::string_view name) -> IdentValue {
    return {quote_ident(name)};
}

inline auto ident(std::string_view schema, std::string_view name) -> IdentValue {
    return {qualified(schema, name)};
}

// ─── QueryBuilder — fluent parameterized query construction ─────────
//
// Builds a SQL string with $1/$2 placeholders and collects param values.
// The result is SqlExpr<Parameterized>.

class QueryBuilder {
    std::string sql_;
    std::vector<std::string> params_;
    bool has_where_ = false;

    auto next_placeholder() -> std::string {
        return "$" + std::to_string(params_.size() + 1);
    }

public:
    QueryBuilder() = default;
    explicit QueryBuilder(std::string base) : sql_(std::move(base)) {}

    // ── Raw SQL fragment (trusted, no escaping) ──────────────────
    auto raw(std::string_view sql) -> QueryBuilder& {
        sql_ += sql;
        return *this;
    }

    // ── Append a safely-quoted identifier ────────────────────────
    auto id(std::string_view name) -> QueryBuilder& {
        sql_ += quote_ident(name);
        return *this;
    }

    auto id(std::string_view schema, std::string_view name) -> QueryBuilder& {
        sql_ += qualified(schema, name);
        return *this;
    }

    // ── Append a $N placeholder and store the value ──────────────
    auto val(std::string_view v) -> QueryBuilder& {
        sql_ += next_placeholder();
        params_.emplace_back(v);
        return *this;
    }

    auto val(const std::string& v) -> QueryBuilder& {
        sql_ += next_placeholder();
        params_.push_back(v);
        return *this;
    }

    auto val(int v) -> QueryBuilder& {
        sql_ += next_placeholder();
        params_.push_back(std::to_string(v));
        return *this;
    }

    auto val(long long v) -> QueryBuilder& {
        sql_ += next_placeholder();
        params_.push_back(std::to_string(v));
        return *this;
    }

    // ── SQL clause helpers ───────────────────────────────────────

    auto select(std::string_view cols = "*") -> QueryBuilder& {
        sql_ += "SELECT ";
        sql_ += cols;
        return *this;
    }

    auto from(std::string_view table_expr) -> QueryBuilder& {
        sql_ += " FROM ";
        sql_ += table_expr;
        return *this;
    }

    auto from_table(std::string_view schema, std::string_view table) -> QueryBuilder& {
        sql_ += " FROM ";
        sql_ += qualified(schema, table);
        return *this;
    }

    auto where(std::string_view condition) -> QueryBuilder& {
        sql_ += has_where_ ? " AND " : " WHERE ";
        sql_ += condition;
        has_where_ = true;
        return *this;
    }

    // WHERE col = $N  (appends placeholder + stores value)
    auto where_eq(std::string_view col, std::string_view value) -> QueryBuilder& {
        sql_ += has_where_ ? " AND " : " WHERE ";
        sql_ += col;
        sql_ += " = ";
        sql_ += next_placeholder();
        params_.emplace_back(value);
        has_where_ = true;
        return *this;
    }

    auto where_eq(std::string_view col, int value) -> QueryBuilder& {
        sql_ += has_where_ ? " AND " : " WHERE ";
        sql_ += col;
        sql_ += " = ";
        sql_ += next_placeholder();
        params_.push_back(std::to_string(value));
        has_where_ = true;
        return *this;
    }

    auto where_ilike(std::string_view col, std::string_view pattern) -> QueryBuilder& {
        sql_ += has_where_ ? " AND " : " WHERE ";
        sql_ += col;
        sql_ += " ILIKE ";
        sql_ += next_placeholder();
        params_.emplace_back(pattern);
        has_where_ = true;
        return *this;
    }

    auto order_by(std::string_view expr) -> QueryBuilder& {
        sql_ += " ORDER BY ";
        sql_ += expr;
        return *this;
    }

    auto limit(int n) -> QueryBuilder& {
        sql_ += " LIMIT ";
        sql_ += next_placeholder();
        params_.push_back(std::to_string(n));
        return *this;
    }

    auto offset(int n) -> QueryBuilder& {
        sql_ += " OFFSET ";
        sql_ += next_placeholder();
        params_.push_back(std::to_string(n));
        return *this;
    }

    auto join(std::string_view table_expr, std::string_view on) -> QueryBuilder& {
        sql_ += " JOIN ";
        sql_ += table_expr;
        sql_ += " ON ";
        sql_ += on;
        return *this;
    }

    auto left_join(std::string_view table_expr, std::string_view on) -> QueryBuilder& {
        sql_ += " LEFT JOIN ";
        sql_ += table_expr;
        sql_ += " ON ";
        sql_ += on;
        return *this;
    }

    auto group_by(std::string_view expr) -> QueryBuilder& {
        sql_ += " GROUP BY ";
        sql_ += expr;
        return *this;
    }

    auto having(std::string_view condition) -> QueryBuilder& {
        sql_ += " HAVING ";
        sql_ += condition;
        return *this;
    }

    // ── Build the final parameterized query ──────────────────────
    [[nodiscard]] auto build() -> SqlExpr<Parameterized> {
        return SqlExpr<Parameterized>(
            SqlExpr<Parameterized>::Key{},
            std::move(sql_),
            std::move(params_)
        );
    }
};

// ─── Convenience: start a query builder ─────────────────────────────

inline auto query() -> QueryBuilder { return QueryBuilder(); }
inline auto query(std::string_view base) -> QueryBuilder { return QueryBuilder(std::string(base)); }

// ─── Convenience: common query patterns ─────────────────────────────

// SELECT * FROM "schema"."table" LIMIT n OFFSET o
inline auto select_from(std::string_view schema, std::string_view table,
                        int limit_val, int offset_val = 0) -> SqlExpr<Parameterized> {
    return query()
        .raw("SELECT * FROM ").id(schema, table)
        .limit(limit_val)
        .offset(offset_val)
        .build();
}

// SELECT ctid, * FROM "schema"."table" [ORDER BY ...] LIMIT n OFFSET o
inline auto browse(std::string_view schema, std::string_view table,
                   int limit_val, int offset_val,
                   std::string_view sort_col = "", std::string_view sort_dir = "") -> SqlExpr<Parameterized> {
    auto q = query().raw("SELECT ctid, * FROM ").id(schema, table);
    if (!sort_col.empty()) {
        q.raw(" ORDER BY ").id(sort_col);
        q.raw(sort_dir == "desc" ? " DESC" : " ASC");
    }
    return q.limit(limit_val).offset(offset_val).build();
}

// UPDATE "schema"."table" SET "col" = $1 WHERE ctid = $2
inline auto update_cell(std::string_view schema, std::string_view table,
                        std::string_view col, std::string_view value,
                        std::string_view ctid) -> SqlExpr<Parameterized> {
    return query()
        .raw("UPDATE ").id(schema, table)
        .raw(" SET ").id(col).raw(" = ").val(value)
        .raw(" WHERE ctid = ").val(ctid)
        .build();
}

// DELETE FROM "schema"."table" WHERE ctid = $1
inline auto delete_row(std::string_view schema, std::string_view table,
                       std::string_view ctid) -> SqlExpr<Parameterized> {
    return query()
        .raw("DELETE FROM ").id(schema, table)
        .raw(" WHERE ctid = ").val(ctid)
        .build();
}

// VACUUM "schema"."table"
inline auto vacuum(std::string_view schema, std::string_view table) -> SqlExpr<Parameterized> {
    return query().raw("VACUUM ").id(schema, table).build();
}

// ANALYZE "schema"."table"
inline auto analyze(std::string_view schema, std::string_view table) -> SqlExpr<Parameterized> {
    return query().raw("ANALYZE ").id(schema, table).build();
}

// COUNT(*) FROM "schema"."table"
inline auto count(std::string_view schema, std::string_view table) -> SqlExpr<Parameterized> {
    return query().raw("SELECT COUNT(*) FROM ").id(schema, table).build();
}

// EXPLAIN [ANALYZE] [VERBOSE] user_sql
inline auto explain(std::string_view user_sql, bool do_analyze) -> SqlExpr<Unsafe> {
    std::string s = "EXPLAIN (FORMAT TEXT, COSTS, VERBOSE, BUFFERS";
    if (do_analyze) s += ", ANALYZE, TIMING";
    s += ") ";
    s += user_sql;
    return unsafe(std::move(s));
}

} // namespace getgresql::pg::sql
