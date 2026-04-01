#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeGruvboxDark {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="gruvbox-dark"] {
    --bg-0: #282828;
    --bg-1: #2e2e2e;
    --bg-2: #3c3836;
    --bg-3: #504945;
    --bg-4: #665c54;
    --border: #504945;
    --border-subtle: #3c3836;
    --text-0: #ebdbb2;
    --text-1: #d5c4a1;
    --text-2: #bdae93;
    --text-3: #928374;
    --text-4: #7c6f64;
    --accent: #83a598;
    --accent-dim: #689d6a;
    --accent-subtle: rgba(131, 165, 152, 0.12);
    --success: #b8bb26;
    --success-subtle: rgba(184, 187, 38, 0.12);
    --warning: #fabd2f;
    --warning-subtle: rgba(250, 189, 47, 0.12);
    --danger: #fb4934;
    --danger-subtle: rgba(251, 73, 52, 0.12);
    --info: #83a598;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.35);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.35);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.45);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.4);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
