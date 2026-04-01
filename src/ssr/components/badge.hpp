#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct Badge {
    struct Props {
        std::string_view text;
        std::string_view variant = "default";
    };

    static constexpr auto css() -> std::string_view { return R"(
.badge {
    display: inline-flex;
    align-items: center;
    padding: 1px 6px;
    border-radius: 3px;
    font-size: var(--font-size-xs);
    font-weight: 600;
    letter-spacing: 0.02em;
    line-height: 1.6;
}
.badge-default { background: var(--bg-4); color: var(--text-1); }
.badge-primary { background: var(--accent-subtle); color: var(--accent); border: 1px solid rgba(56, 139, 253, 0.2); }
.badge-secondary { background: none; color: var(--text-2); font-weight: 400; padding: 0; }
.badge-warning { background: var(--warning-subtle); color: var(--warning); border: 1px solid rgba(210, 153, 34, 0.2); }
.badge-danger { background: var(--danger-subtle); color: var(--danger); border: 1px solid rgba(248, 81, 73, 0.2); }
.badge-success { background: var(--success-subtle); color: var(--success); border: 1px solid rgba(63, 185, 80, 0.2); }
)"; }

    static auto render(const Props& p, Html& h) -> void {
        h.raw("<span class=\"badge badge-").raw(p.variant).raw("\">").text(p.text).raw("</span>");
    }
};

} // namespace getgresql::ssr
