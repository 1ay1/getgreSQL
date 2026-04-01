#include "api/handlers_query.hpp"
#include "core/expected.hpp"
#include "ssr/components.hpp"
#include "pg/catalog.hpp"
#include "pg/monitor.hpp"
#include "pg/result_cache.hpp"

#include <chrono>
#include <format>

namespace getgresql::api {

using namespace ssr;

// ─── JSON string escaping ───────────────────────────────────────────

static auto json_escape(std::string_view s) -> std::string {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out += std::format("\\u{:04x}", static_cast<unsigned>(c));
                } else {
                    out += c;
                }
        }
    }
    return out;
}

// ─── URL decoding ───────────────────────────────────────────────────

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

// ─── QueryPageHandler ───────────────────────────────────────────────

auto QueryPageHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    if (req.is_htmx()) {
        return Response::html(R"(<div id="query-workspace" class="query-panel"></div>)");
    }
    return Response::html(render_page_full("Query", "Query", [](Html& h) {
        h.raw(R"(<div id="query-workspace" class="query-panel"></div>)");
    }));
}

// ─── QueryExecHandler ───────────────────────────────────────────────

auto QueryExecHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto body = std::string(req.body());
    auto sql = form_value(body, "sql");

    if (sql.empty()) {
        return Response::html(render_to_string<Alert>({"No SQL provided", "warning"}));
    }

    auto start = std::chrono::steady_clock::now();
    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));
    }

    // Run the query first to check source table info
    auto result = conn->get().exec(sql);
    bool has_ctid = false;

    // If all result columns come from one table, re-run with ctid prepended
    // This enables in-place editing for simple SELECT queries
    if (result && result->col_count() > 0 && result->row_count() > 0) {
        unsigned int common_oid = result->column_table_oid(0);
        bool single_table = common_oid != 0;
        for (int c = 1; c < result->col_count() && single_table; ++c) {
            auto oid = result->column_table_oid(c);
            if (oid == 0 || oid != common_oid) single_table = false;
        }
        if (single_table) {
            // All columns from one table — we can add ctid
            // Resolve table name for the ctid query
            auto oid_q = pg::sql::query()
                .raw("SELECT n.nspname||'.'||quote_ident(c.relname) FROM pg_class c "
                     "JOIN pg_namespace n ON n.oid=c.relnamespace WHERE c.oid=")
                .val(std::to_string(common_oid)).build();
            auto oid_r = conn->get().exec(oid_q);
            if (oid_r && oid_r->row_count() > 0) {
                auto fqn = std::string(oid_r->get(0, 0));
                // Build: SELECT ctid, col1, col2, ... FROM schema.table WHERE (original conditions)
                // Simplification: use the SQL as-is but replace SELECT with SELECT ctid,
                auto upper = sql;
                // Find SELECT keyword (case-insensitive)
                std::string lower_sql = sql;
                for (auto& ch : lower_sql) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                auto sel_pos = lower_sql.find("select");
                if (sel_pos != std::string::npos) {
                    auto ctid_sql = sql.substr(0, sel_pos + 6) + " ctid," + sql.substr(sel_pos + 6);
                    auto ctid_result = conn->get().exec(ctid_sql);
                    if (ctid_result && ctid_result->col_count() > 1) {
                        result = std::move(ctid_result);
                        has_ctid = true;
                    }
                }
            }
        }
    }
    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    if (!result) {
        auto h = Html::with_capacity(1024);
        h.raw("<div class=\"query-error\"><strong>Error:</strong> ").text(error_message(result.error())).raw("</div>");
        return Response::html(std::move(h).finish());
    }

    constexpr int FIRST_BATCH = 200;

    auto h = Html::with_capacity(16384);

    if (result->row_count() > 0 || result->col_count() > 0) {
        int col_start = has_ctid ? 1 : 0;
        int total_rows = result->row_count();

        // Build columns with source table OIDs from PQftable
        std::vector<DCol> cols;
        for (int c = col_start; c < result->col_count(); ++c) {
            cols.push_back({
                .name = result->column_name(c),
                .table_oid = result->column_table_oid(c),
            });
        }

        // If more rows than first batch, cache result for streaming
        std::string stream_id;
        if (total_rows > FIRST_BATCH) {
            stream_id = pg::result_cache().store(
                std::move(*result), has_ctid, col_start,
                conn->get().dbname());
            // Re-acquire pointer via cache for rendering first batch
            auto* cached = pg::result_cache().get(stream_id);
            if (!cached) {
                return Response::html(render_to_string<Alert>({"Internal cache error", "error"}));
            }
            result.emplace(pg::PgResult(nullptr)); // moved away, use cache

            auto view = DataView::readonly(h, {
                .row_count = total_rows,
                .exec_ms = static_cast<int>(ms),
                .db = cached->db_name,
                .stream_id = stream_id,
            });
            view.columns(cols);

            // Render first batch from cache
            auto& res = *cached->result;
            for (int r = 0; r < FIRST_BATCH && r < total_rows; ++r) {
                auto ctid = has_ctid ? std::string_view(res.get(r, 0)) : std::string_view("");
                std::vector<Cell> cells;
                for (int c = col_start; c < res.col_count(); ++c) {
                    if (res.is_null(r, c)) {
                        cells.push_back({.is_null = true});
                    } else {
                        cells.push_back({.value = std::string(res.get(r, c))});
                    }
                }
                view.row(cells, ctid);
            }
        } else {
            // Small result — render all rows directly
            auto view = DataView::readonly(h, {
                .row_count = total_rows,
                .exec_ms = static_cast<int>(ms),
                .db = conn->get().dbname(),
            });
            view.columns(cols);

            for (int r = 0; r < total_rows; ++r) {
                auto ctid = has_ctid ? std::string_view(result->get(r, 0)) : std::string_view("");
                std::vector<Cell> cells;
                for (int c = col_start; c < result->col_count(); ++c) {
                    if (result->is_null(r, c)) {
                        cells.push_back({.is_null = true});
                    } else {
                        cells.push_back({.value = std::string(result->get(r, c))});
                    }
                }
                view.row(cells, ctid);
            }
        }
    } else {
        auto affected = std::string(result->affected_rows());
        auto view = DataView::readonly(h, {
            .row_count = affected.empty() ? 0 : std::stoi(affected),
            .exec_ms = static_cast<int>(ms),
            .command_tag = result->command_tag()});
    }

    return Response::html(std::move(h).finish());
}

// ─── QueryRowsHandler — serve row batches from cache ────────────────

auto QueryRowsHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    auto id = std::string(req.query("id"));
    auto offset_str = req.query("offset");
    auto limit_str = req.query("limit");

    int offset = offset_str.empty() ? 0 : std::stoi(std::string(offset_str));
    int limit = limit_str.empty() ? 200 : std::stoi(std::string(limit_str));
    limit = std::min(limit, 500); // cap batch size

    auto* cached = pg::result_cache().get(id);
    if (!cached) {
        return Response::json(R"({"done":true,"rows":0})", 200);
    }

    auto& res = *cached->result;
    int total = res.row_count();
    int col_start = cached->col_start;
    bool has_ctid = cached->has_ctid;
    int end = std::min(offset + limit, total);

    // Render row batch as HTML <tr> elements
    auto h = Html::with_capacity(static_cast<std::size_t>((end - offset) * 256));

    for (int r = offset; r < end; ++r) {
        auto ctid = has_ctid ? std::string_view(res.get(r, 0)) : std::string_view("");
        h.raw("<tr");
        if (!ctid.empty()) h.raw(" data-ctid=\"").text(ctid).raw("\"");
        h.raw(">");

        for (int c = col_start; c < res.col_count(); ++c) {
            h.raw("<td>");
            if (res.is_null(r, c)) {
                h.raw("<span class=\"null-value dv-cell\">NULL</span>");
            } else {
                auto val = res.get(r, c);
                h.raw("<span class=\"dv-cell\"");
                auto col_name = res.column_name(c);
                if (!col_name.empty()) h.raw(" data-col=\"").text(col_name).raw("\"");
                auto table_oid = res.column_table_oid(c);
                if (table_oid != 0) h.raw(" data-table-oid=\"").raw(std::to_string(table_oid)).raw("\"");
                if (val.size() > 80) {
                    h.raw(" data-full=\"").text(val).raw("\" class=\"dv-cell dv-cell-long\">");
                    h.text(val.substr(0, 60)).raw("&hellip;");
                } else {
                    h.raw(">").text(val);
                }
                h.raw("</span>");
            }
            h.raw("</td>");
        }
        h.raw("</tr>\n");
    }

    // Wrap in JSON with metadata
    auto rows_html = std::move(h).finish();
    auto done = (end >= total);

    // If done, clean up cache
    if (done) {
        pg::result_cache().remove(id);
    }

    // Return as JSON with HTML payload
    auto jh = Html::with_capacity(rows_html.size() + 128);
    jh.raw("{\"html\":\"");
    // JSON-escape the HTML
    for (char c : rows_html) {
        switch (c) {
            case '"':  jh.raw("\\\""); break;
            case '\\': jh.raw("\\\\"); break;
            case '\n': jh.raw("\\n"); break;
            case '\r': break;
            case '\t': jh.raw("\\t"); break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    jh.raw("\\u00");
                    jh.raw("0123456789abcdef"[(c >> 4) & 0xf]);
                    jh.raw("0123456789abcdef"[c & 0xf]);
                } else {
                    jh.raw(c);
                }
        }
    }
    jh.raw("\",\"offset\":").raw(std::to_string(offset));
    jh.raw(",\"count\":").raw(std::to_string(end - offset));
    jh.raw(",\"total\":").raw(std::to_string(total));
    jh.raw(",\"done\":").raw(done ? "true" : "false");
    jh.raw("}");

    return Response::json(std::move(jh).finish());
}

// ─── CompletionsHandler ─────────────────────────────────────────────

auto CompletionsHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::json(R"({"schemas":[],"tables":[]})", 500);

    auto data = pg::completion_metadata(conn->get());
    if (!data) return Response::json(R"({"schemas":[],"tables":[]})", 500);

    // JSON is data, not UI — use Html buffer for fast string building
    auto h = Html::with_capacity(32768);
    h.raw("{\"schemas\":[");
    for (std::size_t i = 0; i < data->schemas.size(); ++i) {
        if (i > 0) h.raw(',');
        h.raw("\"").raw(json_escape(data->schemas[i])).raw("\"");
    }
    h.raw("],\"tables\":[");
    for (std::size_t i = 0; i < data->tables.size(); ++i) {
        if (i > 0) h.raw(',');
        auto& t = data->tables[i];
        h.raw("{\"schema\":\"").raw(json_escape(t.schema))
         .raw("\",\"name\":\"").raw(json_escape(t.name))
         .raw("\",\"type\":\"").raw(json_escape(t.type))
         .raw("\",\"columns\":[");
        for (std::size_t j = 0; j < t.columns.size(); ++j) {
            if (j > 0) h.raw(',');
            h.raw("{\"name\":\"").raw(json_escape(t.columns[j].name))
             .raw("\",\"type\":\"").raw(json_escape(t.columns[j].type)).raw("\"}");
        }
        h.raw("]}");
    }
    h.raw("]}");

    return Response::json(std::move(h).finish());
}

// ─── ExplainExecHandler ─────────────────────────────────────────────

auto ExplainExecHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto body = std::string(req.body());

    auto sql = form_value(body, "sql");
    auto analyze_str = form_value(body, "analyze");
    bool analyze = (analyze_str == "true");

    if (sql.empty()) {
        return Response::html(render_to_string<Alert>({"No SQL provided", "warning"}));
    }

    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(render_to_string<Alert>({error_message(conn.error()), "error"}));
    }

    auto start = std::chrono::steady_clock::now();
    auto result = pg::explain_query(conn->get(), sql, analyze);
    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    if (!result) {
        auto h = Html::with_capacity(1024);
        h.raw(R"(<div class="query-error"><strong>Error:</strong> )").text(error_message(result.error())).raw("</div>");
        return Response::html(std::move(h).finish());
    }

    auto h = Html::with_capacity(4096);

    if (analyze) {
        h.raw(std::format(
            R"(<div class="query-info"><span class="rows-badge">Planning: {:.3f} ms</span> <span class="time-badge">Execution: {:.3f} ms</span> <span class="time-badge">Wall: {} ms</span></div>)",
            result->planning_time, result->execution_time, ms
        ));
    } else {
        h.raw(std::format(
            R"(<div class="query-info"><span class="rows-badge">Cost: {:.2f}</span> <span class="time-badge">Wall: {} ms</span></div>)",
            result->total_cost, ms
        ));
    }

    h.raw("<div style=\"padding:var(--sp-4)\"><div class=\"explain-plan\">").text(result->plan_text).raw("</div></div>");

    // ── Index hints: detect Seq Scan on tables with filter conditions ──
    auto& plan = result->plan_text;
    std::vector<std::pair<std::string, std::string>> hints; // {table, filter_col}

    // Parse: "Seq Scan on table_name" + "Filter: (col = ...)" patterns
    std::size_t pos = 0;
    while ((pos = plan.find("Seq Scan on ", pos)) != std::string::npos) {
        pos += 12;
        auto end = plan.find_first_of(" \n(", pos);
        if (end == std::string::npos) break;
        auto table = plan.substr(pos, end - pos);

        // Look for "rows=" to check if it's a large scan
        auto rows_pos = plan.find("rows=", pos);
        auto next_node = plan.find("->", pos);
        long long rows = 0;
        if (rows_pos != std::string::npos && (next_node == std::string::npos || rows_pos < next_node)) {
            try { rows = std::stoll(plan.substr(rows_pos + 5)); } catch (...) {}
        }

        // Look for Filter condition
        auto filter_pos = plan.find("Filter:", pos);
        if (filter_pos != std::string::npos && (next_node == std::string::npos || filter_pos < next_node)) {
            // Extract column name from filter: (col_name = ...)
            auto paren = plan.find('(', filter_pos);
            if (paren != std::string::npos) {
                auto col_end = plan.find_first_of(" =<>!", paren + 1);
                if (col_end != std::string::npos) {
                    auto col = plan.substr(paren + 1, col_end - paren - 1);
                    // Strip quotes and type casts
                    if (!col.empty() && col[0] == '(') col = col.substr(1);
                    auto cast = col.find("::");
                    if (cast != std::string::npos) col = col.substr(0, cast);

                    if (rows > 100 && !col.empty() && col.find(' ') == std::string::npos) {
                        hints.push_back({table, col});
                    }
                }
            }
        } else if (rows > 1000) {
            // Large seq scan without filter — still worth noting
            hints.push_back({table, ""});
        }
    }

    if (!hints.empty()) {
        h.raw("<div class=\"explain-hints\">");
        h.raw("<div class=\"explain-hints-header\">&#128161; Index Suggestions</div>");
        for (auto& [tbl, col] : hints) {
            h.raw("<div class=\"explain-hint-item\">");
            if (col.empty()) {
                h.raw("<span class=\"explain-hint-text\">Sequential scan on <strong>").text(tbl)
                 .raw("</strong> — consider adding an index if this table is queried with WHERE conditions</span>");
            } else {
                auto idx_name = tbl + "_" + col + "_idx";
                auto create_sql = "CREATE INDEX CONCURRENTLY " + idx_name + " ON " + tbl + " (" + col + ");";
                h.raw("<span class=\"explain-hint-text\">Sequential scan on <strong>").text(tbl)
                 .raw("</strong> filtered by <code>").text(col).raw("</code></span>");
                h.raw("<button class=\"btn btn-sm btn-ghost\" onclick=\"navigator.clipboard.writeText('")
                 .raw(create_sql).raw("'); this.textContent='Copied!'\" title=\"").text(create_sql)
                 .raw("\">Copy CREATE INDEX</button>");
            }
            h.raw("</div>");
        }
        h.raw("</div>");
    }

    return Response::html(std::move(h).finish());
}

} // namespace getgresql::api
