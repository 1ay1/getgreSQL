#pragma once

#include "core/expected.hpp"
#include "core/types.hpp"
#include "pg/result.hpp"
#include "pg/sql_dsl.hpp"

#include <concepts>
#include <libpq-fe.h>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace getgresql::pg {

// ─── Phantom state tags ─────────────────────────────────────────────
// These exist only in the type system. They carry no data.
// The state of a connection is encoded in its type parameter.

struct Disconnected {};
struct Connected {};
struct InTransaction {};

// ─── Concepts for valid query states ────────────────────────────────

template <typename S>
concept Queryable = std::same_as<S, Connected> || std::same_as<S, InTransaction>;

template <typename S>
concept CanBeginTxn = std::same_as<S, Connected>;

template <typename S>
concept CanEndTxn = std::same_as<S, InTransaction>;

// ─── PgConnection<State> ────────────────────────────────────────────
// A connection whose lifecycle state is tracked in the type system.
//
// Valid transitions:
//   Disconnected → Connected    (via connect())
//   Connected    → InTransaction (via begin())
//   InTransaction → Connected    (via commit() / rollback())
//   Any          → Disconnected  (via destructor / close())
//
// Invalid operations are compile errors:
//   PgConnection<Disconnected> c; c.exec("SELECT 1"); // ERROR: concept not satisfied
//   PgConnection<InTransaction> c; c.begin();          // ERROR: concept not satisfied

template <typename State>
class PgConnection {
    // Custom deleter for PGconn
    struct PgConnDeleter {
        void operator()(PGconn* conn) const noexcept {
            if (conn) PQfinish(conn);
        }
    };

    std::unique_ptr<PGconn, PgConnDeleter> conn_;

    // Private: only state transitions construct these
    explicit PgConnection(PGconn* raw) noexcept : conn_(raw) {}

    // Allow other states to access internals for transitions
    template <typename> friend class PgConnection;

public:
    PgConnection(PgConnection&&) noexcept = default;
    PgConnection& operator=(PgConnection&&) noexcept = default;

    // Non-copyable (unique ownership of the PGconn*)
    PgConnection(const PgConnection&) = delete;
    PgConnection& operator=(const PgConnection&) = delete;

    // ── Factory: Disconnected → Connected ───────────────────────────
    static auto connect(std::string_view connstr)
        -> Result<PgConnection<Connected>>
        requires std::same_as<State, Disconnected>
    {
        PGconn* raw = PQconnectdb(std::string(connstr).c_str());
        if (!raw) {
            return make_error(PgConnectionError{"Failed to allocate connection"});
        }
        if (PQstatus(raw) != CONNECTION_OK) {
            std::string msg = PQerrorMessage(raw);
            PQfinish(raw);
            return make_error(PgConnectionError{std::move(msg)});
        }
        return PgConnection<Connected>{raw};
    }

    // ── Connected → InTransaction ───────────────────────────────────
    auto begin() &&
        -> Result<PgConnection<InTransaction>>
        requires CanBeginTxn<State>
    {
        PGresult* res = PQexec(conn_.get(), "BEGIN");
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::string msg = PQerrorMessage(conn_.get());
            PQclear(res);
            return make_error(PgQueryError{std::move(msg), "BEGIN"});
        }
        PQclear(res);
        return PgConnection<InTransaction>{conn_.release()};
    }

    // ── InTransaction → Connected ───────────────────────────────────
    auto commit() &&
        -> Result<PgConnection<Connected>>
        requires CanEndTxn<State>
    {
        PGresult* res = PQexec(conn_.get(), "COMMIT");
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::string msg = PQerrorMessage(conn_.get());
            PQclear(res);
            return make_error(PgQueryError{std::move(msg), "COMMIT"});
        }
        PQclear(res);
        return PgConnection<Connected>{conn_.release()};
    }

    auto rollback() &&
        -> Result<PgConnection<Connected>>
        requires CanEndTxn<State>
    {
        PGresult* res = PQexec(conn_.get(), "ROLLBACK");
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::string msg = PQerrorMessage(conn_.get());
            PQclear(res);
            return make_error(PgQueryError{std::move(msg), "ROLLBACK"});
        }
        PQclear(res);
        return PgConnection<Connected>{conn_.release()};
    }

    // ── Query execution (only in Queryable states) ──────────────────
    auto exec(std::string_view sql) const
        -> Result<PgResult>
        requires Queryable<State>
    {
        PGresult* res = PQexec(conn_.get(), std::string(sql).c_str());
        if (!res) {
            return make_error(PgQueryError{
                PQerrorMessage(conn_.get()),
                std::string(sql.substr(0, 200))
            });
        }

        auto status = PQresultStatus(res);
        if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
            std::string msg = PQresultErrorMessage(res);
            PQclear(res);
            return make_error(PgQueryError{
                std::move(msg),
                std::string(sql.substr(0, 200))
            });
        }

        return PgResult{res};
    }

    // ── Parameterized query (SQL injection safe) ────────────────────
    template <typename... Params>
    auto exec_params(std::string_view sql, Params&&... params) const
        -> Result<PgResult>
        requires Queryable<State>
    {
        constexpr int n_params = sizeof...(Params);
        std::string param_values[n_params > 0 ? n_params : 1];
        const char* param_ptrs[n_params > 0 ? n_params : 1];

        // Convert all parameters to strings
        int idx = 0;
        ((param_values[idx] = to_pg_string(std::forward<Params>(params)),
          param_ptrs[idx] = param_values[idx].c_str(),
          ++idx), ...);

        PGresult* res = PQexecParams(
            conn_.get(),
            std::string(sql).c_str(),
            n_params,
            nullptr,        // let PG infer types
            param_ptrs,
            nullptr,        // text format lengths
            nullptr,        // text format
            0               // text result format
        );

        if (!res) {
            return make_error(PgQueryError{
                PQerrorMessage(conn_.get()),
                std::string(sql.substr(0, 200))
            });
        }

        auto status = PQresultStatus(res);
        if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
            std::string msg = PQresultErrorMessage(res);
            PQclear(res);
            return make_error(PgQueryError{
                std::move(msg),
                std::string(sql.substr(0, 200))
            });
        }

        return PgResult{res};
    }

    // ── Type-safe SQL DSL execution ─────────────────────────────────
    // These overloads accept SqlExpr<Safety> from the DSL.
    // Static and Unsafe queries use PQexec (no params).
    // Parameterized queries use PQexecParams ($1/$2 placeholders).

    template<sql::SqlSafety Safety>
    auto exec(const sql::SqlExpr<Safety>& query) const
        -> Result<PgResult>
        requires Queryable<State>
    {
        if constexpr (std::same_as<Safety, sql::Parameterized>) {
            // Use PQexecParams for injection-safe parameterized queries
            auto ptrs = query.param_ptrs();
            PGresult* res = PQexecParams(
                conn_.get(), query.sql().c_str(),
                query.param_count(), nullptr,
                ptrs.data(), nullptr, nullptr, 0);

            if (!res) return make_error(PgQueryError{PQerrorMessage(conn_.get()), query.sql().substr(0, 200)});
            auto status = PQresultStatus(res);
            if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
                std::string msg = PQresultErrorMessage(res);
                PQclear(res);
                return make_error(PgQueryError{std::move(msg), query.sql().substr(0, 200)});
            }
            return PgResult{res};
        } else {
            // Static and Unsafe: plain exec
            return exec(query.view());
        }
    }

    // ── Status queries ──────────────────────────────────────────────
    auto is_alive() const -> bool
        requires Queryable<State>
    {
        return PQstatus(conn_.get()) == CONNECTION_OK;
    }

    auto server_version() const -> int
        requires Queryable<State>
    {
        return PQserverVersion(conn_.get());
    }

    auto backend_pid() const -> int
        requires Queryable<State>
    {
        return PQbackendPID(conn_.get());
    }

    // Raw handle for advanced usage (catalog, monitor queries)
    auto raw() const noexcept -> PGconn*
        requires Queryable<State>
    {
        return conn_.get();
    }

private:
    // Convert C++ types to PG text format strings
    static auto to_pg_string(std::string_view sv) -> std::string { return std::string(sv); }
    static auto to_pg_string(const std::string& s) -> std::string { return s; }
    static auto to_pg_string(const char* s) -> std::string { return s; }
    static auto to_pg_string(int v) -> std::string { return std::to_string(v); }
    static auto to_pg_string(long v) -> std::string { return std::to_string(v); }
    static auto to_pg_string(long long v) -> std::string { return std::to_string(v); }
    static auto to_pg_string(double v) -> std::string { return std::to_string(v); }
    static auto to_pg_string(bool v) -> std::string { return v ? "true" : "false"; }
};

// Convenience alias
using Connection = PgConnection<Connected>;

} // namespace getgresql::pg
