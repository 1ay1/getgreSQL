#pragma once
#include "ssr/engine.hpp"
#include <format>

namespace getgresql::ssr {

struct ProgressBar {
    struct Props { double percent; std::string_view variant = ""; };
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"progress\"><div class=\"progress-bar");
        if (!p.variant.empty()) h.raw(" progress-").raw(p.variant);
        h.raw("\" style=\"width:").raw(std::format("{:.1f}", p.percent)).raw("%\"></div></div>");
    }
};

} // namespace getgresql::ssr
