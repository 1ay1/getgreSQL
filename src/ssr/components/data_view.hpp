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

    // Private: only DataView::readonly / DataView::editable can construct
    struct Key {};
    friend struct DataView;

public:
    DataViewScope(Key, Html& h) : h_(h) {}
    ~DataViewScope() { if (!ended_) close(); }

    DataViewScope(DataViewScope&& o) noexcept : h_(o.h_), ended_(o.ended_) { o.ended_ = true; }
    DataViewScope(const DataViewScope&) = delete;
    DataViewScope& operator=(const DataViewScope&) = delete;
    DataViewScope& operator=(DataViewScope&&) = delete;

    // ── columns: renders thead ──────────────────────────────────
    auto columns(const std::vector<DCol>& cols) -> void {
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

    // ── row: read-only row — only available on ReadOnly ─────────
    auto row(const std::vector<Cell>& cells) -> void
        requires std::same_as<Mode, ReadOnly>
    {
        h_.raw("<tr>");
        for (auto& cell : cells) {
            h_.raw("<td>");
            if (cell.is_null) {
                h_.raw("<span class=\"null-value\">NULL</span>");
            } else if (cell.value.size() > 200) {
                h_.raw("<span class=\"dv-cell\">");
                h_.text(std::string_view(cell.value).substr(0, 200));
                h_.raw("&hellip;</span>");
            } else {
                h_.raw("<span class=\"dv-cell\">").text(cell.value).raw("</span>");
            }
            h_.raw("</td>");
        }
        h_.raw("</tr>\n");
    }

    // ── editable_row: only available on Editable ────────────────
    auto editable_row(std::string_view ctid, int row_num,
                      const std::vector<DCol>& cols,
                      const std::vector<Cell>& cells,
                      std::string_view db, std::string_view schema,
                      std::string_view table) -> void
        requires std::same_as<Mode, Editable>
    {
        h_.raw("<tr data-ctid=\"").text(ctid).raw("\">");
        h_.raw("<td class=\"row-num\">").raw(std::to_string(row_num)).raw("</td>");

        for (std::size_t i = 0; i < cells.size(); ++i) {
            auto& cell = cells[i];
            auto col_name = (i < cols.size()) ? cols[i].name : std::string_view("");
            h_.raw("<td>");
            if (cell.is_null) {
                h_.raw("<span class=\"null-value editable-cell\" data-col=\"").text(col_name)
                 .raw("\" data-ctid=\"").text(ctid).raw("\">NULL</span>");
            } else if (cell.value.size() > 80) {
                h_.raw("<span class=\"editable-cell dv-cell-long\" data-col=\"").text(col_name)
                 .raw("\" data-ctid=\"").text(ctid).raw("\" data-full=\"").text(cell.value).raw("\">");
                h_.text(std::string_view(cell.value).substr(0, 60));
                h_.raw("&hellip;</span>");
            } else {
                h_.raw("<span class=\"editable-cell\" data-col=\"").text(col_name)
                 .raw("\" data-ctid=\"").text(ctid).raw("\">").text(cell.value).raw("</span>");
            }
            h_.raw("</td>");
        }

        // Delete button
        h_.raw("<td><button class=\"btn btn-sm btn-danger\" "
              "hx-post=\"/db/").raw(db).raw("/schema/").raw(schema).raw("/table/").raw(table)
         .raw("/delete-row\" hx-vals='{\"ctid\":\"").text(ctid)
         .raw("\"}' hx-target=\"#tab-content\" hx-swap=\"innerHTML\" "
              "hx-confirm=\"Delete this row?\">&#10005;</button></td>");
        h_.raw("</tr>\n");
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
        return DataViewScope<ReadOnly>(DataViewScope<ReadOnly>::Key{}, h);
    }

    // ── Editable factory ────────────────────────────────────────
    static auto editable(Html& h, const EditableOpts& o) -> DataViewScope<Editable> {
        h.raw("<div class=\"data-view\" data-editable=\"true\" data-db=\"").text(o.db)
         .raw("\" data-schema=\"").text(o.schema)
         .raw("\" data-table=\"").text(o.table).raw("\">\n");
        render_info_bar(h, o.row_count, 0, {}, o.page, o.limit, o.total_rows);
        render_toolbar(h, true, o.db, o.schema, o.table, o.page, o.limit, o.total_rows, o.base_url);
        render_insert_form(h, o.db, o.schema, o.table);
        return DataViewScope<Editable>(DataViewScope<Editable>::Key{}, h);
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
                                   std::string_view schema, std::string_view table) -> void {
        h.raw("<div class=\"dv-insert-form\" style=\"display:none\">"
              "<form hx-post=\"/db/").raw(db).raw("/schema/").raw(schema)
         .raw("/table/").raw(table).raw("/insert-row\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">"
              "<div class=\"insert-form-grid\" id=\"dv-insert-fields\"></div>"
              "<div class=\"insert-actions\">"
              "<button type=\"submit\" class=\"btn btn-sm btn-primary\">Insert</button>"
              "<button type=\"button\" class=\"btn btn-sm\" data-dv-action=\"toggle-insert\">Cancel</button>"
              "</div></form></div>\n");
    }
};

} // namespace getgresql::ssr
