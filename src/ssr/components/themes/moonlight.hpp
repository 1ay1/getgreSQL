#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeMoonlight {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="moonlight"] {
    --bg-0: #1e2030;
    --bg-1: #222436;
    --bg-2: #2a2c40;
    --bg-3: #31344a;
    --bg-4: #404361;
    --border: #31344a;
    --border-subtle: #2a2c40;
    --text-0: #c8d3f5;
    --text-1: #b4bfdf;
    --text-2: #9dabc4;
    --text-3: #6e7593;
    --text-4: #545a72;
    --accent: #82aaff;
    --accent-dim: #6b94e8;
    --accent-subtle: rgba(130, 170, 255, 0.1);
    --success: #c3e88d;
    --success-subtle: rgba(195, 232, 141, 0.1);
    --warning: #ffc777;
    --warning-subtle: rgba(255, 199, 119, 0.1);
    --danger: #ff757f;
    --danger-subtle: rgba(255, 117, 127, 0.1);
    --info: #77e0e6;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.4);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.4);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.5);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.45);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
