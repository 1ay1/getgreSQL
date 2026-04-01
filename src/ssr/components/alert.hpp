#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct Alert {
    struct Props {
        std::string_view message;
        std::string_view type = "info";
    };

    static constexpr auto css() -> std::string_view { return R"(
.alert {
    padding: var(--sp-3) var(--sp-4);
    border-radius: var(--radius-lg);
    margin-bottom: var(--sp-4);
    font-size: var(--font-size-sm);
    display: flex;
    align-items: flex-start;
    gap: var(--sp-3);
    box-shadow: var(--shadow-sm);
}
.alert::before {
    font-size: 1rem;
    line-height: 1.3;
    flex-shrink: 0;
}
.alert-info {
    background: var(--accent-subtle);
    border: 1px solid rgba(56, 139, 253, 0.2);
    color: var(--accent);
    border-left: 3px solid var(--accent);
}
.alert-info::before { content: "\2139\FE0F"; }
.alert-warning {
    background: var(--warning-subtle);
    border: 1px solid rgba(210, 153, 34, 0.2);
    color: var(--warning);
    border-left: 3px solid var(--warning);
}
.alert-warning::before { content: "\26A0\FE0F"; }
.alert-error {
    background: var(--danger-subtle);
    border: 1px solid rgba(248, 81, 73, 0.2);
    color: var(--danger);
    border-left: 3px solid var(--danger);
}
.alert-error::before { content: "\274C"; }
)"; }

    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"alert alert-").raw(p.type).raw("\">").text(p.message).raw("</div>");
    }
};

} // namespace getgresql::ssr
