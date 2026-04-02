#pragma once
#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/css_dsl.hpp"
#include <string>
#include <string_view>

namespace getgresql::ssr {

struct Alert {
    static constexpr auto scope_name = "alert";

    struct Props {
        std::string_view message;
        std::string_view type = "info";
    };

    static auto styles() -> css::Stylesheet {
        using namespace css;
        return stylesheet({
            rule("&", {
                padding("var(--sp-3) var(--sp-4)"),
                border_radius(var("radius-lg")),
                margin_bottom(var("sp-4")),
                font_size(var("font-size-sm")),
                display_flex(), align_start(), gap(var("sp-3")),
                box_shadow(var("shadow-sm")),
            }),
            // Type variants
            rule("&.info", {
                background(var("accent-subtle")),
                border("1px solid var(--accent-border)"),
                color(var("accent")),
                border_left("3px solid var(--accent)"),
            }),
            rule("&.warning", {
                background(var("warning-subtle")),
                border("1px solid var(--warning-border)"),
                color(var("warning")),
                border_left("3px solid var(--warning)"),
            }),
            rule("&.error", {
                background(var("danger-subtle")),
                border("1px solid var(--danger-border)"),
                color(var("danger")),
                border_left("3px solid var(--danger)"),
            }),
            rule("&.success", {
                background(var("success-subtle")),
                border("1px solid var(--success-border)"),
                color(var("success")),
                border_left("3px solid var(--success)"),
            }),
        });
    }

    static auto css() -> std::string_view {
        static const auto s = styles().scoped(scope_name) +
            ".alert::before { font-size: 1rem; line-height: 1.3; flex-shrink: 0; }\n"
            ".alert.info::before { content: \"\\2139\\FE0F\"; }\n"
            ".alert.warning::before { content: \"\\26A0\\FE0F\"; }\n"
            ".alert.error::before { content: \"\\274C\"; }\n"
            ".alert.success::before { content: \"\\2705\"; }\n";
        return s;
    }

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;
        el<Div>(h, {cls(std::string(scope_name) + " " + std::string(p.type)), role("alert")}, p.message);
    }
};

} // namespace getgresql::ssr
