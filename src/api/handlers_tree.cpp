#include "api/handlers_tree.hpp"
#include "html/escape.hpp"
#include "ssr/components.hpp"
#include "pg/catalog.hpp"

#include <format>

namespace getgresql::api {

auto TreeRootHandler::handle(Request& /*req*/, AppContext& /*ctx*/) -> Response {
    auto h = ssr::Html::with_capacity(2048);

    // Databases (expandable)
    ssr::TreeNode::expandable(h, "db", "Databases", "/tree/databases", 0);

    ssr::TreeNode::separator(h);

    // Quick-access leaf nodes
    ssr::TreeNode::leaf(h, "monitor", "Monitor", "/monitor", 0);
    ssr::TreeNode::leaf(h, "monitor", "Locks", "/monitor/locks", 0);
    ssr::TreeNode::leaf(h, "monitor", "Table Stats", "/monitor/tablestats", 0);
    ssr::TreeNode::leaf(h, "monitor", "Replication", "/monitor/replication", 0);

    ssr::TreeNode::separator(h);

    ssr::TreeNode::leaf(h, "role", "Roles", "/roles", 0);
    ssr::TreeNode::leaf(h, "role", "Permissions", "/admin/permissions", 0);
    ssr::TreeNode::leaf(h, "ext", "Extensions", "/extensions", 0);
    ssr::TreeNode::leaf(h, "settings", "Settings", "/settings", 0);
    ssr::TreeNode::leaf(h, "idx", "Unused Indexes", "/admin/unused-indexes", 0);
    ssr::TreeNode::leaf(h, "idx", "Schema Diff", "/schema-diff", 0);

    return Response::html(std::move(h).finish());
}

auto TreeDatabasesHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::Alert::render({error_message(conn.error()), "error"}, h);
        return Response::html(std::move(h).finish());
    }

    auto dbs = pg::list_databases(conn->get());
    if (!dbs) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::Alert::render({error_message(dbs.error()), "error"}, h);
        return Response::html(std::move(h).finish());
    }

    auto h = ssr::Html::with_capacity(2048);
    for (auto& db : *dbs) {
        auto url = std::format("/tree/db/{}", html::escape(db.name));
        ssr::TreeNode::expandable(h, "db", db.name, url, 1, db.size);
    }
    return Response::html(std::move(h).finish());
}

auto TreeSchemasHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");

    auto conn = ctx.pool.checkout();
    if (!conn) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::Alert::render({error_message(conn.error()), "error"}, h);
        return Response::html(std::move(h).finish());
    }

    auto schemas = pg::list_schemas(conn->get(), db_name);
    if (!schemas) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::Alert::render({error_message(schemas.error()), "error"}, h);
        return Response::html(std::move(h).finish());
    }

    auto h = ssr::Html::with_capacity(2048);
    for (auto& s : *schemas) {
        auto url = std::format("/tree/db/{}/schema/{}", db_name, html::escape(s.name));
        ssr::TreeNode::expandable(h, "schema", s.name, url, 2);
    }
    return Response::html(std::move(h).finish());
}

auto TreeSchemaChildrenHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto h = ssr::Html::with_capacity(2048);

    // Group: Tables
    auto tables_url = std::format("/tree/db/{}/schema/{}/tables", db_name, schema_name);
    ssr::TreeNode::expandable(h, "folder", "Tables", tables_url, 3);

    // Group: Functions
    auto funcs_url = std::format("/tree/db/{}/schema/{}/functions", db_name, schema_name);
    ssr::TreeNode::expandable(h, "folder", "Functions", funcs_url, 3);

    // Group: Sequences
    auto seqs_url = std::format("/tree/db/{}/schema/{}/sequences", db_name, schema_name);
    ssr::TreeNode::expandable(h, "folder", "Sequences", seqs_url, 3);

    // Leaf: Index Analysis
    auto idx_url = std::format("/db/{}/schema/{}/indexes", db_name, schema_name);
    ssr::TreeNode::leaf(h, "idx", "Index Analysis", idx_url, 3);

    // Leaf: ERD (Entity Relationship Diagram)
    auto erd_url = std::format("/db/{}/schema/{}/erd/page", db_name, schema_name);
    ssr::TreeNode::leaf(h, "view", "ERD Diagram", erd_url, 3);

    return Response::html(std::move(h).finish());
}

auto TreeTablesHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::Alert::render({error_message(conn.error()), "error"}, h);
        return Response::html(std::move(h).finish());
    }

    auto tables = pg::list_tables(conn->get(), schema_name);
    if (!tables) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::Alert::render({error_message(tables.error()), "error"}, h);
        return Response::html(std::move(h).finish());
    }

    if (tables->empty()) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::TreeNode::empty_leaf(h, "No tables", 4);
        return Response::html(std::move(h).finish());
    }

    auto h = ssr::Html::with_capacity(2048);
    for (auto& t : *tables) {
        auto href = std::format("/db/{}/schema/{}/table/{}", db_name, schema_name, html::escape(t.name));
        auto icon = t.type == "view" ? "view" : "table";
        auto size_extra = std::format("<span class=\"tree-badge\">{}</span>", html::escape(t.size));
        ssr::TreeNode::leaf(h, icon, t.name, href, 4, size_extra);
    }
    return Response::html(std::move(h).finish());
}

auto TreeFunctionsHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::Alert::render({error_message(conn.error()), "error"}, h);
        return Response::html(std::move(h).finish());
    }

    auto funcs = pg::list_functions(conn->get(), schema_name);
    if (!funcs) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::Alert::render({error_message(funcs.error()), "error"}, h);
        return Response::html(std::move(h).finish());
    }

    if (funcs->empty()) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::TreeNode::empty_leaf(h, "No functions", 4);
        return Response::html(std::move(h).finish());
    }

    auto h = ssr::Html::with_capacity(2048);
    for (auto& f : *funcs) {
        auto href = std::format("/db/{}/schema/{}/function/{}", db_name, schema_name, html::escape(f.name));
        ssr::TreeNode::leaf(h, "func", f.name, href, 4);
    }
    return Response::html(std::move(h).finish());
}

auto TreeSequencesHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db_name = req.param("db");
    auto schema_name = req.param("schema");

    auto conn = ctx.pool.checkout();
    if (!conn) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::Alert::render({error_message(conn.error()), "error"}, h);
        return Response::html(std::move(h).finish());
    }

    auto seqs = pg::list_sequences(conn->get(), schema_name);
    if (!seqs) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::Alert::render({error_message(seqs.error()), "error"}, h);
        return Response::html(std::move(h).finish());
    }

    if (seqs->empty()) {
        auto h = ssr::Html::with_capacity(2048);
        ssr::TreeNode::empty_leaf(h, "No sequences", 4);
        return Response::html(std::move(h).finish());
    }

    auto h = ssr::Html::with_capacity(2048);
    for (auto& s : *seqs) {
        // Sequences don't have a detail page, just link to the list
        auto href = std::format("/db/{}/schema/{}/sequences", db_name, schema_name);
        ssr::TreeNode::leaf(h, "seq", s.name, href, 4);
    }
    return Response::html(std::move(h).finish());
}

} // namespace getgresql::api
