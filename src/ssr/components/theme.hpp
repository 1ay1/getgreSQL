#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct Theme {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
/* ─── getgreSQL — Database IDE Design System ─────────────────────────── */

:root {
    /* Surface colors — dark first */
    --bg-0: #0d1117;
    --bg-1: #161b22;
    --bg-2: #1c2128;
    --bg-3: #21262d;
    --bg-4: #30363d;
    --border: #30363d;
    --border-subtle: #21262d;

    /* Text hierarchy */
    --text-0: #f0f6fc;
    --text-1: #c9d1d9;
    --text-2: #8b949e;
    --text-3: #6e7681;
    --text-4: #484f58;

    /* Accent palette */
    --accent: #58a6ff;
    --accent-dim: #1f6feb;
    --accent-subtle: rgba(56, 139, 253, 0.1);
    --success: #3fb950;
    --success-subtle: rgba(63, 185, 80, 0.1);
    --warning: #d29922;
    --warning-subtle: rgba(210, 153, 34, 0.1);
    --danger: #f85149;
    --danger-subtle: rgba(248, 81, 73, 0.1);
    --info: #58a6ff;

    /* Typography */
    --font-mono: 'JetBrains Mono', 'Fira Code', 'Cascadia Code', 'SF Mono', 'Consolas', monospace;
    --font-sans: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'Noto Sans', Helvetica, Arial, sans-serif;
    --font-size-xs: 0.7rem;
    --font-size-sm: 0.8rem;
    --font-size-base: 0.85rem;
    --font-size-md: 0.95rem;
    --font-size-lg: 1.1rem;

    /* Layout dimensions */
    --toolbar-height: 38px;
    --tab-bar-height: 35px;
    --status-bar-height: 24px;
    --sidebar-width: 260px;
    --sidebar-min: 180px;
    --sidebar-max: 420px;

    /* Spacing */
    --sp-1: 2px;
    --sp-2: 4px;
    --sp-3: 8px;
    --sp-4: 12px;
    --sp-5: 16px;
    --sp-6: 24px;

    --radius: 4px;
    --radius-lg: 6px;

    /* Shadows */
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.3);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.3);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.4);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.35);

    /* Transitions */
    --transition-fast: 0.12s ease;
    --transition-normal: 0.2s ease;
}

[data-theme="light"] {
    --bg-0: #ffffff;
    --bg-1: #f6f8fa;
    --bg-2: #eef1f5;
    --bg-3: #d8dee4;
    --bg-4: #c0c8d2;
    --border: #d0d7de;
    --border-subtle: #e1e4e8;
    --text-0: #1f2328;
    --text-1: #31363f;
    --text-2: #656d76;
    --text-3: #8c959f;
    --text-4: #afb8c1;
    --accent: #0969da;
    --accent-dim: #0550ae;
    --accent-subtle: rgba(9, 105, 218, 0.08);
    --success-subtle: rgba(63, 185, 80, 0.08);
    --warning-subtle: rgba(210, 153, 34, 0.08);
    --danger-subtle: rgba(248, 81, 73, 0.08);
    --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.08);
    --shadow-md: 0 2px 8px rgba(0, 0, 0, 0.1);
    --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.15);
    --shadow-lift: 0 4px 12px rgba(0, 0, 0, 0.12);
}

*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

html {
    font-size: 14px;
    -webkit-font-smoothing: antialiased;
    -moz-osx-font-smoothing: grayscale;
}

body {
    font-family: var(--font-sans);
    background: var(--bg-0);
    color: var(--text-1);
    overflow: hidden;
    height: 100vh;
    width: 100vw;
}

a { color: var(--accent); text-decoration: none; transition: color var(--transition-fast); }
a:hover { text-decoration: underline; }
code, pre, .mono { font-family: var(--font-mono); }

/* ─── Global scrollbar styling ───────────────────────────────────────── */

::-webkit-scrollbar { width: 8px; height: 8px; }
::-webkit-scrollbar-track { background: transparent; }
::-webkit-scrollbar-thumb {
    background: var(--bg-4);
    border-radius: 4px;
    border: 2px solid transparent;
    background-clip: content-box;
}
::-webkit-scrollbar-thumb:hover { background: var(--text-4); background-clip: content-box; }
::-webkit-scrollbar-corner { background: transparent; }

/* Firefox */
* {
    scrollbar-width: thin;
    scrollbar-color: var(--bg-4) transparent;
}

/* ─── Print styles ────────────────────────────────────────────────────── */
@media print {
    .sidebar, .toolbar, .status-bar, .tab-bar, .editor-toolbar, .data-toolbar .btn, .dv-toolbar { display: none !important; }
    .ide { display: block !important; }
    .workspace { margin: 0 !important; }
    .content { padding: 0 !important; }
}
)_CSS_"; }

};

} // namespace getgresql::ssr
