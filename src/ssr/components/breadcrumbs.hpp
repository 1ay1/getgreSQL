#pragma once
#include "ssr/engine.hpp"
#include <string>
#include <vector>

namespace getgresql::ssr {

struct Crumb { std::string label; std::string href; };

struct Breadcrumbs {
    static constexpr auto css() -> std::string_view { return R"(
.breadcrumbs { display: flex; align-items: center; gap: 2px; margin-bottom: var(--sp-4); font-size: var(--font-size-sm); flex-wrap: wrap; }
.bc-sep { color: var(--text-4); font-size: 10px; }
.bc-current { color: var(--text-0); font-weight: 600; }
.bc-link { color: var(--text-2); transition: color var(--transition-fast); }
.bc-link:hover { color: var(--accent); text-decoration: none; }
)"; }

    static auto render(std::initializer_list<Crumb> il, Html& h) -> void {
        std::vector<Crumb> items(il); render(items, h);
    }
    static auto render(const std::vector<Crumb>& items, Html& h) -> void {
        h.raw("<nav class=\"breadcrumbs\">");
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i > 0) h.raw("<span class=\"bc-sep\">&#8250;</span>");
            if (i == items.size() - 1 || items[i].href.empty()) {
                h.raw("<span class=\"bc-current\">").text(items[i].label).raw("</span>");
            } else {
                h.raw("<a href=\"").text(items[i].href).raw("\" class=\"bc-link\">").text(items[i].label).raw("</a>");
            }
        }
        h.raw("</nav>");
    }
};

} // namespace getgresql::ssr
