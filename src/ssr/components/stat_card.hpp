#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct StatCard {
    struct Props {
        std::string_view label;
        std::string_view value;
        std::string_view variant = "";
    };
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"stat-card");
        if (!p.variant.empty()) h.raw(" stat-").raw(p.variant);
        h.raw("\"><div class=\"stat-value\">").text(p.value).raw("</div>");
        h.raw("<div class=\"stat-label\">").text(p.label).raw("</div></div>\n");
    }
};

} // namespace getgresql::ssr
