#pragma once
#include "ssr/engine.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr {

struct SectionTab { std::string label; std::string url; bool active = false; };

struct SectionTabs {
    static auto render(std::initializer_list<SectionTab> il, std::string_view target, Html& h) -> void {
        std::vector<SectionTab> tabs(il); render(tabs, target, h);
    }
    static auto render(const std::vector<SectionTab>& tabs, std::string_view target, Html& h) -> void {
        h.raw("<div class=\"section-tabs\" data-target=\"").raw(target).raw("\">");
        for (auto& t : tabs) {
            h.raw("<button class=\"section-tab");
            if (t.active) h.raw(" active");
            h.raw("\"");
            if (!t.url.empty()) h.raw(" data-tab-url=\"").raw(t.url).raw("\"");
            h.raw(">").text(t.label).raw("</button>");
        }
        h.raw("</div>");
    }
};

} // namespace getgresql::ssr
