#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeVitesseDark {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="vitesse-dark"] {
    --bg-0: #121212;
    --bg-1: #171717;
    --bg-2: #1e1e1e;
    --bg-3: #282828;
    --bg-4: #363636;
    --border: #282828;
    --border-subtle: #1e1e1e;
    --text-0: #dbd7ca;
    --text-1: #c9c5b8;
    --text-2: #a0a0a0;
    --text-3: #6b6b6b;
    --text-4: #4a4a4a;
    --accent: #4d9375;
    --accent-dim: #3d7a60;
    --accent-subtle: rgba(77, 147, 117, 0.1);
    --success: #4d9375;
    --success-subtle: rgba(77, 147, 117, 0.1);
    --warning: #d4976c;
    --warning-subtle: rgba(212, 151, 108, 0.1);
    --danger: #cb7676;
    --danger-subtle: rgba(203, 118, 118, 0.1);
    --info: #6394bf;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.4);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.4);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.5);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.45);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
