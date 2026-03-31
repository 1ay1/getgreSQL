#include "api/handlers_monitor.hpp"
#include "html/templates.hpp"
#include "pg/monitor.hpp"

#include <format>

namespace getgresql::api {

auto MonitorPageHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    std::string content = R"(
<div id="monitor-stats" hx-get="/monitor/stats" hx-trigger="load, every 3s" hx-swap="innerHTML">
    <div class="loading">Loading stats...</div>
</div>

<h3>Active Queries</h3>
<div id="monitor-activity" hx-get="/monitor/activity" hx-trigger="load, every 3s" hx-swap="innerHTML">
    <div class="loading">Loading...</div>
</div>
)";

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

} // namespace getgresql::api
