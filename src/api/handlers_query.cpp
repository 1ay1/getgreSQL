#include "api/handlers_query.hpp"
#include "core/expected.hpp"
#include "ssr/components.hpp"
#include "pg/catalog.hpp"

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

    auto result = conn->get().exec(sql);
    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    if (!result) {
        auto h = Html::with_capacity(1024);
        h.raw("<div class=\"query-error\"><strong>Error:</strong> ").text(error_message(result.error())).raw("</div>");
        return Response::html(std::move(h).finish());
    }

    auto h = Html::with_capacity(16384);

    if (result->row_count() > 0 || result->col_count() > 0) {
        // Result info bar
        h.raw("<div class=\"query-info\"><span class=\"rows-badge\">")
         .raw(std::to_string(result->row_count()))
         .raw(" rows</span> <span class=\"time-badge\">")
         .raw(std::to_string(ms))
         .raw(" ms</span></div>");

        // Build column list
        std::vector<Col> cols;
        for (int c = 0; c < result->col_count(); ++c) {
            cols.push_back({result->column_name(c), "", true});
        }
        Table::begin(h, cols, "results-table");

        for (auto row : *result) {
            std::vector<std::string> cells;
            for (int c = 0; c < row.col_count(); ++c) {
                if (row.is_null(c)) {
                    cells.emplace_back("<span class=\"null-value\">NULL</span>");
                } else {
                    auto val = row[c];
                    auto tmp = Html::with_capacity(val.size() + 16);
                    if (val.size() > 500) {
                        tmp.text(val.substr(0, 500)).raw("...");
                    } else {
                        tmp.text(val);
                    }
                    cells.push_back(std::move(tmp).finish());
                }
            }
            Table::row(h, cells);
        }
        Table::end(h);
    } else {
        // Command result (INSERT, UPDATE, etc.)
        h.raw("<div class=\"query-info\"><span class=\"rows-badge\">")
         .text(result->command_tag())
         .raw(" &mdash; ").text(result->affected_rows())
         .raw(" affected</span> <span class=\"time-badge\">")
         .raw(std::to_string(ms)).raw(" ms</span></div>");
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

} // namespace getgresql::api
