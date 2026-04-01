#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeGruvboxLight {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="gruvbox-light"] {
    --bg-0: #fbf1c7;
    --bg-1: #f2e5bc;
    --bg-2: #ebdbb2;
    --bg-3: #d5c4a1;
    --bg-4: #bdae93;
    --border: #d5c4a1;
    --border-subtle: #ebdbb2;
    --text-0: #3c3836;
    --text-1: #504945;
    --text-2: #665c54;
    --text-3: #928374;
    --text-4: #a89984;
    --accent: #076678;
    --accent-dim: #055560;
    --accent-subtle: rgba(7, 102, 120, 0.08);
    --success: #79740e;
    --success-subtle: rgba(121, 116, 14, 0.08);
    --warning: #b57614;
    --warning-subtle: rgba(181, 118, 20, 0.08);
    --danger: #cc241d;
    --danger-subtle: rgba(204, 36, 29, 0.08);
    --info: #427b58;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.08);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.1);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.15);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.1);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
