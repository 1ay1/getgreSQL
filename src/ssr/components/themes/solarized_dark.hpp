#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeSolarizedDark {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="solarized-dark"] {
    --bg-0: #002b36;
    --bg-1: #003541;
    --bg-2: #073642;
    --bg-3: #0a4050;
    --bg-4: #1a5060;
    --border: #0a4050;
    --border-subtle: #073642;
    --text-0: #fdf6e3;
    --text-1: #eee8d5;
    --text-2: #93a1a1;
    --text-3: #657b83;
    --text-4: #586e75;
    --accent: #268bd2;
    --accent-dim: #1a6da8;
    --accent-subtle: rgba(38, 139, 210, 0.12);
    --success: #859900;
    --success-subtle: rgba(133, 153, 0, 0.12);
    --warning: #b58900;
    --warning-subtle: rgba(181, 137, 0, 0.12);
    --danger: #dc322f;
    --danger-subtle: rgba(220, 50, 47, 0.12);
    --info: #2aa198;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.35);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.35);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.5);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.4);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
