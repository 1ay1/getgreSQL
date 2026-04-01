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

struct PgSetting {
    std::string name;
    std::string setting;
    std::string unit;
    std::string category;
    std::string short_desc;
    std::string source;       // "default", "configuration file", etc.
    std::string boot_val;
    std::string reset_val;
    std::string context;      // "postmaster", "sighup", "superuser", "user"
};

struct ExplainResult {
    std::string plan_text;     // the full EXPLAIN output as text
    double planning_time;      // ms, only if ANALYZE
    double execution_time;     // ms, only if ANALYZE
    double total_cost;
};

struct DbActivitySummary {
    std::string database;
    int active;
    int idle;
    int idle_in_transaction;
    long long xact_commit;
    long long xact_rollback;
    std::string size;
};

// ─── Deep analytics types ───────────────────────────────────────────

struct SlowQuery {
    int pid;
    std::string database;
    std::string user;
    std::string query;
    std::string duration;
    std::string state;
    std::string started;
};

struct DatabaseSize {
    std::string name;
    long long size_bytes;
    std::string size;
    long long connections;
    long long xact_commit;
    long long xact_rollback;
    double cache_hit_ratio;
    long long temp_bytes;
    long long deadlocks;
    std::string stats_reset;
};

struct TableBloat {
    std::string schema;
    std::string table;
    long long real_size;
    std::string real_size_pretty;
    long long bloat_size;
    std::string bloat_size_pretty;
    double bloat_ratio;   // 0.0 - 1.0
};

struct BlockingChain {
    int blocked_pid;
    std::string blocked_user;
    std::string blocked_query;
    std::string blocked_duration;
    int blocking_pid;
    std::string blocking_user;
    std::string blocking_query;
    std::string blocking_duration;
};

struct WALStats {
    std::string current_lsn;
    std::string wal_level;
    long long wal_buffers;
    std::string checkpoint_timeout;
    std::string last_checkpoint;
    long long checkpoints_timed;
    long long checkpoints_req;
    double checkpoint_write_time;
    double checkpoint_sync_time;
    long long buffers_checkpoint;
    long long buffers_backend;
};

struct VacuumProgress {
    int pid;
    std::string database;
    std::string schema;
    std::string table;
    std::string phase;
    long long heap_blks_total;
    long long heap_blks_scanned;
    long long heap_blks_vacuumed;
    double percent_complete;
};

struct IndexBloat {
    std::string schema;
    std::string table;
    std::string index;
    long long real_size;
    std::string real_size_pretty;
    long long bloat_size;
    std::string bloat_size_pretty;
    double bloat_ratio;
};

struct HealthCheck {
    std::string name;
    std::string status;    // "ok", "warning", "critical"
    std::string value;
    std::string detail;
    std::string fix_action; // e.g. "vacuum-all", "terminate-idle", "drop-inactive-slots"
    std::string fix_label;  // e.g. "Run VACUUM", "Terminate", "Drop Slots"
};

// ─── Monitor queries ────────────────────────────────────────────────

auto server_stats(const Connection& conn) -> Result<ServerStats>;
auto active_queries(const Connection& conn) -> Result<std::vector<ActivityEntry>>;
auto all_locks(const Connection& conn) -> Result<std::vector<LockInfo>>;
auto replication_slots(const Connection& conn) -> Result<std::vector<ReplicationSlot>>;
auto table_stats(const Connection& conn, std::string_view schema = "public") -> Result<std::vector<TableStats>>;

// Cancel a running query by backend PID
auto cancel_query(const Connection& conn, int pid) -> Result<void>;

// Terminate a backend by PID
auto terminate_backend(const Connection& conn, int pid) -> Result<void>;

// Server configuration settings
auto server_settings(const Connection& conn, std::string_view search = "") -> Result<std::vector<PgSetting>>;
auto setting_categories(const Connection& conn) -> Result<std::vector<std::string>>;

// Query plan analysis
auto explain_query(const Connection& conn, std::string_view sql, bool analyze = false) -> Result<ExplainResult>;

// Per-database activity summary
auto database_activity(const Connection& conn) -> Result<std::vector<DbActivitySummary>>;

// Deep analytics
auto slow_queries(const Connection& conn, int threshold_ms = 1000) -> Result<std::vector<SlowQuery>>;
auto database_sizes(const Connection& conn) -> Result<std::vector<DatabaseSize>>;
auto table_bloat(const Connection& conn, std::string_view schema = "public") -> Result<std::vector<TableBloat>>;
auto blocking_chains(const Connection& conn) -> Result<std::vector<BlockingChain>>;
auto wal_stats(const Connection& conn) -> Result<WALStats>;
auto vacuum_progress(const Connection& conn) -> Result<std::vector<VacuumProgress>>;
auto health_checks(const Connection& conn) -> Result<std::vector<HealthCheck>>;

} // namespace getgresql::pg
