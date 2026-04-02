#pragma once
#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/css_dsl.hpp"
#include <string>
#include <string_view>

namespace getgresql::ssr {

struct HealthCard {
    static constexpr auto scope_name = "health-card";

    struct Props {
        std::string_view name;
        std::string_view status;
        std::string_view value;
        std::string_view detail;
    };

    static auto styles() -> css::Stylesheet {
        using namespace css;
        return stylesheet({
            rule("&", {
                display_flex(), align_start(), gap(var("sp-3")),
                padding("var(--sp-3) var(--sp-4)"),
                background(var("bg-1")), border("1px solid var(--border-subtle)"),
                border_radius(var("radius-lg")), box_shadow(var("shadow-sm")),
                transition("border-color var(--transition-normal), box-shadow var(--transition-normal)"),
            }),
            rule("&:hover", { prop("border-color", "var(--border)"), box_shadow(var("shadow-md")) }),
            rule(".icon", { font_size(rem(1.2)), flex_shrink(0) }),
            rule("&.success", { border_left("3px solid var(--success)") }),
            rule("&.success .icon", { color(var("success")), prop("filter", "drop-shadow(0 0 4px var(--success-border))") }),
            rule("&.warning", { border_left("3px solid var(--warning)") }),
            rule("&.warning .icon", { color(var("warning")), prop("filter", "drop-shadow(0 0 4px var(--warning-border))") }),
            rule("&.danger", { border_left("3px solid var(--danger)") }),
            rule("&.danger .icon", { color(var("danger")), prop("filter", "drop-shadow(0 0 4px var(--danger-border))") }),
            rule(".name", { font_size(var("font-size-xs")), color(var("text-3")), text_transform("uppercase"), letter_spacing(em(0.03)) }),
            rule(".value", { font_size(var("font-size-md")), font_weight(700), color(var("text-0")), font_family(var("font-mono")) }),
            rule(".detail", { font_size(var("font-size-xs")), color(var("text-3")), margin_top(var("sp-1")) }),
        });
    }

    static auto css() -> std::string_view {
        static const auto s = styles().scoped(scope_name) +
            ".health-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(220px, 1fr)); gap: var(--sp-3); }\n";
        return s;
    }

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;
        auto v = (p.status == "ok") ? "success" : (p.status == "warning") ? "warning" : "danger";
        auto icon = (p.status == "ok") ? "&#10003;" : (p.status == "warning") ? "&#9888;" : "&#10007;";
        {
            auto _ = open<Div>(h, {cls(join(scope_name, v))});
            el_raw<Div>(h, {cls("icon")}, icon);
            {
                auto _info = open<Div>(h, {cls("info")});
                el<Div>(h, {cls("name")}, p.name);
                el<Div>(h, {cls("value")}, p.value);
                el<Div>(h, {cls("detail")}, p.detail);
            }
        }
    }
};

} // namespace getgresql::ssr
