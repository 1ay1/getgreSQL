#pragma once
#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/css_dsl.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr {

struct Col { std::string_view header; std::string_view cls = ""; bool sortable = false; };

struct Table {
    static constexpr auto scope_name = "table-wrapper";

    static auto styles() -> css::Stylesheet {
        using namespace css;
        return build_stylesheet()
            .rules({
                rule("&", { border("1px solid var(--border-subtle)"), border_radius(var("radius-lg")),
                            overflow_hidden(), box_shadow(var("shadow-sm")) }),
                rule("&.scrollable", { overflow_auto(), max_height("calc(100vh - 300px)") }),
            })
            .raw("table { width: 100%; table-layout: auto; border-collapse: collapse; }\n"
                 "th { position: sticky; top: 0; z-index: 1; background: var(--bg-2); font-weight: 600; "
                 "text-align: left; padding: 5px 8px; font-size: var(--font-size-xs); "
                 "border-bottom: 1px solid var(--border); white-space: nowrap; user-select: none; }\n"
                 "th.sortable { cursor: pointer; }\n"
                 "th.sortable:hover { background: var(--bg-3); }\n"
                 "td { padding: 4px 8px; font-size: var(--font-size-xs); border-bottom: 1px solid var(--border-subtle); "
                 "white-space: nowrap; overflow: hidden; text-overflow: ellipsis; max-width: 400px; }\n"
                 "tbody tr:nth-child(even) td { background: rgba(255,255,255,0.01); }\n"
                 "tbody tr:hover td { background: rgba(56,139,253,0.04); }\n"
                 "tbody tr:last-child td { border-bottom: none; }\n"
                 ".num { text-align: right; font-family: var(--font-mono); font-variant-numeric: tabular-nums; }\n"
                 ".wide { max-width: 500px; word-break: break-all; white-space: normal; }\n"
                 ".stat-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(150px, 1fr)); "
                 "gap: var(--sp-3); margin-bottom: var(--sp-4); }\n")
            .build();
    }

    static auto css() -> std::string_view {
        static const auto s = styles().scoped(scope_name);
        return s;
    }

    // Builder: begin opens tags, end closes them.
    // Uses Html::open/close (non-RAII) for cross-call lifecycle.
    static auto begin(Html& h, std::initializer_list<Col> columns, std::string_view id = "") -> void {
        begin_impl(h, columns, id);
    }
    static auto begin(Html& h, const std::vector<Col>& columns, std::string_view id = "") -> void {
        begin_impl(h, columns, id);
    }

    template<typename Cols>
    static auto begin_impl(Html& h, const Cols& columns, std::string_view id) -> void {
        h.open("div", "class=\"table-wrapper scrollable\"");
        if (id.empty()) {
            h.open("table");
        } else {
            auto attr_str = "id=\"" + std::string(id) + "\"";
            h.open("table", attr_str);
        }
        h.open("thead");
        h.open("tr");
        for (auto& c : columns) {
            using namespace html;
            auto th_cls = std::string(c.cls);
            if (c.sortable) {
                if (!th_cls.empty()) th_cls += ' ';
                th_cls += "sortable";
            }
            if (th_cls.empty()) {
                el<Th>(h, {}, c.header);
            } else {
                el<Th>(h, {cls(th_cls)}, c.header);
            }
        }
        h.close("tr");
        h.close("thead");
        h.open("tbody");
    }

    static auto row(Html& h, std::initializer_list<std::string> cells, std::string_view attrs = "") -> void {
        row_impl(h, cells, attrs);
    }
    static auto row(Html& h, const std::vector<std::string>& cells, std::string_view attrs = "") -> void {
        row_impl(h, cells, attrs);
    }

    template<typename Cells>
    static auto row_impl(Html& h, const Cells& cells, std::string_view attrs) -> void {
        using namespace html;
        if (attrs.empty()) h.open("tr");
        else h.open("tr", attrs);
        for (auto& cell : cells) el_raw<Td>(h, {}, cell);
        h.close("tr");
    }

    static auto end(Html& h) -> void {
        h.close("tbody");
        h.close("table");
        h.close("div");
    }
};

} // namespace getgresql::ssr
