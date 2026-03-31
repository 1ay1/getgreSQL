#include "api/handlers_query.hpp"
#include "core/expected.hpp"
#include "html/templates.hpp"
#include "pg/catalog.hpp"

#include <chrono>
#include <format>

namespace getgresql::api {

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

// ─── QueryPageHandler ───────────────────────────────────────────────

auto QueryPageHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    // The JS editor (editor.js) takes over this container via SQLEditor.init()
    std::string content = R"(<div id="query-workspace" class="query-panel"></div>)";

    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::ide_page_full("Query", "Query", std::move(content)));
}

// ─── QueryExecHandler ───────────────────────────────────────────────

auto QueryExecHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto body = std::string(req.body());
    std::string sql;

    auto pos = body.find("sql=");
    if (pos != std::string::npos) {
        sql = body.substr(pos + 4);
        std::string decoded;
        decoded.reserve(sql.size());
        for (std::size_t i = 0; i < sql.size(); ++i) {
            if (sql[i] == '+') {
                decoded += ' ';
            } else if (sql[i] == '%' && i + 2 < sql.size()) {
                auto hex = sql.substr(i + 1, 2);
                decoded += static_cast<char>(std::stoi(hex, nullptr, 16));
                i += 2;
            } else {
                decoded += sql[i];
            }
        }
        sql = std::move(decoded);
    }

    if (sql.empty()) {
        return Response::html(html::alert("No SQL provided", "warning"));
    }

    auto start = std::chrono::steady_clock::now();

    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::html(html::alert(error_message(conn.error()), "error"));
    }

    auto result = conn->get().exec(sql);
    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    if (!result) {
        auto& err = result.error();
        return Response::html(
            std::format("<div class=\"query-error\"><strong>Error:</strong> {}</div>",
                html::escape(error_message(err)))
        );
    }

    std::string content;

    if (result->row_count() > 0 || result->col_count() > 0) {
        content += std::format(
            "<div class=\"query-info\"><span class=\"rows-badge\">{} rows</span> <span class=\"time-badge\">{} ms</span></div>",
            result->row_count(), ms
        );

        std::vector<html::TableColumn> headers;
        for (int c = 0; c < result->col_count(); ++c) {
            headers.push_back({std::string(result->column_name(c)), "", true});
        }
        content += html::table_begin(headers, "results-table");

        for (auto row : *result) {
            std::vector<std::string> cells;
            for (int c = 0; c < row.col_count(); ++c) {
                if (row.is_null(c)) {
                    cells.push_back("<span class=\"null-value\">NULL</span>");
                } else {
                    auto val = row[c];
                    if (val.size() > 500) {
                        cells.push_back(html::escape(val.substr(0, 500)) + "...");
                    } else {
                        cells.push_back(html::escape(val));
                    }
                }
            }
            content += html::table_row(cells);
        }
        content += html::table_end();
    } else {
        content += std::format(
            "<div class=\"query-info\"><span class=\"rows-badge\">{} &mdash; {} affected</span> <span class=\"time-badge\">{} ms</span></div>",
            html::escape(result->command_tag()),
            html::escape(result->affected_rows()),
            ms
        );
    }

    return Response::html(std::move(content));
}

// ─── CompletionsHandler ─────────────────────────────────────────────

auto CompletionsHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) {
        return Response::json(R"({"schemas":[],"tables":[]})", 500);
    }

    auto data = pg::completion_metadata(conn->get());
    if (!data) {
        return Response::json(R"({"schemas":[],"tables":[]})", 500);
    }

    // Build JSON manually
    std::string json = "{\"schemas\":[";
    for (std::size_t i = 0; i < data->schemas.size(); ++i) {
        if (i > 0) json += ',';
        json += '"' + json_escape(data->schemas[i]) + '"';
    }
    json += "],\"tables\":[";

    for (std::size_t i = 0; i < data->tables.size(); ++i) {
        if (i > 0) json += ',';
        auto& t = data->tables[i];
        json += "{\"schema\":\"" + json_escape(t.schema) + "\",";
        json += "\"name\":\"" + json_escape(t.name) + "\",";
        json += "\"type\":\"" + json_escape(t.type) + "\",";
        json += "\"columns\":[";
        for (std::size_t j = 0; j < t.columns.size(); ++j) {
            if (j > 0) json += ',';
            json += "{\"name\":\"" + json_escape(t.columns[j].name) + "\",";
            json += "\"type\":\"" + json_escape(t.columns[j].type) + "\"}";
        }
        json += "]}";
    }
    json += "]}";

    return Response::json(std::move(json));
}

} // namespace getgresql::api
