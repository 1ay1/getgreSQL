#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeHighContrast {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="high-contrast"] {
    --bg-0: #000000;
    --bg-1: #050505;
    --bg-2: #0a0a0a;
    --bg-3: #151515;
    --bg-4: #252525;
    --border: #6e7681;
    --border-subtle: #303030;
    --text-0: #ffffff;
    --text-1: #f0f0f0;
    --text-2: #d0d0d0;
    --text-3: #909090;
    --text-4: #606060;
    --accent: #71b7ff;
    --accent-dim: #409eff;
    --accent-subtle: rgba(113, 183, 255, 0.15);
    --success: #26cd4d;
    --success-subtle: rgba(38, 205, 77, 0.15);
    --warning: #f0b72f;
    --warning-subtle: rgba(240, 183, 47, 0.15);
    --danger: #ff6a69;
    --danger-subtle: rgba(255, 106, 105, 0.15);
    --info: #71b7ff;
    --shadow-sm: 0 0 0 1px var(--border);
    --shadow-md: 0 0 0 1px var(--border), 0 2px 8px rgba(0, 0, 0, 0.5);
    --shadow-lg: 0 0 0 1px var(--border), 0 8px 24px rgba(0, 0, 0, 0.6);
    --shadow-lift: 0 0 0 1px var(--border), 0 4px 12px rgba(0, 0, 0, 0.55);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
