#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeAyuDark {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="ayu-dark"] {
    --bg-0: #0b0e14;
    --bg-1: #0f131a;
    --bg-2: #151920;
    --bg-3: #1c2028;
    --bg-4: #2a2e38;
    --border: #1c2028;
    --border-subtle: #151920;
    --text-0: #bfbdb6;
    --text-1: #acaaa3;
    --text-2: #8b8983;
    --text-3: #636260;
    --text-4: #4c4b48;
    --accent: #e6b450;
    --accent-dim: #c99a30;
    --accent-subtle: rgba(230, 180, 80, 0.1);
    --success: #7fd962;
    --success-subtle: rgba(127, 217, 98, 0.1);
    --warning: #ffb454;
    --warning-subtle: rgba(255, 180, 84, 0.1);
    --danger: #d95757;
    --danger-subtle: rgba(217, 87, 87, 0.1);
    --info: #73b8ff;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.5);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.5);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.6);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.55);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
