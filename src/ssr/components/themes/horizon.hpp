#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeHorizon {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="horizon"] {
    --bg-0: #1c1e26;
    --bg-1: #21232d;
    --bg-2: #272935;
    --bg-3: #2e3140;
    --bg-4: #3d4051;
    --border: #2e3140;
    --border-subtle: #272935;
    --text-0: #e0d8d1;
    --text-1: #cdc5be;
    --text-2: #b0a9a2;
    --text-3: #6f6e6e;
    --text-4: #555555;
    --accent: #e95678;
    --accent-dim: #d14060;
    --accent-subtle: rgba(233, 86, 120, 0.1);
    --success: #29d398;
    --success-subtle: rgba(41, 211, 152, 0.1);
    --warning: #fab795;
    --warning-subtle: rgba(250, 183, 149, 0.1);
    --danger: #e95678;
    --danger-subtle: rgba(233, 86, 120, 0.1);
    --info: #25b0bc;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.4);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.4);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.5);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.45);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
