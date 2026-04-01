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
