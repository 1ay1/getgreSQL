#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeSolarizedLight {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="solarized-light"] {
    --bg-0: #fdf6e3;
    --bg-1: #f5eedb;
    --bg-2: #eee8d5;
    --bg-3: #ddd6c1;
    --bg-4: #ccc5af;
    --border: #d3cbb7;
    --border-subtle: #e6dfcb;
    --text-0: #073642;
    --text-1: #0a4050;
    --text-2: #586e75;
    --text-3: #839496;
    --text-4: #93a1a1;
    --accent: #268bd2;
    --accent-dim: #1a6da8;
    --accent-subtle: rgba(38, 139, 210, 0.08);
    --success: #859900;
    --success-subtle: rgba(133, 153, 0, 0.08);
    --warning: #b58900;
    --warning-subtle: rgba(181, 137, 0, 0.08);
    --danger: #dc322f;
    --danger-subtle: rgba(220, 50, 47, 0.08);
    --info: #2aa198;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.08);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.1);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.15);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.1);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
