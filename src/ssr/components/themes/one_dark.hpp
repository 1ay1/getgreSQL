#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeOneDark {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="one-dark"] {
    --bg-0: #282c34;
    --bg-1: #2c313a;
    --bg-2: #333842;
    --bg-3: #3e4452;
    --bg-4: #4b5263;
    --border: #3e4452;
    --border-subtle: #333842;
    --text-0: #abb2bf;
    --text-1: #9da5b4;
    --text-2: #8b92a0;
    --text-3: #636d83;
    --text-4: #4b5263;
    --accent: #61afef;
    --accent-dim: #4d99d9;
    --accent-subtle: rgba(97, 175, 239, 0.1);
    --success: #98c379;
    --success-subtle: rgba(152, 195, 121, 0.1);
    --warning: #e5c07b;
    --warning-subtle: rgba(229, 192, 123, 0.1);
    --danger: #e06c75;
    --danger-subtle: rgba(224, 108, 117, 0.1);
    --info: #56b6c2;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.35);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.35);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.45);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.4);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
