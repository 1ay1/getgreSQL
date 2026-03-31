#include "api/handlers_monitor.hpp"
#include "ssr/components.hpp"
#include "pg/monitor.hpp"

#include <format>

namespace getgresql::api {

using namespace ssr;

// Helper: escape a string for use in table cells (returns std::string)
static auto escape(std::string_view s) -> std::string {
    auto h = Html::with_capacity(s.size() + 32);
    h.text(s);
    return std::move(h).finish();
}

auto MonitorPageHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    auto h = Html::with_capacity(4096);

    // Health checks panel
    h.raw("<div id=\"health-panel\" hx-get=\"/monitor/health\" hx-trigger=\"load, every 10s\" hx-swap=\"innerHTML\">"
          "<div class=\"loading\">Checking health...</div></div>");

    // Stats
    h.raw("<div id=\"monitor-stats\" hx-get=\"/monitor/stats\" hx-trigger=\"load, every 3s\" hx-swap=\"innerHTML\">"
          "<div class=\"loading\">Loading stats...</div></div>");

    // Section tabs for different monitoring views
    SectionTabs::render({{
        {"Active Queries", "/monitor/activity", true},
        {"Slow Queries", "/monitor/slow"},
        {"Blocking", "/monitor/blocking"},
        {"Vacuum Progress", "/monitor/vacuum-progress"},
        {"WAL", "/monitor/wal"},
        {"Databases", "/monitor/databases"},
        {"Bloat", "/monitor/bloat"},
    }}, "monitor-content", h);

    h.raw("<div id=\"monitor-content\" hx-get=\"/monitor/activity\" hx-trigger=\"load\" hx-swap=\"innerHTML\">"
          "<div class=\"loading\">Loading...</div></div>");

    if (req.is_htmx()) return Response::html(std::move(h).finish());
    return Response::html(render_page("Monitor", "Monitor", [&](Html& ph) {
        ph.raw(h.view());
    }));
}

auto MonitorStatsHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto stats = pg::server_stats(conn->get());
    if (!stats) return Response::html(render_to_string<Alert>({error_message(stats.error()), "error"}));

    auto& s = *stats;
    auto h = Html::with_capacity(2048);
    h.raw("<div class=\"stat-grid\">");
    StatCard::render({"Active", std::to_string(s.active_connections), ""}, h);
    StatCard::render({"Idle", std::to_string(s.idle_connections)}, h);
    StatCard::render({"Idle in Txn", std::to_string(s.idle_in_transaction),
                       s.idle_in_transaction > 0 ? "warning" : ""}, h);
    StatCard::render({"Waiting", std::to_string(s.waiting_connections),
                       s.waiting_connections > 0 ? "warning" : ""}, h);
    StatCard::render({"Cache Hit",
        std::format("{:.1f}%", s.cache_hit_ratio * 100),
        s.cache_hit_ratio < 0.90 ? "danger" : "success"}, h);
    StatCard::render({"Max Connections", std::to_string(s.max_connections)}, h);
    StatCard::render({"Commits", std::to_string(s.total_commits)}, h);
    StatCard::render({"Rollbacks", std::to_string(s.total_rollbacks),
                       s.total_rollbacks > 0 ? "warning" : ""}, h);
    h.raw("</div>");

    return Response::html(std::move(h).finish());
}

auto MonitorActivityHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto activity = pg::active_queries(conn->get());
    if (!activity) return Response::html(render_to_string<Alert>({error_message(activity.error()), "error"}));

    if (activity->empty()) {
        return Response::html(R"(<div class="empty-state">No active queries</div>)");
    }

    auto h = Html::with_capacity(8192);
    Table::begin(h, {{
        {"PID", "num"}, {"Database", ""}, {"User", ""}, {"State", ""},
        {"Duration", "num"}, {"Wait", ""}, {"Query", "wide"}, {"", ""}, {"", ""}
    }});

    for (auto& a : *activity) {
        auto state_variant = a.state == "active" ? "primary"
                           : a.state == "idle in transaction" ? "warning"
                           : "secondary";

        auto query_preview = a.query.size() > 120
            ? escape(a.query.substr(0, 120)) + "..."
            : escape(a.query);

        std::string cancel_btn;
        if (a.state == "active") {
            cancel_btn = std::format(
                R"(<button class="btn btn-sm btn-danger" hx-post="/monitor/cancel/{}" hx-confirm="Cancel query on PID {}?">Cancel</button>)",
                a.pid, a.pid
            );
        }

        std::string terminate_btn;
        if (a.state == "active" || a.state == "idle in transaction") {
            terminate_btn = std::format(
                R"(<button class="btn btn-sm btn-danger" hx-post="/monitor/terminate/{}" hx-confirm="Force terminate PID {}?">Kill</button>)",
                a.pid, a.pid
            );
        }

        Table::row(h, {{
            std::to_string(a.pid),
            escape(a.database),
            escape(a.user),
            render_to_string<Badge>({a.state, state_variant}),
            escape(a.duration),
            a.wait_event.empty() ? std::string("&mdash;")
                : std::format("{}/{}", escape(a.wait_event_type), escape(a.wait_event)),
            std::format(R"(<code class="query-preview">{}</code>)", query_preview),
            cancel_btn,
            terminate_btn,
        }});
    }
    Table::end(h);

    return Response::html(std::move(h).finish());
}

auto MonitorLocksHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto locks = pg::all_locks(conn->get());
    if (!locks) return Response::error(error_message(locks.error()));

    auto h = Html::with_capacity(8192);

    if (locks->empty()) {
        h.raw(R"(<div class="empty-state">No locks detected</div>)");
    } else {
        Table::begin(h, {{
            {"PID", "num"}, {"Database", ""}, {"Relation", ""},
            {"Mode", ""}, {"Granted", ""}, {"Query", "wide"}
        }});
        for (auto& l : *locks) {
            Table::row(h, {{
                std::to_string(l.pid),
                escape(l.database),
                l.relation.empty() ? std::string("&mdash;") : escape(l.relation),
                render_to_string<Badge>({l.mode, l.granted ? "secondary" : "danger"}),
                l.granted ? std::string("&#10003;") : render_to_string<Badge>({"WAITING", "danger"}),
                std::format("<code class=\"query-preview\">{}</code>",
                    escape(l.query.size() > 100 ? l.query.substr(0, 100) + "..." : l.query)),
            }});
        }
        Table::end(h);
    }

    if (req.is_htmx()) return Response::html(std::move(h).finish());
    return Response::html(render_page("Locks", "Monitor", [&](Html& ph) {
        ph.raw(h.view());
    }));
}

auto CancelQueryHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto pid_str = req.param("pid");
    int pid = 0;
    try { pid = std::stoi(std::string(pid_str)); }
    catch (...) { return Response::error("Invalid PID", 400); }

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto result = pg::cancel_query(conn->get(), pid);
    if (!result) {
        return Response::html(render_to_string<Alert>({error_message(result.error()), "error"}));
    }

    return Response::html(render_to_string<Alert>({std::format("Cancelled query on PID {}", pid), "info"}));
}

// ─── HealthCheckHandler ──────────────────────────────────────────────

auto HealthCheckHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto checks = pg::health_checks(conn->get());
    if (!checks) return Response::html(render_to_string<Alert>({error_message(checks.error()), "error"}));

    auto h = Html::with_capacity(4096);
    h.raw("<div class=\"health-grid\">");
    for (auto& c : *checks) {
        HealthCard::render({c.name, c.status, c.value, c.detail}, h);
    }
    h.raw("</div>");
    return Response::html(std::move(h).finish());
}

// ─── SlowQueriesHandler ─────────────────────────────────────────────

auto SlowQueriesHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto queries = pg::slow_queries(conn->get(), 500);
    if (!queries) return Response::html(render_to_string<Alert>({error_message(queries.error()), "error"}));

    if (queries->empty()) {
        return Response::html("<div class=\"empty-state\">No slow queries (>500ms) running</div>");
    }

    auto h = Html::with_capacity(8192);
    Table::begin(h, {{
        {"PID", "num"}, {"Database", ""}, {"User", ""}, {"Duration", "num"},
        {"State", ""}, {"Query", "wide"}, {"", ""}
    }});

    for (auto& q : *queries) {
        auto query_preview = q.query.size() > 150
            ? escape(q.query.substr(0, 150)) + "..."
            : escape(q.query);

        Table::row(h, {{
            std::to_string(q.pid),
            escape(q.database),
            escape(q.user),
            escape(q.duration),
            render_to_string<Badge>({q.state, q.state == "active" ? "primary" : "warning"}),
            std::format("<code class=\"query-preview\">{}</code>", query_preview),
            std::format("<button class=\"btn btn-sm btn-danger\" hx-post=\"/monitor/cancel/{}\" hx-confirm=\"Cancel query PID {}?\">Cancel</button>", q.pid, q.pid),
        }});
    }
    Table::end(h);
    return Response::html(std::move(h).finish());
}

// ─── BlockingHandler ────────────────────────────────────────────────

auto BlockingHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto chains = pg::blocking_chains(conn->get());
    if (!chains) return Response::html(render_to_string<Alert>({error_message(chains.error()), "error"}));

    if (chains->empty()) {
        return Response::html("<div class=\"empty-state\">No blocking detected</div>");
    }

    auto h = Html::with_capacity(8192);
    Table::begin(h, {{
        {"Blocked PID", "num"}, {"Blocked User", ""}, {"Blocked Duration", "num"},
        {"Blocked Query", "wide"},
        {"Blocking PID", "num"}, {"Blocking User", ""}, {"Blocking Duration", "num"},
        {"Blocking Query", "wide"}, {"", ""}
    }});

    for (auto& c : *chains) {
        auto bq = c.blocked_query.size() > 80
            ? escape(c.blocked_query.substr(0, 80)) + "..."
            : escape(c.blocked_query);
        auto bkq = c.blocking_query.size() > 80
            ? escape(c.blocking_query.substr(0, 80)) + "..."
            : escape(c.blocking_query);

        Table::row(h, {{
            std::to_string(c.blocked_pid),
            escape(c.blocked_user),
            escape(c.blocked_duration),
            std::format("<code class=\"query-preview\">{}</code>", bq),
            std::format("<strong>{}</strong>", c.blocking_pid),
            escape(c.blocking_user),
            escape(c.blocking_duration),
            std::format("<code class=\"query-preview\">{}</code>", bkq),
            std::format("<button class=\"btn btn-sm btn-danger\" hx-post=\"/monitor/terminate/{}\" hx-confirm=\"Kill blocking PID {}?\">Kill</button>", c.blocking_pid, c.blocking_pid),
        }});
    }
    Table::end(h);
    return Response::html(std::move(h).finish());
}

// ─── WALStatsHandler ────────────────────────────────────────────────

auto WALStatsHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto w = pg::wal_stats(conn->get());
    if (!w) return Response::html(render_to_string<Alert>({error_message(w.error()), "error"}));

    auto h = Html::with_capacity(2048);
    h.raw("<div class=\"stat-grid\">");
    StatCard::render({"WAL Level", w->wal_level}, h);
    StatCard::render({"Current LSN", w->current_lsn, "accent"}, h);
    StatCard::render({"Checkpoint Timeout", w->checkpoint_timeout}, h);
    StatCard::render({"Checkpoints (timed)", std::to_string(w->checkpoints_timed)}, h);
    StatCard::render({"Checkpoints (requested)", std::to_string(w->checkpoints_req),
                       w->checkpoints_req > w->checkpoints_timed ? "warning" : ""}, h);
    StatCard::render({"Write Time", std::format("{:.1f}ms", w->checkpoint_write_time)}, h);
    StatCard::render({"Sync Time", std::format("{:.1f}ms", w->checkpoint_sync_time)}, h);
    StatCard::render({"Buffers (checkpoint)", std::to_string(w->buffers_checkpoint)}, h);
    StatCard::render({"Buffers (backend)", std::to_string(w->buffers_backend),
                       w->buffers_backend > w->buffers_checkpoint ? "warning" : ""}, h);
    h.raw("</div>");

    return Response::html(std::move(h).finish());
}

// ─── VacuumProgressHandler ──────────────────────────────────────────

auto VacuumProgressHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto progress = pg::vacuum_progress(conn->get());
    if (!progress) return Response::html(render_to_string<Alert>({error_message(progress.error()), "error"}));

    if (progress->empty()) {
        return Response::html("<div class=\"empty-state\">No vacuum operations in progress</div>");
    }

    auto h = Html::with_capacity(4096);
    Table::begin(h, {{
        {"PID", "num"}, {"Database", ""}, {"Table", ""}, {"Phase", ""},
        {"Progress", ""}, {"Blocks", "num"}
    }});

    for (auto& v : *progress) {
        auto table_name = std::format("{}.{}", v.schema, v.table);
        auto progress_str = std::format("{:.1f}%", v.percent_complete);
        auto blocks_str = std::format("{}/{}", v.heap_blks_vacuumed, v.heap_blks_total);

        Table::row(h, {{
            std::to_string(v.pid),
            escape(v.database),
            escape(table_name),
            render_to_string<Badge>({v.phase, "primary"}),
            render_to_string<ProgressBar>({v.percent_complete, v.percent_complete > 80 ? "success" : ""}) + " " + progress_str,
            blocks_str,
        }});
    }
    Table::end(h);
    return Response::html(std::move(h).finish());
}

// ─── DatabaseStatsHandler ───────────────────────────────────────────

auto DatabaseStatsHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto dbs = pg::database_sizes(conn->get());
    if (!dbs) return Response::html(render_to_string<Alert>({error_message(dbs.error()), "error"}));

    // Find max size for bar chart
    double max_size = 1.0;
    for (auto& d : *dbs) if (d.size_bytes > static_cast<long long>(max_size)) max_size = static_cast<double>(d.size_bytes);

    auto h = Html::with_capacity(8192);
    Table::begin(h, {{
        {"Database", "", true}, {"Size", "num", true}, {"", ""},
        {"Connections", "num", true}, {"Commits", "num", true}, {"Rollbacks", "num", true},
        {"Cache Hit", "num", true}, {"Temp Bytes", "num", true}, {"Deadlocks", "num", true}
    }});

    for (auto& d : *dbs) {
        auto bar_pct = static_cast<double>(d.size_bytes) / max_size * 100;
        auto bar = render_to_string<SizeBar>({bar_pct});
        auto cache_variant = d.cache_hit_ratio < 0.90 ? "danger" : d.cache_hit_ratio < 0.99 ? "warning" : "success";
        auto format_bytes = [](long long b) -> std::string {
            if (b >= 1073741824LL) return std::format("{:.1f} GB", static_cast<double>(b) / 1073741824.0);
            if (b >= 1048576LL) return std::format("{:.1f} MB", static_cast<double>(b) / 1048576.0);
            if (b >= 1024LL) return std::format("{:.1f} KB", static_cast<double>(b) / 1024.0);
            return std::format("{} B", b);
        };

        Table::row(h, {{
            std::format("<strong>{}</strong>", escape(d.name)),
            escape(d.size),
            bar,
            std::to_string(d.connections),
            std::to_string(d.xact_commit),
            std::to_string(d.xact_rollback),
            render_to_string<Badge>({std::format("{:.1f}%", d.cache_hit_ratio * 100), cache_variant}),
            format_bytes(d.temp_bytes),
            std::to_string(d.deadlocks),
        }});
    }
    Table::end(h);
    return Response::html(std::move(h).finish());
}

// ─── BloatHandler ───────────────────────────────────────────────────

auto BloatHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto schema = req.query("schema");
    if (schema.empty()) schema = "public";

    auto bloat = pg::table_bloat(conn->get(), schema);
    if (!bloat) return Response::html(render_to_string<Alert>({error_message(bloat.error()), "error"}));

    if (bloat->empty()) {
        return Response::html("<div class=\"empty-state\">No tables found</div>");
    }

    auto h = Html::with_capacity(8192);
    Table::begin(h, {{
        {"Table", "", true}, {"Total Size", "num", true}, {"Est. Bloat", "num", true},
        {"Bloat %", "num", true}, {"", ""}
    }});

    for (auto& b : *bloat) {
        auto pct = b.bloat_ratio * 100;
        auto row_attrs = pct > 30 ? std::string_view(R"(class="row-warning")") : std::string_view("");

        Table::row(h, {{
            std::format("<strong>{}</strong>", escape(b.table)),
            escape(b.real_size_pretty),
            escape(b.bloat_size_pretty),
            render_to_string<ProgressBar>({pct, pct > 50 ? "danger" : pct > 20 ? "warning" : ""}) +
                std::format(" {:.1f}%", pct),
            pct > 20 ? render_to_string<Badge>({"NEEDS VACUUM", "warning"}) : std::string(""),
        }}, row_attrs);
    }
    Table::end(h);
    return Response::html(std::move(h).finish());
}

} // namespace getgresql::api
