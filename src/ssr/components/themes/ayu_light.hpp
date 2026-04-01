#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeAyuLight {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="ayu-light"] {
    --bg-0: #fafafa;
    --bg-1: #f3f4f5;
    --bg-2: #ebeced;
    --bg-3: #d8d9da;
    --bg-4: #c5c6c7;
    --border: #d4d5d6;
    --border-subtle: #e5e6e7;
    --text-0: #575f66;
    --text-1: #6b737a;
    --text-2: #8a9199;
    --text-3: #abb0b6;
    --text-4: #c5c9cd;
    --accent: #f2ae49;
    --accent-dim: #e09b33;
    --accent-subtle: rgba(242, 174, 73, 0.08);
    --success: #6cbf43;
    --success-subtle: rgba(108, 191, 67, 0.08);
    --warning: #f2ae49;
    --warning-subtle: rgba(242, 174, 73, 0.08);
    --danger: #e65050;
    --danger-subtle: rgba(230, 80, 80, 0.08);
    --info: #399ee6;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.06);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.08);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.12);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.08);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
