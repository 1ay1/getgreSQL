#include "api/handlers_db.hpp"
#include "core/expected.hpp"
#include "ssr/components.hpp"
#include "ssr/page.hpp"
#include "pg/catalog.hpp"
#include "pg/monitor.hpp"

#include <format>

namespace getgresql::api {

using namespace ssr;

static auto escape(std::string_view s) -> std::string {
    auto h = Html::with_capacity(s.size() + 32);
    h.text(s);
    return std::move(h).finish();
}

// ─── Form helpers ───────────────────────────────────────────────────

static auto url_decode(std::string_view input) -> std::string {
    std::string decoded;
    decoded.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '+') decoded += ' ';
        else if (input[i] == '%' && i + 2 < input.size()) {
            auto hex = std::string(input.substr(i + 1, 2));
            decoded += static_cast<char>(std::stoi(hex, nullptr, 16));
            i += 2;
        } else decoded += input[i];
    }
    return decoded;
}

static auto form_value(std::string_view body, std::string_view key) -> std::string {
    auto needle = std::string(key) + "=";
    auto pos = body.find(needle);
    if (pos == std::string_view::npos) return {};
    auto start = pos + needle.size();
    auto end = body.find('&', start);
    auto raw = (end == std::string_view::npos) ? body.substr(start) : body.substr(start, end - start);
    return url_decode(raw);
}

static auto json_escape_str(std::string_view s) -> std::string {
    std::string out;
    out.reserve(s.size() + 4);
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

auto IndexHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    using namespace ssr;
    auto render = [](Html& h) {
        // Health + stats: full width stacking
        h.raw("<div id=\"dash-health\" hx-get=\"/dashboard/health\" hx-trigger=\"load\" hx-swap=\"innerHTML\"></div>\n");
        h.raw("<div id=\"dash-stats\" hx-get=\"/dashboard/stats\" hx-trigger=\"load\" hx-swap=\"innerHTML\"></div>\n");
        // Two-column row: activity + top tables
        h.raw("<div class=\"dash-row\">\n");
        h.raw("  <div id=\"dash-activity\" hx-get=\"/dashboard/activity\" hx-trigger=\"load\" hx-swap=\"innerHTML\"></div>\n");
        h.raw("  <div id=\"dash-top-tables\" hx-get=\"/dashboard/top-tables\" hx-trigger=\"load\" hx-swap=\"innerHTML\"></div>\n");
        h.raw("</div>\n");
        // Two-column row: databases + activity by db
        h.raw("<div id=\"dash-content\" hx-get=\"/dashboard/content\" hx-trigger=\"load\" hx-swap=\"innerHTML\"></div>\n");
    };
    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page("Dashboard", "Dashboard", render));
}

auto DashboardHealthHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    using namespace ssr;
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));
    return Response::html(cache().get_or_compute("dashboard:health", std::chrono::seconds(5), [&] {
        auto checks = pg::health_checks(conn->get());
        if (!checks) return render_to_string<Alert>(Alert::Props{error_message(checks.error()), "error"});
        auto h = Html::with_capacity(4096);
        // Status ribbon — compact, glanceable
        int ok_count = 0, warn_count = 0, crit_count = 0;
        for (auto& c : *checks) {
            if (c.status == "ok") ok_count++;
            else if (c.status == "warning") warn_count++;
            else crit_count++;
        }
        auto overall = crit_count > 0 ? "critical" : warn_count > 0 ? "degraded" : "healthy";
        auto overall_cls = crit_count > 0 ? "danger" : warn_count > 0 ? "warning" : "success";
        h.raw("<div class=\"dash-hero\">");
        h.raw("<div class=\"dash-hero-status dash-hero-").raw(overall_cls).raw("\">");
        h.raw("<div class=\"dash-hero-pulse\"></div>");
        h.raw("<span class=\"dash-hero-dot\"></span>");
        h.raw("<span class=\"dash-hero-label\">System ").raw(overall).raw("</span>");
        h.raw("</div>");
        // Mini check indicators
        h.raw("<div class=\"dash-checks\">");
        for (auto& c : *checks) {
            auto v = (c.status == "ok") ? "success" : (c.status == "warning") ? "warning" : "danger";
            auto icon = (c.status == "ok") ? "&#10003;" : (c.status == "warning") ? "&#9888;" : "&#10007;";
            h.raw("<div class=\"dash-check dash-check-").raw(v).raw("\" title=\"")
             .text(c.name).raw(": ").text(c.detail).raw("\">");
            h.raw("<span class=\"dash-check-icon\">").raw(icon).raw("</span>");
            h.raw("<span class=\"dash-check-name\">").text(c.name).raw("</span>");
            h.raw("<span class=\"dash-check-val\">").text(c.value).raw("</span>");
            h.raw("</div>");
        }
        h.raw("</div></div>");
        return std::move(h).finish();
    }));
}

auto DashboardStatsHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    using namespace ssr;
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));
    auto stats = pg::server_stats(conn->get());
    if (!stats) return Response::html(render_to_string<Alert>(Alert::Props{error_message(stats.error()), "error"}));
    auto h = Html::with_capacity(8192);
    auto& s = *stats;

    // Clean uptime: strip fractional seconds
    auto uptime_clean = s.uptime;
    if (auto dot = uptime_clean.find('.'); dot != std::string::npos) uptime_clean = uptime_clean.substr(0, dot);

    // Server hero bar
    h.raw("<div class=\"dash-server\">");
    h.raw("<div class=\"dash-server-info\">");
    h.raw("<span class=\"dash-server-version\">").text(s.version).raw("</span>");
    h.raw("<span class=\"dash-server-uptime\">&#9716; <strong>").text(uptime_clean).raw("</strong></span>");
    h.raw("<span class=\"dash-server-pid\">PID ").raw(std::to_string(s.pid)).raw("</span>");
    h.raw("</div>");
    h.raw("<div class=\"dash-server-actions\">");
    h.raw("<a href=\"/query\" class=\"btn btn-sm btn-ghost\" data-spa>&#9654; Query</a>");
    h.raw("<a href=\"/monitor\" class=\"btn btn-sm btn-ghost\" data-spa>&#9673; Monitor</a>");
    h.raw("<a href=\"/settings\" class=\"btn btn-sm btn-ghost\" data-spa>&#9881; Settings</a>");
    h.raw("</div></div>");

    // Big metric cards row
    h.raw("<div class=\"dash-metrics\">");

    // Connection gauge (SVG ring)
    auto total_conns = s.active_connections + s.idle_connections + s.idle_in_transaction;
    auto conn_pct = s.max_connections > 0 ? static_cast<double>(total_conns) / s.max_connections * 100.0 : 0.0;
    auto conn_cls = conn_pct >= 90 ? "danger" : conn_pct >= 70 ? "warning" : "success";
    h.raw("<div class=\"dash-metric-card\">");
    h.raw("<div class=\"dash-ring-container\">");
    h.raw(std::format(
        "<svg class=\"dash-ring\" viewBox=\"0 0 120 120\">"
        "<circle cx=\"60\" cy=\"60\" r=\"52\" fill=\"none\" stroke=\"var(--bg-3)\" stroke-width=\"8\"/>"
        "<circle cx=\"60\" cy=\"60\" r=\"52\" fill=\"none\" stroke=\"var(--{})\" stroke-width=\"8\" "
        "stroke-dasharray=\"{:.1f} {:.1f}\" stroke-dashoffset=\"81.7\" stroke-linecap=\"round\" "
        "class=\"dash-ring-fill\"/></svg>",
        conn_cls, conn_pct * 3.267, (100.0 - conn_pct) * 3.267
    ));
    h.raw("<div class=\"dash-ring-label\">");
    h.raw("<span class=\"dash-ring-value\">").raw(std::to_string(total_conns)).raw("</span>");
    h.raw("<span class=\"dash-ring-sub\">/ ").raw(std::to_string(s.max_connections)).raw("</span>");
    h.raw("</div></div>");
    h.raw("<div class=\"dash-metric-title\">Connections</div>");
    h.raw("<div class=\"dash-metric-breakdown\">");
    h.raw("<span class=\"dash-metric-item\"><span class=\"dash-dot dash-dot-active\"></span>Active ").raw(std::to_string(s.active_connections)).raw("</span>");
    h.raw("<span class=\"dash-metric-item\"><span class=\"dash-dot dash-dot-idle\"></span>Idle ").raw(std::to_string(s.idle_connections)).raw("</span>");
    if (s.idle_in_transaction > 0) {
        h.raw("<span class=\"dash-metric-item dash-metric-warn\"><span class=\"dash-dot dash-dot-warning\"></span>Idle Txn ").raw(std::to_string(s.idle_in_transaction)).raw("</span>");
    }
    h.raw("</div></div>");

    // Cache hit ratio (SVG ring)
    auto cache_pct = s.cache_hit_ratio * 100.0;
    auto cache_cls = cache_pct < 90 ? "danger" : cache_pct < 99 ? "warning" : "success";
    h.raw("<div class=\"dash-metric-card\">");
    h.raw("<div class=\"dash-ring-container\">");
    h.raw(std::format(
        "<svg class=\"dash-ring\" viewBox=\"0 0 120 120\">"
        "<circle cx=\"60\" cy=\"60\" r=\"52\" fill=\"none\" stroke=\"var(--bg-3)\" stroke-width=\"8\"/>"
        "<circle cx=\"60\" cy=\"60\" r=\"52\" fill=\"none\" stroke=\"var(--{})\" stroke-width=\"8\" "
        "stroke-dasharray=\"{:.1f} {:.1f}\" stroke-dashoffset=\"81.7\" stroke-linecap=\"round\" "
        "class=\"dash-ring-fill\"/></svg>",
        cache_cls, cache_pct * 3.267, (100.0 - cache_pct) * 3.267
    ));
    h.raw("<div class=\"dash-ring-label\">");
    h.raw(std::format("<span class=\"dash-ring-value\">{:.1f}%</span>", cache_pct));
    h.raw("</div></div>");
    h.raw("<div class=\"dash-metric-title\">Cache Hit Ratio</div>");
    h.raw("<div class=\"dash-metric-breakdown\">");
    auto total_blocks = s.blocks_hit + s.blocks_read;
    if (total_blocks > 0) {
        h.raw("<span class=\"dash-metric-item\">Hit ").raw(std::to_string(s.blocks_hit)).raw("</span>");
        h.raw("<span class=\"dash-metric-item\">Read ").raw(std::to_string(s.blocks_read)).raw("</span>");
    }
    h.raw("</div></div>");

    // Transaction throughput
    auto total_txn = s.total_commits + s.total_rollbacks;
    auto rollback_pct = total_txn > 0 ? static_cast<double>(s.total_rollbacks) / total_txn * 100.0 : 0.0;
    h.raw("<div class=\"dash-metric-card\">");
    h.raw("<div class=\"dash-metric-big\">");
    h.raw("<span class=\"dash-big-num\">");
    // Format with K/M suffix
    if (s.total_commits >= 1000000) h.raw(std::format("{:.1f}M", s.total_commits / 1000000.0));
    else if (s.total_commits >= 1000) h.raw(std::format("{:.1f}K", s.total_commits / 1000.0));
    else h.raw(std::to_string(s.total_commits));
    h.raw("</span>");
    h.raw("<span class=\"dash-big-label\">commits</span>");
    h.raw("</div>");
    h.raw("<div class=\"dash-metric-title\">Transactions</div>");
    h.raw("<div class=\"dash-metric-breakdown\">");
    h.raw("<span class=\"dash-metric-item\">Commits ").raw(std::to_string(s.total_commits)).raw("</span>");
    if (s.total_rollbacks > 0) {
        h.raw(std::format("<span class=\"dash-metric-item dash-metric-warn\">Rollbacks {} ({:.1f}%)</span>",
            s.total_rollbacks, rollback_pct));
    } else {
        h.raw("<span class=\"dash-metric-item\">Rollbacks 0</span>");
    }
    h.raw("</div></div>");

    h.raw("</div>"); // end dash-metrics
    return Response::html(std::move(h).finish());
}

auto DashboardContentHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    using namespace ssr;
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));
    auto dbs_res = pg::database_sizes(conn->get());
    auto h = Html::with_capacity(8192);

    if (dbs_res && !dbs_res->empty()) {
        double max_size = 1.0;
        for (auto& d : *dbs_res) if (static_cast<double>(d.size_bytes) > max_size) max_size = static_cast<double>(d.size_bytes);

        h.raw("<div class=\"dashboard-section\">");
        h.raw("<div class=\"dashboard-section-header\">Databases"
              "<a href=\"/databases\" class=\"btn btn-sm btn-ghost\" data-spa>View All &rsaquo;</a></div>");
        h.raw("<div class=\"dashboard-section-body\">");
        h.raw("<div class=\"dash-db-grid\">");
        for (auto& d : *dbs_res) {
            auto pct = static_cast<double>(d.size_bytes) / max_size * 100;
            auto cache_pct = d.cache_hit_ratio * 100.0;
            auto cache_cls = cache_pct < 90 ? "danger" : cache_pct < 99 ? "warning" : "success";

            h.raw("<a href=\"/db/").text(d.name).raw("/schemas\" class=\"dash-db-card\" data-spa>");

            // Header
            h.raw("<div class=\"dash-db-header\">");
            h.raw("<span class=\"dash-db-name\">").text(d.name).raw("</span>");
            h.raw("<span class=\"dash-db-size\">").text(d.size).raw("</span>");
            h.raw("</div>");

            // Size bar
            h.raw(std::format("<div class=\"dash-db-bar\"><div class=\"dash-db-bar-fill\" style=\"width:{:.0f}%\"></div></div>", pct));

            // Stats row
            h.raw("<div class=\"dash-db-stats\">");
            h.raw("<span title=\"Connections\"><span class=\"dash-db-stat-icon\">&#9679;</span>").raw(std::to_string(d.connections)).raw(" conn</span>");
            h.raw("<span title=\"Cache Hit Ratio\" class=\"dash-db-cache-").raw(cache_cls).raw("\">")
             .raw(std::format("{:.0f}%", cache_pct)).raw(" cache</span>");
            if (d.xact_commit > 0) {
                h.raw("<span title=\"Commits\">");
                if (d.xact_commit >= 1000000) h.raw(std::format("{:.1f}M", d.xact_commit / 1e6));
                else if (d.xact_commit >= 1000) h.raw(std::format("{:.1f}K", d.xact_commit / 1e3));
                else h.raw(std::to_string(d.xact_commit));
                h.raw(" txn</span>");
            }
            if (d.deadlocks > 0) {
                h.raw("<span class=\"dash-db-deadlocks\" title=\"Deadlocks\">").raw(std::to_string(d.deadlocks)).raw(" deadlocks</span>");
            }
            h.raw("</div>");

            h.raw("</a>"); // close card
        }
        h.raw("</div>"); // close dash-db-grid
        h.raw("</div></div>"); // close section-body + section
    }

    return Response::html(std::move(h).finish());
}

// ─── DashboardActivityHandler — live queries/connections ────────────

auto DashboardActivityHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    using namespace ssr;
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));

    auto queries = pg::active_queries(conn->get());
    auto h = Html::with_capacity(4096);

    h.raw("<div class=\"dashboard-section\">");
    h.raw("<div class=\"dashboard-section-header\">Live Activity"
          "<a href=\"/monitor\" class=\"btn btn-sm btn-ghost\" data-spa>Monitor &rsaquo;</a></div>");
    h.raw("<div class=\"dashboard-section-body\">");

    if (!queries || queries->empty()) {
        h.raw("<div class=\"dash-empty\">No active queries</div>");
    } else {
        // Count by state
        int n_active = 0, n_idle = 0, n_idle_txn = 0;
        for (auto& q : *queries) {
            if (q.state == "active") n_active++;
            else if (q.state == "idle in transaction") n_idle_txn++;
            else n_idle++;
        }
        h.raw("<div class=\"dash-activity-summary\">");
        h.raw("<span class=\"dash-activity-badge dash-ab-active\">").raw(std::to_string(n_active)).raw(" active</span>");
        h.raw("<span class=\"dash-activity-badge dash-ab-idle\">").raw(std::to_string(n_idle)).raw(" idle</span>");
        if (n_idle_txn > 0) h.raw("<span class=\"dash-activity-badge dash-ab-warn\">").raw(std::to_string(n_idle_txn)).raw(" idle txn</span>");
        h.raw("</div>");

        // Show active (non-idle) queries, skip our own introspection
        int shown = 0;
        h.raw("<div class=\"dash-activity-list\">");
        for (auto& q : *queries) {
            if (q.state == "idle" || q.query.empty()) continue;
            // Skip our own dashboard queries
            if (q.query.find("pg_stat_activity") != std::string::npos && q.query.find("backend_type") != std::string::npos) continue;
            if (shown >= 8) break;
            auto state_cls = q.state == "active" ? "active" : q.state == "idle in transaction" ? "warning" : "idle";
            h.raw("<div class=\"dash-activity-row\">");
            h.raw("<span class=\"dash-activity-state dash-activity-").raw(state_cls).raw("\">&#9679;</span>");
            h.raw("<span class=\"dash-activity-db\">").text(q.database).raw("</span>");
            h.raw("<span class=\"dash-activity-user\">").text(q.user).raw("</span>");
            if (!q.duration.empty()) {
                h.raw("<span class=\"dash-activity-duration\">").text(q.duration).raw("</span>");
            }
            auto query_preview = q.query.size() > 100 ? q.query.substr(0, 100) + "..." : q.query;
            h.raw("<span class=\"dash-activity-query\">").text(query_preview).raw("</span>");
            h.raw("</div>");
            shown++;
        }
        h.raw("</div>");
        if (shown == 0) h.raw("<div class=\"dash-empty\">All connections idle &#8212; no active queries</div>");
    }
    h.raw("</div></div>");
    return Response::html(std::move(h).finish());
}

// ─── DashboardTopTablesHandler — biggest tables bar chart ───────────

auto DashboardTopTablesHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    using namespace ssr;
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));

    // Get top tables by size across all schemas
    auto result = conn->get().exec(
        "SELECT schemaname, relname, "
        "pg_size_pretty(pg_total_relation_size(schemaname||'.'||quote_ident(relname))) as size, "
        "pg_total_relation_size(schemaname||'.'||quote_ident(relname)) as size_bytes, "
        "n_live_tup, n_dead_tup, "
        "coalesce(n_tup_ins + n_tup_upd + n_tup_del, 0) as total_writes, "
        "CASE WHEN n_live_tup > 0 THEN round(n_dead_tup::numeric / n_live_tup * 100, 1) ELSE 0 END as dead_pct "
        "FROM pg_stat_user_tables "
        "ORDER BY pg_total_relation_size(schemaname||'.'||quote_ident(relname)) DESC LIMIT 10"
    );

    auto h = Html::with_capacity(4096);
    h.raw("<div class=\"dashboard-section\">");
    h.raw("<div class=\"dashboard-section-header\">Largest Tables</div>");
    h.raw("<div class=\"dashboard-section-body\">");

    if (!result || result->row_count() == 0) {
        h.raw("<div class=\"dash-empty\">No tables found</div>");
    } else {
        long long max_bytes = 1;
        // First pass: find max
        for (auto row : *result) {
            if (!row.is_null(3)) {
                auto val = std::stoll(std::string(row[3]));
                if (val > max_bytes) max_bytes = val;
            }
        }
        h.raw("<div class=\"dash-bar-chart\">");
        for (auto row : *result) {
            auto schema = std::string(row[0]);
            auto table = std::string(row[1]);
            auto size = std::string(row[2]);
            auto size_bytes = row.is_null(3) ? 0LL : std::stoll(std::string(row[3]));
            auto live = row.is_null(4) ? std::string("0") : std::string(row[4]);
            auto dead_pct = row.is_null(7) ? 0.0 : std::stod(std::string(row[7]));
            auto pct = max_bytes > 0 ? static_cast<double>(size_bytes) / max_bytes * 100.0 : 0.0;

            h.raw("<div class=\"dash-bar-row\">");
            h.raw("<span class=\"dash-bar-name\"><a href=\"/db/postgres/schema/")
             .text(schema).raw("/table/").text(table).raw("\" data-spa>")
             .text(schema).raw(".").text(table).raw("</a></span>");
            h.raw("<span class=\"dash-bar-track\"><div>");
            h.raw(std::format("<div class=\"dash-bar-fill{}\" style=\"width:{:.1f}%\"></div>",
                dead_pct > 20 ? " dash-bar-warn" : "", pct));
            h.raw("</div></span>");
            h.raw("<span class=\"dash-bar-value\">").text(size).raw("</span>");
            h.raw("<span class=\"dash-bar-meta\">").raw(live).raw(" rows");
            if (dead_pct > 5) {
                h.raw(std::format(" <span class=\"dash-bar-dead\">{:.0f}% dead</span>", dead_pct));
            }
            h.raw("</span></div>");
        }
        h.raw("</div>");
    }
    h.raw("</div></div>");
    return Response::html(std::move(h).finish());
}

auto DatabaseListHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_page("Databases", "Dashboard", [&](Html& h) {
        Alert::render({error_message(conn.error()), "error"}, h);
    }));

    auto dbs = pg::list_databases(conn->get());
    if (!dbs) return Response::html(render_page("Databases", "Dashboard", [&](Html& h) {
        Alert::render({error_message(dbs.error()), "error"}, h);
    }));

    auto render = [&](Html& h) {
        Table::begin(h, {{"Name", "", true}, {"Owner", "", true}, {"Encoding", "", true}, {"Size", "num", true}, {"Size (bytes)", "num", true}});
        for (auto& db : *dbs) {
            auto link = std::format("<a href=\"/db/{}/schemas\">{}</a>", escape(db.name), escape(db.name));
            Table::row(h, {{link, escape(db.owner), escape(db.encoding), escape(db.size), std::to_string(db.size_bytes)}});
        }
        Table::end(h);
    };

    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page("Databases", "Dashboard", render));
}

auto SchemaListHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));
    auto schemas = pg::list_schemas(conn->get(), db_name);
    if (!schemas) return Response::error(error_message(schemas.error()));

    auto render = [&](Html& h) {
        Breadcrumbs::render(std::vector<Crumb>{{"Databases", "/databases"}, {std::string(db_name), ""}}, h);
        Table::begin(h, {{"Schema", "", true}, {"Owner", "", true}});
        for (auto& s : *schemas) {
            auto link = std::format("<a href=\"/db/{}/schema/{}/tables\">{}</a>", escape(db_name), escape(s.name), escape(s.name));
            Table::row(h, {{link, escape(s.owner)}});
        }
        Table::end(h);
    };
    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page(std::format("Schemas - {}", db_name), "Dashboard", render));
}

auto TableListHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db = req.param("db"); auto sc = req.param("schema");
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));
    auto tables = pg::list_tables(conn->get(), sc);
    if (!tables) return Response::error(error_message(tables.error()));

    auto render = [&](Html& h) {
        Breadcrumbs::render(std::vector<Crumb>{{"Databases", "/databases"}, {std::string(db), std::format("/db/{}/schemas", db)}, {std::string(sc), ""}}, h);
        h.raw("<div class=\"schema-nav\">");
        for (auto [href_sfx, label, primary] : std::initializer_list<std::tuple<const char*, const char*, bool>>{
            {"tables","Tables",true},{"functions","Functions",false},{"sequences","Sequences",false},
            {"indexes","Index Analysis",false},{"erd/page","ERD",false},{"create-table","+ Create",false}}) {
            h.raw("<a href=\"/db/").raw(db).raw("/schema/").raw(sc).raw("/").raw(href_sfx)
             .raw("\" class=\"btn btn-sm").raw(primary ? " btn-primary" : "").raw("\">").raw(label).raw("</a>");
        }
        h.raw("</div>");
        Table::begin(h, {{"Table","",true},{"Type","",true},{"Rows (est.)","num",true},{"Size","num",true}});
        for (auto& t : *tables) {
            auto link = std::format("<a href=\"/db/{}/schema/{}/table/{}\">{}</a>", escape(db), escape(sc), escape(t.name), escape(t.name));
            Table::row(h, {{link, render_to_string<Badge>(Badge::Props{t.type, t.type=="table"?"primary":"secondary"}), std::to_string(t.row_estimate), escape(t.size)}});
        }
        Table::end(h);
    };
    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page(std::format("Tables - {}.{}", db, sc), "Dashboard", render));
}

auto TableDetailHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    auto db = req.param("db"); auto sc = req.param("schema"); auto tb = req.param("table");
    auto base = std::format("/db/{}/schema/{}/table/{}", db, sc, tb);

    auto render = [&](Html& h) {
        Breadcrumbs::render(std::vector<Crumb>{{"Databases","/databases"},{std::string(db),std::format("/db/{}/schemas",db)},
            {std::string(sc),std::format("/db/{}/schema/{}/tables",db,sc)},{std::string(tb),""}}, h);

        h.raw("<div style=\"display:flex;align-items:center;justify-content:space-between;margin-bottom:var(--sp-3)\">");
        SectionTabs::render({{
            {"Columns", base+"/columns", true},
            {"Data", base+"/browse"},
            {"DDL", base+"/ddl"},
            {"Statistics", base+"/stats"}
        }}, "tab-content", h);
        h.raw("<div class=\"btn-group\">");
        h.raw("<button class=\"btn btn-sm\" hx-post=\"").raw(base).raw("/vacuum\" hx-target=\"#action-result\" hx-swap=\"innerHTML\">Vacuum</button>");
        h.raw("<button class=\"btn btn-sm\" hx-post=\"").raw(base).raw("/analyze\" hx-target=\"#action-result\" hx-swap=\"innerHTML\">Analyze</button>");
        h.raw("<button class=\"btn btn-sm btn-danger\" hx-post=\"").raw(base).raw("/truncate\" hx-target=\"#action-result\" hx-swap=\"innerHTML\" hx-confirm=\"TRUNCATE ").text(sc).raw(".").text(tb).raw("?\">Truncate</button>");
        h.raw("<a href=\"").raw(base).raw("/alter\" class=\"btn btn-sm\">Alter</a>");
        h.raw("<a href=\"").raw(base).raw("/import\" class=\"btn btn-sm\">Import</a>");
        h.raw("<button class=\"btn btn-sm btn-danger\" hx-post=\"").raw(base).raw("/drop\" hx-target=\"#action-result\" hx-swap=\"innerHTML\" hx-confirm=\"DROP TABLE ").text(sc).raw(".").text(tb).raw("? This cannot be undone!\">Drop</button>");
        h.raw("</div></div><div id=\"action-result\"></div>");
        // Tab content loads via JS tab handler — Columns is default, fires on load
        h.raw("<div id=\"tab-content\" hx-get=\"").raw(base).raw("/columns\" hx-trigger=\"load\" hx-swap=\"innerHTML\">");
        h.raw("<div class=\"loading\">Loading columns...</div>");
        h.raw("</div>");
    };
    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page(std::format("{}.{}",sc,tb), "Dashboard", render));
}

// Columns/Indexes/Constraints partial (loaded by default tab)
auto TableColumnsHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema"); auto tb = req.param("table");
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto h = Html::with_capacity(8192);

    auto cols = pg::describe_columns(conn->get(), sc, tb);
    if (cols) {
        Table::begin(h, {{"#","num"},{"Name",""},{"Type",""},{"Nullable",""},{"Default",""},{"PK",""}});
        for (auto& c : *cols) {
            Table::row(h, {{std::to_string(c.ordinal), std::format("<code>{}</code>",escape(c.name)),
                render_to_string<Badge>(Badge::Props{c.type,"secondary"}), c.nullable?std::string("YES"):std::string("<strong>NO</strong>"),
                c.default_value.empty()?std::string("&mdash;"):std::format("<code>{}</code>",escape(c.default_value)),
                c.is_primary_key?render_to_string<Badge>(Badge::Props{"PK","primary"}):std::string("")}});
        }
        Table::end(h);
    }

    auto idxs = pg::list_indexes(conn->get(), sc, tb);
    if (idxs && !idxs->empty()) {
        h.raw("<h3>Indexes</h3>");
        Table::begin(h, {{"Name",""},{"Definition",""},{"Unique",""},{"Primary",""},{"Size","num"}});
        for (auto& i : *idxs) {
            Table::row(h, {{escape(i.name),std::format("<code>{}</code>",escape(i.definition)),
                std::string(i.is_unique?"&#10003;":""),std::string(i.is_primary?"&#10003;":""),escape(i.size)}});
        }
        Table::end(h);
    }

    auto cons = pg::list_constraints(conn->get(), sc, tb);
    if (cons && !cons->empty()) {
        h.raw("<h3>Constraints</h3>");
        Table::begin(h, {{"Name",""},{"Type",""},{"Definition",""}});
        for (auto& c : *cons) {
            Table::row(h, {{escape(c.name),render_to_string<Badge>(Badge::Props{c.type,"secondary"}),std::format("<code>{}</code>",escape(c.definition))}});
        }
        Table::end(h);
    }

    return Response::html(std::move(h).finish());
}

auto TableDataHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema"); auto tb = req.param("table");
    int limit = 100;
    if (auto ls = req.query("limit"); !ls.empty()) { try { limit = std::stoi(std::string(ls)); } catch(...){} }
    if (limit > 1000) limit = 1000;
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));
    auto result = pg::preview_rows(conn->get(), sc, tb, limit);
    if (!result) return Response::error(error_message(result.error()));

    auto h = Html::with_capacity(16384);
    std::vector<DCol> cols;
    for (int c = 0; c < result->col_count(); ++c) cols.push_back({result->column_name(c)});
    {
        auto view = DataView::readonly(h, {.row_count = result->row_count()});
        view.columns(cols);
        for (auto row : *result) {
            std::vector<Cell> cells;
            for (int c = 0; c < row.col_count(); ++c) {
                if (row.is_null(c)) cells.push_back({.is_null = true});
                else cells.push_back({.value = std::string(row[c])});
            }
            view.row(cells);
        }
    }
    return Response::html(std::move(h).finish());
}

auto TableDDLHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema"); auto tb = req.param("table");
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));
    auto ddl = pg::table_ddl(conn->get(), sc, tb);
    if (!ddl) return Response::html(render_to_string<Alert>(Alert::Props{error_message(ddl.error()), "error"}));
    auto h = Html::with_capacity(ddl->size() + 256);
    h.raw("<div class=\"ddl-toolbar\"><button class=\"btn btn-sm\" onclick=\"navigator.clipboard.writeText(document.querySelector('.ddl-source').textContent).then(()=>this.textContent='Copied!')\">Copy to Clipboard</button></div>");
    h.raw("<pre class=\"function-source ddl-source\">").text(*ddl).raw("</pre>");
    return Response::html(std::move(h).finish());
}

auto ColumnStatsHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema"); auto tb = req.param("table");
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));
    auto stats = pg::column_statistics(conn->get(), sc, tb);
    if (!stats) return Response::html(render_to_string<Alert>(Alert::Props{error_message(stats.error()), "error"}));
    if (stats->empty()) return Response::html(render_to_string<Alert>(Alert::Props{"No statistics. Run ANALYZE first.", "warning"}));

    auto h = Html::with_capacity(8192);
    for (auto& s : *stats) {
        auto null_pct = s.null_fraction * 100;
        auto distinct = s.n_distinct < 0 ? std::format("{:.0f}% unique", static_cast<double>(-s.n_distinct)*100.0)
                                          : std::to_string(s.n_distinct) + " distinct";
        h.raw("<div class=\"col-stat-card\"><div class=\"col-stat-header\"><strong>").text(s.column_name)
         .raw("</strong> ").raw(render_to_string<Badge>(Badge::Props{s.data_type,"secondary"}))
         .raw("</div><div class=\"col-stat-body\">");
        h.raw("<div class=\"col-stat-row\"><span class=\"col-stat-label\">Null %</span>"
              "<div class=\"col-stat-bar-track\"><div class=\"col-stat-bar\" style=\"width:")
         .raw(std::format("{:.1f}", null_pct)).raw("%\"></div></div><span class=\"col-stat-value\">")
         .raw(std::format("{:.1f}%", null_pct)).raw("</span></div>");
        h.raw("<div class=\"col-stat-row\"><span class=\"col-stat-label\">Distinct</span><span class=\"col-stat-value\">").raw(distinct).raw("</span></div>");
        h.raw("<div class=\"col-stat-row\"><span class=\"col-stat-label\">Avg Width</span><span class=\"col-stat-value\">").raw(std::to_string(s.avg_width)).raw(" bytes</span></div>");
        h.raw("<div class=\"col-stat-row\"><span class=\"col-stat-label\">Correlation</span><span class=\"col-stat-value\">").raw(std::format("{:.4f}", s.correlation)).raw("</span></div>");
        if (!s.most_common_vals.empty()) {
            auto v = s.most_common_vals.size()>200 ? s.most_common_vals.substr(0,200)+"..." : s.most_common_vals;
            h.raw("<div class=\"col-stat-row\"><span class=\"col-stat-label\">Top Values</span><code class=\"col-stat-vals\">").text(v).raw("</code></div>");
        }
        h.raw("</div></div>");
    }
    return Response::html(std::move(h).finish());
}

auto VacuumHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema"); auto tb = req.param("table");
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));
    auto r = pg::vacuum_table(conn->get(), sc, tb);
    if (!r) return Response::html(render_to_string<Alert>(Alert::Props{error_message(r.error()), "error"}));
    return Response::html(render_to_string<Alert>(Alert::Props{std::format("VACUUM completed on {}.{}", sc, tb), "info"}));
}

auto AnalyzeHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema"); auto tb = req.param("table");
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));
    auto r = pg::analyze_table(conn->get(), sc, tb);
    if (!r) return Response::html(render_to_string<Alert>(Alert::Props{error_message(r.error()), "error"}));
    return Response::html(render_to_string<Alert>(Alert::Props{std::format("ANALYZE completed on {}.{}", sc, tb), "info"}));
}

static auto json_esc(std::string_view s) -> std::string {
    std::string out; out.reserve(s.size()+4);
    for (char c : s) { if (c=='"') out+="\\\""; else if (c=='\\') out+="\\\\"; else if (c=='\n') out+="\\n"; else out+=c; }
    return out;
}

auto ERDDataHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema");
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::json("{\"tables\":[],\"relationships\":[]}", 500);
    auto erd = pg::schema_erd(conn->get(), sc);
    if (!erd) return Response::json("{\"tables\":[],\"relationships\":[]}", 500);

    auto h = Html::with_capacity(16384);
    h.raw("{\"tables\":[");
    for (std::size_t i = 0; i < erd->tables.size(); ++i) {
        if (i) h.raw(',');
        auto& t = erd->tables[i];
        h.raw("{\"name\":\"").raw(json_esc(t.name)).raw("\",\"type\":\"").raw(json_esc(t.type)).raw("\",\"columns\":[");
        for (std::size_t j = 0; j < t.columns.size(); ++j) {
            if (j) h.raw(',');
            h.raw("{\"name\":\"").raw(json_esc(t.columns[j].first)).raw("\",\"type\":\"").raw(json_esc(t.columns[j].second)).raw("\"}");
        }
        h.raw("]}");
    }
    h.raw("],\"relationships\":[");
    for (std::size_t i = 0; i < erd->relationships.size(); ++i) {
        if (i) h.raw(',');
        auto& r = erd->relationships[i];
        h.raw("{\"name\":\"").raw(json_esc(r.constraint_name)).raw("\",\"source\":\"").raw(json_esc(r.source_table))
         .raw("\",\"sourceCol\":\"").raw(json_esc(r.source_columns)).raw("\",\"target\":\"").raw(json_esc(r.target_table))
         .raw("\",\"targetCol\":\"").raw(json_esc(r.target_columns)).raw("\"}");
    }
    h.raw("]}");
    return Response::json(std::move(h).finish());
}

auto ERDPageHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    auto db = req.param("db"); auto sc = req.param("schema");
    auto render = [&](Html& h) {
        Breadcrumbs::render(std::vector<Crumb>{{"Databases","/databases"},{std::string(db),std::format("/db/{}/schemas",db)},
            {std::string(sc),std::format("/db/{}/schema/{}/tables",db,sc)},{"ERD",""}}, h);
        h.raw("<div id=\"erd-container\" class=\"erd-container\" data-url=\"/db/").raw(db).raw("/schema/").raw(sc)
         .raw("/erd\"><div class=\"loading\">Loading ERD...</div></div>");
    };
    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page(std::format("ERD - {}.{}", db, sc), "Dashboard", render));
}

// ─── TableBrowseHandler — fast paginated editable data grid ─────────

auto TableBrowseHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");

    auto page_str = req.query("page");
    auto limit_str = req.query("limit");
    auto sort_col = req.query("sort");
    auto sort_dir = req.query("dir");

    int page = 1, limit = 50;
    if (!page_str.empty()) try { page = std::stoi(std::string(page_str)); } catch (...) {}
    if (!limit_str.empty()) try { limit = std::stoi(std::string(limit_str)); } catch (...) {}
    if (page < 1) page = 1;
    if (limit < 1 || limit > 500) limit = 50;
    int offset = (page - 1) * limit;

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));

    // Count total rows (use estimate for speed on large tables)
    auto count_res = conn->get().exec(std::format(
        "SELECT reltuples::bigint FROM pg_class c JOIN pg_namespace n ON n.oid=c.relnamespace "
        "WHERE n.nspname='{}' AND c.relname='{}'", schema_name, table_name));
    long long total_rows = 0;
    if (count_res && count_res->row_count() > 0) {
        total_rows = count_res->get_int(0, 0).value_or(0);
        if (total_rows < 0) total_rows = 0; // -1 means never analyzed
    }


    // Type-safe: identifiers quoted, limit/offset use $1/$2
    auto browse_query = pg::sql::browse(schema_name, table_name, limit, offset,
                                         sort_col, sort_dir);
    auto result = conn->get().exec(browse_query);
    if (!result) return Response::html(
        std::format("<div class=\"query-error\">{}</div>", escape(error_message(result.error())))
    );

    auto base_url = std::format("/db/{}/schema/{}/table/{}/browse", db_name, schema_name, table_name);

    auto h = Html::with_capacity(16384);

    // Build column metadata (skip column 0 = ctid)
    std::vector<DCol> cols;
    for (int c = 1; c < result->col_count(); ++c) {
        cols.push_back({result->column_name(c)});
    }

    {
        // Type-state: Editable view — row() would be a compile error here
        auto view = DataView::editable(h, {
            .row_count = result->row_count(),
            .db = db_name, .schema = schema_name, .table = table_name,
            .page = page, .limit = limit, .total_rows = total_rows,
            .base_url = base_url
        });
        view.columns(cols);

        // Insert form fields are now rendered by columns() in the SSR component

        for (auto row : *result) {
            auto ctid = std::string(row[0]);
            auto row_num = offset + row.index() + 1;

            std::vector<Cell> cells;
            for (int c = 1; c < row.col_count(); ++c) {
                if (row.is_null(c)) cells.push_back({.is_null = true});
                else cells.push_back({.value = std::string(row[c])});
            }
            view.row(cells, ctid, row_num);
        }
    }

    return Response::html(std::move(h).finish());
}

// ─── CellUpdateHandler ──────────────────────────────────────────────

auto CellUpdateHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");
    auto body = std::string(req.body());

    auto col = form_value(body, "col");
    auto val = form_value(body, "val");
    auto ctid = form_value(body, "ctid");

    if (col.empty() || ctid.empty()) return Response::json("{\"error\":\"Missing parameters\"}", 400);

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::json("{\"error\":\"Connection failed\"}", 500);

    // Type-safe: uses $1/$2 placeholders — SQL injection impossible
    auto query = pg::sql::update_cell(schema_name, table_name, col, val, ctid);
    auto result = conn->get().exec(query);
    if (!result) {
        return Response::json(std::format("{{\"error\":\"{}\"}}", json_escape_str(error_message(result.error()))), 400);
    }
    return Response::json("{\"ok\":true}");
}

// ─── RowDeleteHandler ───────────────────────────────────────────────

auto RowDeleteHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");
    auto body = std::string(req.body());

    auto ctid = form_value(body, "ctid");
    if (ctid.empty()) return Response::error("No row identified", 400);

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));

    auto query = pg::sql::delete_row(schema_name, table_name, ctid);
    auto result = conn->get().exec(query);
    if (!result) return Response::html(render_to_string<Alert>(Alert::Props{error_message(result.error()), "error"}));

    auto browse_url = std::format("/db/{}/schema/{}/table/{}/browse", db_name, schema_name, table_name);
    return Response::html(std::format("<div hx-get=\"{}\" hx-trigger=\"load\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">"
        "<div class=\"alert alert-info\">Row deleted</div></div>", browse_url));
}

// ─── RowInsertHandler ───────────────────────────────────────────────

auto RowInsertHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");
    auto body = std::string(req.body());

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));

    // Get column names
    auto cols_res = pg::describe_columns(conn->get(), schema_name, table_name);
    if (!cols_res) return Response::html(render_to_string<Alert>(Alert::Props{error_message(cols_res.error()), "error"}));

    // Collect non-empty columns and values
    std::vector<std::string> insert_cols;
    std::vector<std::string> insert_vals;
    int col_idx = 0;
    for (auto& c : *cols_res) {
        auto v = form_value(body, std::format("col_{}", col_idx));
        if (!v.empty()) {
            insert_cols.push_back(c.name);
            insert_vals.push_back(v);
        }
        col_idx++;
    }

    if (insert_cols.empty()) return Response::html(render_to_string<Alert>(Alert::Props{"No values provided", "warning"}));

    // Build INSERT with $1/$2 placeholders — SQL injection impossible
    auto q = pg::sql::query();
    q.raw("INSERT INTO ").id(schema_name, table_name).raw(" (");
    for (std::size_t i = 0; i < insert_cols.size(); ++i) {
        if (i > 0) q.raw(", ");
        q.id(insert_cols[i]);
    }
    q.raw(") VALUES (");
    for (std::size_t i = 0; i < insert_vals.size(); ++i) {
        if (i > 0) q.raw(", ");
        q.val(insert_vals[i]);
    }
    q.raw(")");
    auto query = q.build();
    auto result = conn->get().exec(query);
    if (!result) return Response::html(
        std::format("<div class=\"query-error\">{}</div>", escape(error_message(result.error())))
    );

    auto browse_url = std::format("/db/{}/schema/{}/table/{}/browse", db_name, schema_name, table_name);
    return Response::html(std::format("<div hx-get=\"{}\" hx-trigger=\"load\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">"
        "<div class=\"alert alert-info\">Row inserted</div></div>", browse_url));
}

// ─── TruncateTableHandler ───────────────────────────────────────────

auto TruncateTableHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));

    auto sql = std::format("TRUNCATE TABLE \"{}\".\"{}\"", schema_name, table_name);
    auto result = conn->get().exec(sql);
    if (!result) return Response::html(render_to_string<Alert>(Alert::Props{error_message(result.error()), "error"}));

    return Response::html(render_to_string<Alert>(Alert::Props{std::format("Table {}.{} truncated", schema_name, table_name), "info"}));
}

// ─── TableExportHandler (CSV) ───────────────────────────────────────

auto TableExportHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto result = conn->get().exec(std::format("SELECT * FROM \"{}\".\"{}\"", schema_name, table_name));
    if (!result) return Response::error(error_message(result.error()));

    std::string csv;
    // Header
    for (int c = 0; c < result->col_count(); ++c) {
        if (c > 0) csv += ',';
        csv += std::format("\"{}\"", result->column_name(c));
    }
    csv += '\n';

    // Rows
    for (auto row : *result) {
        for (int c = 0; c < row.col_count(); ++c) {
            if (c > 0) csv += ',';
            if (row.is_null(c)) continue;
            auto val = row[c];
            // CSV escape: if contains comma, quote, or newline, wrap in quotes
            bool needs_quote = false;
            for (char ch : val) {
                if (ch == ',' || ch == '"' || ch == '\n' || ch == '\r') { needs_quote = true; break; }
            }
            if (needs_quote) {
                csv += '"';
                for (char ch : val) { if (ch == '"') csv += "\"\""; else csv += ch; }
                csv += '"';
            } else {
                csv += val;
            }
        }
        csv += '\n';
    }

    Response r = Response::text(std::move(csv));
    r.set("Content-Type", "text/csv");
    r.set("Content-Disposition", std::format("attachment; filename=\"{}.csv\"", table_name));
    return r;
}

} // namespace getgresql::api
