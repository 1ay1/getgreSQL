#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeCatppuccinMocha {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="catppuccin-mocha"] {
    --bg-0: #1e1e2e;
    --bg-1: #232334;
    --bg-2: #2a2a3c;
    --bg-3: #313244;
    --bg-4: #45475a;
    --border: #45475a;
    --border-subtle: #313244;
    --text-0: #cdd6f4;
    --text-1: #bac2de;
    --text-2: #a6adc8;
    --text-3: #7f849c;
    --text-4: #585b70;
    --accent: #89b4fa;
    --accent-dim: #74a8fc;
    --accent-subtle: rgba(137, 180, 250, 0.1);
    --success: #a6e3a1;
    --success-subtle: rgba(166, 227, 161, 0.1);
    --warning: #f9e2af;
    --warning-subtle: rgba(249, 226, 175, 0.1);
    --danger: #f38ba8;
    --danger-subtle: rgba(243, 139, 168, 0.1);
    --info: #89dceb;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.35);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.35);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.45);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.4);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
