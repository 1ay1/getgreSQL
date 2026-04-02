#include "api/handlers_admin.hpp"
#include "core/expected.hpp"
#include "ssr/components.hpp"
#include "pg/catalog.hpp"
#include "pg/monitor.hpp"

#include <format>

namespace getgresql::api {

using namespace ssr;

// ─── RolesHandler ───────────────────────────────────────────────────

auto RolesHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(render_page("Roles", "Dashboard", [&](Html& h) {
            Alert::render({error_message(conn.error()), "error"}, h);
        }));
    }

    auto roles = pg::list_roles(conn->get());
    if (!roles) {
        return Response::html(render_page("Roles", "Dashboard", [&](Html& h) {
            Alert::render({error_message(roles.error()), "error"}, h);
        }));
    }

    auto h = Html::with_capacity(8192);
    Table::begin(h, {{
        {"Name", "", true}, {"Superuser", ""}, {"Login", ""}, {"Create DB", ""},
        {"Create Role", ""}, {"Connection Limit", "num", true}, {"Valid Until", ""},
        {"Member Of", ""}
    }});

    for (auto& r : *roles) {
        auto conn_limit = r.connection_limit < 0
            ? std::string("unlimited")
            : std::to_string(r.connection_limit);

        Table::row(h, {{
            markup::strong(r.name),
            markup::bool_yes_no(r.is_superuser),
            markup::bool_yes_no(r.can_login),
            markup::bool_yes_no(r.can_create_db),
            markup::bool_yes_no(r.can_create_role),
            conn_limit,
            r.valid_until.empty() ? markup::mdash() : markup::detail::esc(r.valid_until),
            r.member_of.empty() ? markup::mdash() : markup::detail::esc(r.member_of),
        }});
    }
    Table::end(h);

    if (req.is_htmx()) return Response::html(std::move(h).finish());
    return Response::html(render_page("Roles", "Dashboard", [&](Html& ph) {
        ph.raw(h.view());
    }));
}

// ─── ExtensionsHandler ──────────────────────────────────────────────

auto ExtensionsHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(render_page("Extensions", "Dashboard", [&](Html& h) {
            Alert::render({error_message(conn.error()), "error"}, h);
        }));
    }

    auto exts = pg::list_extensions(conn->get());
    if (!exts) {
        return Response::html(render_page("Extensions", "Dashboard", [&](Html& h) {
            Alert::render({error_message(exts.error()), "error"}, h);
        }));
    }

    auto h = Html::with_capacity(8192);
    Table::begin(h, {{
        {"Name", "", true}, {"Version", "", true}, {"Schema", "", true}, {"Description", "wide"}
    }});

    for (auto& e : *exts) {
        Table::row(h, {{
            markup::strong(e.name),
            render_to_string<Badge>({e.version, "secondary"}),
            markup::detail::esc(e.schema),
            markup::detail::esc(e.description),
        }});
    }
    Table::end(h);

    if (req.is_htmx()) return Response::html(std::move(h).finish());
    return Response::html(render_page("Extensions", "Dashboard", [&](Html& ph) {
        ph.raw(h.view());
    }));
}

// ─── SettingsHandler ────────────────────────────────────────────────

auto SettingsHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    using namespace html;
    auto h = Html::with_capacity(4096);
    {
        auto toolbar = open<Div>(h, {cls("settings-toolbar")});
        void_el<Input>(h, {type("search"), name("q"), placeholder("Search settings..."),
            cls("search-input"), aria_label("Search settings"),
            hx_get("/settings/search"), hx_trigger("input changed delay:300ms"),
            hx_target("#settings-results")});
        el_raw<Button>(h, {cls("btn btn-sm btn-warning"),
            hx_post("/settings/reload"), hx_target("#reload-status"), hx_swap("innerHTML"),
            hx_confirm("Reload PostgreSQL configuration?")},
            std::string(icon::reload) + " Reload Config");
        el<Span>(h, {id("reload-status")});
    }
    {
        auto results = open<Div>(h, {id("settings-results"),
            hx_get("/settings/search"), hx_trigger("load")});
        ui::loading(h, "Loading settings...");
    }

    if (req.is_htmx()) return Response::html(std::move(h).finish());
    return Response::html(render_page("Settings", "Dashboard", [&](Html& ph) {
        ph.raw(h.view());
    }));
}

// ─── SettingsSearchHandler ──────────────────────────────────────────

auto SettingsSearchHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto search = req.query("q");
    auto settings = pg::server_settings(conn->get(), search);
    if (!settings) return Response::html(render_to_string<Alert>({error_message(settings.error()), "error"}));

    if (settings->empty()) {
        return Response::html(render_partial([](Html& h) {
            ui::empty_state(h, "No settings found");
        }));
    }

    // Group by category
    std::string current_category;
    auto h = Html::with_capacity(16384);

    bool table_open = false;

    for (auto& s : *settings) {
        if (s.category != current_category) {
            if (table_open) {
                Table::end(h);
            }
            current_category = s.category;
            html::el<html::H4>(h, {}, current_category);
            Table::begin(h, {{
                {"Name", ""}, {"Value", ""}, {"Source", ""}, {"Context", ""},
                {"Description", "wide"}
            }});
            table_open = true;
        }

        auto context_variant =
            s.context == "postmaster" ? "danger" :
            s.context == "sighup"     ? "warning" :
            s.context == "superuser"  ? "primary" :
            s.context == "user"       ? "success" : "secondary";

        auto value_str = s.unit.empty()
            ? markup::code(s.setting)
            : markup::code(std::string(s.setting) + " " + std::string(s.unit));

        Table::row(h, {{
            markup::code(s.name),
            value_str,
            markup::detail::esc(s.source),
            render_to_string<Badge>({s.context, context_variant}),
            markup::detail::esc(s.short_desc),
        }});
    }

    if (table_open) {
        Table::end(h);
    }

    return Response::html(std::move(h).finish());
}

// ─── FunctionListHandler ────────────────────────────────────────────

auto FunctionListHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto funcs = pg::list_functions(conn->get(), schema_name);
    if (!funcs) return Response::error(error_message(funcs.error()));

    auto h = Html::with_capacity(8192);
    std::vector<Crumb> crumbs = {
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {std::string(schema_name), std::format("/db/{}/schema/{}/tables", db_name, schema_name)},
        {"Functions", ""},
    };
    Breadcrumbs::render(crumbs, h);

    Table::begin(h, {{
        {"Name", "", true}, {"Arguments", "wide"}, {"Return Type", "", true},
        {"Language", "", true}, {"Volatility", ""}
    }});

    for (auto& f : *funcs) {
        auto lang_variant =
            f.language == "plpgsql" ? "primary" :
            f.language == "sql"     ? "success" :
            f.language == "c"       ? "danger"  : "secondary";

        auto func_url = std::format("/db/{}/schema/{}/function/{}",
            db_name, schema_name, f.name);

        Table::row(h, {{
            markup::spa_link(func_url, f.name),
            markup::code(f.arguments),
            markup::detail::esc(f.return_type),
            render_to_string<Badge>({f.language, lang_variant}),
            render_to_string<Badge>({f.volatility, "secondary"}),
        }});
    }
    Table::end(h);

    auto title = std::format("Functions - {}.{}", db_name, schema_name);
    if (req.is_htmx()) return Response::html(std::move(h).finish());
    return Response::html(render_page(title, "Dashboard", [&](Html& ph) {
        ph.raw(h.view());
    }));
}

// ─── FunctionDetailHandler ──────────────────────────────────────────

auto FunctionDetailHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");
    auto func_name = req.param("func");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto source = pg::get_function_source(conn->get(), schema_name, func_name);
    if (!source) return Response::error(error_message(source.error()));

    auto h = Html::with_capacity(8192);
    std::vector<Crumb> crumbs = {
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {std::string(schema_name), std::format("/db/{}/schema/{}/tables", db_name, schema_name)},
        {"Functions", std::format("/db/{}/schema/{}/functions", db_name, schema_name)},
        {std::string(func_name), ""},
    };
    Breadcrumbs::render(crumbs, h);

    {
        auto p = html::open<html::P>(h);
        html::el<html::Strong>(h, {}, "Language:");
        h.raw(" ");
        Badge::render({source->language, "primary"}, h);
    }
    html::el<html::H3>(h, {}, "Source");
    html::el<html::Div>(h, {html::cls("function-source")}, source->source);

    if (!source->definition.empty()) {
        html::el<html::H3>(h, {}, "Full Definition");
        html::el<html::Div>(h, {html::cls("function-source")}, source->definition);
    }

    auto title = std::format("Function: {}", func_name);
    if (req.is_htmx()) return Response::html(std::move(h).finish());
    return Response::html(render_page(title, "Dashboard", [&](Html& ph) {
        ph.raw(h.view());
    }));
}

// ─── SequenceListHandler ────────────────────────────────────────────

auto SequenceListHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto seqs = pg::list_sequences(conn->get(), schema_name);
    if (!seqs) return Response::error(error_message(seqs.error()));

    auto h = Html::with_capacity(8192);
    std::vector<Crumb> crumbs = {
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {std::string(schema_name), std::format("/db/{}/schema/{}/tables", db_name, schema_name)},
        {"Sequences", ""},
    };
    Breadcrumbs::render(crumbs, h);

    Table::begin(h, {{
        {"Name", "", true}, {"Type", ""}, {"Current Value", "num", true}, {"Start", "num", true},
        {"Increment", "num", true}, {"Min", "num", true}, {"Max", "num", true}, {"Cycle", ""}
    }});

    for (auto& s : *seqs) {
        Table::row(h, {{
            markup::strong(s.name),
            render_to_string<Badge>({s.data_type, "secondary"}),
            std::to_string(s.current_value),
            std::to_string(s.start_value),
            std::to_string(s.increment),
            std::to_string(s.min_value),
            std::to_string(s.max_value),
            markup::bool_yes_no(s.cycle),
        }});
    }
    Table::end(h);

    auto title = std::format("Sequences - {}.{}", db_name, schema_name);
    if (req.is_htmx()) return Response::html(std::move(h).finish());
    return Response::html(render_page(title, "Dashboard", [&](Html& ph) {
        ph.raw(h.view());
    }));
}

// ─── IndexAnalysisHandler ───────────────────────────────────────────

auto IndexAnalysisHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto indexes = pg::index_usage_stats(conn->get(), schema_name);
    if (!indexes) return Response::error(error_message(indexes.error()));

    auto h = Html::with_capacity(8192);
    std::vector<Crumb> crumbs = {
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {std::string(schema_name), std::format("/db/{}/schema/{}/tables", db_name, schema_name)},
        {"Index Analysis", ""},
    };
    Breadcrumbs::render(crumbs, h);

    Table::begin(h, {{
        {"Index", "", true}, {"Table", "", true}, {"Size", "num", true}, {"Scans", "num", true},
        {"Tuples Read", "num", true}, {"Tuples Fetched", "num", true}
    }});

    for (auto& idx : *indexes) {
        auto row_attrs = idx.idx_scan == 0
            ? std::string_view(R"(class="row-warning")")
            : std::string_view("");

        auto name_cell = idx.idx_scan == 0
            ? markup::detail::esc(idx.index_name) + " " + render_to_string<Badge>({"UNUSED", "warning"})
            : markup::detail::esc(idx.index_name);

        Table::row(h, {{
            name_cell,
            markup::detail::esc(idx.table),
            markup::detail::esc(idx.index_size),
            std::to_string(idx.idx_scan),
            std::to_string(idx.idx_tup_read),
            std::to_string(idx.idx_tup_fetch),
        }}, row_attrs);
    }
    Table::end(h);

    auto title = std::format("Index Analysis - {}.{}", db_name, schema_name);
    if (req.is_htmx()) return Response::html(std::move(h).finish());
    return Response::html(render_page(title, "Dashboard", [&](Html& ph) {
        ph.raw(h.view());
    }));
}

// ─── DatabaseSizeHandler ────────────────────────────────────────────

auto DatabaseSizeHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto breakdown = pg::database_size_breakdown(conn->get());
    if (!breakdown) return Response::error(error_message(breakdown.error()));

    auto h = Html::with_capacity(8192);
    std::vector<Crumb> crumbs = {
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {"Size Breakdown", ""},
    };
    Breadcrumbs::render(crumbs, h);

    // Calculate total size
    long long total_bytes = 0;
    int total_tables = 0;
    int total_indexes = 0;
    for (auto& s : *breakdown) {
        total_bytes += s.size_bytes;
        total_tables += s.table_count;
        total_indexes += s.index_count;
    }

    auto format_size = [](long long bytes) -> std::string {
        if (bytes >= 1073741824LL) return std::format("{:.1f} GB", static_cast<double>(bytes) / 1073741824.0);
        if (bytes >= 1048576LL)    return std::format("{:.1f} MB", static_cast<double>(bytes) / 1048576.0);
        if (bytes >= 1024LL)       return std::format("{:.1f} KB", static_cast<double>(bytes) / 1024.0);
        return std::format("{} B", bytes);
    };

    ui::stat_grid(h, [&](Html& h) {
        StatCard::render({"Total Size", format_size(total_bytes)}, h);
        StatCard::render({"Tables", std::to_string(total_tables)}, h);
        StatCard::render({"Indexes", std::to_string(total_indexes)}, h);
        StatCard::render({"Schemas", std::to_string(breakdown->size())}, h);
    });

    Table::begin(h, {{
        {"Schema", "", true}, {"Size", "num", true}, {"Tables", "num", true}, {"Indexes", "num", true}
    }});

    for (auto& s : *breakdown) {
        Table::row(h, {{
            markup::detail::esc(s.schema),
            markup::detail::esc(s.size),
            std::to_string(s.table_count),
            std::to_string(s.index_count),
        }});
    }
    Table::end(h);

    auto title = std::format("Size - {}", db_name);
    if (req.is_htmx()) return Response::html(std::move(h).finish());
    return Response::html(render_page(title, "Dashboard", [&](Html& ph) {
        ph.raw(h.view());
    }));
}

// ─── ReplicationHandler ─────────────────────────────────────────────

auto ReplicationHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(render_page("Replication", "Monitor", [&](Html& h) {
            Alert::render({error_message(conn.error()), "error"}, h);
        }));
    }

    auto slots = pg::replication_slots(conn->get());
    if (!slots) {
        return Response::html(render_page("Replication", "Monitor", [&](Html& h) {
            Alert::render({error_message(slots.error()), "error"}, h);
        }));
    }

    auto h = Html::with_capacity(8192);

    if (slots->empty()) {
        ui::empty_state(h, "No replication slots configured");
    } else {
        Table::begin(h, {{
            {"Slot Name", "", true}, {"Type", ""}, {"Database", "", true}, {"Active", ""},
            {"Restart LSN", ""}, {"Confirmed Flush LSN", ""}
        }});

        for (auto& s : *slots) {
            Table::row(h, {{
                markup::strong(s.slot_name),
                render_to_string<Badge>({s.slot_type, "secondary"}),
                markup::detail::esc(s.database),
                s.active ? render_to_string<Badge>({"YES", "success"}) : render_to_string<Badge>({"NO", "danger"}),
                s.restart_lsn.empty() ? markup::mdash() : markup::code(s.restart_lsn),
                s.confirmed_flush_lsn.empty() ? markup::mdash() : markup::code(s.confirmed_flush_lsn),
            }});
        }
        Table::end(h);
    }

    if (req.is_htmx()) return Response::html(std::move(h).finish());
    return Response::html(render_page("Replication", "Monitor", [&](Html& ph) {
        ph.raw(h.view());
    }));
}

// ─── TableStatsHandler ──────────────────────────────────────────────

auto TableStatsHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    using namespace html;
    auto h = Html::with_capacity(4096);
    {
        auto selector = open<Div>(h, {cls("schema-selector")});
        el<Label>(h, {for_("schema-select")}, "Schema:");
        void_el<Input>(h, {type("text"), id("schema-select"), name("schema"), value("public"),
            hx_get("/monitor/tablestats/data"), hx_trigger("change, load"),
            hx_target("#tablestats-results"), hx_include("this")});
    }
    {
        auto results = open<Div>(h, {id("tablestats-results"),
            hx_get("/monitor/tablestats/data?schema=public"), hx_trigger("load")});
        ui::loading(h, "Loading table statistics...");
    }

    if (req.is_htmx()) return Response::html(std::move(h).finish());
    return Response::html(render_page("Table Statistics", "Monitor", [&](Html& ph) {
        ph.raw(h.view());
    }));
}

// ─── TableStatsDataHandler ──────────────────────────────────────────

auto TableStatsDataHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto schema = req.query("schema");
    if (schema.empty()) schema = "public";

    auto stats = pg::table_stats(conn->get(), schema);
    if (!stats) return Response::html(render_to_string<Alert>({error_message(stats.error()), "error"}));

    if (stats->empty()) {
        return Response::html(render_partial([](Html& h) {
            ui::empty_state(h, "No tables found in this schema");
        }));
    }

    auto h = Html::with_capacity(16384);
    Table::begin(h, {{
        {"Table", "", true}, {"Seq Scans", "num", true}, {"Idx Scans", "num", true},
        {"Inserts", "num", true}, {"Updates", "num", true}, {"Deletes", "num", true},
        {"Live Rows", "num", true}, {"Dead Rows", "num", true},
        {"Last Vacuum", ""}, {"Last Analyze", ""}
    }});

    for (auto& t : *stats) {
        bool high_dead = t.n_live_tup > 0 && t.n_dead_tup > static_cast<long long>(static_cast<double>(t.n_live_tup) * 0.1);

        auto dead_cell = high_dead
            ? std::to_string(t.n_dead_tup) + " " + render_to_string<Badge>({"HIGH", "warning"})
            : std::to_string(t.n_dead_tup);

        auto row_attrs = high_dead
            ? std::string_view(R"(class="row-warning")")
            : std::string_view("");

        auto last_vacuum = t.last_vacuum.empty()
            ? (t.last_autovacuum.empty() ? markup::mdash() : markup::detail::esc(t.last_autovacuum))
            : markup::detail::esc(t.last_vacuum);

        Table::row(h, {{
            markup::strong(t.table),
            std::to_string(t.seq_scan),
            std::to_string(t.idx_scan),
            std::to_string(t.n_tup_ins),
            std::to_string(t.n_tup_upd),
            std::to_string(t.n_tup_del),
            std::to_string(t.n_live_tup),
            dead_cell,
            last_vacuum,
            t.last_analyze.empty() ? markup::mdash() : markup::detail::esc(t.last_analyze),
        }}, row_attrs);
    }
    Table::end(h);

    return Response::html(std::move(h).finish());
}

// ─── TerminateHandler ───────────────────────────────────────────────

auto TerminateHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto pid_str = req.param("pid");
    int pid = 0;
    try { pid = std::stoi(std::string(pid_str)); }
    catch (...) { return Response::error("Invalid PID", 400); }

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto result = pg::terminate_backend(conn->get(), pid);
    if (!result) {
        return Response::html(render_to_string<Alert>({error_message(result.error()), "error"}));
    }

    return Response::html(render_to_string<Alert>({std::format("Terminated backend PID {}", pid), "info"}));
}

// ─── SettingsReloadHandler ──────────────────────────────────────────

auto SettingsReloadHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Badge>({error_message(conn.error()), "danger"}));

    auto result = conn->get().exec("SELECT pg_reload_conf()");
    if (!result) {
        return Response::html(render_to_string<Badge>({error_message(result.error()), "danger"}));
    }
    return Response::html(render_to_string<Badge>({"Config reloaded", "success"}));
}

// ─── UnusedIndexesHandler ──────────────────────────────────────────

auto UnusedIndexesHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto result = conn->get().exec(
        "SELECT s.schemaname, s.relname AS table, s.indexrelname AS index, "
        "pg_size_pretty(pg_relation_size(s.indexrelid)) AS size, "
        "pg_relation_size(s.indexrelid) AS size_bytes, "
        "s.idx_scan, "
        "i.indisunique, i.indisprimary, "
        "pg_get_indexdef(s.indexrelid) AS definition "
        "FROM pg_stat_user_indexes s "
        "JOIN pg_index i ON i.indexrelid = s.indexrelid "
        "WHERE s.idx_scan = 0 "
        "AND NOT i.indisprimary "
        "AND NOT i.indisunique "
        "ORDER BY pg_relation_size(s.indexrelid) DESC"
    );

    // Build data for the component
    std::vector<UnusedIndex> indexes;
    if (result) {
        for (int r = 0; r < result->row_count(); ++r) {
            indexes.push_back({
                .schema = std::string(result->get(r, 0)),
                .table = std::string(result->get(r, 1)),
                .index = std::string(result->get(r, 2)),
                .size = std::string(result->get(r, 3)),
                .size_bytes = result->get_int(r, 4).value_or(0),
                .definition = std::string(result->get(r, 8)),
            });
        }
    }

    auto render = [&](Html& h) {
        UnusedIndexPage::render({.indexes = indexes}, h);
    };

    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page("Unused Indexes", "Dashboard", render));
}

// ─── UnusedIndexDropHandler ──────────────────────────────────────────────

auto UnusedIndexDropHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto body = std::string(req.body());
    // Parse JSON body from hx-vals
    auto extract = [&](std::string_view key) -> std::string {
        auto needle = "\"" + std::string(key) + "\":\"";
        auto pos = body.find(needle);
        if (pos == std::string::npos) return {};
        auto start = pos + needle.size();
        auto end = body.find('"', start);
        return end == std::string::npos ? std::string{} : std::string(body.substr(start, end - start));
    };

    auto schema = extract("schema");
    auto index = extract("index");
    if (schema.empty() || index.empty()) {
        return Response::html(render_to_string<Alert>({"Missing schema or index name", "warning"}));
    }

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    auto sql = "DROP INDEX CONCURRENTLY \"" + schema + "\".\"" + index + "\"";
    auto result = conn->get().exec(sql);
    if (!result) {
        return Response::html(render_partial([&](Html& h) {
            auto tr = html::open<html::Tr>(h);
            html::el_raw<html::Td>(h, {html::attr("colspan", "6")},
                render_to_string<Alert>({error_message(result.error()), "error"}));
        }));
    }

    // Return empty (row disappears via outerHTML swap)
    return Response::html("");
}

// ─── PermissionsHandler ────────────────────────────────────────────

auto PermissionsHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));

    // Get role → table permissions
    auto perms = conn->get().exec(
        "SELECT grantee, table_schema, table_name, privilege_type, is_grantable "
        "FROM information_schema.table_privileges "
        "WHERE grantee NOT IN ('PUBLIC', 'pg_monitor', 'pg_read_all_settings', "
        "'pg_read_all_stats', 'pg_stat_scan_tables', 'pg_signal_backend', "
        "'pg_read_server_files', 'pg_write_server_files', 'pg_execute_server_program') "
        "AND table_schema NOT IN ('pg_catalog', 'information_schema') "
        "ORDER BY grantee, table_schema, table_name, privilege_type"
    );

    // Get role memberships
    auto memberships = conn->get().exec(
        "SELECT r.rolname AS role, m.rolname AS member_of "
        "FROM pg_auth_members am "
        "JOIN pg_roles r ON r.oid = am.roleid "
        "JOIN pg_roles m ON m.oid = am.member "
        "ORDER BY m.rolname, r.rolname"
    );

    // Build data for the component
    std::vector<RoleMembership> membership_data;
    if (memberships) {
        for (int r = 0; r < memberships->row_count(); ++r) {
            membership_data.push_back({
                .role = std::string(memberships->get(r, 0)),
                .member_of = std::string(memberships->get(r, 1)),
            });
        }
    }

    std::vector<PermissionGrant> grant_data;
    if (perms) {
        for (int r = 0; r < perms->row_count(); ++r) {
            grant_data.push_back({
                .grantee = std::string(perms->get(r, 0)),
                .schema = std::string(perms->get(r, 1)),
                .table = std::string(perms->get(r, 2)),
                .privilege = std::string(perms->get(r, 3)),
                .is_grantable = std::string(perms->get(r, 4)) == "YES",
            });
        }
    }

    auto render = [&](Html& h) {
        PermissionAuditPage::render({.memberships = membership_data, .grants = grant_data}, h);
    };

    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page("Permissions", "Dashboard", render));
}

} // namespace getgresql::api
