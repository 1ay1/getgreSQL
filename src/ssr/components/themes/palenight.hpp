#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemePalenight {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="palenight"] {
    --bg-0: #292d3e;
    --bg-1: #2f3344;
    --bg-2: #34394d;
    --bg-3: #3c4158;
    --bg-4: #4a4f65;
    --border: #3c4158;
    --border-subtle: #34394d;
    --text-0: #a6accd;
    --text-1: #959dbe;
    --text-2: #8087a8;
    --text-3: #606882;
    --text-4: #4a5068;
    --accent: #82aaff;
    --accent-dim: #6b94e8;
    --accent-subtle: rgba(130, 170, 255, 0.1);
    --success: #c3e88d;
    --success-subtle: rgba(195, 232, 141, 0.1);
    --warning: #ffcb6b;
    --warning-subtle: rgba(255, 203, 107, 0.1);
    --danger: #f07178;
    --danger-subtle: rgba(240, 113, 120, 0.1);
    --info: #89ddff;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.35);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.35);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.45);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.4);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
