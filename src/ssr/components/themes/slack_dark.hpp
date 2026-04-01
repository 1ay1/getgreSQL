#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeSlackDark {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="slack-dark"] {
    --bg-0: #1a1d21;
    --bg-1: #1e2126;
    --bg-2: #222529;
    --bg-3: #2c2d31;
    --bg-4: #3a3b3f;
    --border: #38393d;
    --border-subtle: #2c2d31;
    --text-0: #d1d2d3;
    --text-1: #babbbf;
    --text-2: #9a9b9f;
    --text-3: #6b6c70;
    --text-4: #515256;
    --accent: #1d9bd1;
    --accent-dim: #1585b5;
    --accent-subtle: rgba(29, 155, 209, 0.1);
    --success: #2bac76;
    --success-subtle: rgba(43, 172, 118, 0.1);
    --warning: #e8a820;
    --warning-subtle: rgba(232, 168, 32, 0.1);
    --danger: #e23d4d;
    --danger-subtle: rgba(226, 61, 77, 0.1);
    --info: #1d9bd1;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.3);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.3);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.4);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.35);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
