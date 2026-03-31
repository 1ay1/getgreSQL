#pragma once

#include "core/expected.hpp"
#include "pg/connection.hpp"

#include <string>
#include <vector>

namespace getgresql::pg {

// ─── Server-level monitoring data ───────────────────────────────────

struct ServerStats {
    std::string version;
    int pid;
    std::string uptime;
    int max_connections;
    int active_connections;
    int idle_connections;
    int idle_in_transaction;
    int waiting_connections;
    double cache_hit_ratio;     // 0.0 to 1.0
    long long total_commits;
    long long total_rollbacks;
    long long blocks_read;
    long long blocks_hit;
};

struct ActivityEntry {
    int pid;
    std::string database;
    std::string user;
    std::string client_addr;
    std::string state;
    std::string query;
    std::string wait_event_type;
    std::string wait_event;
    std::string backend_start;
    std::string query_start;
    std::string state_change;
    std::string duration;       // how long the current query has been running
};

struct LockInfo {
    int pid;
    std::string database;
    std::string relation;
    std::string mode;
    bool granted;
    std::string query;
};

struct ReplicationSlot {
    std::string slot_name;
    std::string slot_type;
    std::string database;
    bool active;
    std::string restart_lsn;
    std::string confirmed_flush_lsn;
};

struct TableStats {
    std::string schema;
    std::string table;
    long long seq_scan;
    long long seq_tup_read;
    long long idx_scan;
    long long idx_tup_fetch;
    long long n_tup_ins;
    long long n_tup_upd;
    long long n_tup_del;
    long long n_live_tup;
    long long n_dead_tup;
    std::string last_vacuum;
    std::string last_autovacuum;
    std::string last_analyze;
};

// ─── Monitor queries ────────────────────────────────────────────────

auto server_stats(const Connection& conn) -> Result<ServerStats>;
auto active_queries(const Connection& conn) -> Result<std::vector<ActivityEntry>>;
auto all_locks(const Connection& conn) -> Result<std::vector<LockInfo>>;
auto replication_slots(const Connection& conn) -> Result<std::vector<ReplicationSlot>>;
auto table_stats(const Connection& conn, std::string_view schema = "public") -> Result<std::vector<TableStats>>;

// Cancel a running query by backend PID
auto cancel_query(const Connection& conn, int pid) -> Result<void>;

} // namespace getgresql::pg
