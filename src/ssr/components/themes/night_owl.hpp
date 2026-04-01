#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeNightOwl {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="night-owl"] {
    --bg-0: #011627;
    --bg-1: #041e30;
    --bg-2: #0b253a;
    --bg-3: #13324b;
    --bg-4: #1d3b58;
    --border: #13324b;
    --border-subtle: #0b253a;
    --text-0: #d6deeb;
    --text-1: #c5cdd8;
    --text-2: #a1aab5;
    --text-3: #637777;
    --text-4: #4a6060;
    --accent: #82aaff;
    --accent-dim: #6b94e8;
    --accent-subtle: rgba(130, 170, 255, 0.1);
    --success: #22da6e;
    --success-subtle: rgba(34, 218, 110, 0.1);
    --warning: #ecc48d;
    --warning-subtle: rgba(236, 196, 141, 0.1);
    --danger: #ef5350;
    --danger-subtle: rgba(239, 83, 80, 0.1);
    --info: #7fdbca;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.45);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.45);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.55);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.5);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
