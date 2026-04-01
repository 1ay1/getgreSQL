#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeMidnight {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="midnight"] {
    --bg-0: #0f0f1e;
    --bg-1: #141428;
    --bg-2: #1a1a32;
    --bg-3: #22223e;
    --bg-4: #2e2e4e;
    --border: #22223e;
    --border-subtle: #1a1a32;
    --text-0: #c8c8e8;
    --text-1: #b0b0d0;
    --text-2: #8888b0;
    --text-3: #606088;
    --text-4: #444468;
    --accent: #7b68ee;
    --accent-dim: #6050d0;
    --accent-subtle: rgba(123, 104, 238, 0.12);
    --success: #66cc99;
    --success-subtle: rgba(102, 204, 153, 0.1);
    --warning: #e8c858;
    --warning-subtle: rgba(232, 200, 88, 0.1);
    --danger: #ee6868;
    --danger-subtle: rgba(238, 104, 104, 0.1);
    --info: #68b8ee;
    --shadow-sm: 0 1px 3px rgba(123, 104, 238, 0.08);
    --shadow-md: 0 2px 8px rgba(123, 104, 238, 0.06);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.5);
    --shadow-lift: 0 4px 12px rgba(123, 104, 238, 0.1);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
