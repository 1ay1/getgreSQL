#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeShadesOfPurple {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="shades-of-purple"] {
    --bg-0: #1e1e3f;
    --bg-1: #232347;
    --bg-2: #2a2a50;
    --bg-3: #33335c;
    --bg-4: #41416e;
    --border: #33335c;
    --border-subtle: #2a2a50;
    --text-0: #fafafa;
    --text-1: #e0e0e0;
    --text-2: #b3b3cc;
    --text-3: #7575a3;
    --text-4: #5a5a80;
    --accent: #fad000;
    --accent-dim: #d4b000;
    --accent-subtle: rgba(250, 208, 0, 0.1);
    --success: #a5ff90;
    --success-subtle: rgba(165, 255, 144, 0.1);
    --warning: #fad000;
    --warning-subtle: rgba(250, 208, 0, 0.1);
    --danger: #ff628c;
    --danger-subtle: rgba(255, 98, 140, 0.1);
    --info: #9effff;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.4);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.4);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.5);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.45);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
