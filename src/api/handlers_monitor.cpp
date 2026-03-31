#include "api/handlers_monitor.hpp"
#include "html/templates.hpp"
#include "pg/monitor.hpp"

#include <format>

namespace getgresql::api {

auto MonitorPageHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    std::string content;

    // Health checks panel
    content += "<div id=\"health-panel\" hx-get=\"/monitor/health\" hx-trigger=\"load, every 10s\" hx-swap=\"innerHTML\">"
               "<div class=\"loading\">Checking health...</div></div>";

    // Stats
    content += "<div id=\"monitor-stats\" hx-get=\"/monitor/stats\" hx-trigger=\"load, every 3s\" hx-swap=\"innerHTML\">"
               "<div class=\"loading\">Loading stats...</div></div>";

    // Section tabs for different monitoring views
    content += "<div class=\"section-tabs\">"
        "<button class=\"section-tab active\" onclick=\"this.parentElement.querySelectorAll('.active').forEach(e=>e.classList.remove('active')); this.classList.add('active')\" "
        "hx-get=\"/monitor/activity\" hx-target=\"#monitor-content\" hx-swap=\"innerHTML\">Active Queries</button>"
        "<button class=\"section-tab\" onclick=\"this.parentElement.querySelectorAll('.active').forEach(e=>e.classList.remove('active')); this.classList.add('active')\" "
        "hx-get=\"/monitor/slow\" hx-target=\"#monitor-content\" hx-swap=\"innerHTML\">Slow Queries</button>"
        "<button class=\"section-tab\" onclick=\"this.parentElement.querySelectorAll('.active').forEach(e=>e.classList.remove('active')); this.classList.add('active')\" "
        "hx-get=\"/monitor/blocking\" hx-target=\"#monitor-content\" hx-swap=\"innerHTML\">Blocking</button>"
        "<button class=\"section-tab\" onclick=\"this.parentElement.querySelectorAll('.active').forEach(e=>e.classList.remove('active')); this.classList.add('active')\" "
        "hx-get=\"/monitor/vacuum-progress\" hx-target=\"#monitor-content\" hx-swap=\"innerHTML\">Vacuum Progress</button>"
        "<button class=\"section-tab\" onclick=\"this.parentElement.querySelectorAll('.active').forEach(e=>e.classList.remove('active')); this.classList.add('active')\" "
        "hx-get=\"/monitor/wal\" hx-target=\"#monitor-content\" hx-swap=\"innerHTML\">WAL</button>"
        "<button class=\"section-tab\" onclick=\"this.parentElement.querySelectorAll('.active').forEach(e=>e.classList.remove('active')); this.classList.add('active')\" "
        "hx-get=\"/monitor/databases\" hx-target=\"#monitor-content\" hx-swap=\"innerHTML\">Databases</button>"
        "<button class=\"section-tab\" onclick=\"this.parentElement.querySelectorAll('.active').forEach(e=>e.classList.remove('active')); this.classList.add('active')\" "
        "hx-get=\"/monitor/bloat\" hx-target=\"#monitor-content\" hx-swap=\"innerHTML\">Bloat</button>"
        "</div>";

    content += "<div id=\"monitor-content\" hx-get=\"/monitor/activity\" hx-trigger=\"load\" hx-swap=\"innerHTML\">"
               "<div class=\"loading\">Loading...</div></div>";

    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page("Monitor", "Monitor", std::move(content)));
}

auto MonitorStatsHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto stats = pg::server_stats(conn->get());
    if (!stats) return Response::html(html::alert(error_message(stats.error()), "error"));

    auto& s = *stats;
    std::string content = R"(<div class="stat-grid">)";
    content += html::stat_card("Active", std::to_string(s.active_connections), "");
    content += html::stat_card("Idle", std::to_string(s.idle_connections));
    content += html::stat_card("Idle in Txn", std::to_string(s.idle_in_transaction),
                               s.idle_in_transaction > 0 ? "warning" : "");
    content += html::stat_card("Waiting", std::to_string(s.waiting_connections),
                               s.waiting_connections > 0 ? "warning" : "");
    content += html::stat_card("Cache Hit",
        std::format("{:.1f}%", s.cache_hit_ratio * 100),
        s.cache_hit_ratio < 0.90 ? "danger" : "success");
    content += html::stat_card("Max Connections", std::to_string(s.max_connections));
    content += html::stat_card("Commits", std::to_string(s.total_commits));
    content += html::stat_card("Rollbacks", std::to_string(s.total_rollbacks),
                               s.total_rollbacks > 0 ? "warning" : "");
    content += "</div>";

    return Response::html(std::move(content));
}

auto MonitorActivityHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto activity = pg::active_queries(conn->get());
    if (!activity) return Response::html(html::alert(error_message(activity.error()), "error"));

    if (activity->empty()) {
        return Response::html(R"(<div class="empty-state">No active queries</div>)");
    }

    std::string content;
    content += html::table_begin({
        {"PID", "num"}, {"Database", ""}, {"User", ""}, {"State", ""},
        {"Duration", "num"}, {"Wait", ""}, {"Query", "wide"}, {"", ""}, {"", ""}
    });

    for (auto& a : *activity) {
        auto state_variant = a.state == "active" ? "primary"
                           : a.state == "idle in transaction" ? "warning"
                           : "secondary";

        auto query_preview = a.query.size() > 120
            ? html::escape(a.query.substr(0, 120)) + "..."
            : html::escape(a.query);

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

        content += html::table_row({
            std::to_string(a.pid),
            html::escape(a.database),
            html::escape(a.user),
            html::badge(a.state, state_variant),
            html::escape(a.duration),
            a.wait_event.empty() ? std::string("&mdash;")
                : std::format("{}/{}", html::escape(a.wait_event_type), html::escape(a.wait_event)),
            std::format(R"(<code class="query-preview">{}</code>)", query_preview),
            cancel_btn,
            terminate_btn,
        });
    }
    content += html::table_end();

    return Response::html(std::move(content));
}

auto MonitorLocksHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto locks = pg::all_locks(conn->get());
    if (!locks) return Response::error(error_message(locks.error()));

    std::string content;

    if (locks->empty()) {
        content = R"(<div class="empty-state">No locks detected</div>)";
    } else {
        content += html::table_begin({
            {"PID", "num"}, {"Database", ""}, {"Relation", ""},
            {"Mode", ""}, {"Granted", ""}, {"Query", "wide"}
        });
        for (auto& l : *locks) {
            content += html::table_row({
                std::to_string(l.pid),
                html::escape(l.database),
                l.relation.empty() ? "&mdash;" : html::escape(l.relation),
                html::badge(l.mode, l.granted ? "secondary" : "danger"),
                l.granted ? std::string("&#10003;") : html::badge("WAITING", "danger"),
                std::format("<code class=\"query-preview\">{}</code>",
                    html::escape(l.query.size() > 100 ? l.query.substr(0, 100) + "..." : l.query)),
            });
        }
        content += html::table_end();
    }

    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page("Locks", "Monitor", std::move(content)));
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
        return Response::html(html::alert(error_message(result.error()), "error"));
    }

    return Response::html(html::alert(std::format("Cancelled query on PID {}", pid), "info"));
}

// ─── HealthCheckHandler ──────────────────────────────────────────────

auto HealthCheckHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto checks = pg::health_checks(conn->get());
    if (!checks) return Response::html(html::alert(error_message(checks.error()), "error"));

    std::string content = "<div class=\"health-grid\">";
    for (auto& c : *checks) {
        auto variant = c.status == "ok" ? "success" : c.status == "warning" ? "warning" : "danger";
        auto icon = c.status == "ok" ? "&#10003;" : c.status == "warning" ? "&#9888;" : "&#10007;";
        content += std::format(
            "<div class=\"health-card health-{}\">"
            "<div class=\"health-icon\">{}</div>"
            "<div class=\"health-info\"><div class=\"health-name\">{}</div>"
            "<div class=\"health-value\">{}</div>"
            "<div class=\"health-detail\">{}</div></div></div>",
            variant, icon, html::escape(c.name), html::escape(c.value), html::escape(c.detail)
        );
    }
    content += "</div>";
    return Response::html(std::move(content));
}

// ─── SlowQueriesHandler ─────────────────────────────────────────────

auto SlowQueriesHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto queries = pg::slow_queries(conn->get(), 500);
    if (!queries) return Response::html(html::alert(error_message(queries.error()), "error"));

    if (queries->empty()) {
        return Response::html("<div class=\"empty-state\">No slow queries (>500ms) running</div>");
    }

    std::string content;
    content += html::table_begin({
        {"PID", "num"}, {"Database", ""}, {"User", ""}, {"Duration", "num"},
        {"State", ""}, {"Query", "wide"}, {"", ""}
    });

    for (auto& q : *queries) {
        auto query_preview = q.query.size() > 150
            ? html::escape(q.query.substr(0, 150)) + "..."
            : html::escape(q.query);

        content += html::table_row({
            std::to_string(q.pid),
            html::escape(q.database),
            html::escape(q.user),
            html::escape(q.duration),
            html::badge(q.state, q.state == "active" ? "primary" : "warning"),
            std::format("<code class=\"query-preview\">{}</code>", query_preview),
            std::format("<button class=\"btn btn-sm btn-danger\" hx-post=\"/monitor/cancel/{}\" hx-confirm=\"Cancel query PID {}?\">Cancel</button>", q.pid, q.pid),
        });
    }
    content += html::table_end();
    return Response::html(std::move(content));
}

// ─── BlockingHandler ────────────────────────────────────────────────

auto BlockingHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto chains = pg::blocking_chains(conn->get());
    if (!chains) return Response::html(html::alert(error_message(chains.error()), "error"));

    if (chains->empty()) {
        return Response::html("<div class=\"empty-state\">No blocking detected</div>");
    }

    std::string content;
    content += html::table_begin({
        {"Blocked PID", "num"}, {"Blocked User", ""}, {"Blocked Duration", "num"},
        {"Blocked Query", "wide"},
        {"Blocking PID", "num"}, {"Blocking User", ""}, {"Blocking Duration", "num"},
        {"Blocking Query", "wide"}, {"", ""}
    });

    for (auto& c : *chains) {
        auto bq = c.blocked_query.size() > 80
            ? html::escape(c.blocked_query.substr(0, 80)) + "..."
            : html::escape(c.blocked_query);
        auto bkq = c.blocking_query.size() > 80
            ? html::escape(c.blocking_query.substr(0, 80)) + "..."
            : html::escape(c.blocking_query);

        content += html::table_row({
            std::to_string(c.blocked_pid),
            html::escape(c.blocked_user),
            html::escape(c.blocked_duration),
            std::format("<code class=\"query-preview\">{}</code>", bq),
            std::format("<strong>{}</strong>", c.blocking_pid),
            html::escape(c.blocking_user),
            html::escape(c.blocking_duration),
            std::format("<code class=\"query-preview\">{}</code>", bkq),
            std::format("<button class=\"btn btn-sm btn-danger\" hx-post=\"/monitor/terminate/{}\" hx-confirm=\"Kill blocking PID {}?\">Kill</button>", c.blocking_pid, c.blocking_pid),
        });
    }
    content += html::table_end();
    return Response::html(std::move(content));
}

// ─── WALStatsHandler ────────────────────────────────────────────────

auto WALStatsHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto w = pg::wal_stats(conn->get());
    if (!w) return Response::html(html::alert(error_message(w.error()), "error"));

    std::string content = "<div class=\"stat-grid\">";
    content += html::stat_card("WAL Level", w->wal_level);
    content += html::stat_card("Current LSN", w->current_lsn, "accent");
    content += html::stat_card("Checkpoint Timeout", w->checkpoint_timeout);
    content += html::stat_card("Checkpoints (timed)", std::to_string(w->checkpoints_timed));
    content += html::stat_card("Checkpoints (requested)", std::to_string(w->checkpoints_req),
                               w->checkpoints_req > w->checkpoints_timed ? "warning" : "");
    content += html::stat_card("Write Time", std::format("{:.1f}ms", w->checkpoint_write_time));
    content += html::stat_card("Sync Time", std::format("{:.1f}ms", w->checkpoint_sync_time));
    content += html::stat_card("Buffers (checkpoint)", std::to_string(w->buffers_checkpoint));
    content += html::stat_card("Buffers (backend)", std::to_string(w->buffers_backend),
                               w->buffers_backend > w->buffers_checkpoint ? "warning" : "");
    content += "</div>";

    return Response::html(std::move(content));
}

// ─── VacuumProgressHandler ──────────────────────────────────────────

auto VacuumProgressHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto progress = pg::vacuum_progress(conn->get());
    if (!progress) return Response::html(html::alert(error_message(progress.error()), "error"));

    if (progress->empty()) {
        return Response::html("<div class=\"empty-state\">No vacuum operations in progress</div>");
    }

    std::string content;
    content += html::table_begin({
        {"PID", "num"}, {"Database", ""}, {"Table", ""}, {"Phase", ""},
        {"Progress", ""}, {"Blocks", "num"}
    });

    for (auto& v : *progress) {
        auto table_name = std::format("{}.{}", v.schema, v.table);
        auto progress_str = std::format("{:.1f}%", v.percent_complete);
        auto blocks_str = std::format("{}/{}", v.heap_blks_vacuumed, v.heap_blks_total);

        content += html::table_row({
            std::to_string(v.pid),
            html::escape(v.database),
            html::escape(table_name),
            html::badge(v.phase, "primary"),
            html::progress_bar(v.percent_complete, v.percent_complete > 80 ? "success" : "") + " " + progress_str,
            blocks_str,
        });
    }
    content += html::table_end();
    return Response::html(std::move(content));
}

// ─── DatabaseStatsHandler ───────────────────────────────────────────

auto DatabaseStatsHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto dbs = pg::database_sizes(conn->get());
    if (!dbs) return Response::html(html::alert(error_message(dbs.error()), "error"));

    // Find max size for bar chart
    double max_size = 1.0;
    for (auto& d : *dbs) if (d.size_bytes > static_cast<long long>(max_size)) max_size = static_cast<double>(d.size_bytes);

    std::string content;
    content += html::table_begin({
        {"Database", "", true}, {"Size", "num", true}, {"", ""},
        {"Connections", "num", true}, {"Commits", "num", true}, {"Rollbacks", "num", true},
        {"Cache Hit", "num", true}, {"Temp Bytes", "num", true}, {"Deadlocks", "num", true}
    });

    for (auto& d : *dbs) {
        auto bar_pct = static_cast<double>(d.size_bytes) / max_size * 100;
        auto bar = std::format("<div class=\"size-bar\"><div class=\"size-bar-fill\" style=\"width:{:.0f}px\"></div></div>", bar_pct * 1.5);
        auto cache_variant = d.cache_hit_ratio < 0.90 ? "danger" : d.cache_hit_ratio < 0.99 ? "warning" : "success";
        auto format_bytes = [](long long b) -> std::string {
            if (b >= 1073741824LL) return std::format("{:.1f} GB", static_cast<double>(b) / 1073741824.0);
            if (b >= 1048576LL) return std::format("{:.1f} MB", static_cast<double>(b) / 1048576.0);
            if (b >= 1024LL) return std::format("{:.1f} KB", static_cast<double>(b) / 1024.0);
            return std::format("{} B", b);
        };

        content += html::table_row({
            std::format("<strong>{}</strong>", html::escape(d.name)),
            html::escape(d.size),
            bar,
            std::to_string(d.connections),
            std::to_string(d.xact_commit),
            std::to_string(d.xact_rollback),
            html::badge(std::format("{:.1f}%", d.cache_hit_ratio * 100), cache_variant),
            format_bytes(d.temp_bytes),
            std::to_string(d.deadlocks),
        });
    }
    content += html::table_end();
    return Response::html(std::move(content));
}

// ─── BloatHandler ───────────────────────────────────────────────────

auto BloatHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto schema = req.query("schema");
    if (schema.empty()) schema = "public";

    auto bloat = pg::table_bloat(conn->get(), schema);
    if (!bloat) return Response::html(html::alert(error_message(bloat.error()), "error"));

    if (bloat->empty()) {
        return Response::html("<div class=\"empty-state\">No tables found</div>");
    }

    std::string content;
    content += html::table_begin({
        {"Table", "", true}, {"Total Size", "num", true}, {"Est. Bloat", "num", true},
        {"Bloat %", "num", true}, {"", ""}
    });

    for (auto& b : *bloat) {
        auto pct = b.bloat_ratio * 100;
        auto row_attrs = pct > 30 ? std::string_view(R"(class="row-warning")") : std::string_view("");

        content += html::table_row({
            std::format("<strong>{}</strong>", html::escape(b.table)),
            html::escape(b.real_size_pretty),
            html::escape(b.bloat_size_pretty),
            html::progress_bar(pct, pct > 50 ? "danger" : pct > 20 ? "warning" : "") +
                std::format(" {:.1f}%", pct),
            pct > 20 ? html::badge("NEEDS VACUUM", "warning") : std::string(""),
        }, row_attrs);
    }
    content += html::table_end();
    return Response::html(std::move(content));
}

} // namespace getgresql::api
