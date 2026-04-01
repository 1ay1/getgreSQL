#pragma once

// ─── DataView — Type-State Data Grid Component ─────────────────────
//
// Uses phantom types to make invalid states unrepresentable:
//
//   ReadOnly  — query results: row() only, no editing
//   Editable  — table browse: editable_row() only, with insert/delete
//
// The mode is a compile-time type parameter. Calling editable_row()
// on a ReadOnly view is a compile error. Calling row() on an Editable
// view is a compile error. The options struct differs per mode so you
// can't forget required fields (e.g. db/schema/table for Editable).
//
// The builder uses RAII — destructor closes the HTML tags.
// Move-only semantics prevent double-close or use-after-move.
//
// Usage:
//   // Read-only (query results):
//   {
//       auto view = DataView::readonly(h, {.row_count=42, .exec_ms=10});
//       view.columns(cols);
//       for (...) view.row(cells);
//   } // RAII closes tags
//
//   // Editable (table browse):
//   {
//       auto view = DataView::editable(h, {.row_count=50, .db="mydb", ...});
//       view.columns(cols);
//       for (...) view.editable_row(ctid, row_num, cols, cells);
//   } // RAII closes tags

#include "ssr/engine.hpp"

#include <algorithm>
#include <concepts>
#include <format>
#include <string>
#include <string_view>

namespace getgresql::ssr::detail {
// URL-encode a string for use in query parameters
inline auto url_encode(std::string_view s) -> std::string {
    std::string out;
    out.reserve(s.size() + 16);
    for (unsigned char c : s) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out += static_cast<char>(c);
        } else {
            out += '%';
            out += "0123456789ABCDEF"[c >> 4];
            out += "0123456789ABCDEF"[c & 0xF];
        }
    }
    return out;
}
} // namespace getgresql::ssr::detail
#include <vector>

namespace getgresql::ssr {

// ── Phantom mode tags ───────────────────────────────────────────────

struct ReadOnly {};
struct Editable {};

// ── Mode concept ────────────────────────────────────────────────────

template<typename M>
concept DataViewMode = std::same_as<M, ReadOnly> || std::same_as<M, Editable>;

// ── Options types — separate per mode, so required fields are enforced ──

struct ReadOnlyOpts {
    int row_count = 0;
    int exec_ms = 0;
    std::string_view command_tag;  // non-empty for INSERT/UPDATE/DELETE results
    std::string_view db;           // for htmx edit URLs on query result cells
};

struct EditableOpts {
    int row_count = 0;
    // These are REQUIRED — no defaults, compiler forces you to provide them
    std::string_view db;
    std::string_view schema;
    std::string_view table;
    int page;
    int limit;
    long long total_rows;
    std::string_view base_url;
};

// ── Column descriptor ───────────────────────────────────────────────

struct DCol {
    std::string_view name;
    std::string_view type_hint = "";  // "numeric", "text", "boolean", "json", "date"
    bool sortable = true;
    unsigned int table_oid = 0;       // source table OID (0 = computed expression)
};

// ── Cell value ──────────────────────────────────────────────────────

struct Cell {
    std::string value;
    bool is_null = false;
};

// ── DataViewScope<Mode> — the type-state builder ────────────────────

template<DataViewMode Mode>
class DataViewScope {
    Html& h_;
    bool ended_ = false;
    std::vector<DCol> cols_;         // stored for row rendering (source OIDs)
    std::string_view db_;            // for htmx edit URLs
    std::string_view schema_;        // for insert form (Editable only)
    std::string_view table_;         // for insert form (Editable only)

    // Private: only DataView::readonly / DataView::editable can construct
    struct Key {};
    friend struct DataView;

public:
    DataViewScope(Key, Html& h, std::string_view db = "",
                  std::string_view schema = "", std::string_view table = "")
        : h_(h), db_(db), schema_(schema), table_(table) {}
    ~DataViewScope() { if (!ended_) close(); }

    DataViewScope(DataViewScope&& o) noexcept : h_(o.h_), ended_(o.ended_), cols_(std::move(o.cols_)), db_(o.db_), schema_(o.schema_), table_(o.table_) { o.ended_ = true; }
    DataViewScope(const DataViewScope&) = delete;
    DataViewScope& operator=(const DataViewScope&) = delete;
    DataViewScope& operator=(DataViewScope&&) = delete;

    // ── columns: renders insert form (Editable) + thead ────────
    auto columns(const std::vector<DCol>& cols) -> void {
        cols_ = cols;

        // Editable: render insert form now that we have column names
        if constexpr (std::same_as<Mode, Editable>) {
            h_.raw("<div class=\"dv-insert-form\" style=\"display:none\">"
                  "<form hx-post=\"/db/").raw(db_).raw("/schema/").raw(schema_)
             .raw("/table/").raw(table_).raw("/insert-row\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">"
                  "<div class=\"insert-form-grid\">");
            for (std::size_t ci = 0; ci < cols.size(); ++ci) {
                h_.raw("<div class=\"insert-field\"><label>").text(cols[ci].name)
                 .raw("</label><input type=\"text\" name=\"col_").raw(std::to_string(ci))
                 .raw("\" placeholder=\"").text(cols[ci].name).raw("\" class=\"insert-input\"></div>");
            }
            h_.raw("</div><div class=\"insert-actions\">"
                  "<button type=\"submit\" class=\"btn btn-sm btn-primary\">Insert</button>"
                  "<button type=\"button\" class=\"btn btn-sm\" data-dv-action=\"toggle-insert\">Cancel</button>"
                  "</div></form></div>\n");
        }

        bool with_row_num = std::same_as<Mode, Editable>;
        bool with_actions = std::same_as<Mode, Editable>;

        h_.raw("<div class=\"table-wrapper scrollable\"><table class=\"dv-table\"><thead><tr>");
        if (with_row_num) h_.raw("<th class=\"row-num-header\">#</th>");
        for (auto& c : cols) {
            h_.raw("<th class=\"sortable\" data-col=\"").text(c.name).raw("\"");
            if (!c.type_hint.empty()) h_.raw(" data-type=\"").raw(c.type_hint).raw("\"");
            h_.raw("><span class=\"dv-th-text\">").text(c.name).raw("</span>"
                  "<span class=\"dv-resize-handle\"></span></th>");
        }
        if (with_actions) h_.raw("<th class=\"dv-actions-header\"></th>");
        h_.raw("</tr></thead><tbody>");
    }

    // ── row: unified cell rendering for BOTH modes ──────────────
    // Every cell with a known source (table_oid != 0) + ctid gets
    // htmx double-click-to-edit. Saves go directly to the DB.
    // This is the same for query results AND table browse.
    auto row(const std::vector<Cell>& cells, std::string_view ctid = "",
             int row_num = 0) -> void {
        h_.raw("<tr");
        if (!ctid.empty()) h_.raw(" data-ctid=\"").text(ctid).raw("\"");
        h_.raw(">");

        // Row number column (Editable mode only)
        if constexpr (std::same_as<Mode, Editable>) {
            h_.raw("<td class=\"row-num\">").raw(std::to_string(row_num)).raw("</td>");
        }

        for (std::size_t i = 0; i < cells.size(); ++i) {
            auto& cell = cells[i];
            auto col_name = (i < cols_.size()) ? cols_[i].name : std::string_view("");
            auto has_source = (i < cols_.size() && cols_[i].table_oid != 0);
            bool can_edit = !ctid.empty() && has_source;
            // For Editable mode, all columns are editable (table_oid is the table itself)
            if constexpr (std::same_as<Mode, Editable>) { can_edit = !ctid.empty(); }

            h_.raw("<td>");
            if (can_edit) {
                render_editable_cell(cell, col_name, ctid, i);
            } else {
                render_readonly_cell(cell);
            }
            h_.raw("</td>");
        }

        // Actions column (Editable mode only)
        if constexpr (std::same_as<Mode, Editable>) {
            h_.raw("<td class=\"dv-actions\"><button class=\"btn btn-sm btn-danger\" "
                  "hx-post=\"/db/").raw(db_).raw("/schema/").raw(schema_).raw("/table/").raw(table_)
             .raw("/delete-row\" hx-vals='{\"ctid\":\"").text(ctid)
             .raw("\"}' hx-target=\"#tab-content\" hx-swap=\"innerHTML\" "
                  "hx-confirm=\"Delete this row?\">&#10005;</button></td>");
        }

        h_.raw("</tr>\n");
    }

private:
    // ── Render an editable cell with htmx double-click trigger ──
    auto render_editable_cell(const Cell& cell, std::string_view col_name,
                              std::string_view ctid, std::size_t col_idx) -> void {
        auto oid_str = (col_idx < cols_.size()) ? std::to_string(cols_[col_idx].table_oid) : std::string("0");
        auto val = cell.is_null ? std::string_view("") : std::string_view(cell.value);

        if (cell.is_null) {
            h_.raw("<span class=\"null-value editable-cell\" data-col=\"").text(col_name).raw("\"");
        } else {
            bool is_long = cell.value.size() > 80;
            h_.raw("<span class=\"editable-cell");
            if (is_long) h_.raw(" dv-cell-long");
            h_.raw("\" data-col=\"").text(col_name).raw("\"");
            if (is_long) h_.raw(" data-full=\"").text(cell.value).raw("\"");
        }

        // htmx: double-click → server returns inline edit form
        // URL-encode parameters to handle UTF-8, spaces, special chars
        h_.raw(" hx-get=\"/dv/edit-cell?db=").raw(detail::url_encode(db_))
         .raw("&amp;schema=").raw(detail::url_encode(schema_))
         .raw("&amp;table=").raw(detail::url_encode(table_))
         .raw("&amp;table_oid=").raw(oid_str)
         .raw("&amp;col=").raw(detail::url_encode(col_name))
         .raw("&amp;ctid=").raw(detail::url_encode(ctid))
         .raw("&amp;val=").raw(detail::url_encode(val))
         .raw("\" hx-trigger=\"dblclick\" hx-target=\"closest td\" hx-swap=\"innerHTML\">");

        if (cell.is_null) {
            h_.raw("NULL</span>");
        } else if (cell.value.size() > 80) {
            h_.text(std::string_view(cell.value).substr(0, 60)).raw("&hellip;</span>");
        } else {
            h_.text(cell.value).raw("</span>");
        }
    }

    // ── Render a plain read-only cell (no source table) ─────────
    auto render_readonly_cell(const Cell& cell) -> void {
        if (cell.is_null) {
            h_.raw("<span class=\"null-value dv-cell\">NULL</span>");
        } else if (cell.value.size() > 200) {
            h_.raw("<span class=\"dv-cell dv-cell-long\" data-full=\"").text(cell.value).raw("\">");
            h_.text(std::string_view(cell.value).substr(0, 200)).raw("&hellip;</span>");
        } else {
            h_.raw("<span class=\"dv-cell\">").text(cell.value).raw("</span>");
        }
    }

private:
    auto close() -> void {
        ended_ = true;
        h_.raw("</tbody></table></div>\n"); // close table-wrapper
        h_.raw("</div>\n"); // close data-view
    }
};

// ── DataView factory — constructs the right mode ────────────────────

struct DataView {
    // ── ReadOnly factory ────────────────────────────────────────
    static auto readonly(Html& h, const ReadOnlyOpts& o) -> DataViewScope<ReadOnly> {
        h.raw("<div class=\"data-view\">\n");
        render_info_bar(h, o.row_count, o.exec_ms, o.command_tag);
        render_toolbar(h, false, {}, {}, {}, 0, 0, 0, {});
        return DataViewScope<ReadOnly>(DataViewScope<ReadOnly>::Key{}, h, o.db);
    }

    // ── Editable factory ────────────────────────────────────────
    static auto editable(Html& h, const EditableOpts& o) -> DataViewScope<Editable> {
        h.raw("<div class=\"data-view\" data-editable=\"true\" data-db=\"").text(o.db)
         .raw("\" data-schema=\"").text(o.schema)
         .raw("\" data-table=\"").text(o.table).raw("\">\n");
        render_info_bar(h, o.row_count, 0, {}, o.page, o.limit, o.total_rows);
        render_toolbar(h, true, o.db, o.schema, o.table, o.page, o.limit, o.total_rows, o.base_url);
        // Insert form is rendered in columns() after column names are known
        return DataViewScope<Editable>(DataViewScope<Editable>::Key{}, h, o.db, o.schema, o.table);
    }

private:
    static auto render_info_bar(Html& h, int row_count, int exec_ms,
                                std::string_view command_tag,
                                int page = 0, int limit = 0, long long total_rows = 0) -> void {
        if (command_tag.empty()) {
            h.raw("<div class=\"query-info\"><span class=\"rows-badge\">")
             .raw(std::to_string(row_count)).raw(" rows</span>");
            if (exec_ms > 0) h.raw(" <span class=\"time-badge\">").raw(std::to_string(exec_ms)).raw(" ms</span>");
            if (page > 0) {
                auto total_pages = std::max(1LL, (total_rows + limit - 1) / limit);
                h.raw(" <span class=\"time-badge\">~").raw(std::to_string(total_rows))
                 .raw(" total &middot; page ").raw(std::to_string(page))
                 .raw(" of ~").raw(std::to_string(total_pages)).raw("</span>");
            }
            h.raw("</div>\n");
        } else {
            h.raw("<div class=\"query-info\"><span class=\"rows-badge\">").text(command_tag)
             .raw(" &mdash; ").raw(std::to_string(row_count)).raw(" affected</span>");
            if (exec_ms > 0) h.raw(" <span class=\"time-badge\">").raw(std::to_string(exec_ms)).raw(" ms</span>");
            h.raw("</div>\n");
        }
    }

    static auto render_toolbar(Html& h, bool editable_mode,
                               std::string_view db, std::string_view schema, std::string_view table,
                               int page, int limit, long long total_rows, std::string_view base_url) -> void {
        h.raw("<div class=\"dv-toolbar\">");
        h.raw("<input type=\"search\" class=\"dv-filter-input\" placeholder=\"Filter rows...\" title=\"Type to filter visible rows\">");

        if (page > 0) {
            auto total_pages = std::max(1LL, (total_rows + limit - 1) / limit);
            h.raw("<div class=\"btn-group dv-pagination\">");
            if (page > 1) {
                h.raw("<button class=\"btn btn-sm\" hx-get=\"").raw(base_url)
                 .raw("?page=1&limit=").raw(std::to_string(limit))
                 .raw("\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">&laquo;</button>");
                h.raw("<button class=\"btn btn-sm\" hx-get=\"").raw(base_url)
                 .raw("?page=").raw(std::to_string(page - 1)).raw("&limit=").raw(std::to_string(limit))
                 .raw("\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">&lsaquo; Prev</button>");
            }
            if (page < static_cast<int>(total_pages)) {
                h.raw("<button class=\"btn btn-sm\" hx-get=\"").raw(base_url)
                 .raw("?page=").raw(std::to_string(page + 1)).raw("&limit=").raw(std::to_string(limit))
                 .raw("\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">Next &rsaquo;</button>");
            }
            h.raw("</div>");
            h.raw("<div class=\"btn-group dv-page-size\">");
            for (auto sz : {25, 50, 100, 250}) {
                h.raw("<button class=\"").raw((sz == limit) ? "btn btn-sm btn-primary" : "btn btn-sm")
                 .raw("\" hx-get=\"").raw(base_url).raw("?page=1&limit=").raw(std::to_string(sz))
                 .raw("\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">").raw(std::to_string(sz)).raw("</button>");
            }
            h.raw("</div>");
        }

        h.raw("<div class=\"btn-group dv-export\">"
              "<button class=\"btn btn-sm btn-ghost\" data-dv-export=\"csv\">CSV</button>"
              "<button class=\"btn btn-sm btn-ghost\" data-dv-export=\"json\">JSON</button>"
              "<button class=\"btn btn-sm btn-ghost\" data-dv-export=\"sql\">SQL</button>"
              "</div>");
        h.raw("<button class=\"btn btn-sm btn-ghost\" data-dv-action=\"copy\">Copy</button>");

        if (editable_mode) {
            h.raw("<a href=\"/db/").raw(db).raw("/schema/").raw(schema).raw("/table/").raw(table)
             .raw("/export\" class=\"btn btn-sm btn-ghost\">&#8615; Full CSV</a>");
            h.raw("<button class=\"btn btn-sm btn-success\" data-dv-action=\"toggle-insert\">+ Insert</button>");
        }
        h.raw("</div>\n");
    }

    static auto render_insert_form(Html& h, std::string_view db,
                                   std::string_view schema, std::string_view table,
                                   const std::vector<DCol>& cols) -> void {
        h.raw("<div class=\"dv-insert-form\" style=\"display:none\">"
              "<form hx-post=\"/db/").raw(db).raw("/schema/").raw(schema)
         .raw("/table/").raw(table).raw("/insert-row\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">"
              "<div class=\"insert-form-grid\">");
        for (std::size_t i = 0; i < cols.size(); ++i) {
            h.raw("<div class=\"insert-field\"><label>").text(cols[i].name)
             .raw("</label><input type=\"text\" name=\"col_").raw(std::to_string(i))
             .raw("\" placeholder=\"").text(cols[i].name).raw("\" class=\"insert-input\"></div>");
        }
        h.raw("</div>"
              "<div class=\"insert-actions\">"
              "<button type=\"submit\" class=\"btn btn-sm btn-primary\">Insert</button>"
              "<button type=\"button\" class=\"btn btn-sm\" data-dv-action=\"toggle-insert\">Cancel</button>"
              "</div></form></div>\n");
    }
};

} // namespace getgresql::ssr
