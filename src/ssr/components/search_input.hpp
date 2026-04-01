#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct SearchInput {
    struct Props {
        std::string_view target;
        std::string_view url;
        std::string_view placeholder = "Search...";
    };

    static constexpr auto css() -> std::string_view { return R"(
.search-box { margin-bottom: var(--sp-4); }
.search-input {
    width: 100%; max-width: 360px;
    padding: var(--sp-2) var(--sp-3);
    background: var(--bg-1); border: 1px solid var(--border-subtle);
    border-radius: var(--radius); color: var(--text-1);
    font-size: var(--font-size-sm); font-family: var(--font-sans); outline: none;
    transition: border-color var(--transition-fast), box-shadow var(--transition-fast);
}
.search-input:focus { border-color: var(--accent); box-shadow: 0 0 0 2px var(--accent-subtle); }
.search-input::placeholder { color: var(--text-4); }
)"; }

    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"search-box\"><input type=\"search\" name=\"q\" placeholder=\"")
         .text(p.placeholder)
         .raw("\" hx-get=\"").raw(p.url)
         .raw("\" hx-trigger=\"input changed delay:300ms, search\" hx-target=\"#")
         .raw(p.target).raw("\" class=\"search-input\"></div>");
    }
};

} // namespace getgresql::ssr
