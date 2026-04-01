#include "pg/pool.hpp"

#include <chrono>

namespace getgresql::pg {

PoolGuard::~PoolGuard() {
    if (pool_ && conn_) {
        pool_->return_connection(std::move(conn_));
    }
}

void Pool::return_connection(std::unique_ptr<PgConnection<Connected>> conn) {
    std::lock_guard lock(mutex_);
    // Check if connection is still alive before returning to pool
    if (conn && conn->is_alive()) {
        idle_.push_back(std::move(conn));
    }
    --active_count_;
    cv_.notify_one();
}

auto Pool::checkout() -> Result<PoolGuard> {
    std::unique_lock lock(mutex_);

    // Wait until a connection is available or we can create one
    cv_.wait(lock, [this] {
        return !idle_.empty() || active_count_ < max_size_;
    });

    if (!idle_.empty()) {
        auto conn = std::move(idle_.back());
        idle_.pop_back();
        ++active_count_;

        // Verify the connection is still good
        if (!conn->is_alive()) {
            // Dead connection — try to create a new one
            --active_count_; // will re-increment below
            lock.unlock();
            auto new_conn = PgConnection<Disconnected>::connect(connstr_);
            if (!new_conn) return std::unexpected(new_conn.error());
            lock.lock();
            ++active_count_;
            return PoolGuard{this,
                std::make_unique<PgConnection<Connected>>(std::move(*new_conn))};
        }

        return PoolGuard{this, std::move(conn)};
    }

    // Create a new connection
    ++active_count_;
    lock.unlock();

    auto conn = PgConnection<Disconnected>::connect(connstr_);
    if (!conn) {
        lock.lock();
        --active_count_;
        cv_.notify_one();
        return std::unexpected(conn.error());
    }

    return PoolGuard{this,
        std::make_unique<PgConnection<Connected>>(std::move(*conn))};
}

auto Pool::warm(std::size_t count) -> Result<void> {
    for (std::size_t i = 0; i < count && i < max_size_; ++i) {
        auto conn = PgConnection<Disconnected>::connect(connstr_);
        if (!conn) return std::unexpected(conn.error());

        std::lock_guard lock(mutex_);
        idle_.push_back(
            std::make_unique<PgConnection<Connected>>(std::move(*conn)));
    }
    return {};
}

auto Pool::idle_count() const -> std::size_t {
    std::lock_guard lock(const_cast<std::mutex&>(mutex_));
    return idle_.size();
}

auto Pool::active_count() const -> std::size_t {
    std::lock_guard lock(const_cast<std::mutex&>(mutex_));
    return active_count_;
}

auto Pool::reconnect(std::string new_connstr) -> Result<void> {
    // Drain idle connections and switch to new connection string
    {
        std::lock_guard lock(mutex_);
        idle_.clear();
        connstr_ = std::move(new_connstr);
    }
    // Warm with 2 connections to verify the new string works
    return warm(2);
}

} // namespace getgresql::pg
