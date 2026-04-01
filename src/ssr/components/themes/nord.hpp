#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeNord {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="nord"] {
    --bg-0: #2e3440;
    --bg-1: #333a47;
    --bg-2: #3b4252;
    --bg-3: #434c5e;
    --bg-4: #4c566a;
    --border: #4c566a;
    --border-subtle: #3b4252;
    --text-0: #eceff4;
    --text-1: #e5e9f0;
    --text-2: #d8dee9;
    --text-3: #7b88a1;
    --text-4: #5c6a82;
    --accent: #88c0d0;
    --accent-dim: #5e81ac;
    --accent-subtle: rgba(136, 192, 208, 0.1);
    --success: #a3be8c;
    --success-subtle: rgba(163, 190, 140, 0.1);
    --warning: #ebcb8b;
    --warning-subtle: rgba(235, 203, 139, 0.1);
    --danger: #bf616a;
    --danger-subtle: rgba(191, 97, 106, 0.1);
    --info: #81a1c1;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.3);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.3);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.4);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.35);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
