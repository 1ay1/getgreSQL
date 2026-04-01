#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemePoimandres {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="poimandres"] {
    --bg-0: #1b1e28;
    --bg-1: #1f2230;
    --bg-2: #252837;
    --bg-3: #2c3044;
    --bg-4: #3a3e54;
    --border: #2c3044;
    --border-subtle: #252837;
    --text-0: #e4f0fb;
    --text-1: #c8d7e6;
    --text-2: #a6b0c4;
    --text-3: #6e7a8f;
    --text-4: #505a6e;
    --accent: #add7ff;
    --accent-dim: #8ac4f0;
    --accent-subtle: rgba(173, 215, 255, 0.1);
    --success: #5de4c7;
    --success-subtle: rgba(93, 228, 199, 0.1);
    --warning: #fffac2;
    --warning-subtle: rgba(255, 250, 194, 0.1);
    --danger: #d0679d;
    --danger-subtle: rgba(208, 103, 157, 0.1);
    --info: #89ddff;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.4);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.4);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.5);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.45);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
