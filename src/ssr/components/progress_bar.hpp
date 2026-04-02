#pragma once
#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/css_dsl.hpp"
#include <format>
#include <string>

namespace getgresql::ssr {

struct ProgressBar {
    static constexpr auto scope_name = "progress";

    struct Props { double percent; std::string_view variant = ""; };

    static auto styles() -> css::Stylesheet {
        using namespace css;
        return stylesheet({
            rule("&", { height(px(4)), background(var("bg-4")), border_radius(px(2)), margin_top(var("sp-2")), overflow_hidden() }),
            rule(".bar", { height("100%"), background(var("accent")), transition("width var(--transition-normal)"), border_radius(px(2)) }),
            rule(".bar.success", { background(var("success")) }),
            rule(".bar.warning", { background(var("warning")) }),
            rule(".bar.danger",  { background(var("danger")) }),
        });
    }

    static auto css() -> std::string_view {
        static const auto s = styles().scoped(scope_name);
        return s;
    }

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;
        auto bar_cls = std::string("bar");
        if (!p.variant.empty()) { bar_cls += ' '; bar_cls += p.variant; }
        {
            auto _ = open<Div>(h, {cls(scope_name), role("progressbar"),
                                   aria("valuenow", std::format("{:.0f}", p.percent)),
                                   aria("valuemin", "0"), aria("valuemax", "100")});
            el<Div>(h, {cls(bar_cls), style("width:" + std::format("{:.1f}", p.percent) + "%")});
        }
    }
};

} // namespace getgresql::ssr
