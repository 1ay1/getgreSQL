#pragma once
#include "ssr/engine.hpp"
#include <format>

namespace getgresql::ssr {

struct SizeBar {
    struct Props { double percent; };
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"size-bar\"><div class=\"size-bar-fill\" style=\"width:")
         .raw(std::format("{:.0f}", p.percent)).raw("%\"></div></div>");
    }
};

} // namespace getgresql::ssr
