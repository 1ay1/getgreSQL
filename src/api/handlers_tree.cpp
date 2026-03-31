#include "api/handlers_tree.hpp"
#include "html/templates.hpp"
#include "pg/catalog.hpp"

#include <format>

namespace getgresql::api {

auto TreeRootHandler::handle(Request& /*req*/, AppContext& /*ctx*/) -> Response {
    std::string out;

    // Databases (expandable)
    out += html::tree_node_expandable("db", "Databases", "/tree/databases", 0);

    out += html::tree_separator();

    // Quick-access leaf nodes
    out += html::tree_node_leaf("monitor", "Monitor", "/monitor", 0);
    out += html::tree_node_leaf("monitor", "Locks", "/monitor/locks", 0);
    out += html::tree_node_leaf("monitor", "Table Stats", "/monitor/tablestats", 0);
    out += html::tree_node_leaf("monitor", "Replication", "/monitor/replication", 0);

    out += html::tree_separator();

    out += html::tree_node_leaf("role", "Roles", "/roles", 0);
    out += html::tree_node_leaf("ext", "Extensions", "/extensions", 0);
    out += html::tree_node_leaf("settings", "Settings", "/settings", 0);

    return Response::html(std::move(out));
}

auto TreeDatabasesHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto dbs = pg::list_databases(conn->get());
    if (!dbs) return Response::html(html::alert(error_message(dbs.error()), "error"));

    std::string out;
    for (auto& db : *dbs) {
        auto url = std::format("/tree/db/{}", html::escape(db.name));
        auto size_badge = std::format("<span class=\"tree-badge\">{}</span>", html::escape(db.size));
        out += html::tree_node_expandable("db", db.name, url, 1, db.size);
    }
    return Response::html(std::move(out));
}

auto TreeSchemasHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto schemas = pg::list_schemas(conn->get(), db_name);
    if (!schemas) return Response::html(html::alert(error_message(schemas.error()), "error"));

    std::string out;
    for (auto& s : *schemas) {
        auto url = std::format("/tree/db/{}/schema/{}", db_name, html::escape(s.name));
        out += html::tree_node_expandable("schema", s.name, url, 2);
    }
    return Response::html(std::move(out));
}

auto TreeSchemaChildrenHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    std::string out;

    // Group: Tables
    auto tables_url = std::format("/tree/db/{}/schema/{}/tables", db_name, schema_name);
    out += html::tree_node_expandable("folder", "Tables", tables_url, 3);

    // Group: Functions
    auto funcs_url = std::format("/tree/db/{}/schema/{}/functions", db_name, schema_name);
    out += html::tree_node_expandable("folder", "Functions", funcs_url, 3);

    // Group: Sequences
    auto seqs_url = std::format("/tree/db/{}/schema/{}/sequences", db_name, schema_name);
    out += html::tree_node_expandable("folder", "Sequences", seqs_url, 3);

    // Leaf: Index Analysis
    auto idx_url = std::format("/db/{}/schema/{}/indexes", db_name, schema_name);
    out += html::tree_node_leaf("idx", "Index Analysis", idx_url, 3);

    return Response::html(std::move(out));
}

auto TreeTablesHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto tables = pg::list_tables(conn->get(), schema_name);
    if (!tables) return Response::html(html::alert(error_message(tables.error()), "error"));

    if (tables->empty()) {
        return Response::html(R"(<li class="tree-item"><span class="tree-row" style="--tree-depth:4"><span class="tree-chevron empty"></span><span class="tree-text" style="color:var(--text-4);font-style:italic">No tables</span></span></li>)");
    }

    std::string out;
    for (auto& t : *tables) {
        auto href = std::format("/db/{}/schema/{}/table/{}", db_name, schema_name, html::escape(t.name));
        auto icon = t.type == "view" ? "view" : "table";
        auto size_extra = std::format("<span class=\"tree-badge\">{}</span>", html::escape(t.size));
        out += html::tree_node_leaf(icon, t.name, href, 4, size_extra);
    }
    return Response::html(std::move(out));
}

auto TreeFunctionsHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto funcs = pg::list_functions(conn->get(), schema_name);
    if (!funcs) return Response::html(html::alert(error_message(funcs.error()), "error"));

    if (funcs->empty()) {
        return Response::html(R"(<li class="tree-item"><span class="tree-row" style="--tree-depth:4"><span class="tree-chevron empty"></span><span class="tree-text" style="color:var(--text-4);font-style:italic">No functions</span></span></li>)");
    }

    std::string out;
    for (auto& f : *funcs) {
        auto href = std::format("/db/{}/schema/{}/function/{}", db_name, schema_name, html::escape(f.name));
        out += html::tree_node_leaf("func", f.name, href, 4);
    }
    return Response::html(std::move(out));
}

auto TreeSequencesHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(html::alert(error_message(conn.error()), "error"));

    auto seqs = pg::list_sequences(conn->get(), schema_name);
    if (!seqs) return Response::html(html::alert(error_message(seqs.error()), "error"));

    if (seqs->empty()) {
        return Response::html(R"(<li class="tree-item"><span class="tree-row" style="--tree-depth:4"><span class="tree-chevron empty"></span><span class="tree-text" style="color:var(--text-4);font-style:italic">No sequences</span></span></li>)");
    }

    std::string out;
    for (auto& s : *seqs) {
        // Sequences don't have a detail page, just link to the list
        auto href = std::format("/db/{}/schema/{}/sequences", db_name, schema_name);
        out += html::tree_node_leaf("seq", s.name, href, 4);
    }
    return Response::html(std::move(out));
}

} // namespace getgresql::api
