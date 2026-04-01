#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeAmberTerminal {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="amber"] {
    --bg-0: #0c0800;
    --bg-1: #120e04;
    --bg-2: #1a1508;
    --bg-3: #241e0e;
    --bg-4: #302a16;
    --border: #241e0e;
    --border-subtle: #1a1508;
    --text-0: #ffb000;
    --text-1: #dd9900;
    --text-2: #bb8000;
    --text-3: #886000;
    --text-4: #554000;
    --accent: #ffb000;
    --accent-dim: #cc8d00;
    --accent-subtle: rgba(255, 176, 0, 0.1);
    --success: #ffb000;
    --success-subtle: rgba(255, 176, 0, 0.08);
    --warning: #ffdd00;
    --warning-subtle: rgba(255, 221, 0, 0.08);
    --danger: #ff4400;
    --danger-subtle: rgba(255, 68, 0, 0.08);
    --info: #ffb000;
    --shadow-sm: 0 0 4px rgba(255, 176, 0, 0.12);
    --shadow-md: 0 0 8px rgba(255, 176, 0, 0.08);
    --shadow-lg: 0 0 20px rgba(255, 176, 0, 0.06);
    --shadow-lift: 0 0 12px rgba(255, 176, 0, 0.1);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
