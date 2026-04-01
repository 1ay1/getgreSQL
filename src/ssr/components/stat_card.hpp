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

    static constexpr auto css() -> std::string_view { return R"(
.stat-card {
    background: var(--bg-1);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
    padding: var(--sp-4);
    transition: border-color var(--transition-normal), transform var(--transition-normal), box-shadow var(--transition-normal);
    box-shadow: var(--shadow-sm);
}
.stat-card:hover {
    border-color: var(--border);
    transform: translateY(-1px);
    box-shadow: var(--shadow-md);
}
.stat-value {
    font-size: 1.3rem;
    font-weight: 700;
    color: var(--text-0);
    font-family: var(--font-mono);
    line-height: 1.2;
}
.stat-label {
    font-size: var(--font-size-xs);
    color: var(--text-3);
    margin-top: var(--sp-1);
    text-transform: uppercase;
    letter-spacing: 0.03em;
}
.stat-warning .stat-value { color: var(--warning); }
.stat-danger .stat-value { color: var(--danger); }
.stat-accent .stat-value { color: var(--accent); font-size: var(--font-size-md); }
.stat-success .stat-value { color: var(--success); }
)"; }

    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"stat-card");
        if (!p.variant.empty()) h.raw(" stat-").raw(p.variant);
        h.raw("\"><div class=\"stat-value\">").text(p.value).raw("</div>");
        h.raw("<div class=\"stat-label\">").text(p.label).raw("</div></div>\n");
    }
};

} // namespace getgresql::ssr
