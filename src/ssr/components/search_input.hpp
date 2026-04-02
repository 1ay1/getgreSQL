#pragma once
#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/js_dsl.hpp"
#include "ssr/ui.hpp"
#include "ssr/css_dsl.hpp"
#include <string>
#include <string_view>

namespace getgresql::ssr {

struct SearchInput {
    static constexpr auto scope_name = "search-box";

    struct Props {
        std::string_view target;
        std::string_view url;
        std::string_view placeholder = "Search...";
    };

    static auto styles() -> css::Stylesheet {
        using namespace css;
        return stylesheet({
            rule("&", { margin_bottom(var("sp-4")) }),
            rule(".input", {
                width("100%"), max_width(px(360)),
                padding("var(--sp-2) var(--sp-3)"),
                background(var("bg-1")), border("1px solid var(--border-subtle)"),
                border_radius(var("radius")), color(var("text-1")),
                font_size(var("font-size-sm")), font_family(var("font-sans")),
                prop("outline", "none"),
                transition("border-color var(--transition-fast), box-shadow var(--transition-fast)"),
            }),
            rule(".input:focus", {
                border_color(var("accent")),
                box_shadow("0 0 0 2px var(--accent-subtle)"),
            }),
        });
    }

    static auto css() -> std::string_view {
        static const auto s = styles().scoped(scope_name) +
            ".search-box .input::placeholder { color: var(--text-4); }\n";
        return s;
    }

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;
        auto behavior = hx::get(p.url)
            .trigger("input changed delay:300ms, search")
            .target("#" + std::string(p.target));
        {
            auto _ = open<Div>(h, {cls(scope_name)});
            ui::void_el<Input>(h,
                {type("search"), name("q"), placeholder(p.placeholder), cls("input"),
                 aria_label(p.placeholder)},
                behavior);
        }
    }
};

} // namespace getgresql::ssr
