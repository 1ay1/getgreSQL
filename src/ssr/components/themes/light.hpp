#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeLight {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="light"] {
    --bg-0: #ffffff;
    --bg-1: #f6f8fa;
    --bg-2: #eef1f5;
    --bg-3: #d8dee4;
    --bg-4: #c0c8d2;
    --border: #d0d7de;
    --border-subtle: #e1e4e8;
    --text-0: #1f2328;
    --text-1: #31363f;
    --text-2: #656d76;
    --text-3: #8c959f;
    --text-4: #afb8c1;
    --accent: #0969da;
    --accent-dim: #0550ae;
    --accent-subtle: rgba(9, 105, 218, 0.08);
    --success: #1a7f37;
    --success-subtle: rgba(26, 127, 55, 0.08);
    --warning: #9a6700;
    --warning-subtle: rgba(154, 103, 0, 0.08);
    --danger: #cf222e;
    --danger-subtle: rgba(207, 34, 46, 0.08);
    --info: #0969da;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.08);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.1);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.15);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.12);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
