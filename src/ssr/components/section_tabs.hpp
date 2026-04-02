#pragma once
#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/css_dsl.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr {

struct SectionTab { std::string label; std::string url; bool active = false; };

struct SectionTabs {
    static constexpr auto scope_name = "section-tabs";

    static auto styles() -> css::Stylesheet {
        using namespace css;
        return stylesheet({
            rule("&", { display_flex(), gap("0"), border_bottom("1px solid var(--border)"), margin_bottom(var("sp-4")) }),
            rule(".tab", {
                padding("var(--sp-2) var(--sp-4)"),
                background("none"), border("none"),
                prop("border-bottom", "2px solid transparent"),
                color(var("text-2")), cursor("pointer"),
                font_size(var("font-size-sm")), font_family(var("font-sans")),
                transition("all var(--transition-fast)"),
            }),
            rule(".tab:hover", { color(var("text-0")) }),
            rule(".tab:focus-visible", { color(var("text-0")), outline("2px solid var(--accent)"), outline_offset("-2px") }),
            rule(".tab.active", {
                color(var("accent")),
                prop("border-bottom", "3px solid var(--accent)"),
                font_weight(600),
            }),
        });
    }

    static auto css() -> std::string_view {
        static const auto s = styles().scoped(scope_name);
        return s;
    }

    static auto render(std::initializer_list<SectionTab> il, std::string_view target, Html& h) -> void {
        std::vector<SectionTab> tabs(il); render(tabs, target, h);
    }

    static auto render(const std::vector<SectionTab>& tabs, std::string_view target, Html& h) -> void {
        using namespace html;
        {
            auto _ = open<Div>(h, {cls(scope_name), role("tablist"), data("target", target)});
            for (auto& t : tabs) {
                auto tab_cls = std::string("tab section-tab");
                if (t.active) tab_cls += " active";
                if (!t.url.empty()) {
                    el<Button>(h, {cls(tab_cls), role("tab"),
                                   aria("selected", t.active ? "true" : "false"),
                                   data("tab-url", t.url)}, t.label);
                } else {
                    el<Button>(h, {cls(tab_cls), role("tab"),
                                   aria("selected", t.active ? "true" : "false")}, t.label);
                }
            }
        }
    }
};

} // namespace getgresql::ssr
