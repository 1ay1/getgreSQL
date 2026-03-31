#include "api/handlers_db.hpp"
#include "core/expected.hpp"
#include "html/templates.hpp"
#include "pg/catalog.hpp"
#include "pg/monitor.hpp"

#include <format>

namespace getgresql::api {

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

auto IndexHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(html::page("Error", "Dashboard",
            html::alert(error_message(conn.error()), "error")));
    }

    auto stats_res = pg::server_stats(conn->get());
    auto dbs_res = pg::database_sizes(conn->get());
    auto health_res = pg::health_checks(conn->get());
    auto activity_res = pg::database_activity(conn->get());

    std::string content;

    // ── Health checks panel ─────────────────────────────────────────
    if (health_res) {
        content += "<div class=\"health-grid\">";
        for (auto& c : *health_res) {
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
    }

    // ── Server stats ────────────────────────────────────────────────
    if (stats_res) {
        auto& s = *stats_res;
        content += "<div class=\"stat-grid\">";
        content += html::stat_card("Active", std::to_string(s.active_connections));
        content += html::stat_card("Idle", std::to_string(s.idle_connections));
        content += html::stat_card("Idle in Txn", std::to_string(s.idle_in_transaction),
                                   s.idle_in_transaction > 0 ? "warning" : "");
        content += html::stat_card("Cache Hit", std::format("{:.1f}%", s.cache_hit_ratio * 100),
                                   s.cache_hit_ratio < 0.90 ? "danger" : "success");
        content += html::stat_card("Commits", std::to_string(s.total_commits));
        content += html::stat_card("Rollbacks", std::to_string(s.total_rollbacks),
                                   s.total_rollbacks > 0 ? "warning" : "");
        content += html::stat_card("Max Connections", std::to_string(s.max_connections));
        content += html::stat_card("Uptime", s.uptime, "accent");
        content += "</div>";
        content += "<div class=\"server-info\"><code>" + html::escape(s.version) + "</code></div>";
    }

    // ── Dashboard grid: databases + activity ────────────────────────
    content += "<div class=\"dashboard-grid\">";

    // Database sizes panel
    if (dbs_res) {
        long long max_size = 1;
        for (auto& d : *dbs_res) if (d.size_bytes > max_size) max_size = d.size_bytes;

        content += "<div class=\"dashboard-section\">";
        content += "<div class=\"dashboard-section-header\">Databases</div>";
        content += "<div class=\"dashboard-section-body\">";
        content += "<table><thead><tr><th>Name</th><th>Size</th><th></th><th>Connections</th><th>Cache</th></tr></thead><tbody>";
        for (auto& d : *dbs_res) {
            auto bar_pct = static_cast<double>(d.size_bytes) / max_size * 100;
            auto cache_variant = d.cache_hit_ratio < 0.90 ? "danger" : d.cache_hit_ratio < 0.99 ? "warning" : "success";
            content += std::format(
                "<tr><td><a href=\"/db/{}/schemas\"><strong>{}</strong></a></td>"
                "<td class=\"num\">{}</td>"
                "<td><div class=\"size-bar\"><div class=\"size-bar-fill\" style=\"width:{:.0f}px\"></div></div></td>"
                "<td class=\"num\">{}</td>"
                "<td>{}</td></tr>",
                html::escape(d.name), html::escape(d.name), html::escape(d.size),
                bar_pct * 0.8, d.connections,
                html::badge(std::format("{:.0f}%", d.cache_hit_ratio * 100), cache_variant)
            );
        }
        content += "</tbody></table></div></div>";
    }

    // Activity per database panel
    if (activity_res) {
        content += "<div class=\"dashboard-section\">";
        content += "<div class=\"dashboard-section-header\">Activity by Database</div>";
        content += "<div class=\"dashboard-section-body\">";
        content += "<table><thead><tr><th>Database</th><th>Active</th><th>Idle</th><th>Idle Txn</th><th>Commits</th><th>Rollbacks</th></tr></thead><tbody>";
        for (auto& a : *activity_res) {
            content += std::format(
                "<tr><td><strong>{}</strong></td><td class=\"num\">{}</td><td class=\"num\">{}</td>"
                "<td class=\"num\">{}</td><td class=\"num\">{}</td><td class=\"num\">{}</td></tr>",
                html::escape(a.database), a.active, a.idle, a.idle_in_transaction,
                a.xact_commit, a.xact_rollback
            );
        }
        content += "</tbody></table></div></div>";
    }

    content += "</div>"; // dashboard-grid

    return Response::html(html::page("Dashboard", "Dashboard", std::move(content)));
}

auto DatabaseListHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(html::page("Databases", "Dashboard",
            html::alert(error_message(conn.error()), "error")));
    }

    auto dbs = pg::list_databases(conn->get());
    if (!dbs) {
        return Response::html(html::page("Databases", "Dashboard",
            html::alert(error_message(dbs.error()), "error")));
    }

    std::string content;
    content += html::table_begin({
        {"Name", "", true}, {"Owner", "", true}, {"Encoding", "", true},
        {"Size", "num", true}, {"Size (bytes)", "num", true}
    });

    for (auto& db : *dbs) {
        content += html::table_row({
            std::format("<a href=\"/db/{}/schemas\">{}</a>", html::escape(db.name), html::escape(db.name)),
            html::escape(db.owner),
            html::escape(db.encoding),
            html::escape(db.size),
            std::to_string(db.size_bytes),
        });
    }
    content += html::table_end();

    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page("Databases", "Dashboard", std::move(content)));
}

auto SchemaListHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto schemas = pg::list_schemas(conn->get(), db_name);
    if (!schemas) return Response::error(error_message(schemas.error()));

    std::string content;
    content += html::breadcrumbs({
        {"Databases", "/databases"},
        {std::string(db_name), ""},
    });

    content += html::table_begin({{"Schema", "", true}, {"Owner", "", true}});
    for (auto& s : *schemas) {
        content += html::table_row({
            std::format("<a href=\"/db/{}/schema/{}/tables\">{}</a>",
                html::escape(db_name), html::escape(s.name), html::escape(s.name)),
            html::escape(s.owner),
        });
    }
    content += html::table_end();

    auto title = std::format("Schemas - {}", db_name);
    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page(title, "Dashboard", std::move(content)));
}

auto TableListHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto tables = pg::list_tables(conn->get(), schema_name);
    if (!tables) return Response::error(error_message(tables.error()));

    std::string content;
    content += html::breadcrumbs({
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {std::string(schema_name), ""},
    });

    content += "<div class=\"schema-nav\">";
    content += std::format("<a href=\"/db/{}/schema/{}/tables\" class=\"btn btn-sm btn-primary\">Tables</a>", db_name, schema_name);
    content += std::format("<a href=\"/db/{}/schema/{}/functions\" class=\"btn btn-sm\">Functions</a>", db_name, schema_name);
    content += std::format("<a href=\"/db/{}/schema/{}/sequences\" class=\"btn btn-sm\">Sequences</a>", db_name, schema_name);
    content += std::format("<a href=\"/db/{}/schema/{}/indexes\" class=\"btn btn-sm\">Index Analysis</a>", db_name, schema_name);
    content += std::format("<a href=\"/db/{}/schema/{}/erd/page\" class=\"btn btn-sm\">ERD</a>", db_name, schema_name);
    content += "</div>";

    content += html::table_begin({
        {"Table", "", true}, {"Type", "", true}, {"Rows (est.)", "num", true}, {"Size", "num", true}
    });
    for (auto& t : *tables) {
        auto type_badge = t.type == "table" ? std::string("primary") : std::string("secondary");
        content += html::table_row({
            std::format("<a href=\"/db/{}/schema/{}/table/{}\">{}</a>",
                html::escape(db_name), html::escape(schema_name),
                html::escape(t.name), html::escape(t.name)),
            html::badge(t.type, type_badge),
            std::to_string(t.row_estimate),
            html::escape(t.size),
        });
    }
    content += html::table_end();

    auto title = std::format("Tables - {}.{}", db_name, schema_name);
    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page(title, "Dashboard", std::move(content)));
}

auto TableDetailHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    std::string content;
    content += html::breadcrumbs({
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {std::string(schema_name), std::format("/db/{}/schema/{}/tables", db_name, schema_name)},
        {std::string(table_name), ""},
    });

    // Section tabs
    auto browse_url = std::format("/db/{}/schema/{}/table/{}/browse", db_name, schema_name, table_name);
    auto ddl_url = std::format("/db/{}/schema/{}/table/{}/ddl", db_name, schema_name, table_name);
    auto stats_url = std::format("/db/{}/schema/{}/table/{}/stats", db_name, schema_name, table_name);
    auto vacuum_url = std::format("/db/{}/schema/{}/table/{}/vacuum", db_name, schema_name, table_name);
    auto analyze_url = std::format("/db/{}/schema/{}/table/{}/analyze", db_name, schema_name, table_name);
    auto truncate_url = std::format("/db/{}/schema/{}/table/{}/truncate", db_name, schema_name, table_name);

    auto tab_btn = [](std::string_view url) {
        return std::format(
            " hx-get=\"{}\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\""
            " onclick=\"this.parentElement.querySelectorAll('.active').forEach(e=>e.classList.remove('active')); this.classList.add('active')\"", url);
    };

    content += "<div style=\"display:flex;align-items:center;justify-content:space-between;margin-bottom:var(--sp-3)\">";
    content += "<div class=\"section-tabs\" style=\"margin-bottom:0\">";
    content += "<button class=\"section-tab active\">Columns</button>";
    content += std::format("<button class=\"section-tab\"{}>Data</button>", tab_btn(browse_url));
    content += std::format("<button class=\"section-tab\"{}>DDL</button>", tab_btn(ddl_url));
    content += std::format("<button class=\"section-tab\"{}>Statistics</button>", tab_btn(stats_url));
    content += "</div>";
    content += "<div class=\"btn-group\">";
    content += std::format("<button class=\"btn btn-sm\" hx-post=\"{}\" hx-target=\"#action-result\" hx-swap=\"innerHTML\">Vacuum</button>", vacuum_url);
    content += std::format("<button class=\"btn btn-sm\" hx-post=\"{}\" hx-target=\"#action-result\" hx-swap=\"innerHTML\">Analyze</button>", analyze_url);
    content += std::format("<button class=\"btn btn-sm btn-danger\" hx-post=\"{}\" hx-target=\"#action-result\" hx-swap=\"innerHTML\" hx-confirm=\"TRUNCATE all data from {}.{}?\">Truncate</button>", truncate_url, schema_name, table_name);
    content += "</div>";
    content += "</div>";
    content += "<div id=\"action-result\"></div>";
    content += "<div id=\"tab-content\">";

    // Columns
    auto cols = pg::describe_columns(conn->get(), schema_name, table_name);
    if (cols) {
        content += html::table_begin({
            {"#", "num"}, {"Name", ""}, {"Type", ""}, {"Nullable", ""},
            {"Default", ""}, {"PK", ""}
        });
        for (auto& c : *cols) {
            auto nullable_str = c.nullable ? std::string("YES") : std::string("<strong>NO</strong>");
            auto default_str = c.default_value.empty()
                ? std::string("&mdash;")
                : std::format("<code>{}</code>", html::escape(c.default_value));
            auto pk_str = c.is_primary_key ? html::badge("PK", "primary") : std::string("");

            content += html::table_row({
                std::to_string(c.ordinal),
                std::format("<code>{}</code>", html::escape(c.name)),
                html::badge(c.type, "secondary"),
                nullable_str,
                default_str,
                pk_str,
            });
        }
        content += html::table_end();
    }

    // Indexes
    auto idxs = pg::list_indexes(conn->get(), schema_name, table_name);
    if (idxs && !idxs->empty()) {
        content += "<h3>Indexes</h3>";
        content += html::table_begin({
            {"Name", ""}, {"Definition", ""}, {"Unique", ""}, {"Primary", ""}, {"Size", "num"}
        });
        for (auto& idx : *idxs) {
            content += html::table_row({
                html::escape(idx.name),
                std::format("<code>{}</code>", html::escape(idx.definition)),
                std::string(idx.is_unique ? "&#10003;" : ""),
                std::string(idx.is_primary ? "&#10003;" : ""),
                html::escape(idx.size),
            });
        }
        content += html::table_end();
    }

    // Constraints
    auto cons = pg::list_constraints(conn->get(), schema_name, table_name);
    if (cons && !cons->empty()) {
        content += "<h3>Constraints</h3>";
        content += html::table_begin({{"Name", ""}, {"Type", ""}, {"Definition", ""}});
        for (auto& c : *cons) {
            content += html::table_row({
                html::escape(c.name),
                html::badge(c.type, "secondary"),
                std::format("<code>{}</code>", html::escape(c.definition)),
            });
        }
        content += html::table_end();
    }

    content += "</div>";

    auto title = std::format("{}.{}", schema_name, table_name);
    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page(title, "Dashboard", std::move(content)));
}

auto TableDataHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");

    auto limit_str = req.query("limit");
    int limit = 100;
    if (!limit_str.empty()) {
        try { limit = std::stoi(std::string(limit_str)); }
        catch (...) {}
        if (limit > 1000) limit = 1000;
    }

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto result = pg::preview_rows(conn->get(), schema_name, table_name, limit);
    if (!result) return Response::error(error_message(result.error()));

    std::string content;

    content += std::format("<div class=\"query-info\"><span class=\"rows-badge\">{} rows</span> <span class=\"time-badge\">limit {}</span></div>",
                           result->row_count(), limit);

    std::vector<html::TableColumn> headers;
    for (int c = 0; c < result->col_count(); ++c) {
        headers.push_back({std::string(result->column_name(c)), "", true});
    }
    content += html::table_begin(headers);

    for (auto row : *result) {
        std::vector<std::string> cells;
        for (int c = 0; c < row.col_count(); ++c) {
            if (row.is_null(c)) {
                cells.push_back("<span class=\"null-value\">NULL</span>");
            } else {
                auto val = row[c];
                if (val.size() > 200) {
                    cells.push_back(html::escape(val.substr(0, 200)) + "...");
                } else {
                    cells.push_back(html::escape(val));
                }
            }
        }
        content += html::table_row(cells);
    }
    content += html::table_end();

    return Response::html(html::partial(std::move(content)));
}

// ─── TableDDLHandler ────────────────────────────────────────────────

auto TableDDLHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto ddl = pg::table_ddl(conn->get(), schema_name, table_name);
    if (!ddl) return Response::html(html::alert(error_message(ddl.error()), "error"));

    std::string content;
    content += "<div class=\"ddl-toolbar\">";
    content += "<button class=\"btn btn-sm\" onclick=\"navigator.clipboard.writeText(document.querySelector('.ddl-source').textContent).then(()=>this.textContent='Copied!')\">";
    content += "Copy to Clipboard</button></div>";
    content += std::format("<pre class=\"function-source ddl-source\">{}</pre>", html::escape(*ddl));
    return Response::html(html::partial(std::move(content)));
}

// ─── ColumnStatsHandler ─────────────────────────────────────────────

auto ColumnStatsHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto stats = pg::column_statistics(conn->get(), schema_name, table_name);
    if (!stats) return Response::html(html::alert(error_message(stats.error()), "error"));

    if (stats->empty()) {
        return Response::html(html::alert("No statistics available. Run ANALYZE on this table first.", "warning"));
    }

    std::string content;
    for (auto& s : *stats) {
        content += std::format("<div class=\"col-stat-card\">");
        content += std::format("<div class=\"col-stat-header\">");
        content += std::format("<strong>{}</strong> <span class=\"badge badge-secondary\">{}</span>", html::escape(s.column_name), html::escape(s.data_type));
        content += "</div>";
        content += "<div class=\"col-stat-body\">";

        // Null fraction bar
        auto null_pct = s.null_fraction * 100;
        content += std::format("<div class=\"col-stat-row\"><span class=\"col-stat-label\">Null %</span>"
            "<div class=\"col-stat-bar-track\"><div class=\"col-stat-bar\" style=\"width:{:.1f}%\"></div></div>"
            "<span class=\"col-stat-value\">{:.1f}%</span></div>", null_pct, null_pct);

        // Distinct values
        auto distinct_str = s.n_distinct < 0
            ? std::format("{:.0f}% unique", static_cast<double>(-s.n_distinct) * 100.0)
            : std::to_string(s.n_distinct) + " distinct";
        content += std::format("<div class=\"col-stat-row\"><span class=\"col-stat-label\">Distinct</span>"
            "<span class=\"col-stat-value\">{}</span></div>", distinct_str);

        // Avg width
        content += std::format("<div class=\"col-stat-row\"><span class=\"col-stat-label\">Avg Width</span>"
            "<span class=\"col-stat-value\">{} bytes</span></div>", s.avg_width);

        // Correlation
        content += std::format("<div class=\"col-stat-row\"><span class=\"col-stat-label\">Correlation</span>"
            "<span class=\"col-stat-value\">{:.4f}</span></div>", s.correlation);

        // Most common values
        if (!s.most_common_vals.empty()) {
            auto vals = s.most_common_vals.size() > 200
                ? s.most_common_vals.substr(0, 200) + "..."
                : s.most_common_vals;
            content += std::format("<div class=\"col-stat-row\"><span class=\"col-stat-label\">Top Values</span>"
                "<code class=\"col-stat-vals\">{}</code></div>", html::escape(vals));
        }

        content += "</div></div>";
    }

    return Response::html(html::partial(std::move(content)));
}

// ─── VacuumHandler ──────────────────────────────────────────────────

auto VacuumHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto result = pg::vacuum_table(conn->get(), schema_name, table_name);
    if (!result) return Response::html(html::alert(error_message(result.error()), "error"));

    return Response::html(html::alert(std::format("VACUUM completed on {}.{}", schema_name, table_name), "info"));
}

// ─── AnalyzeHandler ─────────────────────────────────────────────────

auto AnalyzeHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto result = pg::analyze_table(conn->get(), schema_name, table_name);
    if (!result) return Response::html(html::alert(error_message(result.error()), "error"));

    return Response::html(html::alert(std::format("ANALYZE completed on {}.{}", schema_name, table_name), "info"));
}

// ─── ERDDataHandler ─────────────────────────────────────────────────

static auto json_esc(std::string_view s) -> std::string {
    std::string out;
    out.reserve(s.size() + 4);
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

auto ERDDataHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::json("{\"tables\":[],\"relationships\":[]}", 500);

    auto erd = pg::schema_erd(conn->get(), schema_name);
    if (!erd) return Response::json("{\"tables\":[],\"relationships\":[]}", 500);

    std::string json = "{\"tables\":[";
    for (std::size_t i = 0; i < erd->tables.size(); ++i) {
        if (i > 0) json += ',';
        auto& t = erd->tables[i];
        json += "{\"name\":\"" + json_esc(t.name) + "\",\"type\":\"" + json_esc(t.type) + "\",\"columns\":[";
        for (std::size_t j = 0; j < t.columns.size(); ++j) {
            if (j > 0) json += ',';
            json += "{\"name\":\"" + json_esc(t.columns[j].first) + "\",\"type\":\"" + json_esc(t.columns[j].second) + "\"}";
        }
        json += "]}";
    }
    json += "],\"relationships\":[";
    for (std::size_t i = 0; i < erd->relationships.size(); ++i) {
        if (i > 0) json += ',';
        auto& r = erd->relationships[i];
        json += "{\"name\":\"" + json_esc(r.constraint_name) + "\","
                "\"source\":\"" + json_esc(r.source_table) + "\","
                "\"sourceCol\":\"" + json_esc(r.source_columns) + "\","
                "\"target\":\"" + json_esc(r.target_table) + "\","
                "\"targetCol\":\"" + json_esc(r.target_columns) + "\"}";
    }
    json += "]}";
    return Response::json(std::move(json));
}

// ─── ERDPageHandler ─────────────────────────────────────────────────

auto ERDPageHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    std::string content;
    content += html::breadcrumbs({
        {"Databases", "/databases"},
        {std::string(db_name), std::format("/db/{}/schemas", db_name)},
        {std::string(schema_name), std::format("/db/{}/schema/{}/tables", db_name, schema_name)},
        {"ERD", ""},
    });

    content += std::format(
        "<div id=\"erd-container\" class=\"erd-container\" data-url=\"/db/{}/schema/{}/erd\">"
        "<div class=\"loading\">Loading ERD...</div>"
        "</div>",
        db_name, schema_name
    );

    auto title = std::format("ERD - {}.{}", db_name, schema_name);
    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page(title, "Dashboard", std::move(content)));
}

// ─── TableBrowseHandler — paginated editable data grid ──────────────

auto TableBrowseHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");

    auto page_str = req.query("page");
    auto limit_str = req.query("limit");
    auto sort_col = req.query("sort");
    auto sort_dir = req.query("dir");
    auto filter = req.query("filter");

    int page = 1, limit = 50;
    if (!page_str.empty()) try { page = std::stoi(std::string(page_str)); } catch (...) {}
    if (!limit_str.empty()) try { limit = std::stoi(std::string(limit_str)); } catch (...) {}
    if (page < 1) page = 1;
    if (limit < 1) limit = 1;
    if (limit > 500) limit = 500;
    int offset = (page - 1) * limit;

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    // Count total rows
    auto count_sql = std::format("SELECT COUNT(*) FROM \"{}\".\"{}\"\t", schema_name, table_name);
    if (!filter.empty()) {
        count_sql = std::format("SELECT COUNT(*) FROM \"{}\".\"{}\" WHERE {}", schema_name, table_name, filter);
    }
    auto count_res = conn->get().exec(count_sql);
    long long total_rows = 0;
    if (count_res) total_rows = count_res->get_int(0, 0).value_or(0);
    int total_pages = static_cast<int>((total_rows + limit - 1) / limit);

    // Fetch data
    auto data_sql = std::format("SELECT * FROM \"{}\".\"{}\"", schema_name, table_name);
    if (!filter.empty()) data_sql += std::format(" WHERE {}", filter);
    if (!sort_col.empty()) {
        auto dir = (sort_dir == "desc") ? "DESC" : "ASC";
        data_sql += std::format(" ORDER BY \"{}\" {}", sort_col, dir);
    }
    data_sql += std::format(" LIMIT {} OFFSET {}", limit, offset);

    auto result = conn->get().exec(data_sql);
    if (!result) return Response::html(
        std::format("<div class=\"query-error\">{}</div>", html::escape(error_message(result.error())))
    );

    auto base_url = std::format("/db/{}/schema/{}/table/{}/browse", db_name, schema_name, table_name);

    std::string content;

    // Toolbar row
    content += "<div class=\"data-toolbar\">";
    content += std::format("<span class=\"data-info\">{} rows | Page {} of {}</span>", total_rows, page, std::max(total_pages, 1));
    content += "<div class=\"btn-group\">";
    if (page > 1) {
        content += std::format("<button class=\"btn btn-sm\" hx-get=\"{}?page={}&limit={}\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">&laquo; Prev</button>", base_url, page-1, limit);
    }
    if (page < total_pages) {
        content += std::format("<button class=\"btn btn-sm\" hx-get=\"{}?page={}&limit={}\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">Next &raquo;</button>", base_url, page+1, limit);
    }
    content += "</div>";
    content += std::format("<a href=\"/db/{}/schema/{}/table/{}/export\" class=\"btn btn-sm\">Export CSV</a>", db_name, schema_name, table_name);
    content += std::format("<button class=\"btn btn-sm btn-success\" onclick=\"document.getElementById('insert-form').style.display=''\">+ Insert Row</button>");
    content += "</div>";

    // Insert row form (hidden by default)
    content += "<div id=\"insert-form\" style=\"display:none\" class=\"insert-form\">";
    content += std::format("<form hx-post=\"/db/{}/schema/{}/table/{}/insert-row\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">", db_name, schema_name, table_name);
    content += "<div class=\"insert-form-grid\">";
    for (int c = 0; c < result->col_count(); ++c) {
        auto col_name = std::string(result->column_name(c));
        content += std::format(
            "<div class=\"insert-field\"><label>{}</label>"
            "<input type=\"text\" name=\"col_{}\" placeholder=\"{}\" class=\"insert-input\"></div>",
            html::escape(col_name), c, html::escape(col_name)
        );
    }
    content += "</div>";
    content += "<div class=\"insert-actions\"><button type=\"submit\" class=\"btn btn-sm btn-primary\">Insert</button>";
    content += "<button type=\"button\" class=\"btn btn-sm\" onclick=\"this.closest('#insert-form').style.display='none'\">Cancel</button></div>";
    content += "</form></div>";

    // Data table with editable cells
    std::vector<html::TableColumn> headers;
    headers.push_back({"#", "num"});
    for (int c = 0; c < result->col_count(); ++c) {
        headers.push_back({std::string(result->column_name(c)), "", true});
    }
    headers.push_back({"", ""}); // actions column
    content += html::table_begin(headers, "data-grid");

    for (auto row : *result) {
        int row_idx = row.index();
        // Build a WHERE clause identifying this row (use all columns for safety)
        std::string where_parts;
        for (int c = 0; c < row.col_count(); ++c) {
            if (!where_parts.empty()) where_parts += " AND ";
            if (row.is_null(c)) {
                where_parts += std::format("\"{}\" IS NULL", result->column_name(c));
            } else {
                auto val = std::string(row[c]);
                // Escape single quotes
                std::string escaped;
                for (char ch : val) { if (ch == '\'') escaped += "''"; else escaped += ch; }
                where_parts += std::format("\"{}\" = '{}'", result->column_name(c), escaped);
            }
        }

        std::vector<std::string> cells;
        cells.push_back(std::to_string(offset + row_idx + 1));

        for (int c = 0; c < row.col_count(); ++c) {
            if (row.is_null(c)) {
                cells.push_back("<span class=\"null-value\">NULL</span>");
            } else {
                auto val = row[c];
                auto display = val.size() > 200 ? html::escape(val.substr(0, 200)) + "..." : html::escape(val);
                cells.push_back(std::format(
                    "<span class=\"editable-cell\" data-col=\"{}\" data-schema=\"{}\" data-table=\"{}\" "
                    "data-db=\"{}\" data-where=\"{}\" ondblclick=\"editCell(this)\">{}</span>",
                    html::escape(result->column_name(c)), html::escape(schema_name),
                    html::escape(table_name), html::escape(db_name),
                    html::escape(where_parts), display
                ));
            }
        }

        // Delete button
        cells.push_back(std::format(
            "<button class=\"btn btn-sm btn-danger\" "
            "hx-post=\"/db/{}/schema/{}/table/{}/delete-row\" "
            "hx-vals='{{\"where\":\"{}\"}}' "
            "hx-target=\"#tab-content\" hx-swap=\"innerHTML\" "
            "hx-confirm=\"Delete this row?\">Del</button>",
            db_name, schema_name, table_name, json_escape_str(where_parts)
        ));

        content += html::table_row(cells);
    }
    content += html::table_end();

    // Page size selector
    content += "<div class=\"data-toolbar\">";
    content += "<span class=\"data-info\">Rows per page: ";
    for (auto sz : {25, 50, 100, 250, 500}) {
        if (sz == limit) {
            content += std::format("<strong>{}</strong> ", sz);
        } else {
            content += std::format("<a href=\"#\" hx-get=\"{}?page=1&limit={}\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">{}</a> ", base_url, sz, sz);
        }
    }
    content += "</span></div>";

    return Response::html(html::partial(std::move(content)));
}

// ─── CellUpdateHandler ──────────────────────────────────────────────

auto CellUpdateHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto schema_name = req.param("schema");
    auto table_name = req.param("table");
    auto body = std::string(req.body());

    auto col = form_value(body, "col");
    auto val = form_value(body, "val");
    auto where = form_value(body, "where");

    if (col.empty() || where.empty()) return Response::error("Missing parameters", 400);

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    // Escape single quotes in value
    std::string escaped_val;
    for (char c : val) { if (c == '\'') escaped_val += "''"; else escaped_val += c; }

    auto sql = std::format("UPDATE \"{}\".\"{}\"\t SET \"{}\" = '{}' WHERE {}", schema_name, table_name, col, escaped_val, where);
    auto result = conn->get().exec(sql);
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

    auto where = form_value(body, "where");
    if (where.empty()) return Response::error("No row identified", 400);

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto sql = std::format("DELETE FROM \"{}\".\"{}\"\t WHERE {} LIMIT 1", schema_name, table_name, where);
    // PostgreSQL doesn't support LIMIT in DELETE, use subquery
    sql = std::format("DELETE FROM \"{}\".\"{}\" WHERE ctid = (SELECT ctid FROM \"{}\".\"{}\"\t WHERE {} LIMIT 1)",
        schema_name, table_name, schema_name, table_name, where);
    auto result = conn->get().exec(sql);
    if (!result) return Response::html(html::alert(error_message(result.error()), "error"));

    // Return updated browse view
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
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    // Get column names
    auto cols_res = pg::describe_columns(conn->get(), schema_name, table_name);
    if (!cols_res) return Response::html(html::alert(error_message(cols_res.error()), "error"));

    std::string col_names, values;
    int col_idx = 0;
    for (auto& c : *cols_res) {
        auto val = form_value(body, std::format("col_{}", col_idx));
        if (!val.empty()) {
            if (!col_names.empty()) { col_names += ", "; values += ", "; }
            col_names += std::format("\"{}\"", c.name);
            // Escape single quotes
            std::string escaped;
            for (char ch : val) { if (ch == '\'') escaped += "''"; else escaped += ch; }
            values += std::format("'{}'", escaped);
        }
        col_idx++;
    }

    if (col_names.empty()) return Response::html(html::alert("No values provided", "warning"));

    auto sql = std::format("INSERT INTO \"{}\".\"{}\" ({}) VALUES ({})", schema_name, table_name, col_names, values);
    auto result = conn->get().exec(sql);
    if (!result) return Response::html(
        std::format("<div class=\"query-error\">{}</div>", html::escape(error_message(result.error())))
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
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto sql = std::format("TRUNCATE TABLE \"{}\".\"{}\"", schema_name, table_name);
    auto result = conn->get().exec(sql);
    if (!result) return Response::html(html::alert(error_message(result.error()), "error"));

    return Response::html(html::alert(std::format("Table {}.{} truncated", schema_name, table_name), "info"));
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
