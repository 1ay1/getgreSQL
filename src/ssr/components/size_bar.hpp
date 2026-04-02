#pragma once
#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/css_dsl.hpp"
#include <format>
#include <string>

namespace getgresql::ssr {

struct SizeBar {
    static constexpr auto scope_name = "size-bar";

    struct Props { double percent; };

    static auto styles() -> css::Stylesheet {
        using namespace css;
        return stylesheet({
            rule("&", { width(px(100)), height(px(6)), background(var("bg-3")), border_radius(var("radius")), overflow_hidden() }),
            rule(".fill", { height("100%"), background(var("accent")), transition("width var(--transition-normal)"), border_radius(var("radius")), min_width(px(1)) }),
        });
    }

    static auto css() -> std::string_view {
        static const auto s = styles().scoped(scope_name);
        return s;
    }

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;
        {
            auto _ = open<Div>(h, {cls(scope_name), role("progressbar"),
                                   aria("valuenow", std::format("{:.0f}", p.percent)),
                                   aria("valuemin", "0"), aria("valuemax", "100")});
            el<Div>(h, {cls("fill"), style("width:" + std::format("{:.0f}", p.percent) + "%")});
        }
    }
};

} // namespace getgresql::ssr
