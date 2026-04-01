#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeAyuMirage {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="ayu-mirage"] {
    --bg-0: #1f2430;
    --bg-1: #242936;
    --bg-2: #2a2f3c;
    --bg-3: #333846;
    --bg-4: #3e4454;
    --border: #333846;
    --border-subtle: #2a2f3c;
    --text-0: #cbccc6;
    --text-1: #b8b9b3;
    --text-2: #9a9b95;
    --text-3: #6c6e68;
    --text-4: #535550;
    --accent: #ffcc66;
    --accent-dim: #e6b44d;
    --accent-subtle: rgba(255, 204, 102, 0.1);
    --success: #87d96c;
    --success-subtle: rgba(135, 217, 108, 0.1);
    --warning: #ffa759;
    --warning-subtle: rgba(255, 167, 89, 0.1);
    --danger: #f28779;
    --danger-subtle: rgba(242, 135, 121, 0.1);
    --info: #5ccfe6;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.4);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.4);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.5);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.45);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
