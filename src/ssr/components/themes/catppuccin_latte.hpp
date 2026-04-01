#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeCatppuccinLatte {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="catppuccin-latte"] {
    --bg-0: #eff1f5;
    --bg-1: #e6e9ef;
    --bg-2: #dce0e8;
    --bg-3: #ccd0da;
    --bg-4: #bcc0cc;
    --border: #ccd0da;
    --border-subtle: #dce0e8;
    --text-0: #4c4f69;
    --text-1: #5c5f77;
    --text-2: #6c6f85;
    --text-3: #8c8fa1;
    --text-4: #9ca0b0;
    --accent: #1e66f5;
    --accent-dim: #1558d8;
    --accent-subtle: rgba(30, 102, 245, 0.08);
    --success: #40a02b;
    --success-subtle: rgba(64, 160, 43, 0.08);
    --warning: #df8e1d;
    --warning-subtle: rgba(223, 142, 29, 0.08);
    --danger: #d20f39;
    --danger-subtle: rgba(210, 15, 57, 0.08);
    --info: #04a5e5;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.06);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.08);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.12);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.1);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
