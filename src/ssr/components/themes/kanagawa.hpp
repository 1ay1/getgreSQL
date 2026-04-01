#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeKanagawa {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="kanagawa"] {
    --bg-0: #1f1f28;
    --bg-1: #24242e;
    --bg-2: #2a2a37;
    --bg-3: #363646;
    --bg-4: #44445a;
    --border: #363646;
    --border-subtle: #2a2a37;
    --text-0: #dcd7ba;
    --text-1: #c8c093;
    --text-2: #a6a08e;
    --text-3: #727169;
    --text-4: #54546d;
    --accent: #7e9cd8;
    --accent-dim: #6a88c4;
    --accent-subtle: rgba(126, 156, 216, 0.1);
    --success: #98bb6c;
    --success-subtle: rgba(152, 187, 108, 0.1);
    --warning: #e6c384;
    --warning-subtle: rgba(230, 195, 132, 0.1);
    --danger: #ff5d62;
    --danger-subtle: rgba(255, 93, 98, 0.1);
    --info: #7fb4ca;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.4);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.4);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.5);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.45);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
