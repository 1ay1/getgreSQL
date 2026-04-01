#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ThemeAndromeda {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
[data-theme="andromeda"] {
    --bg-0: #23262e;
    --bg-1: #282b33;
    --bg-2: #2e323c;
    --bg-3: #363a46;
    --bg-4: #454955;
    --border: #363a46;
    --border-subtle: #2e323c;
    --text-0: #d5ced9;
    --text-1: #c5bec9;
    --text-2: #a0a0b0;
    --text-3: #6e6e80;
    --text-4: #555566;
    --accent: #00e8c6;
    --accent-dim: #00c4a8;
    --accent-subtle: rgba(0, 232, 198, 0.1);
    --success: #96e072;
    --success-subtle: rgba(150, 224, 114, 0.1);
    --warning: #ffe66d;
    --warning-subtle: rgba(255, 230, 109, 0.1);
    --danger: #ee5d43;
    --danger-subtle: rgba(238, 93, 67, 0.1);
    --info: #00e8c6;
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.35);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.35);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.45);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.4);
}
)_CSS_"; }
};

} // namespace getgresql::ssr
