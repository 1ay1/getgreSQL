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
    auto render = [](Html& h) { DashboardShell::render(h); };
    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page("Dashboard", "Dashboard", render));
}

// Convert pg::HealthCheck vector to SSR component data
static auto to_health_data(const std::vector<pg::HealthCheck>& checks) -> std::vector<HealthCheckData> {
    std::vector<HealthCheckData> data;
    for (auto& c : checks) {
        data.push_back({c.name, c.status, c.value, c.detail, c.fix_action, c.fix_label});
    }
    return data;
}

auto DashboardHealthHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    using namespace ssr;
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));
    return Response::html(cache().get_or_compute("dashboard:health", std::chrono::seconds(5), [&] {
        auto checks = pg::health_checks(conn->get());
        if (!checks) return render_to_string<Alert>(Alert::Props{error_message(checks.error()), "error"});
        auto h = Html::with_capacity(4096);
        DashboardHealthRibbon::render({.checks = to_health_data(*checks)}, h);
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

    auto uptime_clean = s.uptime;
    if (auto dot = uptime_clean.find('.'); dot != std::string::npos) uptime_clean = uptime_clean.substr(0, dot);

    DashboardServer::render({.version = s.version, .uptime = uptime_clean, .pid = s.pid}, h);

    {
        auto metrics = html::open<html::Div>(h, {html::cls("dash-metrics")});

        // Connection gauge
        auto total_conns = s.active_connections + s.idle_connections + s.idle_in_transaction;
        auto conn_pct = s.max_connections > 0 ? static_cast<double>(total_conns) / s.max_connections * 100.0 : 0.0;
        auto conn_cls = conn_pct >= 90 ? "danger" : conn_pct >= 70 ? "warning" : "success";
        std::vector<std::pair<std::string, std::string>> conn_bd = {
            {"Active", std::to_string(s.active_connections)},
            {"Idle", std::to_string(s.idle_connections)},
        };
        if (s.idle_in_transaction > 0) conn_bd.push_back({"Idle Txn", std::to_string(s.idle_in_transaction)});
        DashboardRing::render({
            .percent = conn_pct, .color_class = conn_cls,
            .value_text = std::to_string(total_conns),
            .sub_text = "/ " + std::to_string(s.max_connections),
            .title = "Connections", .breakdown = conn_bd,
        }, h);

        // Cache hit ratio
        auto cache_pct = s.cache_hit_ratio * 100.0;
        auto cache_cls = cache_pct < 90 ? "danger" : cache_pct < 99 ? "warning" : "success";
        std::vector<std::pair<std::string, std::string>> cache_bd;
        if (s.blocks_hit + s.blocks_read > 0) {
            cache_bd = {{"Hit", std::to_string(s.blocks_hit)}, {"Read", std::to_string(s.blocks_read)}};
        }
        DashboardRing::render({
            .percent = cache_pct, .color_class = cache_cls,
            .value_text = std::format("{:.1f}%", cache_pct),
            .title = "Cache Hit Ratio", .breakdown = cache_bd,
        }, h);

        // Transaction throughput
        auto rollback_pct = (s.total_commits + s.total_rollbacks) > 0
            ? static_cast<double>(s.total_rollbacks) / (s.total_commits + s.total_rollbacks) * 100.0 : 0.0;
        std::string commit_str;
        if (s.total_commits >= 1000000) commit_str = std::format("{:.1f}M", s.total_commits / 1e6);
        else if (s.total_commits >= 1000) commit_str = std::format("{:.1f}K", s.total_commits / 1e3);
        else commit_str = std::to_string(s.total_commits);
        std::vector<std::pair<std::string, std::string>> txn_bd = {
            {"Commits", std::to_string(s.total_commits)},
        };
        if (s.total_rollbacks > 0)
            txn_bd.push_back({"Rollbacks", std::format("{} ({:.1f}%)", s.total_rollbacks, rollback_pct)});
        else
            txn_bd.push_back({"Rollbacks", "0"});
        DashboardBigNumber::render({
            .value = commit_str, .label = "commits",
            .title = "Transactions", .breakdown = txn_bd,
        }, h);

        // Database size
        auto db_size_result = conn->get().exec(
            "SELECT pg_size_pretty(pg_database_size(current_database())), "
            "pg_database_size(current_database()), "
            "(SELECT count(*) FROM pg_stat_user_tables), "
            "(SELECT count(*) FROM pg_stat_user_indexes)"
        );
        if (db_size_result && db_size_result->row_count() > 0) {
            StatCard::render({
                .label = "Database Size",
                .value = std::string((*db_size_result).get(0, 0)),
                .detail = std::string((*db_size_result).get(0, 2)) + " tables, "
                         + std::string((*db_size_result).get(0, 3)) + " indexes",
                .variant = "big",
            }, h);
        }
    } // dash-metrics
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

        DashboardSection::begin(h, "Databases",
            markup::spa_link_raw("btn btn-sm btn-ghost", "/databases", "View All &rsaquo;"));
        {
            auto grid = html::open<html::Div>(h, {html::cls("dash-db-grid")});
            for (auto& d : *dbs_res) {
                DatabaseCard::render({
                    .name = d.name, .size = d.size,
                    .size_pct = static_cast<double>(d.size_bytes) / max_size * 100,
                    .cache_hit_pct = d.cache_hit_ratio * 100.0,
                    .connections = d.connections,
                    .xact_commit = d.xact_commit,
                    .deadlocks = d.deadlocks,
                }, h);
            }
        }
        DashboardSection::end(h);
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

    DashboardSection::begin(h, "Live Activity",
        markup::spa_link_raw("btn btn-sm btn-ghost", "/monitor", "Monitor &rsaquo;"));

    auto is_self = [](const pg::ActivityEntry& q) {
        return q.query.find("pg_stat_activity") != std::string::npos
            && q.query.find("backend_type") != std::string::npos;
    };

    if (!queries || queries->empty()) {
        DashEmpty::render(h, "No connections");
    } else {
        int n_active = 0, n_idle = 0, n_idle_txn = 0, n_total = 0;
        for (auto& q : *queries) {
            if (is_self(q)) continue;
            n_total++;
            if (q.state == "active") n_active++;
            else if (q.state == "idle in transaction") n_idle_txn++;
            else n_idle++;
        }
        {
            using namespace html;
            auto summary = open<Div>(h, {cls("dash-activity-summary")});
            el<Span>(h, {cls("dash-activity-badge dash-ab-active")}, std::to_string(n_active) + " active");
            el<Span>(h, {cls("dash-activity-badge dash-ab-idle")}, std::to_string(n_idle) + " idle");
            if (n_idle_txn > 0) el<Span>(h, {cls("dash-activity-badge dash-ab-warn")}, std::to_string(n_idle_txn) + " idle txn");
            el<Span>(h, {cls("dash-activity-badge dash-ab-idle")}, std::to_string(n_total) + " total");
        }

        int shown = 0;
        {
            auto list = html::open<html::Div>(h, {html::cls("dash-activity-list")});
            for (auto& q : *queries) {
                if (q.state == "idle" || q.query.empty()) continue;
                if (is_self(q)) continue;
                if (shown >= 8) break;
                auto preview = q.query.size() > 100 ? q.query.substr(0, 100) + "..." : q.query;
                ActivityRow::render({.state = q.state, .database = q.database,
                    .user = q.user, .duration = q.duration, .query = preview}, h);
                shown++;
            }
        }
        if (shown == 0) DashEmpty::render(h, "All connections idle — no active queries");
    }
    DashboardSection::end(h);
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
    DashboardSection::begin(h, "Largest Tables");

    auto current_db = std::string(conn->get().dbname());

    if (!result || result->row_count() == 0) {
        DashEmpty::render(h, "No tables found");
    } else {
        long long max_bytes = 1;
        for (auto row : *result) {
            if (!row.is_null(3)) {
                auto val = std::stoll(std::string(row[3]));
                if (val > max_bytes) max_bytes = val;
            }
        }
        {
            auto chart = html::open<html::Div>(h, {html::cls("dash-bar-chart")});
            for (auto row : *result) {
                auto schema = std::string(row[0]);
                auto table = std::string(row[1]);
                auto size = std::string(row[2]);
                auto size_bytes = row.is_null(3) ? 0LL : std::stoll(std::string(row[3]));
                auto live = row.is_null(4) ? std::string("0") : std::string(row[4]);
                auto dead_pct = row.is_null(7) ? 0.0 : std::stod(std::string(row[7]));
                auto pct = max_bytes > 0 ? static_cast<double>(size_bytes) / max_bytes * 100.0 : 0.0;

                auto meta = live + " rows";
                if (dead_pct > 5) meta += std::format(" <span class=\"dash-bar-dead\">{:.0f}% dead</span>", dead_pct);

                BarChartRow::render({
                    .name = schema + "." + table,
                    .href_url = "/db/" + current_db + "/schema/" + schema + "/table/" + table,
                    .percent = pct, .is_warning = dead_pct > 20,
                    .size = size, .meta = meta,
                }, h);
            }
        }
    }
    DashboardSection::end(h);
    return Response::html(std::move(h).finish());
}

// ─── DashboardFixHandler — remediate health check issues ─────────────

auto DashboardFixHandler::handle(Request& req, AppContext& ctx) -> Response {
    using namespace ssr;
    auto body = std::string(req.body());
    auto action = form_value(body, "action");

    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));
    }

    std::string result_msg;

    if (action == "terminate-idle") {
        // Terminate idle connections older than 5 minutes (not our own)
        auto r = conn->get().exec(
            "SELECT pg_terminate_backend(pid) FROM pg_stat_activity "
            "WHERE state = 'idle' AND pid != pg_backend_pid() "
            "AND state_change < now() - interval '5 minutes'"
        );
        auto cnt = r ? r->row_count() : 0;
        result_msg = std::format("Terminated {} idle connections", cnt);

    } else if (action == "terminate-idle-txn") {
        // Terminate idle-in-transaction sessions older than 5 minutes
        auto r = conn->get().exec(
            "SELECT pg_terminate_backend(pid) FROM pg_stat_activity "
            "WHERE state = 'idle in transaction' AND pid != pg_backend_pid() "
            "AND xact_start < now() - interval '5 minutes'"
        );
        auto cnt = r ? r->row_count() : 0;
        result_msg = std::format("Terminated {} idle-in-transaction sessions", cnt);

    } else if (action == "vacuum-bloated") {
        // VACUUM tables with >20% dead rows
        auto tables = conn->get().exec(
            "SELECT schemaname, relname FROM pg_stat_user_tables "
            "WHERE n_dead_tup > n_live_tup * 0.2 AND n_live_tup > 100"
        );
        int vacuumed = 0;
        if (tables) {
            for (auto row : *tables) {
                auto schema = std::string(row[0]);
                auto table = std::string(row[1]);
                auto vq = "VACUUM ANALYZE " + schema + "." + table;
                conn->get().exec(vq);
                vacuumed++;
            }
        }
        result_msg = std::format("Vacuumed {} tables", vacuumed);

    } else if (action == "drop-inactive-slots") {
        // Drop inactive replication slots
        auto slots = conn->get().exec(
            "SELECT slot_name FROM pg_replication_slots WHERE NOT active"
        );
        int dropped = 0;
        if (slots) {
            for (auto row : *slots) {
                auto name = std::string(row[0]);
                conn->get().exec("SELECT pg_drop_replication_slot('" + name + "')");
                dropped++;
            }
        }
        result_msg = std::format("Dropped {} inactive replication slots", dropped);

    } else if (action.starts_with("show-setting:")) {
        auto setting = action.substr(13);
        return Response::redirect("/settings/search?q=" + setting);

    } else if (action == "show-blocking") {
        return Response::redirect("/monitor/blocking");

    } else {
        return Response::html(render_to_string<Alert>({"Unknown action: " + action, "warning"}));
    }

    // Clear the health cache so it re-checks after fix
    cache().clear();

    // Re-render health section with success message + fresh checks
    auto checks = pg::health_checks(conn->get());
    auto h = Html::with_capacity(4096);

    if (!checks) {
        Alert::render({error_message(checks.error()), "error"}, h);
        return Response::html(std::move(h).finish());
    }

    DashboardHealthRibbon::render({
        .checks = to_health_data(*checks),
        .toast_message = result_msg,
    }, h);
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
            auto link = markup::spa_link("/db/" + db.name + "/schemas", db.name);
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
            auto link = markup::spa_link("/db/" + std::string(db_name) + "/schema/" + s.name + "/tables", s.name);
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
        using namespace html;
        Breadcrumbs::render(std::vector<Crumb>{{"Databases", "/databases"}, {std::string(db), std::format("/db/{}/schemas", db)}, {std::string(sc), ""}}, h);
        {
            auto nav = open<Div>(h, {cls("schema-nav")});
            for (auto [href_sfx, label, primary] : std::initializer_list<std::tuple<const char*, const char*, bool>>{
                {"tables","Tables",true},{"functions","Functions",false},{"sequences","Sequences",false},
                {"indexes","Index Analysis",false},{"erd/page","ERD",false},{"create-table","+ Create",false}}) {
                el<A>(h, {href("/db/" + std::string(db) + "/schema/" + std::string(sc) + "/" + href_sfx),
                          cls(primary ? "btn btn-sm btn-primary" : "btn btn-sm")}, label);
            }
        }
        Table::begin(h, {{"Table","",true},{"Type","",true},{"Rows (est.)","num",true},{"Size","num",true}});
        for (auto& t : *tables) {
            auto link = markup::spa_link("/db/" + std::string(db) + "/schema/" + std::string(sc) + "/table/" + t.name, t.name);
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

        using namespace html;
        {
            auto toolbar = open<Div>(h, {style("display:flex;align-items:center;justify-content:space-between;margin-bottom:var(--sp-3)")});
            SectionTabs::render({{
                {"Columns", base+"/columns", true},
                {"Data", base+"/browse"},
                {"DDL", base+"/ddl"},
                {"Statistics", base+"/stats"}
            }}, "tab-content", h);
            {
                auto btns = open<Div>(h, {cls("btn-group")});
                el<Button>(h, {cls("btn btn-sm"), hx_post(base+"/vacuum"), hx_target("#action-result"), hx_swap("innerHTML")}, "Vacuum");
                el<Button>(h, {cls("btn btn-sm"), hx_post(base+"/analyze"), hx_target("#action-result"), hx_swap("innerHTML")}, "Analyze");
                el<Button>(h, {cls("btn btn-sm btn-danger"), hx_post(base+"/truncate"), hx_target("#action-result"), hx_swap("innerHTML"),
                    hx_confirm("TRUNCATE " + std::string(sc) + "." + std::string(tb) + "?")}, "Truncate");
                el<A>(h, {href(base+"/alter"), cls("btn btn-sm")}, "Alter");
                el<A>(h, {href(base+"/import"), cls("btn btn-sm")}, "Import");
                el<Button>(h, {cls("btn btn-sm btn-danger"), hx_post(base+"/drop"), hx_target("#action-result"), hx_swap("innerHTML"),
                    hx_confirm("DROP TABLE " + std::string(sc) + "." + std::string(tb) + "? This cannot be undone!")}, "Drop");
            }
        }
        el<Div>(h, {id("action-result")});
        {
            auto tab = open<Div>(h, {id("tab-content"), hx_get(base+"/columns"), hx_trigger("load"), hx_swap("innerHTML")});
            el<Div>(h, {cls("loading")}, "Loading columns...");
        }
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
            Table::row(h, {{std::to_string(c.ordinal), markup::code(c.name),
                render_to_string<Badge>(Badge::Props{c.type,"secondary"}), c.nullable?std::string("YES"):markup::strong("NO"),
                c.default_value.empty()?markup::mdash():markup::code(c.default_value),
                c.is_primary_key?render_to_string<Badge>(Badge::Props{"PK","primary"}):markup::empty()}});
        }
        Table::end(h);
    }

    auto idxs = pg::list_indexes(conn->get(), sc, tb);
    if (idxs && !idxs->empty()) {
        html::el<html::H3>(h, {}, "Indexes");
        Table::begin(h, {{"Name",""},{"Definition",""},{"Unique",""},{"Primary",""},{"Size","num"}});
        for (auto& i : *idxs) {
            Table::row(h, {{escape(i.name),markup::code(i.definition),
                markup::bool_check(i.is_unique),markup::bool_check(i.is_primary),escape(i.size)}});
        }
        Table::end(h);
    }

    auto cons = pg::list_constraints(conn->get(), sc, tb);
    if (cons && !cons->empty()) {
        html::el<html::H3>(h, {}, "Constraints");
        Table::begin(h, {{"Name",""},{"Type",""},{"Definition",""}});
        for (auto& c : *cons) {
            Table::row(h, {{escape(c.name),render_to_string<Badge>(Badge::Props{c.type,"secondary"}),markup::code(c.definition)}});
        }
        Table::end(h);
    }

    return Response::html(std::move(h).finish());
}

auto TableDataHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto sc = req.param("schema"); auto tb = req.param("table");
    int limit = 100;
    if (auto ls = req.query("limit"); !ls.empty()) { try { limit = std::stoi(std::string(ls)); } catch(...){} }
    if (limit > 1000) limit = 1000;
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    // Get table OID for Explain This
    unsigned int tbl_oid = 0;
    if (auto oid_r = conn->get().exec(std::format(
            "SELECT c.oid FROM pg_class c JOIN pg_namespace n ON n.oid=c.relnamespace "
            "WHERE n.nspname='{}' AND c.relname='{}'", sc, tb));
        oid_r && oid_r->row_count() > 0) {
        tbl_oid = static_cast<unsigned int>(oid_r->get_int(0, 0).value_or(0));
    }

    auto result = pg::preview_rows(conn->get(), sc, tb, limit);
    if (!result) return Response::error(error_message(result.error()));

    auto h = Html::with_capacity(16384);
    std::vector<DCol> cols;
    for (int c = 0; c < result->col_count(); ++c) cols.push_back({.name = result->column_name(c), .table_oid = tbl_oid});
    {
        auto view = DataView::readonly(h, {.row_count = result->row_count(), .db = db_name});
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
    {
        using namespace html;
        auto tb = open<Div>(h, {cls("ddl-toolbar")});
        el<Button>(h, {cls("btn btn-sm"),
            js::on_click(js::raw_js("navigator.clipboard.writeText(document.querySelector('.ddl-source').textContent).then(()=>this.textContent='Copied!'))"))},
            "Copy to Clipboard");
    }
    html::el<html::Pre>(h, {html::cls("function-source ddl-source")}, *ddl);
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
        using namespace html;
        auto null_pct = s.null_fraction * 100;
        auto distinct = s.n_distinct < 0 ? std::format("{:.0f}% unique", static_cast<double>(-s.n_distinct)*100.0)
                                          : std::to_string(s.n_distinct) + " distinct";
        {
            auto card = open<Div>(h, {cls("col-stat-card")});
            {
                auto hdr = open<Div>(h, {cls("col-stat-header")});
                html::el<html::Strong>(h, {}, s.column_name);
                h.raw(" ");
                Badge::render({s.data_type, "secondary"}, h);
            }
            {
                auto body = open<Div>(h, {cls("col-stat-body")});
                // Null %
                {
                    auto row = open<Div>(h, {cls("col-stat-row")});
                    el<Span>(h, {cls("col-stat-label")}, "Null %");
                    {
                        auto track = open<Div>(h, {cls("col-stat-bar-track")});
                        el<Div>(h, {cls("col-stat-bar"), style("width:" + std::format("{:.1f}", null_pct) + "%")});
                    }
                    el<Span>(h, {cls("col-stat-value")}, std::format("{:.1f}%", null_pct));
                }
                // Distinct
                {
                    auto row = open<Div>(h, {cls("col-stat-row")});
                    el<Span>(h, {cls("col-stat-label")}, "Distinct");
                    el<Span>(h, {cls("col-stat-value")}, distinct);
                }
                // Avg Width
                {
                    auto row = open<Div>(h, {cls("col-stat-row")});
                    el<Span>(h, {cls("col-stat-label")}, "Avg Width");
                    el<Span>(h, {cls("col-stat-value")}, std::to_string(s.avg_width) + " bytes");
                }
                // Correlation
                {
                    auto row = open<Div>(h, {cls("col-stat-row")});
                    el<Span>(h, {cls("col-stat-label")}, "Correlation");
                    el<Span>(h, {cls("col-stat-value")}, std::format("{:.4f}", s.correlation));
                }
                if (!s.most_common_vals.empty()) {
                    auto v = s.most_common_vals.size() > 200 ? s.most_common_vals.substr(0, 200) + "..." : s.most_common_vals;
                    auto row = open<Div>(h, {cls("col-stat-row")});
                    el<Span>(h, {cls("col-stat-label")}, "Top Values");
                    el<Code>(h, {cls("col-stat-vals")}, v);
                }
            }
        }
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
        {
            using namespace html;
            auto erd = open<Div>(h, {id("erd-container"), cls("erd-container"),
                data("url", "/db/" + std::string(db) + "/schema/" + std::string(sc) + "/erd")});
            el<Div>(h, {cls("loading")}, "Loading ERD...");
        }
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

    // Get table OID and row count estimate in one query
    auto meta_res = conn->get().exec(std::format(
        "SELECT c.oid, reltuples::bigint FROM pg_class c JOIN pg_namespace n ON n.oid=c.relnamespace "
        "WHERE n.nspname='{}' AND c.relname='{}'", schema_name, table_name));
    long long total_rows = 0;
    unsigned int table_oid = 0;
    if (meta_res && meta_res->row_count() > 0) {
        table_oid = static_cast<unsigned int>(meta_res->get_int(0, 0).value_or(0));
        total_rows = meta_res->get_int(0, 1).value_or(0);
        if (total_rows < 0) total_rows = 0;
    }


    // Type-safe: identifiers quoted, limit/offset use $1/$2
    auto browse_query = pg::sql::browse(schema_name, table_name, limit, offset,
                                         sort_col, sort_dir);
    auto result = conn->get().exec(browse_query);
    if (!result) return Response::html(
        render_to_string<Alert>(Alert::Props{error_message(result.error()), "error"})
    );

    auto base_url = std::format("/db/{}/schema/{}/table/{}/browse", db_name, schema_name, table_name);

    // Build column metadata (skip column 0 = ctid)
    // Set table_oid on every column so Explain This works
    std::vector<DCol> cols;
    for (int c = 1; c < result->col_count(); ++c) {
        cols.push_back({.name = result->column_name(c), .table_oid = table_oid});
    }

    auto h = Html::with_capacity(16384);

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
    return Response::html(render_partial([&](Html& h) {
        using namespace html;
        auto wrap = open<Div>(h, {hx_get(browse_url), hx_trigger("load"), hx_target("#tab-content"), hx_swap("innerHTML")});
        Alert::render({"Row deleted", "info"}, h);
    }));
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
        render_to_string<Alert>(Alert::Props{error_message(result.error()), "error"})
    );

    auto browse_url = std::format("/db/{}/schema/{}/table/{}/browse", db_name, schema_name, table_name);
    return Response::html(render_partial([&](Html& h) {
        using namespace html;
        auto wrap = open<Div>(h, {hx_get(browse_url), hx_trigger("load"), hx_target("#tab-content"), hx_swap("innerHTML")});
        Alert::render({"Row inserted", "info"}, h);
    }));
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
