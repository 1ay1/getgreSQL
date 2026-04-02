#pragma once

// ─── DataView — Type-State Data Grid Component ─────────────────────
//
// Uses phantom types to make invalid states unrepresentable:
//   ReadOnly  — query results: row() only, no editing
//   Editable  — table browse: editable_row() only, with insert/delete
//
// Uses HTML DSL for all element construction. Builder pattern uses
// Html::open/close (non-RAII) because the scope spans multiple calls.

#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"

#include <algorithm>
#include <concepts>
#include <format>
#include <string>
#include <string_view>

namespace getgresql::ssr::detail {
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

template<typename M>
concept DataViewMode = std::same_as<M, ReadOnly> || std::same_as<M, Editable>;

// ── Options ─────────────────────────────────────────────────────────

struct ReadOnlyOpts {
    int row_count = 0;
    int exec_ms = 0;
    std::string_view command_tag;
    std::string_view db;
    std::string_view stream_id;
};

struct EditableOpts {
    int row_count = 0;
    std::string_view db;
    std::string_view schema;
    std::string_view table;
    int page;
    int limit;
    long long total_rows;
    std::string_view base_url;
};

struct DCol {
    std::string_view name;
    std::string_view type_hint = "";
    bool sortable = true;
    unsigned int table_oid = 0;
};

struct Cell {
    std::string value;
    bool is_null = false;
};

// ── DataViewScope<Mode> — the type-state builder ────────────────────

template<DataViewMode Mode>
class DataViewScope {
    Html& h_;
    bool ended_ = false;
    std::vector<DCol> cols_;
    std::string_view db_;
    std::string_view schema_;
    std::string_view table_;

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

    auto columns(const std::vector<DCol>& cols) -> void {
        using namespace html;
        cols_ = cols;

        // Editable: insert form
        if constexpr (std::same_as<Mode, Editable>) {
            auto insert_url = "/db/" + std::string(db_) + "/schema/" + std::string(schema_)
                + "/table/" + std::string(table_) + "/insert-row";
            {
                auto form_wrap = open<Div>(h_, {cls("dv-insert-form"), style("display:none")});
                {
                    auto form = open<Form>(h_, {hx_post(insert_url), hx_target("#tab-content"), hx_swap("innerHTML")});
                    {
                        auto grid = open<Div>(h_, {cls("insert-form-grid")});
                        for (std::size_t ci = 0; ci < cols.size(); ++ci) {
                            auto fld = open<Div>(h_, {cls("insert-field")});
                            el<Label>(h_, {}, cols[ci].name);
                            void_el<Input>(h_, {type("text"), name("col_" + std::to_string(ci)),
                                               placeholder(cols[ci].name), cls("insert-input")});
                        }
                    }
                    {
                        auto actions = open<Div>(h_, {cls("insert-actions")});
                        el<Button>(h_, {type("submit"), cls("btn btn-sm btn-primary")}, "Insert");
                        el<Button>(h_, {type("button"), cls("btn btn-sm"), data("dv-action", "toggle-insert")}, "Cancel");
                    }
                }
            }
        }

        bool with_row_num = std::same_as<Mode, Editable>;
        bool with_actions = std::same_as<Mode, Editable>;

        // Non-RAII open because close() is deferred to destructor
        h_.open("div", "class=\"table-wrapper scrollable\"");
        h_.open("table", "class=\"dv-table\"");
        h_.open("thead");
        h_.open("tr");
        if (with_row_num) el<Th>(h_, {cls("row-num-header")}, "#");
        for (auto& c : cols) {
            auto th_attrs = "class=\"sortable\" data-col=\"" + std::string(c.name) + "\"";
            if (!c.type_hint.empty()) th_attrs += " data-type=\"" + std::string(c.type_hint) + "\"";
            h_.open("th", th_attrs);
            el<Span>(h_, {cls("dv-th-text")}, c.name);
            el<Span>(h_, {cls("dv-resize-handle")});
            h_.close("th");
        }
        if (with_actions) el<Th>(h_, {cls("dv-actions-header")});
        h_.close("tr");
        h_.close("thead");
        h_.open("tbody");
    }

    auto row(const std::vector<Cell>& cells, std::string_view ctid = "",
             int row_num = 0) -> void {
        using namespace html;

        if (ctid.empty()) h_.open("tr");
        else h_.open("tr", "data-ctid=\"" + std::string(ctid) + "\"");

        if constexpr (std::same_as<Mode, Editable>) {
            el_raw<Td>(h_, {cls("row-num")}, std::to_string(row_num));
        }

        for (std::size_t i = 0; i < cells.size(); ++i) {
            auto& cell = cells[i];
            auto col_name = (i < cols_.size()) ? cols_[i].name : std::string_view("");
            auto has_source = (i < cols_.size() && cols_[i].table_oid != 0);
            bool can_edit = !ctid.empty() && has_source;
            if constexpr (std::same_as<Mode, Editable>) { can_edit = !ctid.empty(); }

            h_.open("td");
            render_cell(cell, col_name, ctid, i, can_edit);
            h_.close("td");
        }

        if constexpr (std::same_as<Mode, Editable>) {
            auto delete_url = "/db/" + std::string(db_) + "/schema/" + std::string(schema_)
                + "/table/" + std::string(table_) + "/delete-row";
            {
                auto td = open<Td>(h_, {cls("dv-actions")});
                el_raw<Button>(h_, {
                    cls("btn btn-sm btn-danger"),
                    hx_post(delete_url),
                    hx_vals("{\"ctid\":\"" + std::string(ctid) + "\"}"),
                    hx_target("#tab-content"), hx_swap("innerHTML"),
                    hx_confirm("Delete this row?"),
                }, "&#10005;");
            }
        }

        h_.close("tr");
    }

private:
    auto render_cell(const Cell& cell, std::string_view col_name,
                     std::string_view ctid, std::size_t col_idx, bool can_edit) -> void {
        using namespace html;
        auto oid_str = (col_idx < cols_.size()) ? std::to_string(cols_[col_idx].table_oid) : std::string("0");
        auto val = cell.is_null ? std::string_view("") : std::string_view(cell.value);
        bool is_long = !cell.is_null && cell.value.size() > 80;

        // Build class
        auto span_cls = std::string();
        if (cell.is_null) span_cls += "null-value ";
        span_cls += can_edit ? "editable-cell" : "dv-cell";
        if (is_long) span_cls += " dv-cell-long";

        // Build attributes
        std::string attr_str = "class=\"" + span_cls + "\"";
        if (!col_name.empty()) attr_str += " data-col=\"" + std::string(col_name) + "\"";
        if (oid_str != "0") attr_str += " data-table-oid=\"" + oid_str + "\"";
        if (is_long) {
            // data-full needs escaping — use text() approach
            auto escaped_val = Html::with_capacity(cell.value.size() + 16);
            escaped_val.text(cell.value);
            attr_str += " data-full=\"" + std::move(escaped_val).finish() + "\"";
        }

        if (can_edit && !ctid.empty()) {
            attr_str += " hx-get=\"/dv/edit-cell?db=" + detail::url_encode(db_)
                + "&amp;schema=" + detail::url_encode(schema_)
                + "&amp;table=" + detail::url_encode(table_)
                + "&amp;table_oid=" + oid_str
                + "&amp;col=" + detail::url_encode(col_name)
                + "&amp;ctid=" + detail::url_encode(ctid)
                + "&amp;val=" + detail::url_encode(val)
                + "\" hx-trigger=\"dblclick\" hx-target=\"closest td\" hx-swap=\"innerHTML\"";
        }

        h_.open("span", attr_str);
        if (cell.is_null) {
            h_.raw("NULL");
        } else if (is_long) {
            h_.text(std::string_view(cell.value).substr(0, 60));
            h_.raw("&hellip;");
        } else {
            h_.text(cell.value);
        }
        h_.close("span");
    }

    auto close() -> void {
        ended_ = true;
        h_.close("tbody");
        h_.close("table");
        h_.close("div"); // table-wrapper
        h_.close("div"); // data-view
    }
};

// ── DataView factory ────────────────────────────────────────────────

struct DataView {
    static auto readonly(Html& h, const ReadOnlyOpts& o) -> DataViewScope<ReadOnly> {
        using namespace html;
        auto dv_attrs = std::string("class=\"data-view\"");
        if (!o.stream_id.empty()) {
            dv_attrs += " data-stream-id=\"" + std::string(o.stream_id) + "\"";
            dv_attrs += " data-stream-total=\"" + std::to_string(o.row_count) + "\"";
        }
        h.open("div", dv_attrs);
        render_info_bar(h, o.row_count, o.exec_ms, o.command_tag);
        render_toolbar(h, false, {}, {}, {}, 0, 0, 0, {});
        return DataViewScope<ReadOnly>(DataViewScope<ReadOnly>::Key{}, h, o.db);
    }

    static auto editable(Html& h, const EditableOpts& o) -> DataViewScope<Editable> {
        using namespace html;
        auto dv_attrs = "class=\"data-view\" data-editable=\"true\" data-db=\""
            + std::string(o.db) + "\" data-schema=\"" + std::string(o.schema)
            + "\" data-table=\"" + std::string(o.table) + "\"";
        h.open("div", dv_attrs);
        render_info_bar(h, o.row_count, 0, {}, o.page, o.limit, o.total_rows);
        render_toolbar(h, true, o.db, o.schema, o.table, o.page, o.limit, o.total_rows, o.base_url);
        return DataViewScope<Editable>(DataViewScope<Editable>::Key{}, h, o.db, o.schema, o.table);
    }

private:
    static auto render_info_bar(Html& h, int row_count, int exec_ms,
                                std::string_view command_tag,
                                int page = 0, int limit = 0, long long total_rows = 0) -> void {
        using namespace html;
        {
            auto info = open<Div>(h, {cls("query-info")});
            if (command_tag.empty()) {
                el<Span>(h, {cls("rows-badge")}, std::to_string(row_count) + " rows");
                if (exec_ms > 0) el<Span>(h, {cls("time-badge")}, std::to_string(exec_ms) + " ms");
                if (page > 0) {
                    auto total_pages = std::max(1LL, (total_rows + limit - 1) / limit);
                    el_raw<Span>(h, {cls("time-badge")},
                        "~" + std::to_string(total_rows) + " total &middot; page "
                        + std::to_string(page) + " of ~" + std::to_string(total_pages));
                }
            } else {
                {
                    auto badge = open<Span>(h, {cls("rows-badge")});
                    h.text(command_tag);
                    h.raw(" &mdash; ");
                    h.raw(std::to_string(row_count));
                    h.raw(" affected");
                }
                if (exec_ms > 0) el<Span>(h, {cls("time-badge")}, std::to_string(exec_ms) + " ms");
            }
        }
    }

    static auto render_toolbar(Html& h, bool editable_mode,
                               std::string_view db, std::string_view schema, std::string_view table,
                               int page, int limit, long long total_rows, std::string_view base_url) -> void {
        using namespace html;
        {
            auto tb = open<Div>(h, {cls("dv-toolbar")});
            void_el<Input>(h, {type("search"), cls("dv-filter-input"),
                              placeholder("Filter rows..."), title("Type to filter visible rows")});

            if (page > 0) {
                auto total_pages = std::max(1LL, (total_rows + limit - 1) / limit);
                {
                    auto pg = open<Div>(h, {cls("btn-group dv-pagination")});
                    if (page > 1) {
                        el_raw<Button>(h, {cls("btn btn-sm"),
                            hx_get(std::string(base_url) + "?page=1&limit=" + std::to_string(limit)),
                            hx_target("#tab-content"), hx_swap("innerHTML")}, "&laquo;");
                        el_raw<Button>(h, {cls("btn btn-sm"),
                            hx_get(std::string(base_url) + "?page=" + std::to_string(page - 1) + "&limit=" + std::to_string(limit)),
                            hx_target("#tab-content"), hx_swap("innerHTML")}, "&lsaquo; Prev");
                    }
                    if (page < static_cast<int>(total_pages)) {
                        el_raw<Button>(h, {cls("btn btn-sm"),
                            hx_get(std::string(base_url) + "?page=" + std::to_string(page + 1) + "&limit=" + std::to_string(limit)),
                            hx_target("#tab-content"), hx_swap("innerHTML")}, "Next &rsaquo;");
                    }
                }
                {
                    auto ps = open<Div>(h, {cls("btn-group dv-page-size")});
                    for (auto sz : {25, 50, 100, 250}) {
                        el<Button>(h, {
                            cls(sz == limit ? "btn btn-sm btn-primary" : "btn btn-sm"),
                            hx_get(std::string(base_url) + "?page=1&limit=" + std::to_string(sz)),
                            hx_target("#tab-content"), hx_swap("innerHTML"),
                        }, std::to_string(sz));
                    }
                }
            }

            {
                auto exp = open<Div>(h, {cls("btn-group dv-export")});
                el<Button>(h, {cls("btn btn-sm btn-ghost"), data("dv-export", "csv")}, "CSV");
                el<Button>(h, {cls("btn btn-sm btn-ghost"), data("dv-export", "json")}, "JSON");
                el<Button>(h, {cls("btn btn-sm btn-ghost"), data("dv-export", "sql")}, "SQL");
            }
            el<Button>(h, {cls("btn btn-sm btn-ghost"), data("dv-action", "copy")}, "Copy");

            if (editable_mode) {
                auto export_url = "/db/" + std::string(db) + "/schema/" + std::string(schema)
                    + "/table/" + std::string(table) + "/export";
                el_raw<A>(h, {href(export_url), cls("btn btn-sm btn-ghost")}, "&#8615; Full CSV");
                el<Button>(h, {cls("btn btn-sm btn-success"), data("dv-action", "toggle-insert")}, "+ Insert");
            }
        }
    }
};

} // namespace getgresql::ssr
