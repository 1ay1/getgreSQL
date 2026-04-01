#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeDracula {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="dracula"] {
    --bg-0: #282a36;
    --bg-1: #2d2f3d;
    --bg-2: #343746;
    --bg-3: #3e4154;
    --bg-4: #4d5066;
    --border: #44475a;
    --border-subtle: #363949;
    --text-0: #f8f8f2;
    --text-1: #e2e2dc;
    --text-2: #bfbfb6;
    --text-3: #7c7f8a;
    --text-4: #565867;
    --accent: #bd93f9;
    --accent-dim: #9d6ff5;
    --accent-subtle: rgba(189, 147, 249, 0.1);
    --success: #50fa7b;
    --success-subtle: rgba(80, 250, 123, 0.1);
    --warning: #f1fa8c;
    --warning-subtle: rgba(241, 250, 140, 0.1);
    --danger: #ff5555;
    --danger-subtle: rgba(255, 85, 85, 0.1);
    --info: #8be9fd;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.35);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.35);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.45);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.4);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
