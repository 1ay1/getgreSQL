#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeMonokai {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="monokai"] {
    --bg-0: #272822;
    --bg-1: #2d2e27;
    --bg-2: #383930;
    --bg-3: #44453b;
    --bg-4: #57584d;
    --border: #49483e;
    --border-subtle: #3b3c31;
    --text-0: #f8f8f2;
    --text-1: #e6e6e1;
    --text-2: #b3b3a6;
    --text-3: #75715e;
    --text-4: #5c5b4f;
    --accent: #66d9ef;
    --accent-dim: #38b6d2;
    --accent-subtle: rgba(102, 217, 239, 0.1);
    --success: #a6e22e;
    --success-subtle: rgba(166, 226, 46, 0.1);
    --warning: #e6db74;
    --warning-subtle: rgba(230, 219, 116, 0.1);
    --danger: #f92672;
    --danger-subtle: rgba(249, 38, 114, 0.1);
    --info: #66d9ef;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.35);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.35);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.45);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.4);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
