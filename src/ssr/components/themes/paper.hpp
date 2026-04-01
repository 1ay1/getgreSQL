#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemePaper {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="paper"] {
    --bg-0: #f5f0eb;
    --bg-1: #ede7e0;
    --bg-2: #e5ddd5;
    --bg-3: #d4cbc1;
    --bg-4: #c2b8ae;
    --border: #d4cbc1;
    --border-subtle: #e5ddd5;
    --text-0: #2e2b28;
    --text-1: #433f3b;
    --text-2: #6b6560;
    --text-3: #9a948e;
    --text-4: #b5afa9;
    --accent: #8b5c34;
    --accent-dim: #6e4828;
    --accent-subtle: rgba(139, 92, 52, 0.08);
    --success: #4a7c4e;
    --success-subtle: rgba(74, 124, 78, 0.08);
    --warning: #a87a3d;
    --warning-subtle: rgba(168, 122, 61, 0.08);
    --danger: #a84040;
    --danger-subtle: rgba(168, 64, 64, 0.08);
    --info: #4a6d8b;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.06);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.08);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.1);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.08);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
