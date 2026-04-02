#pragma once
#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/css_dsl.hpp"
#include <string>
#include <vector>

namespace getgresql::ssr {

struct Crumb { std::string label; std::string href; };

struct Breadcrumbs {
    static constexpr auto scope_name = "breadcrumbs";

    static auto styles() -> css::Stylesheet {
        using namespace css;
        return stylesheet({
            rule("&", { display_flex(), align_center(), gap(var("sp-1")), margin_bottom(var("sp-4")), font_size(var("font-size-sm")), flex_wrap("wrap") }),
            rule(".sep", { color(var("text-4")), font_size(px(10)), prop("user-select", "none") }),
            rule(".current", { color(var("text-0")), font_weight(600) }),
            rule(".link", { color(var("text-2")), transition("color var(--transition-fast)"), border_radius(var("radius")) }),
            rule(".link:hover", { color(var("accent")), text_decoration("none") }),
            rule(".link:focus-visible", { color(var("accent")), outline("2px solid var(--accent)"), outline_offset("1px") }),
        });
    }

    static auto css() -> std::string_view {
        static const auto s = styles().scoped(scope_name);
        return s;
    }

    static auto render(std::initializer_list<Crumb> il, Html& h) -> void {
        std::vector<Crumb> items(il); render(items, h);
    }

    static auto render(const std::vector<Crumb>& items, Html& h) -> void {
        using namespace html;
        {
            auto _ = open<Nav>(h, {cls(scope_name), aria_label("Breadcrumb")});
            for (std::size_t i = 0; i < items.size(); ++i) {
                if (i > 0) el_raw<Span>(h, {cls("sep"), aria_hidden()}, "&#8250;");
                if (i == items.size() - 1 || items[i].href.empty()) {
                    el<Span>(h, {cls("current")}, items[i].label);
                } else {
                    el<A>(h, {href(items[i].href), cls("link")}, items[i].label);
                }
            }
        }
    }
};

} // namespace getgresql::ssr
