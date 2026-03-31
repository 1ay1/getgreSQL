#include "api/handlers_db.hpp"
#include "core/expected.hpp"
#include "html/templates.hpp"
#include "pg/catalog.hpp"
#include "pg/monitor.hpp"

#include <format>

namespace getgresql::api {

auto IndexHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(html::page("Error", "Dashboard",
            html::alert(error_message(conn.error()), "error")));
    }

    auto stats_res = pg::server_stats(conn->get());
    auto dbs_res = pg::list_databases(conn->get());

    std::string content;

    if (stats_res) {
        auto& s = *stats_res;
        content += "<div class=\"stat-grid\">";
        content += html::stat_card("Active Connections", std::to_string(s.active_connections));
        content += html::stat_card("Idle Connections", std::to_string(s.idle_connections));
        content += html::stat_card("Cache Hit Ratio", std::format("{:.1f}%", s.cache_hit_ratio * 100),
                                   s.cache_hit_ratio < 0.90 ? "danger" : "success");
        content += html::stat_card("Commits", std::to_string(s.total_commits));
        content += html::stat_card("Rollbacks", std::to_string(s.total_rollbacks),
                                   s.total_rollbacks > 0 ? "warning" : "");
        content += html::stat_card("Max Connections", std::to_string(s.max_connections));
        content += html::stat_card("Uptime", s.uptime, "accent");
        content += "</div>";

        content += "<div class=\"server-info\"><code>" + html::escape(s.version) + "</code></div>";
    }

    if (dbs_res) {
        content += "<h3>Databases</h3>";
        content += html::table_begin({
            {"Name", "", true}, {"Owner", "", true}, {"Encoding", "", true}, {"Size", "num", true}
        });
        for (auto& db : *dbs_res) {
            content += html::table_row({
                std::format("<a href=\"/db/{}/schemas\">{}</a>", html::escape(db.name), html::escape(db.name)),
                html::escape(db.owner),
                html::escape(db.encoding),
                std::format("<a href=\"/db/{}/size\">{}</a>", html::escape(db.name), html::escape(db.size)),
            });
        }
        content += html::table_end();
    }

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
    auto data_url = std::format("/db/{}/schema/{}/table/{}/data", db_name, schema_name, table_name);
    content += "<div class=\"section-tabs\">";
    content += "<button class=\"section-tab active\">Columns</button>";
    content += std::format(
        "<button class=\"section-tab\" hx-get=\"{}\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\""
        " onclick=\"this.parentElement.querySelector('.active')?.classList.remove('active'); this.classList.add('active')\">Data</button>",
        data_url);
    content += "</div>";
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

} // namespace getgresql::api
