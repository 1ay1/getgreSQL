#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct LayoutCSS {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
/* ─── IDE Layout Grid ─────────────────────────────────────────────────── */

.ide {
    display: grid;
    grid-template-rows: var(--toolbar-height) 1fr var(--status-bar-height);
    grid-template-columns: var(--sidebar-width) 1fr;
    grid-template-areas:
        "toolbar toolbar"
        "sidebar workspace"
        "statusbar statusbar";
    height: 100vh;
    width: 100vw;
}

/* ─── Toolbar ─────────────────────────────────────────────────────────── */

.toolbar {
    grid-area: toolbar;
    background: var(--bg-1);
    border-bottom: 1px solid var(--border);
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.2);
    display: flex;
    align-items: center;
    padding: 0 var(--sp-4);
    gap: var(--sp-3);
    z-index: var(--z-fixed);
    overflow: hidden;
    min-width: 0;
}

.toolbar-brand {
    font-family: var(--font-mono);
    font-weight: 700;
    font-size: var(--font-size-md);
    color: var(--text-0);
    letter-spacing: -0.02em;
    white-space: nowrap;
    user-select: none;
}

.toolbar-brand .version {
    font-weight: 400;
    font-size: var(--font-size-xs);
    color: var(--text-3);
    margin-left: var(--sp-2);
}

.toolbar-sep {
    width: 1px;
    height: 16px;
    background: var(--border);
}

.toolbar-nav {
    display: flex;
    gap: 1px;
    -webkit-app-region: no-drag;
}

.toolbar-btn {
    display: flex;
    align-items: center;
    gap: var(--sp-2);
    padding: 4px 10px;
    border: none;
    border-radius: var(--radius);
    background: transparent;
    color: var(--text-2);
    font-size: var(--font-size-sm);
    font-family: var(--font-sans);
    cursor: pointer;
    white-space: nowrap;
    transition: all var(--transition-fast);
    text-decoration: none;
}
.toolbar-btn:hover {
    background: var(--bg-3);
    color: var(--text-0);
    text-decoration: none;
}
.toolbar-btn:active {
    background: var(--bg-4);
    transform: scale(0.97);
}
.toolbar-btn:focus-visible {
    outline: 2px solid var(--accent);
    outline-offset: 1px;
}
.toolbar-btn.active {
    background: var(--accent-subtle);
    color: var(--accent);
}
.toolbar-btn .icon {
    font-size: 1rem;
    line-height: 1;
    opacity: 0.7;
}
.toolbar-btn.active .icon { opacity: 1; }

.toolbar-spacer { flex: 1; }

.toolbar-conn {
    display: flex;
    align-items: center;
    gap: var(--sp-2);
    padding: var(--sp-1) var(--sp-3);
    border-radius: var(--radius);
    font-size: var(--font-size-xs);
    color: var(--text-2);
    font-family: var(--font-mono);
    cursor: pointer;
    transition: background var(--transition-fast);
    text-decoration: none;
    white-space: nowrap;
    -webkit-app-region: no-drag;
}
.toolbar-conn:hover {
    background: var(--bg-3);
    color: var(--text-0);
    text-decoration: none;
}
.toolbar-conn .conn-label { font-weight: 600; }

.conn-dot {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--success);
    box-shadow: 0 0 6px var(--success), 0 0 12px rgba(63, 185, 80, 0.3);
}

.toolbar-actions {
    display: flex;
    gap: var(--sp-1);
    -webkit-app-region: no-drag;
}

.toolbar-icon-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 28px;
    height: 28px;
    border: none;
    border-radius: var(--radius);
    background: transparent;
    color: var(--text-2);
    cursor: pointer;
    font-size: 1rem;
    transition: all var(--transition-fast);
}
.toolbar-icon-btn:hover {
    background: var(--bg-3);
    color: var(--text-0);
}
.toolbar-icon-btn:active {
    transform: scale(0.92);
}
.toolbar-icon-btn:focus-visible {
    outline: 2px solid var(--accent);
    outline-offset: 1px;
}

/* ─── Object Explorer (Sidebar) ───────────────────────────────────────── */

.sidebar {
    grid-area: sidebar;
    background: var(--bg-1);
    border-right: 1px solid var(--border);
    display: flex;
    flex-direction: column;
    overflow: hidden;
    min-width: var(--sidebar-min);
    position: relative;
    z-index: var(--z-sidebar);
}

.sidebar-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: var(--sp-3) var(--sp-4);
    border-bottom: 1px solid var(--border-subtle);
    min-height: 40px;
    gap: var(--sp-2);
    background: var(--bg-1);
}

.sidebar-title {
    font-size: var(--font-size-xs);
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.08em;
    color: var(--text-2);
    user-select: none;
    flex-shrink: 0;
}

.sidebar-actions {
    display: flex;
    gap: 2px;
}

.sidebar-icon {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 26px;
    height: 26px;
    border: none;
    border-radius: var(--radius);
    background: transparent;
    color: var(--text-3);
    cursor: pointer;
    font-size: 0.85rem;
    transition: all var(--transition-fast);
}
.sidebar-icon:hover {
    background: var(--bg-3);
    color: var(--text-0);
}
.sidebar-icon:active {
    transform: scale(0.92);
}

/* Sidebar search */
.sidebar-search {
    padding: var(--sp-2) var(--sp-3);
    border-bottom: 1px solid var(--border-subtle);
}
.sidebar-search-input {
    width: 100%;
    padding: 5px 8px 5px 28px;
    background: var(--bg-0);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius);
    color: var(--text-1);
    font-size: var(--font-size-xs);
    font-family: var(--font-sans);
    outline: none;
    transition: border-color var(--transition-fast), box-shadow var(--transition-fast), background var(--transition-fast);
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='%236e7681' stroke-width='2'%3E%3Ccircle cx='11' cy='11' r='8'/%3E%3Cline x1='21' y1='21' x2='16.65' y2='16.65'/%3E%3C/svg%3E");
    background-repeat: no-repeat;
    background-position: 7px center;
}
.sidebar-search-input:focus {
    border-color: var(--accent);
    box-shadow: 0 0 0 2px var(--accent-subtle);
    background-color: color-mix(in srgb, var(--bg-0) 90%, var(--accent-subtle));
}
.sidebar-search-input::placeholder { color: var(--text-4); }

.sidebar-tree {
    flex: 1;
    overflow-y: auto;
    overflow-x: hidden;
    padding: var(--sp-2) 0;
    scrollbar-width: thin;
    scrollbar-color: var(--bg-4) transparent;
}

.sidebar-tree::-webkit-scrollbar { width: 5px; }
.sidebar-tree::-webkit-scrollbar-track { background: transparent; }
.sidebar-tree::-webkit-scrollbar-thumb {
    background: var(--bg-4);
    border-radius: 3px;
}
.sidebar-tree::-webkit-scrollbar-thumb:hover { background: var(--text-4); }

/* ─── Tree View ───────────────────────────────────────────────────────── */

.tree { list-style: none; margin: 0; padding: 0; }

.tree-item {
    user-select: none;
    position: relative;
}

/* Indent guide lines — subtle vertical connectors showing hierarchy */
.tree-children {
    list-style: none;
    margin: 0;
    padding: 0;
    overflow: hidden;
    position: relative;
}
.tree-children::before {
    content: '';
    position: absolute;
    left: calc(var(--sp-3) + var(--guide-depth, 0) * 14px + 6px);
    top: 0;
    bottom: 8px;
    width: 1px;
    background: var(--border-subtle);
    pointer-events: none;
    z-index: 1;
}

.tree-row {
    display: flex;
    align-items: center;
    padding: 0 var(--sp-3);
    padding-left: calc(var(--sp-3) + var(--tree-depth, 0) * 14px);
    cursor: pointer;
    color: var(--text-1);
    font-size: var(--font-size-xs);
    transition: background 0.1s ease, color 0.1s ease, padding-left 0.15s ease;
    min-height: 26px;
    gap: 5px;
    text-decoration: none;
    margin: 0 var(--sp-1);
    border-radius: var(--radius);
    position: relative;
}
.tree-row:hover {
    background: var(--bg-2);
    text-decoration: none;
}
.tree-row:active {
    background: var(--bg-3);
}
.tree-row:focus-visible {
    outline: 2px solid var(--accent);
    outline-offset: -2px;
}
.tree-row.selected {
    background: var(--accent-subtle);
    color: var(--text-0);
    position: relative;
}
.tree-row.selected::before {
    content: '';
    position: absolute;
    left: 0;
    top: 2px;
    bottom: 2px;
    width: 2px;
    background: var(--accent);
    border-radius: 0 2px 2px 0;
}
.tree-row.selected:hover {
    background: color-mix(in srgb, var(--accent-subtle) 80%, var(--bg-2));
}

/* Active row when navigating (SPA) */
.tree-row[aria-current="page"],
.tree-row.current {
    background: var(--accent-subtle);
    color: var(--accent);
    font-weight: 600;
}

.tree-chevron {
    width: 16px;
    height: 16px;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 8px;
    color: var(--text-4);
    transition: transform 0.15s cubic-bezier(0.4, 0, 0.2, 1), color 0.1s;
    flex-shrink: 0;
    border-radius: 3px;
}
.tree-row:hover .tree-chevron {
    color: var(--text-2);
    background: rgba(255,255,255,0.04);
}
.tree-chevron.expanded { transform: rotate(90deg); }
.tree-chevron.empty { visibility: hidden; width: 16px; }

.tree-icon {
    width: 18px;
    height: 18px;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 11px;
    flex-shrink: 0;
    color: var(--text-3);
    transition: color 0.1s, transform 0.1s;
    border-radius: 3px;
}
.tree-row:hover .tree-icon {
    filter: brightness(1.15);
    transform: scale(1.05);
}

/* Icon colors — each type gets a distinct, saturated hue */
.tree-icon.db       { color: #f0b429; }
.tree-icon.schema   { color: #a78bfa; }
.tree-icon.table    { color: #58a6ff; }
.tree-icon.view     { color: #bc85fa; }
.tree-icon.func     { color: #fb923c; }
.tree-icon.seq      { color: #22d3ee; }
.tree-icon.idx      { color: #3fb950; }
.tree-icon.role     { color: #f472b6; }
.tree-icon.ext      { color: #a78bfa; }
.tree-icon.settings { color: var(--text-3); }
.tree-icon.folder   { color: var(--text-3); }
.tree-icon.monitor  { color: #d29922; }

.tree-text {
    flex: 1;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    line-height: 1.4;
    font-family: var(--font-sans);
}

.tree-badge {
    font-size: 0.6rem;
    color: var(--text-4);
    font-family: var(--font-mono);
    padding: 1px 5px;
    flex-shrink: 0;
    background: var(--bg-2);
    border-radius: 3px;
    opacity: 0.6;
    transition: opacity 0.15s, color 0.15s;
}
.tree-row:hover .tree-badge { opacity: 1; color: var(--text-2); }
.tree-row.selected .tree-badge { opacity: 0.8; }

.tree-group-label {
    display: flex;
    align-items: center;
    padding: 1px var(--sp-3);
    padding-left: calc(var(--sp-3) + var(--tree-depth, 0) * 14px);
    font-size: 0.6rem;
    font-weight: 700;
    color: var(--text-4);
    text-transform: uppercase;
    letter-spacing: 0.06em;
    margin-top: var(--sp-2);
    cursor: default;
    min-height: 20px;
    gap: var(--sp-2);
}

.tree-empty-text { color: var(--text-4); font-style: italic; }

.tree-separator {
    height: 0;
    border-top: 1px solid var(--border-subtle);
    margin: var(--sp-3) var(--sp-4);
    opacity: 0.6;
}

/* ─── Resize Handle ───────────────────────────────────────────────────── */

.resize-handle {
    position: absolute;
    top: 0;
    right: -3px;
    width: 6px;
    height: 100%;
    cursor: col-resize;
    z-index: var(--z-resize);
    transition: background var(--transition-fast), opacity var(--transition-fast);
}
.resize-handle:hover,
.resize-handle.dragging {
    background: var(--accent);
    opacity: 0.5;
}

/* ─── Workspace ───────────────────────────────────────────────────────── */

.workspace {
    grid-area: workspace;
    display: flex;
    flex-direction: column;
    overflow: hidden;
    background: var(--bg-0);
}

/* ─── Tab Bar ─────────────────────────────────────────────────────────── */

.tab-bar {
    display: flex;
    align-items: stretch;
    background: var(--bg-1);
    border-bottom: 1px solid var(--border);
    min-height: var(--tab-bar-height);
    max-height: var(--tab-bar-height);
    overflow-x: auto;
    overflow-y: hidden;
}

.tab-bar::-webkit-scrollbar { height: 0; }

.tab {
    display: flex;
    align-items: center;
    gap: var(--sp-2);
    padding: 0 var(--sp-4);
    border-right: 1px solid var(--border-subtle);
    color: var(--text-2);
    font-size: var(--font-size-sm);
    cursor: pointer;
    white-space: nowrap;
    min-width: 0;
    max-width: 180px;
    transition: all var(--transition-fast);
    text-decoration: none;
    background: var(--bg-2);
    position: relative;
}
.tab:hover {
    color: var(--text-0);
    background: var(--bg-3);
    text-decoration: none;
}
.tab.active {
    background: var(--bg-0);
    color: var(--text-0);
    border-bottom: 3px solid var(--accent);
    margin-bottom: -1px;
    box-shadow: inset 0 0 12px rgba(56, 139, 253, 0.04);
}
.tab .tab-icon {
    font-size: 11px;
    opacity: 0.6;
}
.tab.active .tab-icon { opacity: 1; }
.tab-label {
    overflow: hidden;
    text-overflow: ellipsis;
}

.tab-close {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 18px;
    height: 18px;
    border: none;
    border-radius: var(--radius);
    background: transparent;
    color: var(--text-3);
    cursor: pointer;
    font-size: 12px;
    margin-left: var(--sp-1);
    opacity: 0;
    transition: all var(--transition-fast);
}
.tab:hover .tab-close { opacity: 1; }
.tab-close:hover { background: var(--bg-4); color: var(--text-0); }

.tab-bar-end {
    flex: 1;
    background: var(--bg-2);
    border-bottom: 1px solid var(--border);
    min-width: 20px;
}

/* ─── Content Area ────────────────────────────────────────────────────── */

.content {
    flex: 1;
    overflow-y: auto;
    overflow-x: hidden;
    padding: var(--sp-5);
}

.content::-webkit-scrollbar { width: 8px; }
.content::-webkit-scrollbar-track { background: transparent; }
.content::-webkit-scrollbar-thumb { background: var(--bg-4); border-radius: 4px; }
.content::-webkit-scrollbar-thumb:hover { background: var(--text-4); }

.content-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: var(--sp-5);
    min-height: 28px;
}

.content-header h2 {
    font-size: var(--font-size-lg);
    font-weight: 600;
    color: var(--text-0);
    display: flex;
    align-items: center;
    gap: var(--sp-3);
}

.content-header .header-actions {
    display: flex;
    gap: var(--sp-2);
}

h3 {
    font-size: var(--font-size-md);
    font-weight: 600;
    color: var(--text-0);
    margin: var(--sp-5) 0 var(--sp-3);
    display: flex;
    align-items: center;
    gap: var(--sp-2);
}

h4 {
    font-size: var(--font-size-sm);
    font-weight: 700;
    color: var(--text-2);
    text-transform: uppercase;
    letter-spacing: 0.04em;
    padding-bottom: var(--sp-2);
    border-bottom: 1px solid var(--border-subtle);
    margin-bottom: var(--sp-3);
    margin-top: var(--sp-5);
}

/* ─── Status Bar ──────────────────────────────────────────────────────── */

.status-bar {
    grid-area: statusbar;
    background: var(--accent-dim);
    display: flex;
    align-items: center;
    padding: 0 var(--sp-4);
    font-size: var(--font-size-xs);
    color: rgba(255, 255, 255, 0.8);
    gap: var(--sp-4);
    z-index: var(--z-fixed);
}

.status-item {
    display: flex;
    align-items: center;
    gap: var(--sp-2);
    white-space: nowrap;
}

.status-sep {
    width: 1px;
    height: 14px;
    background: rgba(255, 255, 255, 0.2);
}

.status-spacer { flex: 1; }

.status-bar a {
    color: rgba(255, 255, 255, 0.9);
    text-decoration: none;
}
.status-bar a:hover {
    color: #fff;
    text-decoration: underline;
}

)_CSS_"; }

};

} // namespace getgresql::ssr
