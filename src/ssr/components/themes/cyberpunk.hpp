#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeCyberpunk {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="cyberpunk"] {
    --bg-0: #0a0a12;
    --bg-1: #0f0f1a;
    --bg-2: #161625;
    --bg-3: #1e1e32;
    --bg-4: #2a2a44;
    --border: #1e1e32;
    --border-subtle: #161625;
    --text-0: #00ff9c;
    --text-1: #00dd88;
    --text-2: #00aa6a;
    --text-3: #007a4c;
    --text-4: #004d30;
    --accent: #ff003c;
    --accent-dim: #cc0030;
    --accent-subtle: rgba(255, 0, 60, 0.12);
    --success: #00ff9c;
    --success-subtle: rgba(0, 255, 156, 0.1);
    --warning: #ffff00;
    --warning-subtle: rgba(255, 255, 0, 0.1);
    --danger: #ff003c;
    --danger-subtle: rgba(255, 0, 60, 0.1);
    --info: #00d4ff;
    --shadow-sm: 0 0 4px rgba(0, 255, 156, 0.1);
    --shadow-md: 0 0 12px rgba(0, 255, 156, 0.08);
    --shadow-lg: 0 0 24px rgba(0, 255, 156, 0.06);
    --shadow-lift: 0 0 16px rgba(255, 0, 60, 0.1);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
