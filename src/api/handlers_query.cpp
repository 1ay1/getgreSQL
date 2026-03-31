#include "api/handlers_query.hpp"
#include "core/expected.hpp"
#include "html/templates.hpp"

#include <chrono>
#include <format>

namespace getgresql::api {

auto QueryPageHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    std::string content = R"(
<div class="query-editor">
    <form hx-post="/query/exec" hx-target="#query-results" hx-swap="innerHTML"
          hx-indicator="#query-spinner">
        <div class="editor-wrapper">
            <textarea name="sql" id="sql-editor" class="sql-input"
                placeholder="SELECT * FROM pg_stat_activity;"
                rows="8" spellcheck="false"></textarea>
        </div>
        <div class="editor-toolbar">
            <button type="submit" class="btn btn-primary">
                Run Query <kbd>Ctrl+Enter</kbd>
            </button>
            <span id="query-spinner" class="htmx-indicator">Running…</span>
        </div>
    </form>
</div>
<div id="query-results" class="query-results">
    <div class="empty-state">Enter a SQL query above and press Run or Ctrl+Enter</div>
</div>
)";

    if (req.is_htmx()) return Response::html(html::partial(std::move(content)));
    return Response::html(html::page("Query", "Query", std::move(content)));
}

auto QueryExecHandler::handle(Request& req, AppContext& ctx) -> Response {
    // Parse the SQL from the form body (application/x-www-form-urlencoded)
    auto body = std::string(req.body());
    std::string sql;

    // Simple form parsing for "sql=..."
    auto pos = body.find("sql=");
    if (pos != std::string::npos) {
        sql = body.substr(pos + 4);
        // URL decode (basic: +→space, %XX→char)
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

    // Measure execution time
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
            std::format(R"(<div class="query-error"><strong>Error:</strong> {}</div>)",
                html::escape(error_message(err)))
        );
    }

    std::string content;

    // If it's a SELECT-like query with rows
    if (result->row_count() > 0 || result->col_count() > 0) {
        content += std::format(
            R"(<div class="query-info">{} rows returned in {} ms</div>)",
            result->row_count(), ms
        );

        std::vector<html::TableColumn> headers;
        for (int c = 0; c < result->col_count(); ++c) {
            headers.push_back({std::string(result->column_name(c)), ""});
        }
        content += html::table_begin(headers, "results-table");

        for (auto row : *result) {
            std::vector<std::string> cells;
            for (int c = 0; c < row.col_count(); ++c) {
                if (row.is_null(c)) {
                    cells.push_back(R"(<span class="null-value">NULL</span>)");
                } else {
                    auto val = row[c];
                    if (val.size() > 500) {
                        cells.push_back(html::escape(val.substr(0, 500)) + "…");
                    } else {
                        cells.push_back(html::escape(val));
                    }
                }
            }
            content += html::table_row(cells);
        }
        content += html::table_end();
    } else {
        // Command (INSERT, UPDATE, DELETE, etc.)
        content += std::format(
            R"(<div class="query-info">{} — {} affected rows — {} ms</div>)",
            html::escape(result->command_tag()),
            html::escape(result->affected_rows()),
            ms
        );
    }

    return Response::html(std::move(content));
}

} // namespace getgresql::api
