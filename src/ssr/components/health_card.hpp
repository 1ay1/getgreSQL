#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct HealthCard {
    struct Props {
        std::string_view name;
        std::string_view status;
        std::string_view value;
        std::string_view detail;
    };

    static constexpr auto css() -> std::string_view { return R"(
.health-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(220px, 1fr)); gap: var(--sp-3); }
.health-card {
    display: flex; align-items: flex-start; gap: var(--sp-3);
    padding: var(--sp-3) var(--sp-4);
    background: var(--bg-1); border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg); box-shadow: var(--shadow-sm);
    transition: border-color var(--transition-normal), box-shadow var(--transition-normal);
}
.health-card:hover { border-color: var(--border); box-shadow: var(--shadow-md); }
.health-icon { font-size: 1.2rem; flex-shrink: 0; }
.health-success { border-left: 3px solid var(--success); }
.health-success .health-icon { color: var(--success); filter: drop-shadow(0 0 4px rgba(63, 185, 80, 0.4)); }
.health-warning { border-left: 3px solid var(--warning); }
.health-warning .health-icon { color: var(--warning); filter: drop-shadow(0 0 4px rgba(210, 153, 34, 0.4)); }
.health-danger { border-left: 3px solid var(--danger); }
.health-danger .health-icon { color: var(--danger); filter: drop-shadow(0 0 4px rgba(248, 81, 73, 0.4)); }
.health-name { font-size: var(--font-size-xs); color: var(--text-3); text-transform: uppercase; letter-spacing: 0.03em; }
.health-value { font-size: var(--font-size-md); font-weight: 700; color: var(--text-0); font-family: var(--font-mono); }
.health-detail { font-size: var(--font-size-xs); color: var(--text-3); margin-top: var(--sp-1); }
)"; }

    static auto render(const Props& p, Html& h) -> void {
        auto v = (p.status == "ok") ? "success" : (p.status == "warning") ? "warning" : "danger";
        auto icon = (p.status == "ok") ? "&#10003;" : (p.status == "warning") ? "&#9888;" : "&#10007;";
        h.raw("<div class=\"health-card health-").raw(v).raw("\">");
        h.raw("<div class=\"health-icon\">").raw(icon).raw("</div>");
        h.raw("<div class=\"health-info\">");
        h.raw("<div class=\"health-name\">").text(p.name).raw("</div>");
        h.raw("<div class=\"health-value\">").text(p.value).raw("</div>");
        h.raw("<div class=\"health-detail\">").text(p.detail).raw("</div>");
        h.raw("</div></div>");
    }
};

} // namespace getgresql::ssr
