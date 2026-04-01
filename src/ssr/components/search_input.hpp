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
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"search-box\"><input type=\"search\" name=\"q\" placeholder=\"")
         .text(p.placeholder)
         .raw("\" hx-get=\"").raw(p.url)
         .raw("\" hx-trigger=\"input changed delay:300ms, search\" hx-target=\"#")
         .raw(p.target).raw("\" class=\"search-input\"></div>");
    }
};

} // namespace getgresql::ssr
