#include "api/handlers_admin.hpp"
#include "core/expected.hpp"
#include "html/templates.hpp"
#include "pg/catalog.hpp"
#include "pg/monitor.hpp"

#include <format>
#include <chrono>

namespace getgresql::api {

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
        return Response::html(html::page("Roles", "Dashboard",
            html::alert(error_message(conn.error()), "error")));
    }

    auto roles = pg::list_roles(conn->get());
    if (!roles) {
        return Response::html(html::page("Roles", "Dashboard",
            html::alert(error_message(roles.error()), "error")));
    }

    std::string content;
    content += html::table_begin({
        {"Name", "", true}, {"Superuser", ""}, {"Login", ""}, {"Create DB", ""},
        {"Create Role", ""}, {"Connection Limit", "num", true}, {"Valid Until", ""},
        {"Member Of", ""}
    });

    for (auto& r : *roles) {
        auto bool_badge = [](bool val) -> std::string {
            return val ? html::badge("YES", "success") : html::badge("NO", "secondary");
        };

        auto conn_limit = r.connection_limit < 0
            ? std::string("unlimited")
            : std::to_string(r.connection_limit);

        content += html::table_row({
            std::format("<strong>{}</strong>", html::escape(r.name)),
            bool_badge(r.is_superuser),
            bool_badge(r.can_login),
            bool_badge(r.can_create_db),
            bool_badge(r.can_create_role),
            conn_limit,
            r.valid_until.empty() ? std::string("&mdash;") : html::escape(r.valid_until),
            r.member_of.empty() ? std::string("&mdash;") : html::escape(r.member_of),
        });
    }
    content += html::table_end();

    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page("Roles", "Dashboard", std::move(content)));
}

// ─── ExtensionsHandler ──────────────────────────────────────────────

auto ExtensionsHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(html::page("Extensions", "Dashboard",
            html::alert(error_message(conn.error()), "error")));
    }

    auto exts = pg::list_extensions(conn->get());
    if (!exts) {
        return Response::html(html::page("Extensions", "Dashboard",
            html::alert(error_message(exts.error()), "error")));
    }

    std::string content;
    content += html::table_begin({
        {"Name", "", true}, {"Version", "", true}, {"Schema", "", true}, {"Description", "wide"}
    });

    for (auto& e : *exts) {
        content += html::table_row({
            std::format("<strong>{}</strong>", html::escape(e.name)),
            html::badge(e.version, "secondary"),
            html::escape(e.schema),
            html::escape(e.description),
        });
    }
    content += html::table_end();

    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page("Extensions", "Dashboard", std::move(content)));
}

// ─── SettingsHandler ────────────────────────────────────────────────

auto SettingsHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    std::string content = R"(
<div class="search-box">
    <input type="search" name="q" placeholder="Search settings..."
        hx-get="/settings/search" hx-trigger="input changed delay:300ms"
        hx-target="#settings-results" class="search-input">
</div>
<div id="settings-results" hx-get="/settings/search" hx-trigger="load">
    <div class="loading">Loading settings...</div>
</div>
)";

    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page("Settings", "Dashboard", std::move(content)));
}

// ─── SettingsSearchHandler ──────────────────────────────────────────

auto SettingsSearchHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto search = req.query("q");
    auto settings = pg::server_settings(conn->get(), search);
    if (!settings) return Response::html(html::alert(error_message(settings.error()), "error"));

    if (settings->empty()) {
        return Response::html(R"(<div class="empty-state">No settings found</div>)");
    }

    // Group by category
    std::string current_category;
    std::string content;

    bool table_open = false;

    for (auto& s : *settings) {
        if (s.category != current_category) {
            if (table_open) {
                content += html::table_end();
            }
            current_category = s.category;
            content += std::format("<h4>{}</h4>", html::escape(current_category));
            content += html::table_begin({
                {"Name", ""}, {"Value", ""}, {"Source", ""}, {"Context", ""},
                {"Description", "wide"}
            });
            table_open = true;
        }

        auto context_variant =
            s.context == "postmaster" ? "danger" :
            s.context == "sighup"     ? "warning" :
            s.context == "superuser"  ? "primary" :
            s.context == "user"       ? "success" : "secondary";

        auto value_str = s.unit.empty()
            ? html::escape(s.setting)
            : std::format("{} {}", html::escape(s.setting), html::escape(s.unit));

        content += html::table_row({
            std::format("<code>{}</code>", html::escape(s.name)),
            std::format("<code>{}</code>", value_str),
            html::escape(s.source),
            html::badge(s.context, context_variant),
            html::escape(s.short_desc),
        });
    }

    if (table_open) {
        content += html::table_end();
    }

    return Response::html(html::partial(std::move(content)));
}

// ─── FunctionListHandler ────────────────────────────────────────────

auto FunctionListHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto funcs = pg::list_functions(conn->get(), schema_name);
    if (!funcs) return Response::error(error_message(funcs.error()));

    std::string content;
    content += html::breadcrumbs({
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {std::string(schema_name), std::format("/db/{}/schema/{}/tables", db_name, schema_name)},
        {"Functions", ""},
    });

    content += html::table_begin({
        {"Name", "", true}, {"Arguments", "wide"}, {"Return Type", "", true},
        {"Language", "", true}, {"Volatility", ""}
    });

    for (auto& f : *funcs) {
        auto lang_variant =
            f.language == "plpgsql" ? "primary" :
            f.language == "sql"     ? "success" :
            f.language == "c"       ? "danger"  : "secondary";

        content += html::table_row({
            std::format("<a href=\"/db/{}/schema/{}/function/{}\">{}</a>",
                html::escape(db_name), html::escape(schema_name),
                html::escape(f.name), html::escape(f.name)),
            std::format("<code>{}</code>", html::escape(f.arguments)),
            html::escape(f.return_type),
            html::badge(f.language, lang_variant),
            html::badge(f.volatility, "secondary"),
        });
    }
    content += html::table_end();

    auto title = std::format("Functions - {}.{}", db_name, schema_name);
    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page(title, "Dashboard", std::move(content)));
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

    std::string content;
    content += html::breadcrumbs({
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {std::string(schema_name), std::format("/db/{}/schema/{}/tables", db_name, schema_name)},
        {"Functions", std::format("/db/{}/schema/{}/functions", db_name, schema_name)},
        {std::string(func_name), ""},
    });

    content += std::format("<p><strong>Language:</strong> {}</p>", html::badge(source->language, "primary"));

    content += "<h3>Source</h3>";
    content += std::format("<div class=\"function-source\">{}</div>", html::escape(source->source));

    if (!source->definition.empty()) {
        content += "<h3>Full Definition</h3>";
        content += std::format("<div class=\"function-source\">{}</div>", html::escape(source->definition));
    }

    auto title = std::format("Function: {}", func_name);
    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page(title, "Dashboard", std::move(content)));
}

// ─── SequenceListHandler ────────────────────────────────────────────

auto SequenceListHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto seqs = pg::list_sequences(conn->get(), schema_name);
    if (!seqs) return Response::error(error_message(seqs.error()));

    std::string content;
    content += html::breadcrumbs({
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {std::string(schema_name), std::format("/db/{}/schema/{}/tables", db_name, schema_name)},
        {"Sequences", ""},
    });

    content += html::table_begin({
        {"Name", "", true}, {"Type", ""}, {"Current Value", "num", true}, {"Start", "num", true},
        {"Increment", "num", true}, {"Min", "num", true}, {"Max", "num", true}, {"Cycle", ""}
    });

    for (auto& s : *seqs) {
        content += html::table_row({
            std::format("<strong>{}</strong>", html::escape(s.name)),
            html::badge(s.data_type, "secondary"),
            std::to_string(s.current_value),
            std::to_string(s.start_value),
            std::to_string(s.increment),
            std::to_string(s.min_value),
            std::to_string(s.max_value),
            s.cycle ? html::badge("YES", "success") : html::badge("NO", "secondary"),
        });
    }
    content += html::table_end();

    auto title = std::format("Sequences - {}.{}", db_name, schema_name);
    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page(title, "Dashboard", std::move(content)));
}

// ─── IndexAnalysisHandler ───────────────────────────────────────────

auto IndexAnalysisHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto indexes = pg::index_usage_stats(conn->get(), schema_name);
    if (!indexes) return Response::error(error_message(indexes.error()));

    std::string content;
    content += html::breadcrumbs({
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {std::string(schema_name), std::format("/db/{}/schema/{}/tables", db_name, schema_name)},
        {"Index Analysis", ""},
    });

    content += html::table_begin({
        {"Index", "", true}, {"Table", "", true}, {"Size", "num", true}, {"Scans", "num", true},
        {"Tuples Read", "num", true}, {"Tuples Fetched", "num", true}
    });

    for (auto& idx : *indexes) {
        auto row_attrs = idx.idx_scan == 0
            ? std::string_view(R"(class="row-warning")")
            : std::string_view("");

        auto name_cell = idx.idx_scan == 0
            ? std::format("{} {}", html::escape(idx.index_name), html::badge("UNUSED", "warning"))
            : html::escape(idx.index_name);

        content += html::table_row({
            name_cell,
            html::escape(idx.table),
            html::escape(idx.index_size),
            std::to_string(idx.idx_scan),
            std::to_string(idx.idx_tup_read),
            std::to_string(idx.idx_tup_fetch),
        }, row_attrs);
    }
    content += html::table_end();

    auto title = std::format("Index Analysis - {}.{}", db_name, schema_name);
    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page(title, "Dashboard", std::move(content)));
}

// ─── DatabaseSizeHandler ────────────────────────────────────────────

auto DatabaseSizeHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto breakdown = pg::database_size_breakdown(conn->get());
    if (!breakdown) return Response::error(error_message(breakdown.error()));

    std::string content;
    content += html::breadcrumbs({
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {"Size Breakdown", ""},
    });

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

    content += R"(<div class="stat-grid">)";
    content += html::stat_card("Total Size", format_size(total_bytes));
    content += html::stat_card("Tables", std::to_string(total_tables));
    content += html::stat_card("Indexes", std::to_string(total_indexes));
    content += html::stat_card("Schemas", std::to_string(breakdown->size()));
    content += "</div>";

    content += html::table_begin({
        {"Schema", "", true}, {"Size", "num", true}, {"Tables", "num", true}, {"Indexes", "num", true}
    });

    for (auto& s : *breakdown) {
        content += html::table_row({
            html::escape(s.schema),
            html::escape(s.size),
            std::to_string(s.table_count),
            std::to_string(s.index_count),
        });
    }
    content += html::table_end();

    auto title = std::format("Size - {}", db_name);
    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page(title, "Dashboard", std::move(content)));
}

// ─── ReplicationHandler ─────────────────────────────────────────────

auto ReplicationHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(html::page("Replication", "Monitor",
            html::alert(error_message(conn.error()), "error")));
    }

    auto slots = pg::replication_slots(conn->get());
    if (!slots) {
        return Response::html(html::page("Replication", "Monitor",
            html::alert(error_message(slots.error()), "error")));
    }

    std::string content;

    if (slots->empty()) {
        content = R"(<div class="empty-state">No replication slots configured</div>)";
    } else {
        content += html::table_begin({
            {"Slot Name", "", true}, {"Type", ""}, {"Database", "", true}, {"Active", ""},
            {"Restart LSN", ""}, {"Confirmed Flush LSN", ""}
        });

        for (auto& s : *slots) {
            content += html::table_row({
                std::format("<strong>{}</strong>", html::escape(s.slot_name)),
                html::badge(s.slot_type, "secondary"),
                html::escape(s.database),
                s.active ? html::badge("YES", "success") : html::badge("NO", "danger"),
                s.restart_lsn.empty() ? std::string("&mdash;") : std::format("<code>{}</code>", html::escape(s.restart_lsn)),
                s.confirmed_flush_lsn.empty() ? std::string("&mdash;") : std::format("<code>{}</code>", html::escape(s.confirmed_flush_lsn)),
            });
        }
        content += html::table_end();
    }

    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page("Replication", "Monitor", std::move(content)));
}

// ─── TableStatsHandler ──────────────────────────────────────────────

auto TableStatsHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    std::string content = R"(
<div class="schema-selector">
    <label for="schema-select">Schema:</label>
    <input type="text" id="schema-select" name="schema" value="public"
        hx-get="/monitor/tablestats/data" hx-trigger="change, load"
        hx-target="#tablestats-results" hx-include="this">
</div>
<div id="tablestats-results" hx-get="/monitor/tablestats/data?schema=public" hx-trigger="load">
    <div class="loading">Loading table statistics...</div>
</div>
)";

    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page("Table Statistics", "Monitor", std::move(content)));
}

// ─── TableStatsDataHandler ──────────────────────────────────────────

auto TableStatsDataHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto schema = req.query("schema");
    if (schema.empty()) schema = "public";

    auto stats = pg::table_stats(conn->get(), schema);
    if (!stats) return Response::html(html::alert(error_message(stats.error()), "error"));

    if (stats->empty()) {
        return Response::html(R"(<div class="empty-state">No tables found in this schema</div>)");
    }

    std::string content;
    content += html::table_begin({
        {"Table", "", true}, {"Seq Scans", "num", true}, {"Idx Scans", "num", true},
        {"Inserts", "num", true}, {"Updates", "num", true}, {"Deletes", "num", true},
        {"Live Rows", "num", true}, {"Dead Rows", "num", true},
        {"Last Vacuum", ""}, {"Last Analyze", ""}
    });

    for (auto& t : *stats) {
        bool high_dead = t.n_live_tup > 0 && t.n_dead_tup > static_cast<long long>(static_cast<double>(t.n_live_tup) * 0.1);

        auto dead_cell = high_dead
            ? std::format("{} {}", std::to_string(t.n_dead_tup), html::badge("HIGH", "warning"))
            : std::to_string(t.n_dead_tup);

        auto row_attrs = high_dead
            ? std::string_view(R"(class="row-warning")")
            : std::string_view("");

        auto last_vacuum = t.last_vacuum.empty()
            ? (t.last_autovacuum.empty() ? std::string("&mdash;") : html::escape(t.last_autovacuum))
            : html::escape(t.last_vacuum);

        content += html::table_row({
            std::format("<strong>{}</strong>", html::escape(t.table)),
            std::to_string(t.seq_scan),
            std::to_string(t.idx_scan),
            std::to_string(t.n_tup_ins),
            std::to_string(t.n_tup_upd),
            std::to_string(t.n_tup_del),
            std::to_string(t.n_live_tup),
            dead_cell,
            last_vacuum,
            t.last_analyze.empty() ? std::string("&mdash;") : html::escape(t.last_analyze),
        }, row_attrs);
    }
    content += html::table_end();

    return Response::html(html::partial(std::move(content)));
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
        return Response::html(html::alert(error_message(result.error()), "error"));
    }

    return Response::html(html::alert(std::format("Terminated backend PID {}", pid), "info"));
}

// ─── ExplainPageHandler ─────────────────────────────────────────────

auto ExplainPageHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    // Uses the same JS editor engine as the Query page, but in "explain" mode
    std::string content = R"(<div id="query-workspace" class="query-panel" data-mode="explain"></div>)";

    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::ide_page_full("Explain", "Explain", std::move(content)));
}

// ─── ExplainExecHandler ─────────────────────────────────────────────

auto ExplainExecHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto body = std::string(req.body());

    auto sql = form_value(body, "sql");
    auto analyze_str = form_value(body, "analyze");
    bool analyze = (analyze_str == "true");

    if (sql.empty()) {
        return Response::html(html::alert("No SQL provided", "warning"));
    }

    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(html::alert(error_message(conn.error()), "error"));
    }

    auto start = std::chrono::steady_clock::now();
    auto result = pg::explain_query(conn->get(), sql, analyze);
    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    if (!result) {
        return Response::html(
            std::format(R"(<div class="query-error"><strong>Error:</strong> {}</div>)",
                html::escape(error_message(result.error())))
        );
    }

    std::string content;

    if (analyze) {
        content += std::format(
            R"(<div class="query-info"><span class="rows-badge">Planning: {:.3f} ms</span> <span class="time-badge">Execution: {:.3f} ms</span> <span class="time-badge">Wall: {} ms</span></div>)",
            result->planning_time, result->execution_time, ms
        );
    } else {
        content += std::format(
            R"(<div class="query-info"><span class="rows-badge">Cost: {:.2f}</span> <span class="time-badge">Wall: {} ms</span></div>)",
            result->total_cost, ms
        );
    }

    content += std::format(
        "<div style=\"padding:var(--sp-4)\"><div class=\"explain-plan\">{}</div></div>",
        html::escape(result->plan_text));

    return Response::html(std::move(content));
}

} // namespace getgresql::api
