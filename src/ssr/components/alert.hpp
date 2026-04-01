#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct Alert {
    struct Props {
        std::string_view message;
        std::string_view type = "info";
    };
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"alert alert-").raw(p.type).raw("\">").text(p.message).raw("</div>");
    }
};

} // namespace getgresql::ssr
