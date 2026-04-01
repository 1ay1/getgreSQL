#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct Badge {
    struct Props {
        std::string_view text;
        std::string_view variant = "default";
    };
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<span class=\"badge badge-").raw(p.variant).raw("\">").text(p.text).raw("</span>");
    }
};

} // namespace getgresql::ssr
