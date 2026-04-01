#pragma once
#include "ssr/engine.hpp"
#include <string>
#include <string_view>

namespace getgresql::ssr {

struct TreeNode {
    static auto expandable(Html& h, std::string_view icon_class, std::string_view label,
                           std::string_view children_url, int depth, std::string_view badge_text = "") -> void {
        h.raw("<li class=\"tree-item\"><div class=\"tree-row\" style=\"--tree-depth:")
         .raw(std::to_string(depth)).raw("\" onclick=\"treeToggle(this)\">");
        h.raw("<span class=\"tree-chevron\">&#9656;</span>");
        h.raw("<span class=\"tree-icon ").raw(icon_class).raw("\">&#9679;</span>");
        h.raw("<span class=\"tree-text\">").text(label).raw("</span>");
        if (!badge_text.empty()) h.raw("<span class=\"tree-badge\">").text(badge_text).raw("</span>");
        h.raw("</div><ul class=\"tree-children\" hx-get=\"").raw(children_url)
         .raw("\" hx-trigger=\"click from:closest .tree-item > .tree-row once\" hx-swap=\"innerHTML\"></ul></li>\n");
    }

    static auto leaf(Html& h, std::string_view icon_class, std::string_view label,
                     std::string_view href, int depth, std::string_view extra = "") -> void {
        h.raw("<li class=\"tree-item\"><a class=\"tree-row\" style=\"--tree-depth:")
         .raw(std::to_string(depth)).raw("\" href=\"").raw(href).raw("\" data-spa>");
        h.raw("<span class=\"tree-chevron empty\"></span>");
        h.raw("<span class=\"tree-icon ").raw(icon_class).raw("\">&#9679;</span>");
        h.raw("<span class=\"tree-text\">").text(label).raw("</span>");
        if (!extra.empty()) h.raw(extra);
        h.raw("</a></li>\n");
    }

    static auto separator(Html& h) -> void {
        h.raw("<li class=\"tree-separator\"></li>\n");
    }

    static auto empty_leaf(Html& h, std::string_view message, int depth) -> void {
        h.raw("<li class=\"tree-item\"><span class=\"tree-row\" style=\"--tree-depth:")
         .raw(std::to_string(depth)).raw("\"><span class=\"tree-chevron empty\"></span>")
         .raw("<span class=\"tree-text\" style=\"color:var(--text-4);font-style:italic\">")
         .text(message).raw("</span></span></li>");
    }
};

} // namespace getgresql::ssr
