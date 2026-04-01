#pragma once
#include "ssr/engine.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr {

struct Col { std::string_view header; std::string_view cls = ""; bool sortable = false; };

struct Table {
    static constexpr auto css() -> std::string_view { return R"(
.table-wrapper { border: 1px solid var(--border-subtle); border-radius: var(--radius-lg); overflow: hidden; box-shadow: var(--shadow-sm); }
.table-wrapper.scrollable { overflow: auto; max-height: calc(100vh - 300px); }
table { width: 100%; table-layout: auto; border-collapse: collapse; }
th { position: sticky; top: 0; z-index: 1; background: var(--bg-2); font-weight: 600; text-align: left; padding: 5px 8px; font-size: var(--font-size-xs); border-bottom: 1px solid var(--border); white-space: nowrap; user-select: none; }
th.sortable { cursor: pointer; }
th.sortable:hover { background: var(--bg-3); }
td { padding: 4px 8px; font-size: var(--font-size-xs); border-bottom: 1px solid var(--border-subtle); white-space: nowrap; overflow: hidden; text-overflow: ellipsis; max-width: 400px; }
tbody tr:nth-child(even) td { background: rgba(255,255,255,0.01); }
tbody tr:hover td { background: rgba(56,139,253,0.04); }
tbody tr:last-child td { border-bottom: none; }
.num { text-align: right; font-family: var(--font-mono); font-variant-numeric: tabular-nums; }
.wide { max-width: 500px; word-break: break-all; white-space: normal; }
.stat-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(150px, 1fr)); gap: var(--sp-3); margin-bottom: var(--sp-4); }
)"; }

    static auto begin(Html& h, std::initializer_list<Col> columns, std::string_view id = "") -> void {
        begin_impl(h, columns, id);
    }
    static auto begin(Html& h, const std::vector<Col>& columns, std::string_view id = "") -> void {
        begin_impl(h, columns, id);
    }
    template<typename Cols>
    static auto begin_impl(Html& h, const Cols& columns, std::string_view id) -> void {
        h.raw("<div class=\"table-wrapper scrollable\"><table");
        if (!id.empty()) h.raw(" id=\"").text(id).raw("\"");
        h.raw("><thead><tr>");
        for (auto& c : columns) {
            h.raw("<th");
            if (!c.cls.empty() || c.sortable) {
                h.raw(" class=\"");
                if (!c.cls.empty()) h.raw(c.cls);
                if (c.sortable) { if (!c.cls.empty()) h.raw(' '); h.raw("sortable"); }
                h.raw("\"");
            }
            h.raw(">").text(c.header).raw("</th>");
        }
        h.raw("</tr></thead><tbody>");
    }

    static auto row(Html& h, std::initializer_list<std::string> cells, std::string_view attrs = "") -> void {
        if (attrs.empty()) h.raw("<tr>"); else h.raw("<tr ").raw(attrs).raw(">");
        for (auto& cell : cells) h.raw("<td>").raw(cell).raw("</td>");
        h.raw("</tr>");
    }

    static auto row(Html& h, const std::vector<std::string>& cells, std::string_view attrs = "") -> void {
        if (attrs.empty()) h.raw("<tr>"); else h.raw("<tr ").raw(attrs).raw(">");
        for (auto& cell : cells) h.raw("<td>").raw(cell).raw("</td>");
        h.raw("</tr>");
    }

    static auto end(Html& h) -> void {
        h.raw("</tbody></table></div>");
    }
};

} // namespace getgresql::ssr
