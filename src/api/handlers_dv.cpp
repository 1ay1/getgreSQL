#include "api/handlers_dv.hpp"
#include "core/expected.hpp"
#include "ssr/components.hpp"
#include "ssr/components/dv_edit.hpp"
#include "pg/catalog.hpp"
#include "pg/sql_dsl.hpp"

#include <format>
#include <string>

namespace getgresql::api {

using namespace ssr;

static auto url_decode(std::string_view input) -> std::string {
    std::string decoded;
    decoded.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '+') decoded += ' ';
        else if (input[i] == '%' && i + 2 < input.size()) {
            decoded += static_cast<char>(std::stoi(std::string(input.substr(i + 1, 2)), nullptr, 16));
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
    return url_decode((end == std::string_view::npos) ? body.substr(start) : body.substr(start, end - start));
}

// Resolve table OID → schema + table name
static auto resolve_oid(pg::PgConnection<pg::Connected>& conn, std::string_view oid)
    -> std::pair<std::string, std::string> {
    auto q = pg::sql::query()
        .raw("SELECT n.nspname, c.relname FROM pg_class c "
             "JOIN pg_namespace n ON n.oid = c.relnamespace WHERE c.oid = ").val(oid)
        .build();
    auto r = conn.exec(q);
    if (r && r->row_count() > 0) return {std::string(r->get(0, 0)), std::string(r->get(0, 1))};
    return {};
}

// ─── DvEditCellHandler ──────────────────────────────────────────────

auto DvEditCellHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    return Response::html(render_to_string<DvEditForm>({
        .db = req.query("db"),
        .schema = req.query("schema"),
        .table = req.query("table"),
        .col = req.query("col"),
        .ctid = req.query("ctid"),
        .val = req.query("val"),
        .table_oid = req.query("table_oid"),
    }));
}

// ─── DvSaveCellHandler ──────────────────────────────────────────────

auto DvSaveCellHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto body = std::string(req.body());
    auto db = form_value(body, "db");
    auto schema = form_value(body, "schema");
    auto table = form_value(body, "table");
    auto col = form_value(body, "col");
    auto ctid = form_value(body, "ctid");
    auto val = form_value(body, "val");
    auto table_oid = form_value(body, "table_oid");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<DvSavedCell>({
        .col = col, .ctid = ctid, .val = val, .is_error = true,
        .error_msg = "Connection failed"
    }));

    // Resolve table from OID if needed (query result mode)
    if (table.empty() && !table_oid.empty() && table_oid != "0") {
        auto [s, t] = resolve_oid(conn->get(), table_oid);
        schema = std::move(s);
        table = std::move(t);
    }

    if (table.empty()) {
        return Response::html(render_to_string<DvSavedCell>({
            .col = col, .ctid = ctid, .val = val, .is_error = true,
            .error_msg = "Cannot determine source table"
        }));
    }

    auto result = conn->get().exec(pg::sql::update_cell(schema, table, col, val, ctid));

    return Response::html(render_to_string<DvSavedCell>({
        .db = db, .schema = schema, .table = table,
        .col = col, .ctid = ctid, .val = val,
        .is_error = !result.has_value(),
        .error_msg = result ? "" : error_message(result.error()),
    }));
}

// ─── DvSetNullHandler ───────────────────────────────────────────────

auto DvSetNullHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto body = std::string(req.body());
    auto db = form_value(body, "db");
    auto schema = form_value(body, "schema");
    auto table = form_value(body, "table");
    auto col = form_value(body, "col");
    auto ctid = form_value(body, "ctid");

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<DvSavedCell>({
        .col = col, .ctid = ctid, .is_null = true, .is_error = true
    }));

    auto q = pg::sql::query()
        .raw("UPDATE ").id(schema, table)
        .raw(" SET ").id(col).raw(" = NULL WHERE ctid = ").val(ctid)
        .build();
    auto result = conn->get().exec(q);

    return Response::html(render_to_string<DvSavedCell>({
        .db = db, .schema = schema, .table = table,
        .col = col, .ctid = ctid, .is_null = true,
        .is_error = !result.has_value()
    }));
}

// ─── DvExplainCellHandler ───────────────────────────────────────────

auto DvExplainCellHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto table_oid = std::string(req.query("table_oid"));
    auto col = std::string(req.query("col"));
    auto val = std::string(req.query("val"));

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>({"Connection failed", "error"}));

    DvLineagePanel::Props props{.col = col, .val = val};

    if (table_oid.empty() || table_oid == "0") {
        // Computed column
        return Response::html(render_to_string<DvLineagePanel>(props));
    }

    // Query column metadata
    auto info_q = pg::sql::query()
        .raw("SELECT n.nspname, c.relname, a.attname, "
             "pg_catalog.format_type(a.atttypid, a.atttypmod), "
             "pg_size_pretty(pg_total_relation_size(c.oid)), "
             "c.reltuples::bigint, "
             "COALESCE(d.description, ''), "
             "COALESCE(td.description, ''), "
             "a.attnotnull, "
             "COALESCE(pg_get_expr(ad.adbin, ad.adrelid), '') "
             "FROM pg_class c "
             "JOIN pg_namespace n ON n.oid = c.relnamespace "
             "JOIN pg_attribute a ON a.attrelid = c.oid AND a.attname = ").val(col)
        .raw(" LEFT JOIN pg_description d ON d.objoid = c.oid AND d.objsubid = a.attnum "
             "LEFT JOIN pg_description td ON td.objoid = c.oid AND td.objsubid = 0 "
             "LEFT JOIN pg_attrdef ad ON ad.adrelid = c.oid AND ad.adnum = a.attnum "
             "WHERE c.oid = ").val(table_oid)
        .build();
    auto info = conn->get().exec(info_q);

    if (!info || info->row_count() == 0) {
        return Response::html(render_to_string<DvLineagePanel>(props));
    }

    props.has_source = true;
    props.source = {
        .schema = std::string(info->get(0, 0)),
        .table = std::string(info->get(0, 1)),
        .column = std::string(info->get(0, 2)),
        .type = std::string(info->get(0, 3)),
        .table_size = std::string(info->get(0, 4)),
        .approx_rows = std::string(info->get(0, 5)),
        .col_comment = std::string(info->get(0, 6)),
        .table_comment = std::string(info->get(0, 7)),
        .not_null = info->get(0, 8) == "t",
        .col_default = std::string(info->get(0, 9)),
    };

    // Foreign keys
    auto fk_q = pg::sql::query()
        .raw("SELECT conname, pg_get_constraintdef(oid) FROM pg_constraint "
             "WHERE conrelid = ").val(table_oid)
        .raw(" AND contype = 'f'")
        .build();
    if (auto fks = conn->get().exec(fk_q); fks) {
        for (auto fk : *fks) {
            props.fks.push_back({std::string(fk[0]), std::string(fk[1])});
        }
    }

    // Indexes involving this column
    auto idx_q = pg::sql::query()
        .raw("SELECT pg_get_indexdef(i.indexrelid) FROM pg_index i "
             "JOIN pg_attribute a ON a.attrelid = i.indrelid AND a.attnum = ANY(i.indkey) "
             "WHERE i.indrelid = ").val(table_oid)
        .raw(" AND a.attname = ").val(col)
        .build();
    if (auto idxs = conn->get().exec(idx_q); idxs) {
        for (auto idx : *idxs) {
            props.indexes.push_back({std::string(idx[0])});
        }
    }

    return Response::html(render_to_string<DvLineagePanel>(props));
}

} // namespace getgresql::api
