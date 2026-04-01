#pragma once

#include "pg/result.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace getgresql::pg {

// ─── Cached query result for batch row serving ──────────────────────
// After executing a query, the result is stored here so the row batch
// endpoint can serve rows without re-executing.

struct CachedResult {
    std::shared_ptr<PgResult> result;
    bool has_ctid;                // whether column 0 is ctid
    int col_start;                // first real data column (1 if has_ctid, 0 otherwise)
    std::string db_name;          // for edit URLs
    std::chrono::steady_clock::time_point expires;
};

class ResultCache {
    std::mutex mu_;
    std::unordered_map<std::string, CachedResult> cache_;
    std::uint64_t counter_ = 0;

public:
    // Store a result and return its cache ID
    auto store(PgResult result, bool has_ctid, int col_start,
               std::string_view db_name,
               std::chrono::seconds ttl = std::chrono::seconds(120)) -> std::string {
        std::lock_guard lock(mu_);
        evict_expired();
        auto id = std::to_string(++counter_);
        cache_[id] = CachedResult{
            .result = std::make_shared<PgResult>(std::move(result)),
            .has_ctid = has_ctid,
            .col_start = col_start,
            .db_name = std::string(db_name),
            .expires = std::chrono::steady_clock::now() + ttl,
        };
        return id;
    }

    // Retrieve a cached result (nullptr if expired/missing)
    auto get(const std::string& id) -> CachedResult* {
        std::lock_guard lock(mu_);
        auto it = cache_.find(id);
        if (it == cache_.end()) return nullptr;
        if (std::chrono::steady_clock::now() > it->second.expires) {
            cache_.erase(it);
            return nullptr;
        }
        return &it->second;
    }

    // Remove a specific result
    auto remove(const std::string& id) -> void {
        std::lock_guard lock(mu_);
        cache_.erase(id);
    }

private:
    void evict_expired() {
        auto now = std::chrono::steady_clock::now();
        for (auto it = cache_.begin(); it != cache_.end(); ) {
            if (now > it->second.expires) it = cache_.erase(it);
            else ++it;
        }
    }
};

// Global instance
inline auto result_cache() -> ResultCache& {
    static ResultCache instance;
    return instance;
}

} // namespace getgresql::pg
