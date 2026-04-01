#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeEverforest {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="everforest"] {
    --bg-0: #2d353b;
    --bg-1: #323c41;
    --bg-2: #3a464c;
    --bg-3: #445055;
    --bg-4: #53605c;
    --border: #4a555b;
    --border-subtle: #3a464c;
    --text-0: #d3c6aa;
    --text-1: #c5b99a;
    --text-2: #a7b08c;
    --text-3: #7a8478;
    --text-4: #5d6b66;
    --accent: #7fbbb3;
    --accent-dim: #6ba39c;
    --accent-subtle: rgba(127, 187, 179, 0.1);
    --success: #a7c080;
    --success-subtle: rgba(167, 192, 128, 0.1);
    --warning: #dbbc7f;
    --warning-subtle: rgba(219, 188, 127, 0.1);
    --danger: #e67e80;
    --danger-subtle: rgba(230, 126, 128, 0.1);
    --info: #83c092;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.3);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.3);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.4);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.35);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
