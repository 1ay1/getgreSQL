#pragma once
#include "ssr/engine.hpp"
#include <format>

namespace getgresql::ssr {

struct ProgressBar {
    struct Props { double percent; std::string_view variant = ""; };

    static constexpr auto css() -> std::string_view { return R"(
.progress { height: 4px; background: var(--bg-4); border-radius: 2px; margin-top: var(--sp-2); overflow: hidden; }
.progress-bar { height: 100%; background: var(--accent); transition: width 0.3s ease; border-radius: 2px; }
.progress-success { background: var(--success); }
.progress-warning { background: var(--warning); }
.progress-danger { background: var(--danger); }
)"; }

    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"progress\"><div class=\"progress-bar");
        if (!p.variant.empty()) h.raw(" progress-").raw(p.variant);
        h.raw("\" style=\"width:").raw(std::format("{:.1f}", p.percent)).raw("%\"></div></div>");
    }
};

} // namespace getgresql::ssr
