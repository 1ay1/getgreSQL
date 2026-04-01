#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeGitHubDimmed {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="github-dimmed"] {
    --bg-0: #22272e;
    --bg-1: #272c34;
    --bg-2: #2d333b;
    --bg-3: #373e47;
    --bg-4: #444c56;
    --border: #444c56;
    --border-subtle: #373e47;
    --text-0: #adbac7;
    --text-1: #99a5b3;
    --text-2: #768390;
    --text-3: #545d68;
    --text-4: #3d444d;
    --accent: #539bf5;
    --accent-dim: #3d8ae0;
    --accent-subtle: rgba(83, 155, 245, 0.1);
    --success: #57ab5a;
    --success-subtle: rgba(87, 171, 90, 0.1);
    --warning: #c69026;
    --warning-subtle: rgba(198, 144, 38, 0.1);
    --danger: #e5534b;
    --danger-subtle: rgba(229, 83, 75, 0.1);
    --info: #539bf5;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.3);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.3);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.4);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.35);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
