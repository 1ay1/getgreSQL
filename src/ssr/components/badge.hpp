#pragma once
#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/css_dsl.hpp"
#include <string>
#include <string_view>

namespace getgresql::ssr {

struct Badge {
    static constexpr auto scope_name = "badge";

    struct Props {
        std::string_view text;
        std::string_view variant = "default";
    };

    static auto styles() -> css::Stylesheet {
        using namespace css;
        return stylesheet({
            rule("&", {
                display_inline_flex(), align_center(),
                padding("var(--sp-1) var(--sp-2)"),
                border_radius(var("radius")),
                font_size(var("font-size-xs")),
                font_weight(600),
                letter_spacing(em(0.02)),
                line_height(1.6),
                transition("all var(--transition-fast)"),
            }),
            rule("&.default", { background(var("bg-4")), color(var("text-1")) }),
            rule("&.primary", {
                background(var("accent-subtle")), color(var("accent")),
                border("1px solid var(--accent-border)"),
            }),
            rule("&.secondary", {
                background("none"), color(var("text-2")),
                font_weight(400), padding("0"),
            }),
            rule("&.warning", {
                background(var("warning-subtle")), color(var("warning")),
                border("1px solid var(--warning-border)"),
            }),
            rule("&.danger", {
                background(var("danger-subtle")), color(var("danger")),
                border("1px solid var(--danger-border)"),
            }),
            rule("&.success", {
                background(var("success-subtle")), color(var("success")),
                border("1px solid var(--success-border)"),
            }),
        });
    }

    static auto css() -> std::string_view {
        static const auto s = styles().scoped(scope_name);
        return s;
    }

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;
        el<Span>(h, {cls(html::join(scope_name, p.variant))}, p.text);
    }
};

} // namespace getgresql::ssr
