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

/* Theme overrides are in src/ssr/components/themes/*.hpp */

/* ─── Theme Picker ────────────────────────────────────────────────────── */

.theme-picker-overlay {
    position: fixed;
    inset: 0;
    background: rgba(0, 0, 0, 0.5);
    z-index: 10000;
    display: flex;
    align-items: center;
    justify-content: center;
}

.theme-picker {
    background: var(--bg-1);
    border: 1px solid var(--border);
    border-radius: var(--radius-lg);
    box-shadow: var(--shadow-lg);
    width: 480px;
    max-width: 90vw;
    max-height: 80vh;
    overflow: auto;
}

.theme-picker-header {
    padding: var(--sp-4) var(--sp-5);
    font-weight: 600;
    border-bottom: 1px solid var(--border-subtle);
    color: var(--text-0);
    display: flex;
    align-items: center;
    justify-content: space-between;
}

.theme-picker-close {
    background: none;
    border: none;
    color: var(--text-3);
    font-size: 1.2rem;
    cursor: pointer;
    padding: 2px 6px;
    border-radius: var(--radius);
    line-height: 1;
}
.theme-picker-close:hover {
    color: var(--text-0);
    background: var(--bg-3);
}

.theme-picker-grid {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: var(--sp-2);
    padding: var(--sp-4);
}

@media (max-width: 480px) {
    .theme-picker-grid { grid-template-columns: repeat(3, 1fr); }
}

.theme-picker-item {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: var(--sp-2);
    padding: var(--sp-2);
    border-radius: var(--radius);
    cursor: pointer;
    transition: background var(--transition-fast);
    border: 2px solid transparent;
}

.theme-picker-item:hover {
    background: var(--bg-3);
}

.theme-picker-item.active {
    border-color: var(--accent);
    background: var(--accent-subtle);
}

.theme-swatch-preview {
    width: 100%;
    aspect-ratio: 16 / 10;
    border-radius: 3px;
    display: flex;
    align-items: center;
    justify-content: center;
    font-family: var(--font-mono);
    font-size: var(--font-size-sm);
    font-weight: 700;
    border: 1px solid rgba(128, 128, 128, 0.2);
}

.theme-name {
    font-size: var(--font-size-xs);
    color: var(--text-2);
    text-align: center;
    line-height: 1.2;
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
