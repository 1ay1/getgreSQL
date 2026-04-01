#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeTron {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="tron"] {
    --bg-0: #0a0a0f;
    --bg-1: #0e0e15;
    --bg-2: #14141e;
    --bg-3: #1a1a28;
    --bg-4: #242438;
    --border: #1a1a28;
    --border-subtle: #14141e;
    --text-0: #6ee2ff;
    --text-1: #5cc8e0;
    --text-2: #4aacbf;
    --text-3: #2e7a8a;
    --text-4: #1e5060;
    --accent: #6ee2ff;
    --accent-dim: #4cc0dd;
    --accent-subtle: rgba(110, 226, 255, 0.08);
    --success: #6ee2ff;
    --success-subtle: rgba(110, 226, 255, 0.08);
    --warning: #ffe64d;
    --warning-subtle: rgba(255, 230, 77, 0.08);
    --danger: #ff3d00;
    --danger-subtle: rgba(255, 61, 0, 0.08);
    --info: #6ee2ff;
    --shadow-sm: 0 0 4px rgba(110, 226, 255, 0.1);
    --shadow-md: 0 0 10px rgba(110, 226, 255, 0.08);
    --shadow-lg: 0 0 20px rgba(110, 226, 255, 0.06);
    --shadow-lift: 0 0 14px rgba(110, 226, 255, 0.1);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
