#include "api/handlers_query.hpp"
#include "core/expected.hpp"
#include "ssr/components.hpp"
#include "pg/catalog.hpp"
#include "pg/monitor.hpp"

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

    auto h = Html::with_capacity(16384);

    if (result->row_count() > 0 || result->col_count() > 0) {
        int col_start = has_ctid ? 1 : 0;  // skip ctid column in output

        // Build columns with source table OIDs from PQftable
        std::vector<DCol> cols;
        for (int c = col_start; c < result->col_count(); ++c) {
            cols.push_back({
                .name = result->column_name(c),
                .table_oid = result->column_table_oid(c),
            });
        }

        auto view = DataView::readonly(h, {
            .row_count = result->row_count(),
            .exec_ms = static_cast<int>(ms),
            .db = "postgres",  // TODO: get from context
        });
        view.columns(cols);

        for (auto row : *result) {
            // Get ctid for this row (column 0 if has_ctid)
            auto ctid = has_ctid ? std::string_view(row[0]) : std::string_view("");

            std::vector<Cell> cells;
            for (int c = col_start; c < row.col_count(); ++c) {
                if (row.is_null(c)) {
                    cells.push_back({.is_null = true});
                } else {
                    cells.push_back({.value = std::string(row[c])});
                }
            }
            view.row(cells, ctid);
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

    return Response::html(std::move(h).finish());
}

} // namespace getgresql::api
