#pragma once
#include "ssr/engine.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr {

struct Col { std::string_view header; std::string_view cls = ""; bool sortable = false; };

struct Table {
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
