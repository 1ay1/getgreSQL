#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeCobalt2 {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="cobalt2"] {
    --bg-0: #122738;
    --bg-1: #15303f;
    --bg-2: #193a4a;
    --bg-3: #1e4558;
    --bg-4: #275468;
    --border: #1e4558;
    --border-subtle: #193a4a;
    --text-0: #ffffff;
    --text-1: #e1e6ea;
    --text-2: #b0bec5;
    --text-3: #6a8da0;
    --text-4: #4a6e80;
    --accent: #ffc600;
    --accent-dim: #dda900;
    --accent-subtle: rgba(255, 198, 0, 0.1);
    --success: #3ad900;
    --success-subtle: rgba(58, 217, 0, 0.1);
    --warning: #ffc600;
    --warning-subtle: rgba(255, 198, 0, 0.1);
    --danger: #ff2600;
    --danger-subtle: rgba(255, 38, 0, 0.1);
    --info: #0088ff;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.45);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.45);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.55);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.5);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
