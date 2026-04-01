#include "api/handlers_admin.hpp"
#include "core/expected.hpp"
#include "ssr/components.hpp"
#include "pg/catalog.hpp"
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

// ─── URL decode helper ──────────────────────────────────────────────

static auto url_decode(std::string_view input) -> std::string {
    std::string decoded;
    decoded.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '+') {
            decoded += ' ';
        } else if (input[i] == '%' && i + 2 < input.size()) {
            auto hex = std::string(input.substr(i + 1, 2));
            decoded += static_cast<char>(std::stoi(hex, nullptr, 16));
            i += 2;
        } else {
            decoded += input[i];
        }
    }
    return decoded;
}

// ─── Form parsing helper ────────────────────────────────────────────

static auto form_value(std::string_view body, std::string_view key) -> std::string {
    auto needle = std::string(key) + "=";
    auto pos = body.find(needle);
    if (pos == std::string_view::npos) return {};

    auto start = pos + needle.size();
    auto end = body.find('&', start);
    auto raw = (end == std::string_view::npos)
        ? body.substr(start)
        : body.substr(start, end - start);
    return url_decode(raw);
}

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
        auto bool_badge = [](bool val) -> std::string {
            return val ? render_to_string<Badge>({"YES", "success"}) : render_to_string<Badge>({"NO", "secondary"});
        };

        auto conn_limit = r.connection_limit < 0
            ? std::string("unlimited")
            : std::to_string(r.connection_limit);

        Table::row(h, {{
            std::format("<strong>{}</strong>", escape(r.name)),
            bool_badge(r.is_superuser),
            bool_badge(r.can_login),
            bool_badge(r.can_create_db),
            bool_badge(r.can_create_role),
            conn_limit,
            r.valid_until.empty() ? std::string("&mdash;") : escape(r.valid_until),
            r.member_of.empty() ? std::string("&mdash;") : escape(r.member_of),
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
            std::format("<strong>{}</strong>", escape(e.name)),
            render_to_string<Badge>({e.version, "secondary"}),
            escape(e.schema),
            escape(e.description),
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
    auto h = Html::with_capacity(4096);
    h.raw(R"(
<div class="search-box">
    <input type="search" name="q" placeholder="Search settings..."
        hx-get="/settings/search" hx-trigger="input changed delay:300ms"
        hx-target="#settings-results" class="search-input">
</div>
<div id="settings-results" hx-get="/settings/search" hx-trigger="load">
    <div class="loading">Loading settings...</div>
</div>
)");

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
        return Response::html(R"(<div class="empty-state">No settings found</div>)");
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
            h.raw("<h4>").text(current_category).raw("</h4>");
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
            ? escape(s.setting)
            : std::format("{} {}", escape(s.setting), escape(s.unit));

        Table::row(h, {{
            std::format("<code>{}</code>", escape(s.name)),
            std::format("<code>{}</code>", value_str),
            escape(s.source),
            render_to_string<Badge>({s.context, context_variant}),
            escape(s.short_desc),
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

        Table::row(h, {{
            std::format("<a href=\"/db/{}/schema/{}/function/{}\">{}</a>",
                escape(db_name), escape(schema_name),
                escape(f.name), escape(f.name)),
            std::format("<code>{}</code>", escape(f.arguments)),
            escape(f.return_type),
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

    h.raw("<p><strong>Language:</strong> ").raw(render_to_string<Badge>({source->language, "primary"})).raw("</p>");

    h.raw("<h3>Source</h3>");
    h.raw("<div class=\"function-source\">").text(source->source).raw("</div>");

    if (!source->definition.empty()) {
        h.raw("<h3>Full Definition</h3>");
        h.raw("<div class=\"function-source\">").text(source->definition).raw("</div>");
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
            std::format("<strong>{}</strong>", escape(s.name)),
            render_to_string<Badge>({s.data_type, "secondary"}),
            std::to_string(s.current_value),
            std::to_string(s.start_value),
            std::to_string(s.increment),
            std::to_string(s.min_value),
            std::to_string(s.max_value),
            s.cycle ? render_to_string<Badge>({"YES", "success"}) : render_to_string<Badge>({"NO", "secondary"}),
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
            ? std::format("{} {}", escape(idx.index_name), render_to_string<Badge>({"UNUSED", "warning"}))
            : escape(idx.index_name);

        Table::row(h, {{
            name_cell,
            escape(idx.table),
            escape(idx.index_size),
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

    h.raw("<div class=\"stat-grid\">");
    StatCard::render({"Total Size", format_size(total_bytes)}, h);
    StatCard::render({"Tables", std::to_string(total_tables)}, h);
    StatCard::render({"Indexes", std::to_string(total_indexes)}, h);
    StatCard::render({"Schemas", std::to_string(breakdown->size())}, h);
    h.raw("</div>");

    Table::begin(h, {{
        {"Schema", "", true}, {"Size", "num", true}, {"Tables", "num", true}, {"Indexes", "num", true}
    }});

    for (auto& s : *breakdown) {
        Table::row(h, {{
            escape(s.schema),
            escape(s.size),
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
        h.raw(R"(<div class="empty-state">No replication slots configured</div>)");
    } else {
        Table::begin(h, {{
            {"Slot Name", "", true}, {"Type", ""}, {"Database", "", true}, {"Active", ""},
            {"Restart LSN", ""}, {"Confirmed Flush LSN", ""}
        }});

        for (auto& s : *slots) {
            Table::row(h, {{
                std::format("<strong>{}</strong>", escape(s.slot_name)),
                render_to_string<Badge>({s.slot_type, "secondary"}),
                escape(s.database),
                s.active ? render_to_string<Badge>({"YES", "success"}) : render_to_string<Badge>({"NO", "danger"}),
                s.restart_lsn.empty() ? std::string("&mdash;") : std::format("<code>{}</code>", escape(s.restart_lsn)),
                s.confirmed_flush_lsn.empty() ? std::string("&mdash;") : std::format("<code>{}</code>", escape(s.confirmed_flush_lsn)),
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
    auto h = Html::with_capacity(4096);
    h.raw(R"(
<div class="schema-selector">
    <label for="schema-select">Schema:</label>
    <input type="text" id="schema-select" name="schema" value="public"
        hx-get="/monitor/tablestats/data" hx-trigger="change, load"
        hx-target="#tablestats-results" hx-include="this">
</div>
<div id="tablestats-results" hx-get="/monitor/tablestats/data?schema=public" hx-trigger="load">
    <div class="loading">Loading table statistics...</div>
</div>
)");

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
        return Response::html(R"(<div class="empty-state">No tables found in this schema</div>)");
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
            ? std::format("{} {}", std::to_string(t.n_dead_tup), render_to_string<Badge>({"HIGH", "warning"}))
            : std::to_string(t.n_dead_tup);

        auto row_attrs = high_dead
            ? std::string_view(R"(class="row-warning")")
            : std::string_view("");

        auto last_vacuum = t.last_vacuum.empty()
            ? (t.last_autovacuum.empty() ? std::string("&mdash;") : escape(t.last_autovacuum))
            : escape(t.last_vacuum);

        Table::row(h, {{
            std::format("<strong>{}</strong>", escape(t.table)),
            std::to_string(t.seq_scan),
            std::to_string(t.idx_scan),
            std::to_string(t.n_tup_ins),
            std::to_string(t.n_tup_upd),
            std::to_string(t.n_tup_del),
            std::to_string(t.n_live_tup),
            dead_cell,
            last_vacuum,
            t.last_analyze.empty() ? std::string("&mdash;") : escape(t.last_analyze),
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

} // namespace getgresql::api
