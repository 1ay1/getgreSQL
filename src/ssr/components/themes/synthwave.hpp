#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeSynthwave {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="synthwave"] {
    --bg-0: #1a1028;
    --bg-1: #211636;
    --bg-2: #2a1c42;
    --bg-3: #352550;
    --bg-4: #443264;
    --border: #352550;
    --border-subtle: #2a1c42;
    --text-0: #e8d4f8;
    --text-1: #d6bfe8;
    --text-2: #b89fd0;
    --text-3: #7b6992;
    --text-4: #5a4e6b;
    --accent: #ff7edb;
    --accent-dim: #e65cc0;
    --accent-subtle: rgba(255, 126, 219, 0.12);
    --success: #72f1b8;
    --success-subtle: rgba(114, 241, 184, 0.12);
    --warning: #fede5d;
    --warning-subtle: rgba(254, 222, 93, 0.12);
    --danger: #fe4450;
    --danger-subtle: rgba(254, 68, 80, 0.12);
    --info: #36f9f6;
    --shadow-sm: 0 1px 3px rgba(255, 126, 219, 0.1);
    --shadow-md: 0 2px 8px rgba(255, 126, 219, 0.12);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.5);
    --shadow-lift: 0 4px 12px rgba(255, 126, 219, 0.15);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
