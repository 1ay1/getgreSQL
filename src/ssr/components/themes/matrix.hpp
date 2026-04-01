#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeMatrix {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="matrix"] {
    --bg-0: #000000;
    --bg-1: #050505;
    --bg-2: #0a0f0a;
    --bg-3: #0f180f;
    --bg-4: #162216;
    --border: #0f180f;
    --border-subtle: #0a0f0a;
    --text-0: #00ff41;
    --text-1: #00dd38;
    --text-2: #00aa2c;
    --text-3: #007720;
    --text-4: #004d14;
    --accent: #00ff41;
    --accent-dim: #00cc34;
    --accent-subtle: rgba(0, 255, 65, 0.08);
    --success: #00ff41;
    --success-subtle: rgba(0, 255, 65, 0.08);
    --warning: #88ff00;
    --warning-subtle: rgba(136, 255, 0, 0.08);
    --danger: #ff0040;
    --danger-subtle: rgba(255, 0, 64, 0.08);
    --info: #00ff41;
    --shadow-sm: 0 0 4px rgba(0, 255, 65, 0.15);
    --shadow-md: 0 0 8px rgba(0, 255, 65, 0.1);
    --shadow-lg: 0 0 20px rgba(0, 255, 65, 0.08);
    --shadow-lift: 0 0 12px rgba(0, 255, 65, 0.12);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
