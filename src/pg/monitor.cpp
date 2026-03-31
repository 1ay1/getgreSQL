#include "pg/monitor.hpp"

#include <format>

namespace getgresql::pg {

auto server_stats(const Connection& conn) -> Result<ServerStats> {
    ServerStats stats{};

    // Version and PID
    stats.pid = conn.backend_pid();
    {
        auto res = conn.exec("SELECT version()");
        if (res) stats.version = std::string(res->get(0, 0));
    }

    // Uptime
    {
        auto res = conn.exec("SELECT pg_postmaster_start_time(), now() - pg_postmaster_start_time()");
        if (res) stats.uptime = std::string(res->get(0, 1));
    }

    // Connection counts
    {
        auto res = conn.exec(
            "SELECT "
            "  (SELECT setting::int FROM pg_settings WHERE name = 'max_connections') AS max_conn, "
            "  COUNT(*) FILTER (WHERE state = 'active') AS active, "
            "  COUNT(*) FILTER (WHERE state = 'idle') AS idle, "
            "  COUNT(*) FILTER (WHERE state = 'idle in transaction') AS idle_txn, "
            "  COUNT(*) FILTER (WHERE wait_event_type IS NOT NULL AND state = 'active') AS waiting "
            "FROM pg_stat_activity "
            "WHERE backend_type = 'client backend'"
        );
        if (res) {
            stats.max_connections = static_cast<int>(res->get_int(0, 0).value_or(0));
            stats.active_connections = static_cast<int>(res->get_int(0, 1).value_or(0));
            stats.idle_connections = static_cast<int>(res->get_int(0, 2).value_or(0));
            stats.idle_in_transaction = static_cast<int>(res->get_int(0, 3).value_or(0));
            stats.waiting_connections = static_cast<int>(res->get_int(0, 4).value_or(0));
        }
    }

    // Cache hit ratio and transaction stats
    {
        auto res = conn.exec(
            "SELECT "
            "  SUM(xact_commit) AS commits, "
            "  SUM(xact_rollback) AS rollbacks, "
            "  SUM(blks_read) AS blks_read, "
            "  SUM(blks_hit) AS blks_hit, "
            "  CASE WHEN SUM(blks_hit) + SUM(blks_read) > 0 "
            "    THEN SUM(blks_hit)::float / (SUM(blks_hit) + SUM(blks_read)) "
            "    ELSE 0 END AS cache_ratio "
            "FROM pg_stat_database"
        );
        if (res) {
            stats.total_commits = res->get_int(0, 0).value_or(0);
            stats.total_rollbacks = res->get_int(0, 1).value_or(0);
            stats.blocks_read = res->get_int(0, 2).value_or(0);
            stats.blocks_hit = res->get_int(0, 3).value_or(0);
            stats.cache_hit_ratio = res->get_double(0, 4).value_or(0.0);
        }
    }

    return stats;
}

auto active_queries(const Connection& conn) -> Result<std::vector<ActivityEntry>> {
    auto res = conn.exec(
        "SELECT pid, datname, usename, client_addr::text, "
        "state, query, "
        "COALESCE(wait_event_type, '') AS wait_event_type, "
        "COALESCE(wait_event, '') AS wait_event, "
        "backend_start::text, "
        "COALESCE(query_start::text, '') AS query_start, "
        "COALESCE(state_change::text, '') AS state_change, "
        "CASE WHEN query_start IS NOT NULL AND state = 'active' "
        "  THEN (now() - query_start)::text "
        "  ELSE '' END AS duration "
        "FROM pg_stat_activity "
        "WHERE backend_type = 'client backend' "
        "ORDER BY state, query_start"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<ActivityEntry> entries;
    for (auto row : *res) {
        entries.push_back({
            .pid = static_cast<int>(res->get_int(row.index(), 0).value_or(0)),
            .database = std::string(row[1]),
            .user = std::string(row[2]),
            .client_addr = std::string(row.is_null(3) ? "" : row[3]),
            .state = std::string(row[4]),
            .query = std::string(row[5]),
            .wait_event_type = std::string(row[6]),
            .wait_event = std::string(row[7]),
            .backend_start = std::string(row[8]),
            .query_start = std::string(row[9]),
            .state_change = std::string(row[10]),
            .duration = std::string(row[11]),
        });
    }
    return entries;
}

auto all_locks(const Connection& conn) -> Result<std::vector<LockInfo>> {
    auto res = conn.exec(
        "SELECT l.pid, d.datname, COALESCE(c.relname, '') AS relation, "
        "l.mode, l.granted, COALESCE(a.query, '') AS query "
        "FROM pg_locks l "
        "LEFT JOIN pg_database d ON d.oid = l.database "
        "LEFT JOIN pg_class c ON c.oid = l.relation "
        "LEFT JOIN pg_stat_activity a ON a.pid = l.pid "
        "WHERE l.pid != pg_backend_pid() "
        "ORDER BY l.granted, l.pid"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<LockInfo> locks;
    for (auto row : *res) {
        locks.push_back({
            .pid = static_cast<int>(res->get_int(row.index(), 0).value_or(0)),
            .database = std::string(row[1]),
            .relation = std::string(row[2]),
            .mode = std::string(row[3]),
            .granted = res->get_bool(row.index(), 4).value_or(false),
            .query = std::string(row[5]),
        });
    }
    return locks;
}

auto replication_slots(const Connection& conn) -> Result<std::vector<ReplicationSlot>> {
    auto res = conn.exec(
        "SELECT slot_name, slot_type, "
        "COALESCE(database, '') AS database, "
        "active, "
        "COALESCE(restart_lsn::text, '') AS restart_lsn, "
        "COALESCE(confirmed_flush_lsn::text, '') AS confirmed_flush_lsn "
        "FROM pg_replication_slots "
        "ORDER BY slot_name"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<ReplicationSlot> slots;
    for (auto row : *res) {
        slots.push_back({
            .slot_name = std::string(row[0]),
            .slot_type = std::string(row[1]),
            .database = std::string(row[2]),
            .active = res->get_bool(row.index(), 3).value_or(false),
            .restart_lsn = std::string(row[4]),
            .confirmed_flush_lsn = std::string(row[5]),
        });
    }
    return slots;
}

auto table_stats(const Connection& conn, std::string_view schema) -> Result<std::vector<TableStats>> {
    auto sql = std::format(
        "SELECT schemaname, relname, "
        "COALESCE(seq_scan, 0), COALESCE(seq_tup_read, 0), "
        "COALESCE(idx_scan, 0), COALESCE(idx_tup_fetch, 0), "
        "COALESCE(n_tup_ins, 0), COALESCE(n_tup_upd, 0), COALESCE(n_tup_del, 0), "
        "COALESCE(n_live_tup, 0), COALESCE(n_dead_tup, 0), "
        "COALESCE(last_vacuum::text, 'never'), "
        "COALESCE(last_autovacuum::text, 'never'), "
        "COALESCE(last_analyze::text, 'never') "
        "FROM pg_stat_user_tables "
        "WHERE schemaname = '{}' "
        "ORDER BY n_live_tup DESC",
        schema
    );

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<TableStats> result;
    for (auto row : *res) {
        result.push_back({
            .schema = std::string(row[0]),
            .table = std::string(row[1]),
            .seq_scan = res->get_int(row.index(), 2).value_or(0),
            .seq_tup_read = res->get_int(row.index(), 3).value_or(0),
            .idx_scan = res->get_int(row.index(), 4).value_or(0),
            .idx_tup_fetch = res->get_int(row.index(), 5).value_or(0),
            .n_tup_ins = res->get_int(row.index(), 6).value_or(0),
            .n_tup_upd = res->get_int(row.index(), 7).value_or(0),
            .n_tup_del = res->get_int(row.index(), 8).value_or(0),
            .n_live_tup = res->get_int(row.index(), 9).value_or(0),
            .n_dead_tup = res->get_int(row.index(), 10).value_or(0),
            .last_vacuum = std::string(row[11]),
            .last_autovacuum = std::string(row[12]),
            .last_analyze = std::string(row[13]),
        });
    }
    return result;
}

auto cancel_query(const Connection& conn, int pid) -> Result<void> {
    auto res = conn.exec_params(
        "SELECT pg_cancel_backend($1)", pid
    );
    if (!res) return std::unexpected(res.error());
    return {};
}

auto terminate_backend(const Connection& conn, int pid) -> Result<void> {
    auto res = conn.exec_params(
        "SELECT pg_terminate_backend($1)", pid
    );
    if (!res) return std::unexpected(res.error());
    return {};
}

auto server_settings(const Connection& conn, std::string_view search) -> Result<std::vector<PgSetting>> {
    std::string sql;
    if (search.empty()) {
        sql = "SELECT name, setting, COALESCE(unit, ''), category, short_desc, "
              "source, boot_val, reset_val, context "
              "FROM pg_settings "
              "ORDER BY category, name";
    } else {
        sql = std::format(
            "SELECT name, setting, COALESCE(unit, ''), category, short_desc, "
            "source, boot_val, reset_val, context "
            "FROM pg_settings "
            "WHERE name ILIKE '%{}%' OR category ILIKE '%{}%' OR short_desc ILIKE '%{}%' "
            "ORDER BY category, name",
            search, search, search
        );
    }

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<PgSetting> settings;
    for (auto row : *res) {
        settings.push_back({
            .name = std::string(row[0]),
            .setting = std::string(row[1]),
            .unit = std::string(row[2]),
            .category = std::string(row[3]),
            .short_desc = std::string(row[4]),
            .source = std::string(row[5]),
            .boot_val = std::string(row[6]),
            .reset_val = std::string(row[7]),
            .context = std::string(row[8]),
        });
    }
    return settings;
}

auto setting_categories(const Connection& conn) -> Result<std::vector<std::string>> {
    auto res = conn.exec(
        "SELECT DISTINCT category FROM pg_settings ORDER BY category"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<std::string> categories;
    for (auto row : *res) {
        categories.push_back(std::string(row[0]));
    }
    return categories;
}

auto explain_query(const Connection& conn, std::string_view sql, bool analyze) -> Result<ExplainResult> {
    auto explain_sql = std::format("EXPLAIN (FORMAT TEXT, COSTS, VERBOSE, BUFFERS{}) {}",
                                    analyze ? ", ANALYZE, TIMING" : "", sql);
    auto res = conn.exec(explain_sql);
    if (!res) return std::unexpected(res.error());

    ExplainResult result{};
    for (int i = 0; i < res->row_count(); ++i) {
        if (i > 0) result.plan_text += "\n";
        result.plan_text += std::string(res->get(i, 0));
    }

    // Try to extract timing from ANALYZE output
    if (analyze) {
        // Planning Time and Execution Time are in the last rows
        auto plan = result.plan_text;
        if (auto pos = plan.find("Planning Time:"); pos != std::string::npos) {
            try { result.planning_time = std::stod(plan.substr(pos + 15)); } catch (...) {}
        }
        if (auto pos = plan.find("Execution Time:"); pos != std::string::npos) {
            try { result.execution_time = std::stod(plan.substr(pos + 16)); } catch (...) {}
        }
    }

    // Extract total cost from first line
    auto& plan = result.plan_text;
    if (auto pos = plan.find("cost="); pos != std::string::npos) {
        auto dots = plan.find("..", pos + 5);
        if (dots != std::string::npos) {
            auto end = plan.find(' ', dots + 2);
            try { result.total_cost = std::stod(plan.substr(dots + 2, end - dots - 2)); } catch (...) {}
        }
    }

    return result;
}

auto database_activity(const Connection& conn) -> Result<std::vector<DbActivitySummary>> {
    auto res = conn.exec(
        "SELECT d.datname, "
        "COUNT(*) FILTER (WHERE a.state = 'active'), "
        "COUNT(*) FILTER (WHERE a.state = 'idle'), "
        "COUNT(*) FILTER (WHERE a.state = 'idle in transaction'), "
        "COALESCE(s.xact_commit, 0), "
        "COALESCE(s.xact_rollback, 0), "
        "pg_size_pretty(pg_database_size(d.datname)) "
        "FROM pg_database d "
        "LEFT JOIN pg_stat_activity a ON a.datname = d.datname AND a.backend_type = 'client backend' "
        "LEFT JOIN pg_stat_database s ON s.datname = d.datname "
        "WHERE d.datallowconn = true "
        "GROUP BY d.datname, s.xact_commit, s.xact_rollback "
        "ORDER BY d.datname"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<DbActivitySummary> summaries;
    for (auto row : *res) {
        summaries.push_back({
            .database = std::string(row[0]),
            .active = static_cast<int>(res->get_int(row.index(), 1).value_or(0)),
            .idle = static_cast<int>(res->get_int(row.index(), 2).value_or(0)),
            .idle_in_transaction = static_cast<int>(res->get_int(row.index(), 3).value_or(0)),
            .xact_commit = res->get_int(row.index(), 4).value_or(0),
            .xact_rollback = res->get_int(row.index(), 5).value_or(0),
            .size = std::string(row[6]),
        });
    }
    return summaries;
}

// ─── Slow Queries ───────────────────────────────────────────────────

auto slow_queries(const Connection& conn, int threshold_ms) -> Result<std::vector<SlowQuery>> {
    auto sql = std::format(
        "SELECT pid, datname, usename, query, "
        "EXTRACT(EPOCH FROM (now() - query_start))::int || 's' AS duration, "
        "state, query_start::text "
        "FROM pg_stat_activity "
        "WHERE state != 'idle' AND query NOT LIKE '%pg_stat_activity%' "
        "AND (now() - query_start) > interval '{} milliseconds' "
        "ORDER BY query_start ASC",
        threshold_ms
    );
    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<SlowQuery> queries;
    for (auto row : *res) {
        queries.push_back({
            .pid = static_cast<int>(res->get_int(row.index(), 0).value_or(0)),
            .database = std::string(row[1]),
            .user = std::string(row[2]),
            .query = std::string(row[3]),
            .duration = std::string(row[4]),
            .state = std::string(row[5]),
            .started = row.is_null(6) ? std::string() : std::string(row[6]),
        });
    }
    return queries;
}

// ─── Database Sizes (detailed) ──────────────────────────────────────

auto database_sizes(const Connection& conn) -> Result<std::vector<DatabaseSize>> {
    auto res = conn.exec(
        "SELECT d.datname, "
        "pg_database_size(d.datname) AS size_bytes, "
        "pg_size_pretty(pg_database_size(d.datname)) AS size, "
        "COALESCE(s.numbackends, 0) AS connections, "
        "COALESCE(s.xact_commit, 0) AS commits, "
        "COALESCE(s.xact_rollback, 0) AS rollbacks, "
        "CASE WHEN COALESCE(s.blks_hit, 0) + COALESCE(s.blks_read, 0) = 0 THEN 1.0 "
        "  ELSE s.blks_hit::float / (s.blks_hit + s.blks_read) END AS cache_ratio, "
        "COALESCE(s.temp_bytes, 0) AS temp_bytes, "
        "COALESCE(s.deadlocks, 0) AS deadlocks, "
        "COALESCE(s.stats_reset::text, '') AS stats_reset "
        "FROM pg_database d "
        "LEFT JOIN pg_stat_database s ON s.datname = d.datname "
        "WHERE d.datallowconn = true "
        "ORDER BY pg_database_size(d.datname) DESC"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<DatabaseSize> sizes;
    for (auto row : *res) {
        sizes.push_back({
            .name = std::string(row[0]),
            .size_bytes = res->get_int(row.index(), 1).value_or(0),
            .size = std::string(row[2]),
            .connections = res->get_int(row.index(), 3).value_or(0),
            .xact_commit = res->get_int(row.index(), 4).value_or(0),
            .xact_rollback = res->get_int(row.index(), 5).value_or(0),
            .cache_hit_ratio = res->get_double(row.index(), 6).value_or(1.0),
            .temp_bytes = res->get_int(row.index(), 7).value_or(0),
            .deadlocks = res->get_int(row.index(), 8).value_or(0),
            .stats_reset = std::string(row[9]),
        });
    }
    return sizes;
}

// ─── Table Bloat ────────────────────────────────────────────────────

auto table_bloat(const Connection& conn, std::string_view schema) -> Result<std::vector<TableBloat>> {
    auto sql = std::format(
        "SELECT schemaname, relname, "
        "pg_total_relation_size(schemaname || '.' || relname) AS real_size, "
        "pg_size_pretty(pg_total_relation_size(schemaname || '.' || relname)) AS real_size_pretty, "
        "COALESCE(n_dead_tup, 0) * "
        "  CASE WHEN COALESCE(n_live_tup, 0) + COALESCE(n_dead_tup, 0) = 0 THEN 0 "
        "    ELSE pg_total_relation_size(schemaname || '.' || relname)::float / "
        "      (COALESCE(n_live_tup, 1) + COALESCE(n_dead_tup, 0)) END AS bloat_est, "
        "COALESCE(n_dead_tup::float / NULLIF(n_live_tup + n_dead_tup, 0), 0) AS bloat_ratio "
        "FROM pg_stat_user_tables "
        "WHERE schemaname = '{}' "
        "ORDER BY bloat_ratio DESC",
        schema
    );
    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<TableBloat> bloat;
    for (auto row : *res) {
        auto bloat_bytes = static_cast<long long>(res->get_double(row.index(), 4).value_or(0));
        auto format_sz = [](long long b) -> std::string {
            if (b >= 1073741824LL) return std::format("{:.1f} GB", static_cast<double>(b) / 1073741824.0);
            if (b >= 1048576LL) return std::format("{:.1f} MB", static_cast<double>(b) / 1048576.0);
            if (b >= 1024LL) return std::format("{:.1f} KB", static_cast<double>(b) / 1024.0);
            return std::format("{} B", b);
        };
        bloat.push_back({
            .schema = std::string(row[0]),
            .table = std::string(row[1]),
            .real_size = res->get_int(row.index(), 2).value_or(0),
            .real_size_pretty = std::string(row[3]),
            .bloat_size = bloat_bytes,
            .bloat_size_pretty = format_sz(bloat_bytes),
            .bloat_ratio = res->get_double(row.index(), 5).value_or(0.0),
        });
    }
    return bloat;
}

// ─── Blocking Chains ────────────────────────────────────────────────

auto blocking_chains(const Connection& conn) -> Result<std::vector<BlockingChain>> {
    auto res = conn.exec(
        "SELECT blocked.pid AS blocked_pid, "
        "blocked_activity.usename AS blocked_user, "
        "blocked_activity.query AS blocked_query, "
        "EXTRACT(EPOCH FROM (now() - blocked_activity.query_start))::int || 's' AS blocked_duration, "
        "blocking.pid AS blocking_pid, "
        "blocking_activity.usename AS blocking_user, "
        "blocking_activity.query AS blocking_query, "
        "EXTRACT(EPOCH FROM (now() - blocking_activity.query_start))::int || 's' AS blocking_duration "
        "FROM pg_locks blocked "
        "JOIN pg_stat_activity blocked_activity ON blocked.pid = blocked_activity.pid "
        "JOIN pg_locks blocking ON blocking.locktype = blocked.locktype "
        "  AND blocking.database IS NOT DISTINCT FROM blocked.database "
        "  AND blocking.relation IS NOT DISTINCT FROM blocked.relation "
        "  AND blocking.page IS NOT DISTINCT FROM blocked.page "
        "  AND blocking.tuple IS NOT DISTINCT FROM blocked.tuple "
        "  AND blocking.pid != blocked.pid "
        "JOIN pg_stat_activity blocking_activity ON blocking.pid = blocking_activity.pid "
        "WHERE NOT blocked.granted AND blocking.granted"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<BlockingChain> chains;
    for (auto row : *res) {
        chains.push_back({
            .blocked_pid = static_cast<int>(res->get_int(row.index(), 0).value_or(0)),
            .blocked_user = std::string(row[1]),
            .blocked_query = std::string(row[2]),
            .blocked_duration = std::string(row[3]),
            .blocking_pid = static_cast<int>(res->get_int(row.index(), 4).value_or(0)),
            .blocking_user = std::string(row[5]),
            .blocking_query = std::string(row[6]),
            .blocking_duration = std::string(row[7]),
        });
    }
    return chains;
}

// ─── WAL Stats ──────────────────────────────────────────────────────

auto wal_stats(const Connection& conn) -> Result<WALStats> {
    auto res = conn.exec(
        "SELECT pg_current_wal_lsn()::text, "
        "current_setting('wal_level'), "
        "current_setting('wal_buffers'), "
        "current_setting('checkpoint_timeout'), "
        "COALESCE(checkpoint_time::text, '') AS last_checkpoint, "
        "checkpoints_timed, checkpoints_req, "
        "checkpoint_write_time, checkpoint_sync_time, "
        "buffers_checkpoint, buffers_backend "
        "FROM pg_stat_bgwriter"
    );
    if (!res) return std::unexpected(res.error());
    if (res->row_count() == 0) return WALStats{};

    return WALStats{
        .current_lsn = std::string(res->get(0, 0)),
        .wal_level = std::string(res->get(0, 1)),
        .wal_buffers = res->get_int(0, 2).value_or(0),
        .checkpoint_timeout = std::string(res->get(0, 3)),
        .last_checkpoint = std::string(res->get(0, 4)),
        .checkpoints_timed = res->get_int(0, 5).value_or(0),
        .checkpoints_req = res->get_int(0, 6).value_or(0),
        .checkpoint_write_time = res->get_double(0, 7).value_or(0.0),
        .checkpoint_sync_time = res->get_double(0, 8).value_or(0.0),
        .buffers_checkpoint = res->get_int(0, 9).value_or(0),
        .buffers_backend = res->get_int(0, 10).value_or(0),
    };
}

// ─── Vacuum Progress ────────────────────────────────────────────────

auto vacuum_progress(const Connection& conn) -> Result<std::vector<VacuumProgress>> {
    auto res = conn.exec(
        "SELECT p.pid, a.datname, n.nspname, c.relname, p.phase, "
        "p.heap_blks_total, p.heap_blks_scanned, p.heap_blks_vacuumed, "
        "CASE WHEN p.heap_blks_total > 0 THEN "
        "  p.heap_blks_vacuumed::float / p.heap_blks_total * 100 ELSE 0 END AS pct "
        "FROM pg_stat_progress_vacuum p "
        "JOIN pg_stat_activity a ON a.pid = p.pid "
        "JOIN pg_class c ON c.oid = p.relid "
        "JOIN pg_namespace n ON n.oid = c.relnamespace "
        "ORDER BY p.pid"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<VacuumProgress> progress;
    for (auto row : *res) {
        progress.push_back({
            .pid = static_cast<int>(res->get_int(row.index(), 0).value_or(0)),
            .database = std::string(row[1]),
            .schema = std::string(row[2]),
            .table = std::string(row[3]),
            .phase = std::string(row[4]),
            .heap_blks_total = res->get_int(row.index(), 5).value_or(0),
            .heap_blks_scanned = res->get_int(row.index(), 6).value_or(0),
            .heap_blks_vacuumed = res->get_int(row.index(), 7).value_or(0),
            .percent_complete = res->get_double(row.index(), 8).value_or(0.0),
        });
    }
    return progress;
}

// ─── Health Checks ──────────────────────────────────────────────────

auto health_checks(const Connection& conn) -> Result<std::vector<HealthCheck>> {
    std::vector<HealthCheck> checks;

    // 1. Cache hit ratio
    auto cache = conn.exec(
        "SELECT COALESCE(SUM(blks_hit)::float / NULLIF(SUM(blks_hit) + SUM(blks_read), 0) * 100, 100) "
        "FROM pg_stat_database"
    );
    if (cache && cache->row_count() > 0) {
        auto ratio = cache->get_double(0, 0).value_or(100.0);
        checks.push_back({
            .name = "Cache Hit Ratio",
            .status = ratio >= 99 ? "ok" : ratio >= 90 ? "warning" : "critical",
            .value = std::format("{:.1f}%", ratio),
            .detail = ratio < 90 ? "Consider increasing shared_buffers" : "Healthy",
        });
    }

    // 2. Connection usage
    auto conns = conn.exec(
        "SELECT count(*), current_setting('max_connections')::int "
        "FROM pg_stat_activity"
    );
    if (conns && conns->row_count() > 0) {
        auto used = conns->get_int(0, 0).value_or(0);
        auto max = conns->get_int(0, 1).value_or(100);
        auto pct = max > 0 ? (static_cast<double>(used) / max * 100) : 0;
        checks.push_back({
            .name = "Connection Usage",
            .status = pct < 70 ? "ok" : pct < 90 ? "warning" : "critical",
            .value = std::format("{}/{} ({:.0f}%)", used, max, pct),
            .detail = pct >= 90 ? "Near max_connections limit" : "Normal",
        });
    }

    // 3. Long running transactions
    auto long_txn = conn.exec(
        "SELECT count(*), COALESCE(max(EXTRACT(EPOCH FROM (now() - xact_start)))::int, 0) "
        "FROM pg_stat_activity "
        "WHERE state = 'idle in transaction' AND xact_start < now() - interval '5 minutes'"
    );
    if (long_txn && long_txn->row_count() > 0) {
        auto cnt = long_txn->get_int(0, 0).value_or(0);
        auto maxsec = long_txn->get_int(0, 1).value_or(0);
        checks.push_back({
            .name = "Idle-in-Transaction",
            .status = cnt == 0 ? "ok" : cnt <= 2 ? "warning" : "critical",
            .value = std::format("{} sessions", cnt),
            .detail = cnt > 0 ? std::format("Longest: {}s — may hold locks", maxsec) : "None",
        });
    }

    // 4. Dead rows (needs vacuum)
    auto dead = conn.exec(
        "SELECT count(*) FROM pg_stat_user_tables "
        "WHERE n_dead_tup > n_live_tup * 0.2 AND n_live_tup > 100"
    );
    if (dead && dead->row_count() > 0) {
        auto cnt = dead->get_int(0, 0).value_or(0);
        checks.push_back({
            .name = "Tables Needing Vacuum",
            .status = cnt == 0 ? "ok" : cnt <= 3 ? "warning" : "critical",
            .value = std::format("{} tables", cnt),
            .detail = cnt > 0 ? "High dead row ratio (>20%)" : "All tables healthy",
        });
    }

    // 5. Replication lag
    auto rep = conn.exec(
        "SELECT slot_name, NOT active AS lagging FROM pg_replication_slots"
    );
    if (rep) {
        int inactive = 0;
        for (auto row : *rep) {
            if (rep->get_bool(row.index(), 1).value_or(false)) inactive++;
        }
        if (rep->row_count() > 0) {
            checks.push_back({
                .name = "Replication Slots",
                .status = inactive == 0 ? "ok" : "critical",
                .value = std::format("{}/{} active", rep->row_count() - inactive, rep->row_count()),
                .detail = inactive > 0 ? "Inactive slots accumulate WAL" : "All slots active",
            });
        }
    }

    // 6. Deadlocks
    auto dlocks = conn.exec(
        "SELECT COALESCE(SUM(deadlocks), 0) FROM pg_stat_database"
    );
    if (dlocks && dlocks->row_count() > 0) {
        auto cnt = dlocks->get_int(0, 0).value_or(0);
        checks.push_back({
            .name = "Deadlocks (total)",
            .status = cnt == 0 ? "ok" : "warning",
            .value = std::to_string(cnt),
            .detail = cnt > 0 ? "Review application lock ordering" : "No deadlocks recorded",
        });
    }

    return checks;
}

} // namespace getgresql::pg
