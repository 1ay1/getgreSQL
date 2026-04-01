#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeRosePine {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="rose-pine"] {
    --bg-0: #191724;
    --bg-1: #1e1c2e;
    --bg-2: #26233a;
    --bg-3: #2a2740;
    --bg-4: #393552;
    --border: #393552;
    --border-subtle: #26233a;
    --text-0: #e0def4;
    --text-1: #d1cfe2;
    --text-2: #908caa;
    --text-3: #6e6a86;
    --text-4: #555169;
    --accent: #c4a7e7;
    --accent-dim: #b08ed6;
    --accent-subtle: rgba(196, 167, 231, 0.1);
    --success: #9ccfd8;
    --success-subtle: rgba(156, 207, 216, 0.1);
    --warning: #f6c177;
    --warning-subtle: rgba(246, 193, 119, 0.1);
    --danger: #eb6f92;
    --danger-subtle: rgba(235, 111, 146, 0.1);
    --info: #9ccfd8;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.4);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.4);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.5);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.45);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
