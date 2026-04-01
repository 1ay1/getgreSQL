#pragma once
#include "ssr/engine.hpp"
#include <format>

namespace getgresql::ssr {

struct SizeBar {
    struct Props { double percent; };

    static constexpr auto css() -> std::string_view { return R"(
.size-bar { width: 100px; height: 6px; background: var(--bg-3); border-radius: 3px; overflow: hidden; }
.size-bar-fill { height: 100%; background: var(--accent); transition: width 0.3s ease; border-radius: 3px; min-width: 1px; }
)"; }

    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"size-bar\"><div class=\"size-bar-fill\" style=\"width:")
         .raw(std::format("{:.0f}", p.percent)).raw("%\"></div></div>");
    }
};

} // namespace getgresql::ssr
