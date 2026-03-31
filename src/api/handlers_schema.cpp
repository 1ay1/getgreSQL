#include "api/handlers_schema.hpp"
#include "core/expected.hpp"
#include "ssr/components.hpp"
#include "ssr/page.hpp"
#include "pg/catalog.hpp"
#include "pg/sql.hpp"

#include <format>
#include <map>
#include <sstream>

namespace getgresql::api {

using namespace ssr;

static auto escape(std::string_view s) -> std::string {
    auto h = Html::with_capacity(s.size() + 32);
    h.text(s);
    return std::move(h).finish();
}

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

// Collect all form values with a given prefix: col_name_0, col_name_1, ...
static auto form_values(std::string_view body, std::string_view prefix) -> std::vector<std::string> {
    std::vector<std::string> vals;
    for (int i = 0; i < 100; ++i) {
        auto key = std::format("{}_{}", prefix, i);
        auto val = form_value(body, key);
        if (val.empty() && i > 0) {
            // Check if there are more (sparse)
            auto next_key = std::format("{}_{}", prefix, i + 1);
            if (form_value(body, next_key).empty()) break;
        }
        vals.push_back(std::move(val));
    }
    return vals;
}

// ─── PG Types (JSON for autocomplete) ───────────────────────────────

auto PgTypesHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::json("[]", 500);
    auto res = conn->get().exec(
        "SELECT typname FROM pg_type WHERE typtype IN ('b','e','d','r') "
        "AND typname NOT LIKE '\\_%' "
        "ORDER BY CASE WHEN typname IN ('integer','int4','bigint','int8','smallint','int2',"
        "'text','varchar','char','boolean','bool','date','timestamp','timestamptz',"
        "'numeric','decimal','real','float4','float8','double precision',"
        "'json','jsonb','uuid','bytea','serial','bigserial') THEN 0 ELSE 1 END, typname"
    );
    if (!res) return Response::json("[]", 500);
    auto h = Html::with_capacity(4096);
    h.raw("[");
    for (auto row : *res) {
        if (row.index() > 0) h.raw(",");
        h.raw("\"").raw(row[0]).raw("\"");
    }
    h.raw("]");
    return Response::json(std::move(h).finish());
}

// ─── CREATE TABLE ───────────────────────────────────────────────────

auto CreateTablePageHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    auto db = req.param("db"); auto sc = req.param("schema");

    auto render = [&](Html& h) {
        Breadcrumbs::render(std::vector<Crumb>{{"Databases","/databases"},
            {std::string(db),std::format("/db/{}/schemas",db)},
            {std::string(sc),std::format("/db/{}/schema/{}/tables",db,sc)},
            {"Create Table",""}}, h);

        h.raw("<form id=\"create-table-form\" hx-post=\"/db/").raw(db).raw("/schema/").raw(sc)
         .raw("/create-table/exec\" hx-target=\"#create-result\" hx-swap=\"innerHTML\">");

        // Table name
        h.raw("<div style=\"margin-bottom:var(--sp-4)\">"
              "<label style=\"font-weight:600;display:block;margin-bottom:var(--sp-2)\">Table Name</label>"
              "<input type=\"text\" name=\"table_name\" required class=\"insert-input\" style=\"max-width:300px\" placeholder=\"my_table\">"
              "</div>");

        // Column definitions
        h.raw("<h3>Columns</h3>"
              "<div id=\"col-defs\">"
              "<div class=\"col-def-row\" style=\"display:grid;grid-template-columns:200px 150px 60px 200px 40px 40px;gap:var(--sp-2);align-items:center;margin-bottom:var(--sp-2)\">"
              "<strong style=\"font-size:var(--font-size-xs);color:var(--text-3)\">Name</strong>"
              "<strong style=\"font-size:var(--font-size-xs);color:var(--text-3)\">Type</strong>"
              "<strong style=\"font-size:var(--font-size-xs);color:var(--text-3)\">PK</strong>"
              "<strong style=\"font-size:var(--font-size-xs);color:var(--text-3)\">Default</strong>"
              "<strong style=\"font-size:var(--font-size-xs);color:var(--text-3)\">NN</strong>"
              "<span></span></div>");

        // Initial 3 column rows
        for (int i = 0; i < 3; ++i) {
            h.raw(std::format(
                "<div class=\"col-def-row\" style=\"display:grid;grid-template-columns:200px 150px 60px 200px 40px 40px;gap:var(--sp-2);align-items:center;margin-bottom:var(--sp-2)\">"
                "<input type=\"text\" name=\"col_name_{}\" class=\"insert-input\" placeholder=\"column_name\">"
                "<input type=\"text\" name=\"col_type_{}\" class=\"insert-input\" placeholder=\"text\" list=\"pg-types\">"
                "<input type=\"checkbox\" name=\"col_pk_{}\" value=\"1\">"
                "<input type=\"text\" name=\"col_default_{}\" class=\"insert-input\" placeholder=\"\">"
                "<input type=\"checkbox\" name=\"col_notnull_{}\" value=\"1\" checked>"
                "<button type=\"button\" class=\"btn btn-sm btn-ghost\" onclick=\"this.closest('.col-def-row').remove()\">&times;</button>"
                "</div>", i, i, i, i, i));
        }
        h.raw("</div>");

        // Add column button
        h.raw("<button type=\"button\" class=\"btn btn-sm\" onclick=\"addColumnDef()\" style=\"margin:var(--sp-2) 0 var(--sp-4)\">+ Add Column</button>");

        // PG types datalist (populated via JS)
        h.raw("<datalist id=\"pg-types\"></datalist>");

        // Preview + Create buttons
        h.raw("<div class=\"btn-group\" style=\"margin-top:var(--sp-4)\">"
              "<button type=\"submit\" class=\"btn btn-primary\">Create Table</button>"
              "<button type=\"button\" class=\"btn\" onclick=\"previewCreateDDL()\">Preview DDL</button>"
              "</div>");

        h.raw("</form>");
        h.raw("<div id=\"create-result\" style=\"margin-top:var(--sp-4)\"></div>");

        // JS for dynamic columns + type autocomplete
        h.raw(R"JS(<script>
var colIdx = 3;
function addColumnDef() {
    var row = document.createElement('div');
    row.className = 'col-def-row';
    row.style.cssText = 'display:grid;grid-template-columns:200px 150px 60px 200px 40px 40px;gap:4px;align-items:center;margin-bottom:4px';
    row.innerHTML = '<input type="text" name="col_name_'+colIdx+'" class="insert-input" placeholder="column_name">' +
        '<input type="text" name="col_type_'+colIdx+'" class="insert-input" placeholder="text" list="pg-types">' +
        '<input type="checkbox" name="col_pk_'+colIdx+'" value="1">' +
        '<input type="text" name="col_default_'+colIdx+'" class="insert-input" placeholder="">' +
        '<input type="checkbox" name="col_notnull_'+colIdx+'" value="1" checked>' +
        '<button type="button" class="btn btn-sm btn-ghost" onclick="this.closest(&quot;.col-def-row&quot;).remove()">&times;</button>';
    document.getElementById('col-defs').appendChild(row);
    colIdx++;
}
function previewCreateDDL() {
    var form = document.getElementById('create-table-form');
    var fd = new FormData(form);
    var params = new URLSearchParams(fd).toString();
    fetch(form.getAttribute('hx-post') + '?preview=1', {method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:params})
        .then(function(r){return r.text()}).then(function(h){document.getElementById('create-result').innerHTML=h;});
}
fetch('/api/pg-types').then(function(r){return r.json()}).then(function(types){
    var dl=document.getElementById('pg-types');
    types.forEach(function(t){var o=document.createElement('option');o.value=t;dl.appendChild(o);});
});
</script>)JS");
    };

    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page("Create Table", "Dashboard", render));
}

auto CreateTableExecHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db = req.param("db"); auto sc = req.param("schema");
    auto body = std::string(req.body());
    auto preview = req.query("preview") == "1";

    auto table_name = form_value(body, "table_name");
    if (table_name.empty()) return Response::html(render_to_string<Alert>(Alert::Props{"Table name required", "warning"}));

    auto names = form_values(body, "col_name");
    auto types = form_values(body, "col_type");
    auto pks = form_values(body, "col_pk");
    auto defaults = form_values(body, "col_default");
    auto notnulls = form_values(body, "col_notnull");

    // Build DDL
    std::string ddl = "CREATE TABLE " + pg::sql::qualified(sc, table_name) + " (\n";
    std::vector<std::string> pk_cols;
    bool first = true;

    for (std::size_t i = 0; i < names.size(); ++i) {
        if (names[i].empty()) continue;
        auto type = (i < types.size() && !types[i].empty()) ? types[i] : "text";
        if (!first) ddl += ",\n";
        first = false;
        ddl += "    " + pg::sql::quote_ident(names[i]) + " " + type;
        if (i < notnulls.size() && notnulls[i] == "1") ddl += " NOT NULL";
        if (i < defaults.size() && !defaults[i].empty()) ddl += " DEFAULT " + defaults[i];
        if (i < pks.size() && pks[i] == "1") pk_cols.push_back(names[i]);
    }

    if (!pk_cols.empty()) {
        ddl += ",\n    PRIMARY KEY (";
        for (std::size_t i = 0; i < pk_cols.size(); ++i) {
            if (i > 0) ddl += ", ";
            ddl += pg::sql::quote_ident(pk_cols[i]);
        }
        ddl += ")";
    }

    ddl += "\n);\n";

    if (preview) {
        auto h = Html::with_capacity(ddl.size() + 128);
        h.raw("<pre class=\"function-source\">").text(ddl).raw("</pre>");
        return Response::html(std::move(h).finish());
    }

    // Execute
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));

    auto result = conn->get().exec(ddl);
    if (!result) {
        auto h = Html::with_capacity(1024);
        h.raw("<div class=\"query-error\"><strong>Error:</strong> ").text(error_message(result.error())).raw("</div>");
        h.raw("<pre class=\"function-source\" style=\"margin-top:var(--sp-3)\">").text(ddl).raw("</pre>");
        return Response::html(std::move(h).finish());
    }

    auto h = Html::with_capacity(512);
    Alert::render({"Table created successfully", "info"}, h);
    h.raw("<script>setTimeout(function(){window.spaNavigate('/db/").raw(db).raw("/schema/").raw(sc).raw("/table/").raw(escape(table_name)).raw("')},500)</script>");
    return Response::html(std::move(h).finish());
}

// ─── ALTER TABLE PAGE ───────────────────────────────────────────────

auto AlterTablePageHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db = req.param("db"); auto sc = req.param("schema"); auto tb = req.param("table");
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));

    auto cols = pg::describe_columns(conn->get(), sc, tb);
    auto idxs = pg::list_indexes(conn->get(), sc, tb);

    auto render = [&](Html& h) {
        Breadcrumbs::render(std::vector<Crumb>{{"Databases","/databases"},
            {std::string(db),std::format("/db/{}/schemas",db)},
            {std::string(sc),std::format("/db/{}/schema/{}/tables",db,sc)},
            {std::string(tb),std::format("/db/{}/schema/{}/table/{}",db,sc,tb)},
            {"Alter",""}}, h);

        auto base = std::format("/db/{}/schema/{}/table/{}", db, sc, tb);

        // Rename table
        h.raw("<h3>Rename Table</h3>");
        h.raw("<form hx-post=\"").raw(base).raw("/rename\" hx-target=\"#alter-result\" hx-swap=\"innerHTML\" style=\"display:flex;gap:var(--sp-2);align-items:center;margin-bottom:var(--sp-4)\">");
        h.raw("<input type=\"text\" name=\"new_name\" class=\"insert-input\" style=\"max-width:200px\" placeholder=\"new_name\" value=\"").text(tb).raw("\">");
        h.raw("<button type=\"submit\" class=\"btn btn-sm\">Rename</button></form>");

        // Existing columns with drop buttons
        if (cols) {
            h.raw("<h3>Columns</h3>");
            Table::begin(h, {{"#","num"},{"Name",""},{"Type",""},{"Nullable",""},{"Default",""},{""}});
            for (auto& c : *cols) {
                Table::row(h, {{
                    std::to_string(c.ordinal),
                    std::format("<code>{}</code>", escape(c.name)),
                    escape(c.type),
                    c.nullable ? std::string("YES") : std::string("NO"),
                    c.default_value.empty() ? std::string("&mdash;") : escape(c.default_value),
                    std::format("<button class=\"btn btn-sm btn-danger\" hx-post=\"{}/drop-column\" hx-vals='{{\"col\":\"{}\"}}' hx-target=\"#alter-result\" hx-swap=\"innerHTML\" hx-confirm=\"Drop column {}?\">Drop</button>",
                        base, c.name, c.name)
                }});
            }
            Table::end(h);
        }

        // Add column form
        h.raw("<h3>Add Column</h3>");
        h.raw("<form hx-post=\"").raw(base).raw("/add-column\" hx-target=\"#alter-result\" hx-swap=\"innerHTML\" style=\"display:flex;gap:var(--sp-2);align-items:center;flex-wrap:wrap;margin-bottom:var(--sp-4)\">");
        h.raw("<input type=\"text\" name=\"col_name\" class=\"insert-input\" style=\"width:150px\" placeholder=\"column_name\" required>");
        h.raw("<input type=\"text\" name=\"col_type\" class=\"insert-input\" style=\"width:120px\" placeholder=\"text\" list=\"pg-types\">");
        h.raw("<label style=\"font-size:var(--font-size-xs);display:flex;gap:4px;align-items:center\"><input type=\"checkbox\" name=\"not_null\" value=\"1\"> NOT NULL</label>");
        h.raw("<input type=\"text\" name=\"default_val\" class=\"insert-input\" style=\"width:150px\" placeholder=\"default value\">");
        h.raw("<button type=\"submit\" class=\"btn btn-sm btn-primary\">Add Column</button></form>");
        h.raw("<datalist id=\"pg-types\"></datalist>");
        h.raw("<script>fetch('/api/pg-types').then(r=>r.json()).then(t=>{var dl=document.getElementById('pg-types');t.forEach(x=>{var o=document.createElement('option');o.value=x;dl.appendChild(o);})});</script>");

        // Indexes with drop buttons
        if (idxs && !idxs->empty()) {
            h.raw("<h3>Indexes</h3>");
            Table::begin(h, {{"Name",""},{"Definition",""},{"Unique",""},{""}});
            for (auto& i : *idxs) {
                Table::row(h, {{
                    escape(i.name),
                    std::format("<code>{}</code>", escape(i.definition)),
                    i.is_unique ? std::string("Yes") : std::string("No"),
                    i.is_primary ? std::string("") : std::format(
                        "<button class=\"btn btn-sm btn-danger\" hx-post=\"{}/drop-index\" hx-vals='{{\"idx\":\"{}\"}}' hx-target=\"#alter-result\" hx-swap=\"innerHTML\" hx-confirm=\"Drop index {}?\">Drop</button>",
                        base, i.name, i.name)
                }});
            }
            Table::end(h);
        }

        // Add index form
        h.raw("<h3>Add Index</h3>");
        h.raw("<form hx-post=\"").raw(base).raw("/add-index\" hx-target=\"#alter-result\" hx-swap=\"innerHTML\" style=\"display:flex;gap:var(--sp-2);align-items:center;flex-wrap:wrap;margin-bottom:var(--sp-4)\">");
        h.raw("<input type=\"text\" name=\"idx_columns\" class=\"insert-input\" style=\"width:200px\" placeholder=\"col1, col2\" required>");
        h.raw("<label style=\"font-size:var(--font-size-xs);display:flex;gap:4px;align-items:center\"><input type=\"checkbox\" name=\"unique\" value=\"1\"> UNIQUE</label>");
        h.raw("<button type=\"submit\" class=\"btn btn-sm btn-primary\">Create Index</button></form>");

        h.raw("<div id=\"alter-result\" style=\"margin-top:var(--sp-4)\"></div>");
    };

    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page(std::format("Alter - {}", tb), "Dashboard", render));
}

// ─── ADD COLUMN ─────────────────────────────────────────────────────

auto AddColumnHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema"); auto tb = req.param("table");
    auto body = std::string(req.body());
    auto col = form_value(body, "col_name");
    auto type = form_value(body, "col_type");
    auto nn = form_value(body, "not_null");
    auto def = form_value(body, "default_val");
    if (col.empty()) return Response::html(render_to_string<Alert>(Alert::Props{"Column name required", "warning"}));
    if (type.empty()) type = "text";

    auto sql = "ALTER TABLE " + pg::sql::qualified(sc, tb) + " ADD COLUMN " + pg::sql::quote_ident(col) + " " + type;
    if (nn == "1") sql += " NOT NULL";
    if (!def.empty()) sql += " DEFAULT " + def;

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));
    auto r = conn->get().exec(sql);
    if (!r) return Response::html(render_to_string<Alert>(Alert::Props{error_message(r.error()), "error"}));
    return Response::html(render_to_string<Alert>(Alert::Props{std::format("Column '{}' added", col), "info"}));
}

// ─── DROP COLUMN ────────────────────────────────────────────────────

auto DropColumnHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema"); auto tb = req.param("table");
    auto col = form_value(std::string(req.body()), "col");
    if (col.empty()) return Response::html(render_to_string<Alert>(Alert::Props{"Column name required", "warning"}));

    auto sql = "ALTER TABLE " + pg::sql::qualified(sc, tb) + " DROP COLUMN " + pg::sql::quote_ident(col);
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));
    auto r = conn->get().exec(sql);
    if (!r) return Response::html(render_to_string<Alert>(Alert::Props{error_message(r.error()), "error"}));
    return Response::html(render_to_string<Alert>(Alert::Props{std::format("Column '{}' dropped", col), "info"}));
}

// ─── ADD INDEX ──────────────────────────────────────────────────────

auto AddIndexHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema"); auto tb = req.param("table");
    auto body = std::string(req.body());
    auto columns = form_value(body, "idx_columns");
    auto unique = form_value(body, "unique") == "1";
    if (columns.empty()) return Response::html(render_to_string<Alert>(Alert::Props{"Columns required", "warning"}));

    auto idx_name = std::format("idx_{}_{}",  tb, columns.substr(0, columns.find(',')));
    // Clean the index name
    for (auto& c : idx_name) if (c == ' ' || c == ',') c = '_';

    auto sql = std::format("CREATE {}INDEX {} ON {} ({})",
        unique ? "UNIQUE " : "", pg::sql::quote_ident(idx_name),
        pg::sql::qualified(sc, tb), columns);

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));
    auto r = conn->get().exec(sql);
    if (!r) return Response::html(render_to_string<Alert>(Alert::Props{error_message(r.error()), "error"}));
    return Response::html(render_to_string<Alert>(Alert::Props{std::format("Index '{}' created", idx_name), "info"}));
}

// ─── DROP INDEX ─────────────────────────────────────────────────────

auto DropIndexHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema");
    auto idx = form_value(std::string(req.body()), "idx");
    if (idx.empty()) return Response::html(render_to_string<Alert>(Alert::Props{"Index name required", "warning"}));

    auto sql = "DROP INDEX " + pg::sql::qualified(sc, idx);
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));
    auto r = conn->get().exec(sql);
    if (!r) return Response::html(render_to_string<Alert>(Alert::Props{error_message(r.error()), "error"}));
    return Response::html(render_to_string<Alert>(Alert::Props{std::format("Index '{}' dropped", idx), "info"}));
}

// ─── RENAME TABLE ───────────────────────────────────────────────────

auto RenameTableHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema"); auto tb = req.param("table");
    auto new_name = form_value(std::string(req.body()), "new_name");
    if (new_name.empty()) return Response::html(render_to_string<Alert>(Alert::Props{"New name required", "warning"}));

    auto sql = "ALTER TABLE " + pg::sql::qualified(sc, tb) + " RENAME TO " + pg::sql::quote_ident(new_name);
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));
    auto r = conn->get().exec(sql);
    if (!r) return Response::html(render_to_string<Alert>(Alert::Props{error_message(r.error()), "error"}));
    return Response::html(render_to_string<Alert>(Alert::Props{std::format("Table renamed to '{}'", new_name), "info"}));
}

// ─── DROP TABLE ─────────────────────────────────────────────────────

auto DropTableHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto db = req.param("db"); auto sc = req.param("schema"); auto tb = req.param("table");
    auto cascade = form_value(std::string(req.body()), "cascade") == "1";

    auto sql = "DROP TABLE " + pg::sql::qualified(sc, tb);
    if (cascade) sql += " CASCADE";

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));
    auto r = conn->get().exec(sql);
    if (!r) return Response::html(render_to_string<Alert>(Alert::Props{error_message(r.error()), "error"}));

    auto h = Html::with_capacity(256);
    Alert::render({"Table dropped", "info"}, h);
    h.raw("<script>setTimeout(function(){window.spaNavigate('/db/").raw(db).raw("/schema/").raw(sc).raw("/tables')},500)</script>");
    return Response::html(std::move(h).finish());
}

// ─── IMPORT DATA ────────────────────────────────────────────────────

auto ImportPageHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    auto db = req.param("db"); auto sc = req.param("schema"); auto tb = req.param("table");

    auto render = [&](Html& h) {
        Breadcrumbs::render(std::vector<Crumb>{{"Databases","/databases"},
            {std::string(db),std::format("/db/{}/schemas",db)},
            {std::string(sc),std::format("/db/{}/schema/{}/tables",db,sc)},
            {std::string(tb),std::format("/db/{}/schema/{}/table/{}",db,sc,tb)},
            {"Import",""}}, h);

        h.raw("<div style=\"max-width:600px\">");
        h.raw("<h3>Import Data</h3>");
        h.raw("<p style=\"color:var(--text-3);font-size:var(--font-size-sm);margin-bottom:var(--sp-4)\">Paste CSV or JSON data below to import into <code>").text(sc).raw(".").text(tb).raw("</code></p>");

        h.raw("<form hx-post=\"/db/").raw(db).raw("/schema/").raw(sc).raw("/table/").raw(tb)
         .raw("/import/exec\" hx-target=\"#import-result\" hx-swap=\"innerHTML\">");
        h.raw("<div style=\"margin-bottom:var(--sp-3)\">"
              "<label style=\"font-weight:600;display:block;margin-bottom:var(--sp-2)\">Format</label>"
              "<select name=\"format\" class=\"settings-select\">"
              "<option value=\"csv\">CSV</option>"
              "<option value=\"json\">JSON</option>"
              "<option value=\"sql\">SQL (INSERT statements)</option>"
              "</select></div>");
        h.raw("<div style=\"margin-bottom:var(--sp-3)\">"
              "<label style=\"font-weight:600;display:block;margin-bottom:var(--sp-2)\">Data</label>"
              "<textarea name=\"data\" class=\"cell-modal-textarea\" style=\"min-height:200px;width:100%;border:1px solid var(--border);border-radius:var(--radius)\" "
              "placeholder=\"Paste CSV, JSON array, or SQL INSERT statements here...\"></textarea></div>");
        h.raw("<div style=\"margin-bottom:var(--sp-3)\">"
              "<label style=\"font-size:var(--font-size-xs);display:flex;gap:4px;align-items:center\">"
              "<input type=\"checkbox\" name=\"has_header\" value=\"1\" checked> CSV has header row</label></div>");
        h.raw("<button type=\"submit\" class=\"btn btn-primary\">Import</button>");
        h.raw("</form>");
        h.raw("<div id=\"import-result\" style=\"margin-top:var(--sp-4)\"></div>");
        h.raw("</div>");
    };

    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page(std::format("Import - {}", tb), "Dashboard", render));
}

auto ImportExecHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema"); auto tb = req.param("table");
    auto body = std::string(req.body());
    auto format = form_value(body, "format");
    auto data = form_value(body, "data");
    auto has_header = form_value(body, "has_header") == "1";

    if (data.empty()) return Response::html(render_to_string<Alert>(Alert::Props{"No data provided", "warning"}));

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));

    int rows_imported = 0;

    if (format == "sql") {
        // Execute SQL directly (split on semicolons)
        std::istringstream stream(data);
        std::string stmt;
        while (std::getline(stream, stmt, ';')) {
            auto trimmed = stmt;
            while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\n')) trimmed.erase(trimmed.begin());
            if (trimmed.empty()) continue;
            auto r = conn->get().exec(trimmed);
            if (!r) return Response::html(render_to_string<Alert>(Alert::Props{error_message(r.error()), "error"}));
            rows_imported++;
        }
    } else if (format == "csv") {
        // Parse CSV and insert
        std::istringstream stream(data);
        std::string line;
        std::vector<std::string> headers;
        bool first_line = true;

        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            // Simple CSV split (handles basic cases)
            std::vector<std::string> fields;
            std::string field;
            bool in_quotes = false;
            for (char c : line) {
                if (c == '"') { in_quotes = !in_quotes; continue; }
                if (c == ',' && !in_quotes) { fields.push_back(field); field.clear(); continue; }
                field += c;
            }
            fields.push_back(field);

            if (first_line && has_header) {
                headers = fields;
                first_line = false;
                continue;
            }
            first_line = false;

            // Build INSERT
            std::string cols_str, vals_str;
            for (std::size_t i = 0; i < fields.size(); ++i) {
                if (i > 0) { cols_str += ", "; vals_str += ", "; }
                if (i < headers.size()) cols_str += pg::sql::quote_ident(headers[i]);
                else cols_str += std::format("column{}", i + 1);
                vals_str += pg::sql::quote_literal(fields[i]);
            }
            auto sql = std::format("INSERT INTO {} ({}) VALUES ({})",
                pg::sql::qualified(sc, tb), cols_str, vals_str);
            auto r = conn->get().exec(sql);
            if (!r) return Response::html(render_to_string<Alert>(Alert::Props{
                std::format("Row {} error: {}", rows_imported + 1, error_message(r.error())), "error"}));
            rows_imported++;
        }
    } else if (format == "json") {
        // Simple JSON array parser: [{"col":"val",...},...]
        // Find array elements between { and }
        std::size_t pos = 0;
        while ((pos = data.find('{', pos)) != std::string::npos) {
            auto end = data.find('}', pos);
            if (end == std::string::npos) break;
            auto obj = data.substr(pos + 1, end - pos - 1);
            pos = end + 1;

            // Parse key-value pairs
            std::string cols_str, vals_str;
            bool first = true;
            std::size_t p = 0;
            while (p < obj.size()) {
                // Find key
                auto ks = obj.find('"', p);
                if (ks == std::string::npos) break;
                auto ke = obj.find('"', ks + 1);
                if (ke == std::string::npos) break;
                auto key = obj.substr(ks + 1, ke - ks - 1);

                // Find value
                auto colon = obj.find(':', ke + 1);
                if (colon == std::string::npos) break;
                p = colon + 1;
                while (p < obj.size() && obj[p] == ' ') p++;

                std::string val;
                if (p < obj.size() && obj[p] == '"') {
                    auto vs = p + 1;
                    auto ve = obj.find('"', vs);
                    val = (ve != std::string::npos) ? obj.substr(vs, ve - vs) : "";
                    p = (ve != std::string::npos) ? ve + 1 : obj.size();
                } else if (p < obj.size() && obj.substr(p, 4) == "null") {
                    val = "";
                    p += 4;
                } else {
                    auto ve = obj.find_first_of(",}", p);
                    val = obj.substr(p, ve - p);
                    while (!val.empty() && val.back() == ' ') val.pop_back();
                    p = (ve != std::string::npos) ? ve : obj.size();
                }

                if (!first) { cols_str += ", "; vals_str += ", "; }
                first = false;
                cols_str += pg::sql::quote_ident(key);
                vals_str += pg::sql::quote_literal(val);

                // Skip comma
                while (p < obj.size() && (obj[p] == ',' || obj[p] == ' ')) p++;
            }

            if (!cols_str.empty()) {
                auto sql = std::format("INSERT INTO {} ({}) VALUES ({})",
                    pg::sql::qualified(sc, tb), cols_str, vals_str);
                auto r = conn->get().exec(sql);
                if (!r) return Response::html(render_to_string<Alert>(Alert::Props{
                    std::format("Row {} error: {}", rows_imported + 1, error_message(r.error())), "error"}));
                rows_imported++;
            }
        }
    }

    return Response::html(render_to_string<Alert>(Alert::Props{
        std::format("{} rows imported into {}.{}", rows_imported, sc, tb), "info"}));
}

// ─── SCHEMA DIFF ────────────────────────────────────────────────────

auto SchemaDiffPageHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::error(error_message(conn.error()));
    auto schemas = pg::list_schemas(conn->get());

    auto render = [&](Html& h) {
        h.raw("<h3>Schema Comparison</h3>");
        h.raw("<p style=\"color:var(--text-3);font-size:var(--font-size-sm);margin-bottom:var(--sp-4)\">Compare two schemas to see differences in tables, columns, indexes, and constraints.</p>");
        h.raw("<form hx-post=\"/schema-diff/exec\" hx-target=\"#diff-result\" hx-swap=\"innerHTML\" style=\"display:flex;gap:var(--sp-3);align-items:center;margin-bottom:var(--sp-4)\">");
        h.raw("<select name=\"left\" class=\"settings-select\">");
        if (schemas) for (auto& s : *schemas) h.raw("<option>").text(s.name).raw("</option>");
        h.raw("</select>");
        h.raw("<span style=\"color:var(--text-3)\">vs</span>");
        h.raw("<select name=\"right\" class=\"settings-select\">");
        if (schemas) for (auto& s : *schemas) h.raw("<option>").text(s.name).raw("</option>");
        h.raw("</select>");
        h.raw("<button type=\"submit\" class=\"btn btn-primary\">Compare</button>");
        h.raw("</form>");
        h.raw("<div id=\"diff-result\"></div>");
    };

    if (req.is_htmx()) return Response::html(render_partial(render));
    return Response::html(render_page("Schema Diff", "Dashboard", render));
}

auto SchemaDiffExecHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto body = std::string(req.body());
    auto left = form_value(body, "left");
    auto right = form_value(body, "right");

    if (left.empty() || right.empty())
        return Response::html(render_to_string<Alert>(Alert::Props{"Select two schemas", "warning"}));
    if (left == right)
        return Response::html(render_to_string<Alert>(Alert::Props{"Schemas are the same", "warning"}));

    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html(render_to_string<Alert>(Alert::Props{error_message(conn.error()), "error"}));

    auto left_tables = pg::list_tables(conn->get(), left);
    auto right_tables = pg::list_tables(conn->get(), right);
    if (!left_tables || !right_tables)
        return Response::html(render_to_string<Alert>(Alert::Props{"Failed to list tables", "error"}));

    // Build sets
    std::map<std::string, pg::TableInfo*> left_map, right_map;
    for (auto& t : *left_tables) left_map[t.name] = &t;
    for (auto& t : *right_tables) right_map[t.name] = &t;

    auto h = Html::with_capacity(8192);
    Table::begin(h, {{"Table",""},{"Status",""},{"Left ("+escape(left)+")",""},{"Right ("+escape(right)+")",""}});

    // Tables only in left
    for (auto& [name, t] : left_map) {
        if (!right_map.count(name)) {
            Table::row(h, {{escape(name), render_to_string<Badge>(Badge::Props{"ONLY LEFT","danger"}),
                escape(t->type) + " (" + escape(t->size) + ")", std::string("&mdash;")}});
        }
    }
    // Tables only in right
    for (auto& [name, t] : right_map) {
        if (!left_map.count(name)) {
            Table::row(h, {{escape(name), render_to_string<Badge>(Badge::Props{"ONLY RIGHT","success"}),
                std::string("&mdash;"), escape(t->type) + " (" + escape(t->size) + ")"}});
        }
    }
    // Tables in both — compare columns
    for (auto& [name, lt] : left_map) {
        auto it = right_map.find(name);
        if (it == right_map.end()) continue;
        auto lc = pg::describe_columns(conn->get(), left, name);
        auto rc = pg::describe_columns(conn->get(), right, name);
        if (!lc || !rc) continue;

        std::map<std::string, std::string> left_cols, right_cols;
        for (auto& c : *lc) left_cols[c.name] = c.type;
        for (auto& c : *rc) right_cols[c.name] = c.type;

        bool different = false;
        for (auto& [cn, ct] : left_cols) if (!right_cols.count(cn) || right_cols[cn] != ct) different = true;
        for (auto& [cn, ct] : right_cols) if (!left_cols.count(cn)) different = true;

        auto status = different ? render_to_string<Badge>(Badge::Props{"DIFFERS","warning"})
                                : render_to_string<Badge>(Badge::Props{"IDENTICAL","secondary"});
        auto l_info = std::format("{} cols, {}", lc->size(), escape(lt->size));
        auto r_info = std::format("{} cols, {}", rc->size(), escape(it->second->size));
        Table::row(h, {{escape(name), status, l_info, r_info}});
    }

    Table::end(h);
    return Response::html(std::move(h).finish());
}

// ─── EXACT ROW COUNT ────────────────────────────────────────────────

auto ExactCountHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto sc = req.param("schema"); auto tb = req.param("table");
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::html("?");
    auto count = pg::table_row_count(conn->get(), sc, tb);
    if (!count) return Response::html("?");
    return Response::html(std::format("<strong>{}</strong> rows (exact)", *count));
}

} // namespace getgresql::api
