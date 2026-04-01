#pragma once

#include <cstddef>
#include <libpq-fe.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace getgresql::pg {

// ─── Column metadata ────────────────────────────────────────────────

struct ColumnInfo {
    std::string name;
    unsigned int oid;       // PostgreSQL type OID
    int mod;                // Type modifier (e.g., varchar length)
    int col_number;
};

// ─── PgResult: RAII wrapper over PGresult* ──────────────────────────
// Provides typed, safe access to query results with zero-copy
// string_views into the PGresult memory.

class PgResult {
    struct PgResultDeleter {
        void operator()(PGresult* r) const noexcept {
            if (r) PQclear(r);
        }
    };

    std::unique_ptr<PGresult, PgResultDeleter> res_;

public:
    explicit PgResult(PGresult* raw) noexcept : res_(raw) {}

    PgResult(PgResult&&) noexcept = default;
    PgResult& operator=(PgResult&&) noexcept = default;

    // ── Metadata ────────────────────────────────────────────────────

    auto row_count() const noexcept -> int {
        return PQntuples(res_.get());
    }

    auto col_count() const noexcept -> int {
        return PQnfields(res_.get());
    }

    auto column_name(int col) const noexcept -> std::string_view {
        return PQfname(res_.get(), col);
    }

    auto column_info(int col) const -> ColumnInfo {
        return {
            .name = std::string(PQfname(res_.get(), col)),
            .oid = static_cast<unsigned int>(PQftype(res_.get(), col)),
            .mod = PQfmod(res_.get(), col),
            .col_number = col,
        };
    }

    // Source table OID for a result column (0 if computed/expression)
    auto column_table_oid(int col) const noexcept -> unsigned int {
        return static_cast<unsigned int>(PQftable(res_.get(), col));
    }

    // Source column number within the table (0 if computed)
    auto column_table_col(int col) const noexcept -> int {
        return PQftablecol(res_.get(), col);
    }

    auto columns() const -> std::vector<ColumnInfo> {
        std::vector<ColumnInfo> cols;
        cols.reserve(static_cast<std::size_t>(col_count()));
        for (int i = 0; i < col_count(); ++i) {
            cols.push_back(column_info(i));
        }
        return cols;
    }

    // ── Cell access ─────────────────────────────────────────────────

    auto is_null(int row, int col) const noexcept -> bool {
        return PQgetisnull(res_.get(), row, col) == 1;
    }

    auto get(int row, int col) const noexcept -> std::string_view {
        return PQgetvalue(res_.get(), row, col);
    }

    auto get_optional(int row, int col) const noexcept -> std::optional<std::string_view> {
        if (is_null(row, col)) return std::nullopt;
        return PQgetvalue(res_.get(), row, col);
    }

    // ── Typed getters ───────────────────────────────────────────────

    auto get_int(int row, int col) const -> std::optional<long long> {
        if (is_null(row, col)) return std::nullopt;
        try {
            return std::stoll(std::string(get(row, col)));
        } catch (...) {
            return std::nullopt;
        }
    }

    auto get_double(int row, int col) const -> std::optional<double> {
        if (is_null(row, col)) return std::nullopt;
        try {
            return std::stod(std::string(get(row, col)));
        } catch (...) {
            return std::nullopt;
        }
    }

    auto get_bool(int row, int col) const -> std::optional<bool> {
        if (is_null(row, col)) return std::nullopt;
        auto val = get(row, col);
        return val == "t" || val == "true" || val == "1";
    }

    // ── Status ──────────────────────────────────────────────────────

    auto status() const noexcept -> ExecStatusType {
        return PQresultStatus(res_.get());
    }

    auto status_message() const noexcept -> std::string_view {
        return PQresStatus(PQresultStatus(res_.get()));
    }

    auto command_tag() const noexcept -> std::string_view {
        return PQcmdStatus(res_.get());
    }

    auto affected_rows() const noexcept -> std::string_view {
        return PQcmdTuples(res_.get());
    }

    // ── Row iterator ────────────────────────────────────────────────

    class Row {
        const PgResult* result_;
        int row_idx_;
    public:
        Row(const PgResult* r, int idx) : result_(r), row_idx_(idx) {}

        auto operator[](int col) const noexcept -> std::string_view {
            return result_->get(row_idx_, col);
        }
        auto get(int col) const noexcept -> std::string_view {
            return result_->get(row_idx_, col);
        }
        auto is_null(int col) const noexcept -> bool {
            return result_->is_null(row_idx_, col);
        }
        auto col_count() const noexcept -> int {
            return result_->col_count();
        }
        auto index() const noexcept -> int { return row_idx_; }
    };

    class Iterator {
        const PgResult* result_;
        int idx_;
    public:
        Iterator(const PgResult* r, int idx) : result_(r), idx_(idx) {}
        auto operator*() const -> Row { return {result_, idx_}; }
        auto operator++() -> Iterator& { ++idx_; return *this; }
        auto operator!=(const Iterator& other) const -> bool { return idx_ != other.idx_; }
    };

    auto begin() const -> Iterator { return {this, 0}; }
    auto end() const -> Iterator { return {this, row_count()}; }
};

} // namespace getgresql::pg
