#pragma once
#include "ssr/engine.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr {

struct SectionTab { std::string label; std::string url; bool active = false; };

struct SectionTabs {
    static constexpr auto css() -> std::string_view { return R"(
.section-tabs {
    display: flex;
    gap: 0;
    border-bottom: 1px solid var(--border);
    margin-bottom: var(--sp-4);
}
.section-tab {
    padding: var(--sp-2) var(--sp-4);
    background: none;
    border: none;
    border-bottom: 2px solid transparent;
    color: var(--text-2);
    cursor: pointer;
    font-size: var(--font-size-sm);
    font-family: var(--font-sans);
    transition: all var(--transition-fast);
}
.section-tab:hover { color: var(--text-0); }
.section-tab.active {
    color: var(--accent);
    border-bottom: 3px solid var(--accent);
    font-weight: 600;
    text-shadow: 0 0 12px rgba(56, 139, 253, 0.3);
}
)"; }

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
