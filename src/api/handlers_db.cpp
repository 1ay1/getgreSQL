#include "api/handlers_db.hpp"
#include "core/expected.hpp"
#include "ssr/components.hpp"
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

auto IndexHandler::handle(Request& req, AppContext& ctx) -> Response {
    using namespace ssr;

    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(render_page("Error", "Dashboard", [&](Html& h) {
            Alert::render({error_message(conn.error()), "error"}, h);
        }));
    }

    // Fetch all data (pure queries — no side effects on Html)
    auto stats_res = pg::server_stats(conn->get());
    auto dbs_res = pg::database_sizes(conn->get());
    auto health_res = pg::health_checks(conn->get());
    auto activity_res = pg::database_activity(conn->get());

    // Render: pure function from data → Html buffer
    auto h = Html::with_capacity(16384);

    PageLayout::render({"Dashboard", "Dashboard"}, h, [&](Html& h) {

        // ── Health checks ───────────────────────────────────────
        if (health_res) {
            h.raw("<div class=\"health-grid\">");
            for (auto& c : *health_res) {
                HealthCard::render({c.name, c.status, c.value, c.detail}, h);
            }
            h.raw("</div>");
        }

        // ── Server stats ────────────────────────────────────────
        if (stats_res) {
            auto& s = *stats_res;
            auto cache_pct = std::format("{:.1f}%", s.cache_hit_ratio * 100);
            h.raw("<div class=\"stat-grid\">");
            StatCard::render({"Active", std::to_string(s.active_connections)}, h);
            StatCard::render({"Idle", std::to_string(s.idle_connections)}, h);
            StatCard::render({"Idle in Txn", std::to_string(s.idle_in_transaction),
                             s.idle_in_transaction > 0 ? "warning" : ""}, h);
            StatCard::render({"Cache Hit", cache_pct,
                             s.cache_hit_ratio < 0.90 ? "danger" : "success"}, h);
            StatCard::render({"Commits", std::to_string(s.total_commits)}, h);
            StatCard::render({"Rollbacks", std::to_string(s.total_rollbacks),
                             s.total_rollbacks > 0 ? "warning" : ""}, h);
            StatCard::render({"Max Connections", std::to_string(s.max_connections)}, h);
            StatCard::render({"Uptime", s.uptime, "accent"}, h);
            h.raw("</div>");
            h.raw("<div class=\"server-info\"><code>").text(s.version).raw("</code></div>");
        }

        // ── Dashboard grid ──────────────────────────────────────
        h.raw("<div class=\"dashboard-grid\">");

        // Databases panel
        if (dbs_res) {
            double max_size = 1.0;
            for (auto& d : *dbs_res) if (static_cast<double>(d.size_bytes) > max_size) max_size = static_cast<double>(d.size_bytes);

            { auto _ = scope(h, "div", "class=\"dashboard-section\"");
                h.raw("<div class=\"dashboard-section-header\">Databases</div>");
                { auto _ = scope(h, "div", "class=\"dashboard-section-body\"");
                    h.raw("<table><thead><tr><th>Name</th><th>Size</th><th>Conn</th><th>Cache</th></tr></thead><tbody>");
                    for (auto& d : *dbs_res) {
                        auto pct = static_cast<double>(d.size_bytes) / max_size * 100;
                        auto cache_v = d.cache_hit_ratio < 0.90 ? "danger" : d.cache_hit_ratio < 0.99 ? "warning" : "success";
                        h.raw("<tr><td><a href=\"/db/").text(d.name).raw("/schemas\"><strong>").text(d.name).raw("</strong></a></td>");
                        h.raw("<td class=\"num\"><div style=\"display:flex;align-items:center;gap:8px;justify-content:flex-end\">");
                        h.text(d.size); SizeBar::render({pct}, h);
                        h.raw("</div></td>");
                        h.raw("<td class=\"num\">").raw(std::to_string(d.connections)).raw("</td>");
                        h.raw("<td>"); Badge::render({std::format("{:.0f}%", d.cache_hit_ratio * 100), cache_v}, h); h.raw("</td>");
                        h.raw("</tr>");
                    }
                    h.raw("</tbody></table>");
                }
            }
        }

        // Activity panel
        if (activity_res) {
            { auto _ = scope(h, "div", "class=\"dashboard-section\"");
                h.raw("<div class=\"dashboard-section-header\">Activity by Database</div>");
                { auto _ = scope(h, "div", "class=\"dashboard-section-body\"");
                    h.raw("<table><thead><tr><th>Database</th><th>Active</th><th>Idle</th><th>Idle Txn</th><th>Commits</th><th>Rollbacks</th></tr></thead><tbody>");
                    for (auto& a : *activity_res) {
                        h.raw("<tr><td><strong>").text(a.database).raw("</strong></td>");
                        h.raw("<td class=\"num\">").raw(std::to_string(a.active)).raw("</td>");
                        h.raw("<td class=\"num\">").raw(std::to_string(a.idle)).raw("</td>");
                        h.raw("<td class=\"num\">").raw(std::to_string(a.idle_in_transaction)).raw("</td>");
                        h.raw("<td class=\"num\">").raw(std::to_string(a.xact_commit)).raw("</td>");
                        h.raw("<td class=\"num\">").raw(std::to_string(a.xact_rollback)).raw("</td>");
                        h.raw("</tr>");
                    }
                    h.raw("</tbody></table>");
                }
            }
        }

        h.raw("</div>"); // dashboard-grid
    });

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
            {"indexes","Index Analysis",false},{"erd/page","ERD",false}}) {
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

auto TableDetailHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db = req.param("db"); auto sc = req.param("schema"); auto tb = req.param("table");
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto base = std::format("/db/{}/schema/{}/table/{}", db, sc, tb);

    auto render = [&](Html& h) {
        Breadcrumbs::render(std::vector<Crumb>{{"Databases","/databases"},{std::string(db),std::format("/db/{}/schemas",db)},
            {std::string(sc),std::format("/db/{}/schema/{}/tables",db,sc)},{std::string(tb),""}}, h);

        // Tabs + actions
        h.raw("<div style=\"display:flex;align-items:center;justify-content:space-between;margin-bottom:var(--sp-3)\">");
        SectionTabs::render({{{"Columns","",true},{"Data",base+"/browse"},{"DDL",base+"/ddl"},{"Statistics",base+"/stats"}}}, "tab-content", h);
        h.raw("<div class=\"btn-group\">");
        h.raw("<button class=\"btn btn-sm\" hx-post=\"").raw(base).raw("/vacuum\" hx-target=\"#action-result\" hx-swap=\"innerHTML\">Vacuum</button>");
        h.raw("<button class=\"btn btn-sm\" hx-post=\"").raw(base).raw("/analyze\" hx-target=\"#action-result\" hx-swap=\"innerHTML\">Analyze</button>");
        h.raw("<button class=\"btn btn-sm btn-danger\" hx-post=\"").raw(base).raw("/truncate\" hx-target=\"#action-result\" hx-swap=\"innerHTML\" hx-confirm=\"TRUNCATE ").text(sc).raw(".").text(tb).raw("?\">Truncate</button>");
        h.raw("</div></div><div id=\"action-result\"></div><div id=\"tab-content\">");

        // Columns
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

        // Indexes
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

        // Constraints
        auto cons = pg::list_constraints(conn->get(), sc, tb);
        if (cons && !cons->empty()) {
            h.raw("<h3>Constraints</h3>");
            Table::begin(h, {{"Name",""},{"Type",""},{"Definition",""}});
            for (auto& c : *cons) {
                Table::row(h, {{escape(c.name),render_to_string<Badge>(Badge::Props{c.type,"secondary"}),std::format("<code>{}</code>",escape(c.definition))}});
            }
            Table::end(h);
        }
        h.raw("</div>");
    };
    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page(std::format("{}.{}",sc,tb), "Dashboard", render));
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
    h.raw("<div class=\"query-info\"><span class=\"rows-badge\">").raw(std::to_string(result->row_count()))
     .raw(" rows</span> <span class=\"time-badge\">limit ").raw(std::to_string(limit)).raw("</span></div>");
    std::vector<Col> cols;
    for (int c = 0; c < result->col_count(); ++c) cols.push_back({result->column_name(c),"",true});
    Table::begin(h, cols);
    for (auto row : *result) {
        std::vector<std::string> cells;
        for (int c = 0; c < row.col_count(); ++c) {
            if (row.is_null(c)) cells.emplace_back("<span class=\"null-value\">NULL</span>");
            else { auto v = row[c]; cells.push_back(v.size()>200 ? escape(v.substr(0,200))+"..." : escape(v)); }
        }
        Table::row(h, cells);
    }
    Table::end(h);
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
    int total_pages = std::max(1, static_cast<int>((total_rows + limit - 1) / limit));

    // Fetch data WITH ctid for row identification (fast, small response)
    auto data_sql = std::format("SELECT ctid, * FROM \"{}\".\"{}\"", schema_name, table_name);
    if (!sort_col.empty()) {
        data_sql += std::format(" ORDER BY \"{}\" {}", sort_col, (sort_dir == "desc") ? "DESC" : "ASC");
    }
    data_sql += std::format(" LIMIT {} OFFSET {}", limit, offset);

    auto result = conn->get().exec(data_sql);
    if (!result) return Response::html(
        std::format("<div class=\"query-error\">{}</div>", escape(error_message(result.error())))
    );

    auto base_url = std::format("/db/{}/schema/{}/table/{}/browse", db_name, schema_name, table_name);
    auto meta = std::format("data-schema=\"{}\" data-table=\"{}\" data-db=\"{}\"",
        escape(schema_name), escape(table_name), escape(db_name));

    std::string content;

    // Toolbar
    content += "<div class=\"data-toolbar\">";
    content += std::format("<span class=\"data-info\">~{} rows | Page {} of ~{}</span>", total_rows, page, total_pages);
    content += "<div class=\"btn-group\">";
    if (page > 1)
        content += std::format("<button class=\"btn btn-sm\" hx-get=\"{}?page=1&limit={}\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">&laquo;</button>", base_url, limit);
    if (page > 1)
        content += std::format("<button class=\"btn btn-sm\" hx-get=\"{}?page={}&limit={}\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">&lsaquo; Prev</button>", base_url, page-1, limit);
    if (page < total_pages)
        content += std::format("<button class=\"btn btn-sm\" hx-get=\"{}?page={}&limit={}\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">Next &rsaquo;</button>", base_url, page+1, limit);
    content += "</div>";
    content += "<div class=\"btn-group\">";
    for (auto sz : {25, 50, 100, 250}) {
        auto cls = (sz == limit) ? "btn btn-sm btn-primary" : "btn btn-sm";
        content += std::format("<button class=\"{}\" hx-get=\"{}?page=1&limit={}\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">{}</button>", cls, base_url, sz, sz);
    }
    content += "</div>";
    content += std::format("<a href=\"/db/{}/schema/{}/table/{}/export\" class=\"btn btn-sm\">CSV</a>", db_name, schema_name, table_name);
    content += std::format("<button class=\"btn btn-sm btn-success\" onclick=\"document.getElementById('insert-form').style.display=document.getElementById('insert-form').style.display==='none'?'':'none'\">+ Insert</button>");
    content += "</div>";

    // Insert row form (hidden)
    content += "<div id=\"insert-form\" style=\"display:none\" class=\"insert-form\">";
    content += std::format("<form hx-post=\"/db/{}/schema/{}/table/{}/insert-row\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">", db_name, schema_name, table_name);
    content += "<div class=\"insert-form-grid\">";
    // Skip column 0 which is ctid
    for (int c = 1; c < result->col_count(); ++c) {
        auto col_name = std::string(result->column_name(c));
        content += std::format(
            "<div class=\"insert-field\"><label>{}</label>"
            "<input type=\"text\" name=\"col_{}\" placeholder=\"{}\" class=\"insert-input\"></div>",
            escape(col_name), c - 1, escape(col_name));
    }
    content += "</div><div class=\"insert-actions\">";
    content += "<button type=\"submit\" class=\"btn btn-sm btn-primary\">Insert</button>";
    content += "<button type=\"button\" class=\"btn btn-sm\" onclick=\"this.closest('#insert-form').style.display='none'\">Cancel</button>";
    content += "</div></form></div>";

    // Data grid — column 0 is ctid (hidden), columns 1+ are data
    content += "<div class=\"table-wrapper scrollable\" id=\"data-grid-wrap\"><table id=\"data-grid\">";
    content += "<thead><tr><th class=\"row-num-header\">#</th>";
    for (int c = 1; c < result->col_count(); ++c) {
        content += std::format("<th class=\"sortable\">{}</th>", escape(result->column_name(c)));
    }
    content += "<th></th></tr></thead><tbody>";

    for (auto row : *result) {
        auto ctid = std::string(row[0]); // ctid is column 0
        auto row_num = offset + row.index() + 1;

        content += std::format("<tr data-ctid=\"{}\" {}>", escape(ctid), meta);
        content += std::format("<td class=\"row-num\">{}</td>", row_num);

        for (int c = 1; c < row.col_count(); ++c) {
            auto col_name = result->column_name(c);
            if (row.is_null(c)) {
                content += std::format("<td><span class=\"null-value editable-cell\" data-col=\"{}\" data-ctid=\"{}\">NULL</span></td>", escape(col_name), escape(ctid));
            } else {
                auto val = std::string(row[c]);
                // Short values: show inline. Long values: truncate with expand button
                if (val.size() <= 80) {
                    content += std::format("<td><span class=\"editable-cell\" data-col=\"{}\" data-ctid=\"{}\">{}</span></td>",
                        escape(col_name), escape(ctid), escape(val));
                } else {
                    auto preview = escape(val.substr(0, 60));
                    content += std::format(
                        "<td><span class=\"editable-cell cell-long\" data-col=\"{}\" data-ctid=\"{}\" "
                        "data-full=\"{}\">{}&hellip;</span></td>",
                        escape(col_name), escape(ctid),
                        escape(val), preview);
                }
            }
        }

        // Delete
        content += std::format(
            "<td><button class=\"btn btn-sm btn-danger\" "
            "hx-post=\"/db/{}/schema/{}/table/{}/delete-row\" "
            "hx-vals='{{\"ctid\":\"{}\"}}' "
            "hx-target=\"#tab-content\" hx-swap=\"innerHTML\" "
            "hx-confirm=\"Delete this row?\">&#10005;</button></td>",
            db_name, schema_name, table_name, json_escape_str(ctid));

        content += "</tr>";
    }
    content += "</tbody></table></div>";

    return Response::html(std::move(content));
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

    std::string escaped_val;
    for (char c : val) { if (c == '\'') escaped_val += "''"; else escaped_val += c; }

    auto sql = std::format("UPDATE \"{}\".\"{}\" SET \"{}\" = '{}' WHERE ctid = '{}'",
        schema_name, table_name, col, escaped_val, ctid);
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

    auto ctid = form_value(body, "ctid");
    if (ctid.empty()) return Response::error("No row identified", 400);

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));

    auto sql = std::format("DELETE FROM \"{}\".\"{}\" WHERE ctid = '{}'", schema_name, table_name, ctid);
    auto result = conn->get().exec(sql);
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

    if (col_names.empty()) return Response::html(render_to_string<Alert>(Alert::Props{"No values provided", "warning"}));

    auto sql = std::format("INSERT INTO \"{}\".\"{}\" ({}) VALUES ({})", schema_name, table_name, col_names, values);
    auto result = conn->get().exec(sql);
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
