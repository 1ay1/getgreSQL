#pragma once

#include "core/expected.hpp"
#include "pg/connection.hpp"

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace getgresql::pg {

// ─── Connection pool ────────────────────────────────────────────────
// RAII-guarded checkout: when PoolGuard is destroyed, the connection
// returns to the pool automatically. The guard provides a reference
// to PgConnection<Connected>, so all type-state safety is preserved.

class Pool;

class PoolGuard {
    Pool* pool_;
    std::unique_ptr<PgConnection<Connected>> conn_;
    friend class Pool;

    PoolGuard(Pool* p, std::unique_ptr<PgConnection<Connected>> c)
        : pool_(p), conn_(std::move(c)) {}

public:
    PoolGuard(PoolGuard&&) noexcept = default;
    PoolGuard& operator=(PoolGuard&&) noexcept = default;
    ~PoolGuard();

    auto operator->() const -> PgConnection<Connected>* { return conn_.get(); }
    auto operator*() const -> PgConnection<Connected>& { return *conn_; }
    auto get() const -> PgConnection<Connected>& { return *conn_; }
};

class Pool {
    std::string connstr_;
    std::size_t max_size_;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::unique_ptr<PgConnection<Connected>>> idle_;
    std::size_t active_count_ = 0;

    friend class PoolGuard;

    void return_connection(std::unique_ptr<PgConnection<Connected>> conn);

public:
    Pool(std::string connstr, std::size_t max_size = 10)
        : connstr_(std::move(connstr)), max_size_(max_size) {}

    // Checkout a connection (blocks if pool is exhausted)
    auto checkout() -> Result<PoolGuard>;

    // Pre-warm the pool with N connections
    auto warm(std::size_t count) -> Result<void>;

    // Reconnect: drain idle connections, switch to a new connection string
    auto reconnect(std::string new_connstr) -> Result<void>;

    // Current connection string
    auto connstr() const -> const std::string& { return connstr_; }

    // Stats
    auto idle_count() const -> std::size_t;
    auto active_count() const -> std::size_t;
    auto max_size() const -> std::size_t { return max_size_; }
};

} // namespace getgresql::pg
