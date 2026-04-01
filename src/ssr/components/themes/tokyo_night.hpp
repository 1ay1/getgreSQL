#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeTokyoNight {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="tokyo-night"] {
    --bg-0: #1a1b26;
    --bg-1: #1e1f2b;
    --bg-2: #24283b;
    --bg-3: #2f3348;
    --bg-4: #3b3f57;
    --border: #3b3f57;
    --border-subtle: #2a2e42;
    --text-0: #c0caf5;
    --text-1: #a9b1d6;
    --text-2: #9aa5ce;
    --text-3: #6b7089;
    --text-4: #545c7e;
    --accent: #7aa2f7;
    --accent-dim: #5d8af0;
    --accent-subtle: rgba(122, 162, 247, 0.1);
    --success: #9ece6a;
    --success-subtle: rgba(158, 206, 106, 0.1);
    --warning: #e0af68;
    --warning-subtle: rgba(224, 175, 104, 0.1);
    --danger: #f7768e;
    --danger-subtle: rgba(247, 118, 142, 0.1);
    --info: #7dcfff;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.4);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.4);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.5);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.45);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
