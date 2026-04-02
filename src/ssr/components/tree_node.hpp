#pragma once
#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/js_dsl.hpp"
#include <string>
#include <string_view>

namespace getgresql::ssr {

    inline auto icon_char(std::string_view cls) -> const char* {
        if (cls == "db")       return "&#9673;";
        if (cls == "schema")   return "&#9671;";
        if (cls == "table")    return "&#9776;";
        if (cls == "view")     return "&#9672;";
        if (cls == "func")     return "&#402;";
        if (cls == "seq")      return "#";
        if (cls == "idx")      return "&#9889;";
        if (cls == "folder")   return "&#9656;";
        if (cls == "role")     return "&#9823;";
        if (cls == "ext")      return "&#10070;";
        if (cls == "settings") return "&#9881;";
        if (cls == "monitor")  return "&#9678;";
        return "&#8226;";
    }

struct TreeNode {
    static auto expandable(Html& h, std::string_view icon_class, std::string_view label,
                           std::string_view children_url, int depth, std::string_view badge_text = "") -> void {
        using namespace html;
        auto depth_style = "--tree-depth:" + std::to_string(depth);
        auto guide_style = "--guide-depth:" + std::to_string(depth);
        {
            auto li = open<Li>(h, {cls("tree-item"), role("treeitem"),
                                   aria("expanded", "false"), aria("level", std::to_string(depth + 1))});
            {
                auto row = open<Div>(h, {cls("tree-row"), tabindex("0"),
                                         style(depth_style),
                                         js::on_click(js::raw_js("treeToggle(this)"))});
                el_raw<Span>(h, {cls("tree-chevron"), aria_hidden()}, "&#9656;");
                el_raw<Span>(h, {cls(html::join("tree-icon", icon_class)), aria_hidden()}, icon_char(icon_class));
                el<Span>(h, {cls("tree-text")}, label);
                if (!badge_text.empty()) el<Span>(h, {cls("tree-badge")}, badge_text);
            }
            el<Ul>(h, {cls("tree-children"), role("group"), style(guide_style),
                       hx_get(children_url),
                       hx_trigger("click from:closest .tree-item > .tree-row once"),
                       hx_swap("innerHTML")});
        }
    }

    static auto leaf(Html& h, std::string_view icon_class, std::string_view label,
                     std::string_view href_url, int depth, std::string_view extra = "") -> void {
        using namespace html;
        auto depth_style = "--tree-depth:" + std::to_string(depth);
        {
            auto li = open<Li>(h, {cls("tree-item"), role("treeitem"), aria("level", std::to_string(depth + 1))});
            {
                auto row = open<A>(h, {cls("tree-row"), style(depth_style),
                                       href(href_url), data("spa", "")});
                el_raw<Span>(h, {cls("tree-chevron empty"), aria_hidden()}, "");
                el_raw<Span>(h, {cls(html::join("tree-icon", icon_class)), aria_hidden()}, icon_char(icon_class));
                el<Span>(h, {cls("tree-text")}, label);
                if (!extra.empty()) h.raw(extra);
            }
        }
    }

    static auto separator(Html& h) -> void {
        using namespace html;
        el<Li>(h, {cls("tree-separator")});
    }

    static auto empty_leaf(Html& h, std::string_view message, int depth) -> void {
        using namespace html;
        auto depth_style = "--tree-depth:" + std::to_string(depth);
        {
            auto li = open<Li>(h, {cls("tree-item")});
            {
                auto row = open<Span>(h, {cls("tree-row"), style(depth_style)});
                el_raw<Span>(h, {cls("tree-chevron empty")}, "");
                el<Span>(h, {cls("tree-text tree-empty-text")}, message);
            }
        }
    }
};

} // namespace getgresql::ssr
