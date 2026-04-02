#pragma once
#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/css_dsl.hpp"
#include <string>
#include <string_view>

namespace getgresql::ssr {

struct StatCard {
    static constexpr auto scope_name = "stat-card";

    struct Props {
        std::string_view label;
        std::string_view value;
        std::string_view detail = "";
        std::string_view variant = "";
    };

    static auto styles() -> css::Stylesheet {
        using namespace css;
        return stylesheet({
            rule("&", {
                background(var("bg-1")),
                border("1px solid var(--border-subtle)"),
                border_radius(var("radius-lg")),
                padding(var("sp-4")),
                transition("border-color var(--transition-normal), "
                           "transform var(--transition-normal), "
                           "box-shadow var(--transition-normal)"),
                box_shadow(var("shadow-sm")),
            }),
            rule("&:hover", {
                prop("border-color", "var(--border)"),
                transform("translateY(-1px)"),
                box_shadow(var("shadow-md")),
            }),
            rule("&:focus-within", {
                prop("border-color", "var(--accent)"),
                box_shadow("0 0 0 2px var(--accent-subtle)"),
            }),
            rule(".value", {
                font_size(rem(1.3)),
                font_weight(700),
                color(var("text-0")),
                font_family(var("font-mono")),
                line_height(1.2),
            }),
            rule(".label", {
                font_size(var("font-size-xs")),
                color(var("text-3")),
                margin_top(var("sp-1")),
                text_transform("uppercase"),
                letter_spacing(em(0.03)),
            }),
            rule(".detail", {
                font_size(var("font-size-xs")),
                color(var("text-3")),
                font_family(var("font-mono")),
                margin_top(var("sp-2")),
            }),
            rule("&.big", {
                display_flex(), flex_column(), align_center(), text_align_center(),
            }),
            rule("&.big .value", {
                font_size(rem(2.0)), font_weight(800),
                letter_spacing(em(-0.03)),
            }),
            rule("&.big .label", { letter_spacing(em(0.08)) }),
            rule("&.warning .value", { color(var("warning")) }),
            rule("&.danger .value",  { color(var("danger")) }),
            rule("&.accent .value",  { color(var("accent")), font_size(var("font-size-md")) }),
            rule("&.success .value", { color(var("success")) }),
        });
    }

    // Bridge to HasCss concept — generated once, cached forever
    static auto css() -> std::string_view {
        static const auto s = styles().scoped(scope_name);
        return s;
    }

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;

        auto classes = std::string(scope_name);
        if (!p.variant.empty()) { classes += ' '; classes += p.variant; }

        {
            auto _ = open<Div>(h, {cls(classes)});
            el<Div>(h, {cls("value")}, p.value);
            el<Div>(h, {cls("label")}, p.label);
            if (!p.detail.empty()) {
                el<Div>(h, {cls("detail")}, p.detail);
            }
        }
    }
};

} // namespace getgresql::ssr
