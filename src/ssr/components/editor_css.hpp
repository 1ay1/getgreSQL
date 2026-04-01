#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct EditorCSS {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
/* ─── Query Editor ────────────────────────────────────────────────────── */

.query-panel {
    display: flex;
    flex-direction: column;
    flex: 1;
    min-height: 0;
    overflow: hidden;
}

.query-editor {
    display: flex;
    flex-direction: column;
    height: 45%;
    min-height: 120px;
    border-bottom: 2px solid var(--border);
    flex-shrink: 0;
    position: relative;
}

.editor-container {
    display: flex;
    flex: 1;
    min-height: 0;
    overflow: hidden;
    background: var(--bg-1);
}

.editor-gutter {
    width: 50px;
    min-width: 50px;
    background: var(--ed-gutter-bg, var(--bg-2));
    border-right: 1px solid var(--border-subtle);
    padding: var(--sp-4) 0;
    font-family: var(--font-mono);
    font-size: var(--ed-font-size, var(--font-size-sm));
    color: var(--ed-gutter-fg, var(--text-4));
    text-align: right;
    line-height: 1.6;
    user-select: none;
    overflow: hidden;
}

.editor-gutter .line-num {
    padding-right: var(--sp-3);
    display: block;
    transition: color 0.05s;
}

.editor-gutter .line-num.active {
    color: var(--ed-gutter-active, var(--text-0));
    font-weight: 600;
}

/* Old textarea-only editor (non-JS pages) */
.sql-input {
    flex: 1;
    background: var(--bg-1);
    color: var(--text-0);
    border: none;
    padding: var(--sp-4);
    resize: none;
    outline: none;
    font-family: var(--font-mono);
    font-size: var(--font-size-sm);
    line-height: 1.6;
    tab-size: 4;
    -moz-tab-size: 4;
    white-space: pre;
    overflow-wrap: normal;
    overflow-x: auto;
}
.sql-input:focus { background: var(--bg-1); }
.sql-input::placeholder { color: var(--text-4); }

.editor-toolbar {
    display: flex;
    align-items: center;
    gap: var(--sp-3);
    padding: var(--sp-2) var(--sp-3);
    background: var(--bg-2);
    border-top: 1px solid var(--border-subtle);
    flex-shrink: 0;
    overflow-x: auto;
}

.editor-toolbar .btn .shortcut {
    font-size: var(--font-size-xs);
    color: var(--text-3);
    margin-left: var(--sp-2);
}

.query-results {
    flex: 1;
    min-height: 0;
    overflow: auto;
    background: var(--bg-0);
}

.query-results .table-wrapper {
    margin: 0;
    border-radius: 0;
    border: none;
    border-top: none;
    box-shadow: none;
}

.query-info {
    display: flex;
    align-items: center;
    gap: var(--sp-4);
    padding: var(--sp-2) var(--sp-4);
    background: var(--bg-2);
    border-bottom: 1px solid var(--border-subtle);
    color: var(--text-2);
    font-size: var(--font-size-xs);
    font-family: var(--font-mono);
    flex-shrink: 0;
}

.query-info .rows-badge {
    color: var(--success);
    font-weight: 600;
}

.query-info .time-badge {
    color: var(--text-2);
}

.query-error {
    padding: var(--sp-4);
    background: var(--danger-subtle);
    border: 1px solid rgba(248, 81, 73, 0.3);
    border-radius: var(--radius);
    color: var(--danger);
    font-family: var(--font-mono);
    font-size: var(--font-size-sm);
    white-space: pre-wrap;
    margin: var(--sp-4);
}

/* Resize handle for query editor */
.editor-resize {
    height: 5px;
    background: var(--border);
    cursor: ns-resize;
    transition: background var(--transition-fast);
    flex-shrink: 0;
}
.editor-resize:hover,
.editor-resize.dragging {
    background: var(--accent);
}

/* ─── File drop overlay ──────────────────────────────────────────────── */

.editor-drop-overlay {
    position: absolute;
    inset: 0;
    background: rgba(56, 139, 253, 0.12);
    border: 2px dashed var(--accent);
    border-radius: var(--radius);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 100;
    pointer-events: none;
}

.editor-drop-message {
    padding: var(--sp-4) var(--sp-5);
    background: var(--bg-2);
    border-radius: var(--radius);
    font-size: var(--font-size-sm);
    font-weight: 600;
    color: var(--accent);
    box-shadow: var(--shadow);
}

/* Breadcrumbs: co-located in breadcrumbs.hpp */
/* Alerts: co-located in alert.hpp */

/* Alert: co-located in src/ssr/components/alert.hpp */

/* SectionTabs: co-located in src/ssr/components/section_tabs.hpp */

/* ─── Schema navigation ──────────────────────────────────────────────── */

.schema-nav {
    display: flex;
    gap: var(--sp-2);
    margin-bottom: var(--sp-4);
}

/* ─── Null values ─────────────────────────────────────────────────────── */

.null-value {
    color: var(--text-4);
    font-style: italic;
    font-size: var(--font-size-xs);
    opacity: 0.4;
}

/* ─── Empty state ─────────────────────────────────────────────────────── */

.empty-state {
    text-align: center;
    padding: var(--sp-6) var(--sp-5);
    color: var(--text-3);
    font-size: var(--font-size-sm);
}

.empty-state .empty-icon {
    font-size: 2.5rem;
    margin-bottom: var(--sp-4);
    opacity: 0.25;
    filter: grayscale(0.3);
}

.empty-state p {
    max-width: 320px;
    margin: 0 auto;
    line-height: 1.6;
}

/* ─── Loading / HTMX ─────────────────────────────────────────────────── */

.loading {
    text-align: center;
    padding: var(--sp-6);
    color: var(--text-3);
    font-size: var(--font-size-sm);
    display: flex;
    align-items: center;
    justify-content: center;
    gap: var(--sp-3);
}

.loading::before {
    content: '';
    display: inline-block;
    width: 16px;
    height: 16px;
    border: 2px solid var(--border);
    border-top-color: var(--accent);
    border-radius: 50%;
    animation: spin 0.6s linear infinite;
}

@keyframes spin { to { transform: rotate(360deg); } }

@keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.4; }
}

@keyframes skeleton-shimmer {
    0% { background-position: -200% 0; }
    100% { background-position: 200% 0; }
}

.htmx-indicator {
    display: none;
    color: var(--text-3);
    font-size: var(--font-size-sm);
    animation: pulse 1.2s ease-in-out infinite;
}
.htmx-request .htmx-indicator { display: inline; }
.htmx-request.htmx-indicator { display: inline; }

/* Skeleton loading placeholder */
.skeleton {
    background: linear-gradient(90deg, var(--bg-2) 25%, var(--bg-3) 50%, var(--bg-2) 75%);
    background-size: 200% 100%;
    animation: skeleton-shimmer 1.5s ease-in-out infinite;
    border-radius: var(--radius);
}

/* ─── Server info ─────────────────────────────────────────────────────── */

.server-info {
    margin-bottom: var(--sp-4);
}
.server-info code {
    color: var(--text-3);
    font-size: var(--font-size-xs);
    background: var(--bg-2);
    padding: 2px 6px;
    border-radius: var(--radius);
}

.table-info {
    font-size: var(--font-size-xs);
    color: var(--text-4);
    padding: var(--sp-2) 0;
    font-family: var(--font-mono);
}

/* ProgressBar: co-located in progress_bar.hpp */

/* ─── EXPLAIN plan ────────────────────────────────────────────────────── */

.explain-plan {
    background: var(--bg-1);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius);
    padding: var(--sp-4);
    overflow-x: auto;
    font-family: var(--font-mono);
    font-size: var(--font-size-xs);
    line-height: 1.7;
    color: var(--text-1);
    white-space: pre;
}

.explain-timing {
    display: flex;
    gap: var(--sp-5);
    margin-top: var(--sp-3);
    padding: var(--sp-2) 0;
    font-size: var(--font-size-sm);
    color: var(--text-2);
}
.explain-timing strong { color: var(--text-0); }

/* ─── Function source ─────────────────────────────────────────────────── */

.function-source {
    background: var(--bg-1);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
    padding: var(--sp-4);
    overflow-x: auto;
    font-family: var(--font-mono);
    font-size: var(--font-size-xs);
    line-height: 1.7;
    white-space: pre;
    color: var(--text-1);
    max-height: 500px;
    overflow-y: auto;
}

/* ─── Settings categories ─────────────────────────────────────────────── */

.settings-category { margin-top: var(--sp-5); }

.setting-value {
    font-family: var(--font-mono);
    font-size: var(--font-size-sm);
    color: var(--text-0);
}

.setting-unit {
    color: var(--text-4);
    font-size: var(--font-size-xs);
    margin-left: var(--sp-1);
}

/* ─── Warning rows ────────────────────────────────────────────────────── */

.row-warning td { background: var(--warning-subtle); }

/* SizeBar: co-located in size_bar.hpp */
/* SearchInput: co-located in search_input.hpp */

/* ─── Command palette ─────────────────────────────────────────────────── */

.command-overlay {
    position: fixed;
    inset: 0;
    background: rgba(0, 0, 0, 0.5);
    z-index: 1000;
    display: flex;
    justify-content: center;
    padding-top: 15vh;
    backdrop-filter: blur(2px);
}

.command-palette {
    width: 520px;
    max-height: 400px;
    background: var(--bg-1);
    border: 1px solid var(--border);
    border-radius: var(--radius-lg);
    box-shadow: 0 16px 48px rgba(0, 0, 0, 0.4);
    overflow: hidden;
    display: flex;
    flex-direction: column;
}

.command-input {
    padding: var(--sp-4);
    background: transparent;
    border: none;
    border-bottom: 1px solid var(--border);
    color: var(--text-0);
    font-size: var(--font-size-md);
    font-family: var(--font-sans);
    outline: none;
    width: 100%;
}

.command-list {
    flex: 1;
    overflow-y: auto;
    padding: var(--sp-2);
}

.command-item {
    display: flex;
    align-items: center;
    gap: var(--sp-3);
    padding: var(--sp-2) var(--sp-3);
    border-radius: var(--radius);
    color: var(--text-1);
    font-size: var(--font-size-sm);
    cursor: pointer;
    text-decoration: none;
    transition: background var(--transition-fast);
}
.command-item:hover, .command-item.selected {
    background: var(--accent-subtle);
    color: var(--accent);
    text-decoration: none;
}
.command-item .icon { color: var(--text-3); width: 20px; text-align: center; }
.command-item .shortcut {
    margin-left: auto;
    color: var(--text-4);
    font-size: var(--font-size-xs);
}

/* ─── Inline code ─────────────────────────────────────────────────────── */

code {
    background: var(--bg-2);
    padding: 1px 4px;
    border-radius: 3px;
    font-size: 0.9em;
}
pre code {
    background: none;
    padding: 0;
}

/* ─── Schema selector ─────────────────────────────────────────────────── */

.schema-selector {
    display: flex;
    align-items: center;
    gap: var(--sp-3);
    margin-bottom: var(--sp-4);
    font-size: var(--font-size-sm);
}
.schema-selector label {
    color: var(--text-2);
    font-weight: 600;
}
.schema-selector input {
    padding: var(--sp-2) var(--sp-3);
    background: var(--bg-1);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    color: var(--text-0);
    font-size: var(--font-size-sm);
    outline: none;
    width: 160px;
    transition: border-color var(--transition-fast), box-shadow var(--transition-fast);
}
.schema-selector input:focus {
    border-color: var(--accent);
    box-shadow: 0 0 0 3px var(--accent-subtle);
}

/* ─── Dashboard specific ──────────────────────────────────────────────── */

.dashboard-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: var(--sp-5);
}

@media (max-width: 900px) {
    .dashboard-grid {
        grid-template-columns: 1fr;
    }
}

.dashboard-section {
    background: var(--bg-1);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
    overflow: hidden;
    box-shadow: var(--shadow-sm);
}

.dashboard-section-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: var(--sp-3) var(--sp-4);
    border-bottom: 1px solid var(--border-subtle);
    font-size: var(--font-size-sm);
    font-weight: 600;
    color: var(--text-0);
}

.dashboard-section-body {
    padding: var(--sp-3);
}

.dashboard-section table {
    font-size: var(--font-size-xs);
}
.dashboard-section td {
    padding: var(--sp-1) var(--sp-3);
}
.dashboard-section th {
    padding: var(--sp-1) var(--sp-3);
    text-transform: none;
    letter-spacing: normal;
}

/* ─── Query preview (in monitoring tables) ────────────────────────────── */

.query-preview {
    font-family: var(--font-mono);
    font-size: var(--font-size-xs);
    color: var(--text-2);
    max-width: 300px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    display: block;
}

/* ─── Responsive (mobile) ─────────────────────────────────────────────── */

@media (max-width: 768px) {
    .ide {
        grid-template-columns: 1fr;
        grid-template-areas:
            "toolbar"
            "workspace"
            "statusbar";
    }
    .sidebar { display: none; }
    .content { padding: var(--sp-3); }
    .stat-grid { grid-template-columns: repeat(2, 1fr); }
}

/* ─── Sidebar collapsed state ─────────────────────────────────────────── */

.ide.sidebar-collapsed {
    grid-template-columns: 0px 1fr;
}
.ide.sidebar-collapsed .sidebar {
    width: 0;
    min-width: 0;
    opacity: 0;
    pointer-events: none;
    overflow: hidden;
    border-right: none;
    transition: width 0.2s ease, opacity 0.15s ease, min-width 0.2s ease;
}
/* Smooth transition for expand too */
.sidebar {
    transition: width 0.2s ease, opacity 0.15s ease, min-width 0.2s ease;
}
.ide.sidebar-collapsed {
    transition: grid-template-columns 0.2s ease;
}
.ide {
    transition: grid-template-columns 0.2s ease;
}

/* ═══════════════════════════════════════════════════════════════════════
   Transitions & Animations — SPA feel
   ═══════════════════════════════════════════════════════════════════════ */

/* SPA page transitions */
.spa-exit {
    opacity: 0;
    transform: translateY(4px);
    transition: opacity 0.12s ease-out, transform 0.12s ease-out;
}

.spa-enter {
    opacity: 0;
    transform: translateY(8px);
}

.content {
    transition: opacity 0.15s ease-out, transform 0.15s ease-out;
}

/* htmx swap transitions */
.htmx-swapping { opacity: 0; transition: opacity 0.1s ease-out; }
.htmx-settling { transition: opacity 0.15s ease-in; }
.htmx-added { opacity: 0; animation: fadeSlideIn 0.2s ease-out forwards; }

@keyframes fadeSlideIn {
    from { opacity: 0; transform: translateY(6px); }
    to { opacity: 1; transform: translateY(0); }
}

/* Modal transitions */
.command-overlay {
    animation: overlayFadeIn 0.15s ease-out;
}
@keyframes overlayFadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
}

.command-palette, .settings-panel, .cell-modal {
    animation: modalSlideIn 0.2s cubic-bezier(0.16, 1, 0.3, 1);
}
@keyframes modalSlideIn {
    from { opacity: 0; transform: translateY(-10px) scale(0.97); }
    to { opacity: 1; transform: translateY(0) scale(1); }
}

/* Tree expand/collapse */
.tree-children {
    overflow: hidden;
    animation: treeExpand 0.15s ease-out;
}
@keyframes treeExpand {
    from { opacity: 0; max-height: 0; }
    to { opacity: 1; max-height: 2000px; }
}

.tree-chevron {
    transition: transform 0.15s ease-out;
}

/* Tab content swap */
#tab-content, #monitor-content {
    animation: fadeSlideIn 0.15s ease-out;
}

/* Stat card entrance */
.stat-grid .stat-card {
    animation: fadeSlideIn 0.2s ease-out backwards;
}
.stat-grid .stat-card:nth-child(1) { animation-delay: 0s; }
.stat-grid .stat-card:nth-child(2) { animation-delay: 0.03s; }
.stat-grid .stat-card:nth-child(3) { animation-delay: 0.06s; }
.stat-grid .stat-card:nth-child(4) { animation-delay: 0.09s; }
.stat-grid .stat-card:nth-child(5) { animation-delay: 0.12s; }
.stat-grid .stat-card:nth-child(6) { animation-delay: 0.15s; }
.stat-grid .stat-card:nth-child(7) { animation-delay: 0.18s; }
.stat-grid .stat-card:nth-child(8) { animation-delay: 0.21s; }

/* Health card entrance */
.health-grid .health-card {
    animation: fadeSlideIn 0.2s ease-out backwards;
}
.health-grid .health-card:nth-child(1) { animation-delay: 0s; }
.health-grid .health-card:nth-child(2) { animation-delay: 0.04s; }
.health-grid .health-card:nth-child(3) { animation-delay: 0.08s; }
.health-grid .health-card:nth-child(4) { animation-delay: 0.12s; }
.health-grid .health-card:nth-child(5) { animation-delay: 0.16s; }
.health-grid .health-card:nth-child(6) { animation-delay: 0.2s; }

/* Table row entrance (subtle) */
tbody tr {
    animation: rowFadeIn 0.1s ease-out backwards;
}
@keyframes rowFadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
}

/* Alert entrance */
.alert {
    animation: fadeSlideIn 0.2s ease-out;
}

/* Autocomplete popup */
.ac-popup {
    animation: acPopIn 0.1s ease-out;
}
@keyframes acPopIn {
    from { opacity: 0; transform: translateY(-4px); }
    to { opacity: 1; transform: translateY(0); }
}

/* Smooth scrolling in content areas */
.content, .sidebar-tree, .query-results {
    scroll-behavior: smooth;
}

/* Progress bar animation */
.progress-bar, .size-bar-fill, .explain-cost-fill, .col-stat-bar {
    transition: width 0.4s cubic-bezier(0.16, 1, 0.3, 1);
}

/* Badge pop-in */
.badge {
    animation: badgeIn 0.15s ease-out;
}
@keyframes badgeIn {
    from { transform: scale(0.8); opacity: 0; }
    to { transform: scale(1); opacity: 1; }
}

/* Cell save flash (smoother) */
@keyframes cellSaveFlash {
    0% { background: rgba(63, 185, 80, 0.3); box-shadow: 0 0 8px rgba(63, 185, 80, 0.2); }
    100% { background: transparent; box-shadow: none; }
}

/* ═══════════════════════════════════════════════════════════════════════
   SQL Editor — syntax highlighting, autocomplete, tabs, find bar
   ═══════════════════════════════════════════════════════════════════════ */

/* ─── Editor tab bar (query tabs) ─────────────────────────────────────── */

.editor-tab-bar {
    display: flex;
    align-items: stretch;
    background: var(--bg-2);
    border-bottom: 1px solid var(--border);
    height: 30px;
    min-height: 30px;
    overflow-x: auto;
    flex-shrink: 0;
}
.editor-tab-bar::-webkit-scrollbar { height: 0; }

.editor-tab {
    display: flex;
    align-items: center;
    gap: var(--sp-2);
    padding: 0 var(--sp-4);
    border-right: 1px solid var(--border-subtle);
    color: var(--text-2);
    font-size: var(--font-size-xs);
    cursor: pointer;
    white-space: nowrap;
    background: var(--bg-2);
    transition: all var(--transition-fast);
    min-height: 30px;
}
.editor-tab:hover { background: var(--bg-3); color: var(--text-0); }
.editor-tab.active {
    background: var(--bg-1);
    color: var(--text-0);
    border-bottom: 3px solid var(--accent);
    box-shadow: inset 0 0 12px rgba(56, 139, 253, 0.04);
}
.editor-tab .tab-icon { font-size: 10px; color: var(--success); opacity: 0.6; }
.editor-tab.active .tab-icon { opacity: 1; }
.editor-tab .tab-close {
    font-size: 14px;
    line-height: 1;
    padding: 0 2px;
    border-radius: 2px;
    opacity: 0;
    color: var(--text-3);
    transition: opacity var(--transition-fast);
}
.editor-tab:hover .tab-close { opacity: 0.6; }
.editor-tab .tab-close:hover { opacity: 1; background: var(--bg-4); color: var(--text-0); }

.editor-tab-add {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 28px;
    color: var(--text-3);
    cursor: pointer;
    font-size: 16px;
    flex-shrink: 0;
    transition: all var(--transition-fast);
}
.editor-tab-add:hover { background: var(--bg-3); color: var(--text-0); }

/* ─── Editor area (textarea + highlight overlay) ──────────────────────── */

/* The editor-area is the container for the overlay approach:
   - Background color lives HERE
   - Textarea is below (z-index 1): transparent bg, transparent text, visible caret
   - Highlight pre is above (z-index 3): colored text, pointer-events:none
   - Current line, indent guides at z-index 2 */

.editor-area {
    position: relative;
    flex: 1;
    min-height: 0;
    min-width: 0;
    overflow: hidden;
    background: var(--ed-bg, var(--bg-1));
}

.editor-highlight,
.editor-input {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    margin: 0;
    padding: var(--sp-4);
    font-family: var(--font-mono);
    font-size: var(--ed-font-size, var(--font-size-sm));
    line-height: 1.6;
    border: none;
    white-space: pre-wrap;
    word-wrap: break-word;
    overflow: auto;
    tab-size: var(--ed-tab-size, 4);
    -moz-tab-size: var(--ed-tab-size, 4);
    box-sizing: border-box;
}

.has-minimap .editor-highlight,
.has-minimap .editor-input {
    width: calc(100% - 64px);
}

/* Textarea: BELOW the highlight. Captures all input. Text invisible. */
.editor-input {
    color: transparent;
    caret-color: var(--ed-cursor, var(--text-0));
    background: transparent;
    outline: none;
    resize: none;
    z-index: 1;
    -webkit-text-fill-color: transparent;
}
.editor-input::selection {
    background: var(--ed-selection, rgba(56,139,253,0.3));
    -webkit-text-fill-color: transparent;
}
.editor-input::placeholder {
    color: var(--text-4);
    -webkit-text-fill-color: var(--text-4);
}

/* Block cursor mode */
.editor-area.block-cursor .editor-input { caret-color: transparent; }
.editor-block-cursor {
    position: absolute;
    width: 0.55em;
    height: 1.2em;
    background: var(--ed-cursor, var(--text-0));
    opacity: 0.7;
    pointer-events: none;
    z-index: 4;
    animation: cursorBlink 1s step-end infinite;
    border-radius: 1px;
}
@keyframes cursorBlink { 50% { opacity: 0; } }

/* Highlighted text: ABOVE textarea. Shows colored syntax. Click-through. */
.editor-highlight {
    color: var(--ed-fg, var(--text-1));
    background: transparent;
    pointer-events: none;
    z-index: 3;
    overflow: hidden;
}

/* ─── Current line highlight ──────────────────────────────────────────── */

.editor-current-line {
    position: absolute;
    left: 0;
    right: 0;
    height: 1.6em;
    background: var(--ed-line-hl, rgba(255,255,255,0.06));
    pointer-events: none;
    z-index: 2;
}

/* ─── Indent guides ───────────────────────────────────────────────────── */

.editor-indent-guides {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    pointer-events: none;
    z-index: 2;
    overflow: hidden;
}
.indent-guide {
    position: absolute;
    width: 1px;
    background: var(--ed-indent-guide, rgba(255,255,255,0.06));
}

/* ─── Selection highlights (all occurrences) ──────────────────────────── */

.sel-match {
    background: rgba(255, 215, 0, 0.12);
    border-bottom: 1px solid rgba(255, 215, 0, 0.4);
    border-radius: 1px;
}

/* ─── Minimap ─────────────────────────────────────────────────────────── */

.editor-minimap {
    position: absolute;
    right: 0;
    top: 0;
    width: 64px;
    height: 100%;
    background: var(--ed-bg, var(--bg-1));
    border-left: 1px solid var(--border-subtle);
    overflow: hidden;
    cursor: pointer;
    z-index: 5;
    opacity: 0.8;
}
.editor-minimap canvas {
    display: block;
    width: 100%;
    height: 100%;
}
.minimap-viewport {
    position: absolute;
    left: 0;
    width: 100%;
    background: rgba(255,255,255,0.06);
    border: 1px solid rgba(255,255,255,0.08);
    pointer-events: none;
    transition: top 0.05s;
}

.editor-cursor-mirror {
    position: absolute;
    top: 0; left: 0;
    visibility: hidden;
    pointer-events: none;
    z-index: -1;
}

/* ─── Editor Theme System ─────────────────────────────────────────────── */

/* Default theme (getgreSQL Dark) */
:root {
    --ed-bg: var(--bg-1);
    --ed-fg: var(--text-1);
    --ed-gutter-bg: var(--bg-2);
    --ed-gutter-fg: var(--text-4);
    --ed-gutter-active: var(--text-0);
    --ed-line-hl: rgba(255, 255, 255, 0.04);
    --ed-cursor: var(--text-0);
    --ed-selection: rgba(56, 139, 253, 0.2);
    --ed-kw: #7ee0ff;
    --ed-ty: #56d4dd;
    --ed-fn: #dcbdfb;
    --ed-st: #a5d6a7;
    --ed-nu: #ffab70;
    --ed-cm: #6e7681;
    --ed-op: #f97583;
    --ed-pu: #8b949e;
    --ed-pa: #ffab70;
    --ed-bracket-1: #ffd700;
    --ed-bracket-2: #da70d6;
    --ed-bracket-3: #179fff;
    --ed-indent-guide: rgba(255, 255, 255, 0.06);
}

[data-theme="light"] {
    --ed-bg: #ffffff;
    --ed-fg: #24292f;
    --ed-gutter-bg: #f6f8fa;
    --ed-gutter-fg: #8c959f;
    --ed-gutter-active: #1f2328;
    --ed-line-hl: rgba(0, 0, 0, 0.04);
    --ed-cursor: #1f2328;
    --ed-selection: rgba(9, 105, 218, 0.15);
    --ed-kw: #0550ae;
    --ed-ty: #0969da;
    --ed-fn: #8250df;
    --ed-st: #116329;
    --ed-nu: #953800;
    --ed-cm: #6e7681;
    --ed-op: #cf222e;
    --ed-pu: #656d76;
    --ed-pa: #953800;
    --ed-bracket-1: #bf8700;
    --ed-bracket-2: #8250df;
    --ed-bracket-3: #0969da;
    --ed-indent-guide: rgba(0, 0, 0, 0.06);
}

/* ─── Named Editor Themes ─────────────────────────────────────────────── */

[data-editor-theme="monokai"] {
    --ed-bg: #272822; --ed-fg: #f8f8f2;
    --ed-gutter-bg: #2f3029; --ed-gutter-fg: #90908a; --ed-gutter-active: #f8f8f0;
    --ed-line-hl: rgba(255,255,255,0.06); --ed-cursor: #f8f8f0; --ed-selection: rgba(73,72,62,0.8);
    --ed-kw: #f92672; --ed-ty: #66d9ef; --ed-fn: #a6e22e; --ed-st: #e6db74;
    --ed-nu: #ae81ff; --ed-cm: #75715e; --ed-op: #f92672; --ed-pu: #f8f8f2; --ed-pa: #fd971f;
    --ed-bracket-1: #f92672; --ed-bracket-2: #a6e22e; --ed-bracket-3: #66d9ef;
    --ed-indent-guide: rgba(255,255,255,0.06);
}

[data-editor-theme="dracula"] {
    --ed-bg: #282a36; --ed-fg: #f8f8f2;
    --ed-gutter-bg: #21222c; --ed-gutter-fg: #6272a4; --ed-gutter-active: #f8f8f2;
    --ed-line-hl: rgba(68,71,90,0.5); --ed-cursor: #f8f8f2; --ed-selection: rgba(68,71,90,0.6);
    --ed-kw: #ff79c6; --ed-ty: #8be9fd; --ed-fn: #50fa7b; --ed-st: #f1fa8c;
    --ed-nu: #bd93f9; --ed-cm: #6272a4; --ed-op: #ff79c6; --ed-pu: #f8f8f2; --ed-pa: #ffb86c;
    --ed-bracket-1: #ff79c6; --ed-bracket-2: #50fa7b; --ed-bracket-3: #8be9fd;
    --ed-indent-guide: rgba(255,255,255,0.05);
}

[data-editor-theme="nord"] {
    --ed-bg: #2e3440; --ed-fg: #d8dee9;
    --ed-gutter-bg: #272c36; --ed-gutter-fg: #4c566a; --ed-gutter-active: #d8dee9;
    --ed-line-hl: rgba(255,255,255,0.03); --ed-cursor: #d8dee9; --ed-selection: rgba(67,76,94,0.6);
    --ed-kw: #81a1c1; --ed-ty: #8fbcbb; --ed-fn: #88c0d0; --ed-st: #a3be8c;
    --ed-nu: #b48ead; --ed-cm: #616e88; --ed-op: #81a1c1; --ed-pu: #d8dee9; --ed-pa: #d08770;
    --ed-bracket-1: #ebcb8b; --ed-bracket-2: #b48ead; --ed-bracket-3: #88c0d0;
    --ed-indent-guide: rgba(255,255,255,0.04);
}

[data-editor-theme="solarized"] {
    --ed-bg: #002b36; --ed-fg: #839496;
    --ed-gutter-bg: #002029; --ed-gutter-fg: #586e75; --ed-gutter-active: #93a1a1;
    --ed-line-hl: rgba(255,255,255,0.03); --ed-cursor: #839496; --ed-selection: rgba(7,54,66,0.8);
    --ed-kw: #268bd2; --ed-ty: #2aa198; --ed-fn: #b58900; --ed-st: #2aa198;
    --ed-nu: #d33682; --ed-cm: #586e75; --ed-op: #cb4b16; --ed-pu: #839496; --ed-pa: #cb4b16;
    --ed-bracket-1: #b58900; --ed-bracket-2: #d33682; --ed-bracket-3: #268bd2;
    --ed-indent-guide: rgba(255,255,255,0.04);
}

[data-editor-theme="one-dark"] {
    --ed-bg: #282c34; --ed-fg: #abb2bf;
    --ed-gutter-bg: #21252b; --ed-gutter-fg: #495162; --ed-gutter-active: #abb2bf;
    --ed-line-hl: rgba(255,255,255,0.04); --ed-cursor: #528bff; --ed-selection: rgba(62,68,81,0.8);
    --ed-kw: #c678dd; --ed-ty: #56b6c2; --ed-fn: #61afef; --ed-st: #98c379;
    --ed-nu: #d19a66; --ed-cm: #5c6370; --ed-op: #c678dd; --ed-pu: #abb2bf; --ed-pa: #d19a66;
    --ed-bracket-1: #d19a66; --ed-bracket-2: #c678dd; --ed-bracket-3: #61afef;
    --ed-indent-guide: rgba(255,255,255,0.05);
}

[data-editor-theme="github-light"] {
    --ed-bg: #ffffff; --ed-fg: #24292f;
    --ed-gutter-bg: #f6f8fa; --ed-gutter-fg: #8c959f; --ed-gutter-active: #1f2328;
    --ed-line-hl: rgba(234,238,242,0.5); --ed-cursor: #1f2328; --ed-selection: rgba(9,105,218,0.15);
    --ed-kw: #cf222e; --ed-ty: #0969da; --ed-fn: #8250df; --ed-st: #0a3069;
    --ed-nu: #953800; --ed-cm: #6e7681; --ed-op: #cf222e; --ed-pu: #24292f; --ed-pa: #953800;
    --ed-bracket-1: #953800; --ed-bracket-2: #8250df; --ed-bracket-3: #0969da;
    --ed-indent-guide: rgba(0,0,0,0.06);
}

[data-editor-theme="high-contrast"] {
    --ed-bg: #000000; --ed-fg: #ffffff;
    --ed-gutter-bg: #0a0a0a; --ed-gutter-fg: #666666; --ed-gutter-active: #ffffff;
    --ed-line-hl: rgba(255,255,255,0.08); --ed-cursor: #ffffff; --ed-selection: rgba(56,139,253,0.4);
    --ed-kw: #6cf; --ed-ty: #3dc9b0; --ed-fn: #dcdcaa; --ed-st: #ce9178;
    --ed-nu: #b5cea8; --ed-cm: #6a9955; --ed-op: #d4d4d4; --ed-pu: #ffffff; --ed-pa: #b5cea8;
    --ed-bracket-1: #ffd700; --ed-bracket-2: #da70d6; --ed-bracket-3: #179fff;
    --ed-indent-guide: rgba(255,255,255,0.1);
}

/* ─── Syntax highlighting tokens (theme-aware) ────────────────────────── */

.tok-kw { color: var(--ed-kw); font-weight: 600; }
.tok-ty { color: var(--ed-ty); }
.tok-fn { color: var(--ed-fn); }
.tok-st { color: var(--ed-st); }
.tok-nu { color: var(--ed-nu); }
.tok-cm { color: var(--ed-cm); font-style: italic; }
.tok-op { color: var(--ed-op); }
.tok-pu { color: var(--ed-pu); }
.tok-pa { color: var(--ed-pa); font-weight: 600; }

/* Bracket pair colorization */
.bracket-1 { color: var(--ed-bracket-1); }
.bracket-2 { color: var(--ed-bracket-2); }
.bracket-3 { color: var(--ed-bracket-3); }
.bracket-match { background: rgba(255,255,255,0.12); outline: 1px solid var(--ed-bracket-1); border-radius: 2px; }

/* ─── Autocomplete popup ─────────────────────────────────────────────── */

.ac-popup {
    position: fixed;
    z-index: 10000;
    background: var(--bg-1);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.4);
    max-height: 260px;
    min-width: 240px;
    max-width: 420px;
    overflow-y: auto;
    overflow-x: hidden;
    padding: var(--sp-1) 0;
}
.ac-popup::-webkit-scrollbar { width: 4px; }
.ac-popup::-webkit-scrollbar-thumb { background: var(--bg-4); border-radius: 2px; }

.ac-item {
    display: flex;
    align-items: center;
    gap: var(--sp-2);
    padding: 3px var(--sp-3);
    cursor: pointer;
    font-size: var(--font-size-sm);
    min-height: 24px;
    transition: background var(--transition-fast);
}
.ac-item:hover, .ac-item.selected {
    background: var(--accent-subtle);
}
.ac-item.selected {
    color: var(--text-0);
}

.ac-icon {
    width: 18px;
    height: 18px;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: 3px;
    font-size: 10px;
    font-weight: 700;
    font-family: var(--font-mono);
    flex-shrink: 0;
    background: var(--bg-3);
    color: var(--text-2);
}
.ac-icon.ac-table { background: rgba(56, 139, 253, 0.15); color: var(--accent); }
.ac-icon.ac-column { background: rgba(63, 185, 80, 0.15); color: var(--success); }
.ac-icon.ac-schema { background: rgba(139, 92, 246, 0.15); color: #a78bfa; }
.ac-icon.ac-keyword { background: rgba(126, 224, 255, 0.1); color: #7ee0ff; }
.ac-icon.ac-function { background: rgba(220, 189, 251, 0.15); color: #dcbdfb; }
.ac-icon.ac-type { background: rgba(86, 212, 221, 0.15); color: #56d4dd; }

.ac-label {
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    font-family: var(--font-mono);
}

.ac-detail {
    color: var(--text-4);
    font-size: var(--font-size-xs);
    flex-shrink: 0;
    margin-left: var(--sp-3);
}

/* ─── Find bar ────────────────────────────────────────────────────────── */

.editor-find-bar {
    display: flex;
    align-items: center;
    gap: var(--sp-2);
    padding: var(--sp-2) var(--sp-3);
    background: var(--bg-2);
    border-bottom: 1px solid var(--border-subtle);
    flex-shrink: 0;
}

.find-input, .replace-input {
    padding: 2px var(--sp-2);
    background: var(--bg-1);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    color: var(--text-0);
    font-size: var(--font-size-xs);
    font-family: var(--font-mono);
    outline: none;
    width: 160px;
    transition: border-color var(--transition-fast), box-shadow var(--transition-fast);
}
.find-input:focus, .replace-input:focus {
    border-color: var(--accent);
    box-shadow: 0 0 0 2px var(--accent-subtle);
}

.find-count {
    font-size: var(--font-size-xs);
    color: var(--text-3);
    margin-left: var(--sp-2);
}

/* ─── Editor status ───────────────────────────────────────────────────── */

.editor-status {
    font-size: var(--font-size-xs);
    color: var(--text-3);
    font-family: var(--font-mono);
    margin-left: auto;
}

.toolbar-spacer { flex: 1; }
.toolbar-sep-v { width: 1px; height: 16px; background: var(--border); flex-shrink: 0; }

/* ─── Column Statistics Cards ─────────────────────────────────────────── */

.col-stat-card {
    background: var(--bg-1);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
    padding: var(--sp-4);
    margin-bottom: var(--sp-3);
    box-shadow: var(--shadow-sm);
}

.col-stat-header {
    display: flex;
    align-items: center;
    gap: var(--sp-3);
    margin-bottom: var(--sp-3);
    padding-bottom: var(--sp-2);
    border-bottom: 1px solid var(--border-subtle);
}

.col-stat-body { display: flex; flex-direction: column; gap: var(--sp-2); }

.col-stat-row {
    display: flex;
    align-items: center;
    gap: var(--sp-3);
    font-size: var(--font-size-xs);
}

.col-stat-label {
    width: 80px;
    flex-shrink: 0;
    color: var(--text-3);
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.03em;
}

.col-stat-value { color: var(--text-0); font-family: var(--font-mono); }

.col-stat-bar-track {
    flex: 1;
    height: 6px;
    background: var(--bg-3);
    border-radius: 3px;
    max-width: 200px;
}

.col-stat-bar {
    height: 100%;
    background: var(--accent);
    border-radius: 3px;
    min-width: 1px;
    transition: width 0.3s ease;
}

.col-stat-vals {
    font-size: var(--font-size-xs);
    color: var(--text-2);
    background: var(--bg-2);
    padding: 2px 4px;
    border-radius: var(--radius);
    word-break: break-all;
    max-width: 400px;
    display: inline-block;
}

.ddl-toolbar { display: flex; gap: var(--sp-3); margin-bottom: var(--sp-3); }

/* ─── Visual EXPLAIN ──────────────────────────────────────────────────── */

.explain-visual { padding: var(--sp-4); }

.explain-node {
    border-left: 2px solid var(--border);
    padding: var(--sp-2) 0 var(--sp-2) var(--sp-4);
    margin-left: var(--sp-2);
}

.explain-node-header { font-size: var(--font-size-sm); margin-bottom: var(--sp-1); }
.explain-node-type { color: var(--accent); font-weight: 600; }
.explain-node-table { color: var(--text-2); font-style: italic; }

.explain-node-stats {
    display: flex;
    align-items: center;
    gap: var(--sp-3);
    font-size: var(--font-size-xs);
    color: var(--text-3);
    margin-bottom: var(--sp-1);
}

.explain-cost-bar {
    width: 80px;
    height: 5px;
    background: var(--bg-3);
    border-radius: 3px;
    overflow: hidden;
    flex-shrink: 0;
}

.explain-cost-fill { height: 100%; border-radius: 3px; }
.explain-stat { font-family: var(--font-mono); white-space: nowrap; }
.explain-children { margin-top: var(--sp-1); }

/* Smart analysis */
.explain-analysis { margin-bottom: var(--sp-4); }

.explain-score {
    display: flex;
    align-items: center;
    gap: var(--sp-4);
    padding: var(--sp-4);
    background: var(--bg-1);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
    margin-bottom: var(--sp-4);
}
.explain-grade {
    width: 48px;
    height: 48px;
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 1.4rem;
    font-weight: 800;
    color: #fff;
    flex-shrink: 0;
}
.explain-grade-info { display: flex; flex-direction: column; gap: 2px; }
.explain-grade-info strong { font-size: var(--font-size-md); color: var(--text-0); }
.explain-grade-info span { font-size: var(--font-size-xs); color: var(--text-3); }

.explain-suggestions { display: flex; flex-direction: column; gap: var(--sp-2); margin-bottom: var(--sp-4); }
.explain-suggestion {
    display: flex;
    align-items: flex-start;
    gap: var(--sp-3);
    padding: var(--sp-3) var(--sp-4);
    border-radius: var(--radius);
    font-size: var(--font-size-sm);
}
.explain-sug-danger { background: var(--danger-subtle); border-left: 3px solid var(--danger); }
.explain-sug-warning { background: var(--warning-subtle); border-left: 3px solid var(--warning); }
.explain-sug-info { background: var(--accent-subtle); border-left: 3px solid var(--accent); }
.explain-sug-success { background: var(--success-subtle); border-left: 3px solid var(--success); }

.explain-sug-icon { font-size: 1.1rem; flex-shrink: 0; margin-top: 1px; }
.explain-sug-body { flex: 1; }
.explain-sug-title { font-weight: 600; color: var(--text-0); margin-bottom: 2px; }
.explain-sug-detail { color: var(--text-2); line-height: 1.5; }
.explain-sug-sql {
    display: block;
    margin-top: var(--sp-2);
    padding: var(--sp-2) var(--sp-3);
    background: var(--bg-2);
    border-radius: var(--radius);
    font-family: var(--font-mono);
    font-size: var(--font-size-xs);
    color: var(--text-1);
    cursor: pointer;
}
.explain-sug-sql:hover { background: var(--bg-3); }

.explain-bottleneck { border-left-color: var(--danger); border-left-width: 3px; }
.explain-node-extra {
    font-size: var(--font-size-xs);
    color: var(--text-3);
    font-family: var(--font-mono);
    padding: 1px 0;
}

/* ─── ERD Diagram ─────────────────────────────────────────────────────── */

.erd-container {
    width: 100%;
    min-height: 400px;
    overflow: auto;
    background: var(--bg-0);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
}

.erd-toolbar {
    display: flex;
    align-items: center;
    gap: var(--sp-3);
    padding: var(--sp-3) var(--sp-4);
    background: var(--bg-1);
    border-bottom: 1px solid var(--border-subtle);
    font-size: var(--font-size-xs);
}

.erd-info { color: var(--text-3); }
.erd-svg { width: 100%; height: auto; min-height: 300px; }
.erd-box { fill: var(--bg-1); stroke: var(--border); stroke-width: 1; }
.erd-header { fill: var(--bg-2); stroke: none; }
.erd-title { fill: var(--text-0); font-size: 12px; font-weight: 600; font-family: var(--font-sans); }
.erd-col { fill: var(--text-1); font-size: 10px; font-family: var(--font-mono); }
.erd-col-type { fill: var(--text-3); font-size: 9px; font-family: var(--font-mono); }
.erd-line { fill: none; stroke: var(--accent); stroke-width: 1.5; opacity: 0.5; }
.erd-arrow { fill: var(--accent); opacity: 0.5; }

/* ─── Saved Queries ───────────────────────────────────────────────────── */

.saved-queries-list { display: flex; flex-direction: column; gap: var(--sp-3); }

.saved-query-item {
    background: var(--bg-1);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
    padding: var(--sp-4);
    box-shadow: var(--shadow-sm);
    transition: box-shadow var(--transition-normal), border-color var(--transition-normal);
}
.saved-query-item:hover {
    box-shadow: var(--shadow-md);
    border-color: var(--border);
}

.saved-query-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: var(--sp-2);
}

.saved-query-date { font-size: var(--font-size-xs); color: var(--text-4); }

.saved-query-preview {
    display: block;
    font-size: var(--font-size-xs);
    color: var(--text-2);
    background: var(--bg-2);
    padding: var(--sp-2) var(--sp-3);
    border-radius: var(--radius);
    margin-bottom: var(--sp-3);
    white-space: pre-wrap;
    max-height: 60px;
    overflow: hidden;
}

.saved-query-actions { display: flex; gap: var(--sp-2); }

/* ─── Settings Panel ──────────────────────────────────────────────────── */

.settings-panel {
    width: 480px;
    max-height: 70vh;
    background: var(--bg-1);
    border: 1px solid var(--border);
    border-radius: var(--radius-lg);
    box-shadow: 0 16px 48px rgba(0, 0, 0, 0.4);
    overflow-y: auto;
    display: flex;
    flex-direction: column;
}

.settings-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: var(--sp-4);
    border-bottom: 1px solid var(--border);
    font-weight: 600;
    color: var(--text-0);
}
.settings-header button {
    background: none;
    border: none;
    color: var(--text-3);
    cursor: pointer;
    font-size: 1.2rem;
    transition: color var(--transition-fast);
}
.settings-header button:hover { color: var(--text-0); }

.settings-body { padding: var(--sp-4); }

.settings-group {
    margin-bottom: var(--sp-5);
}

.settings-group-title {
    font-size: var(--font-size-xs);
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    color: var(--text-3);
    margin-bottom: var(--sp-3);
}

.settings-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: var(--sp-2) 0;
    min-height: 32px;
}

.settings-label {
    font-size: var(--font-size-sm);
    color: var(--text-1);
}

.settings-select {
    padding: 3px 8px;
    background: var(--bg-2);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    color: var(--text-0);
    font-size: var(--font-size-xs);
    font-family: var(--font-sans);
    outline: none;
    min-width: 140px;
    transition: border-color var(--transition-fast);
}
.settings-select:focus { border-color: var(--accent); box-shadow: 0 0 0 2px var(--accent-subtle); }

.settings-input-num {
    width: 60px;
    padding: 3px 8px;
    background: var(--bg-2);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    color: var(--text-0);
    font-size: var(--font-size-xs);
    font-family: var(--font-mono);
    text-align: center;
    outline: none;
    transition: border-color var(--transition-fast);
}
.settings-input-num:focus { border-color: var(--accent); box-shadow: 0 0 0 2px var(--accent-subtle); }

.settings-toggle {
    position: relative;
    width: 36px;
    height: 20px;
    background: var(--bg-4);
    border: none;
    border-radius: 10px;
    cursor: pointer;
    transition: background 0.15s;
}
.settings-toggle.on { background: var(--accent); }
.settings-toggle::after {
    content: '';
    position: absolute;
    top: 2px;
    left: 2px;
    width: 16px;
    height: 16px;
    border-radius: 50%;
    background: white;
    transition: transform 0.15s;
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.3);
}
.settings-toggle.on::after { transform: translateX(16px); }

/* Theme preview swatches */
.theme-preview {
    display: flex;
    gap: var(--sp-2);
    flex-wrap: wrap;
    margin-top: var(--sp-2);
}

.theme-swatch {
    width: 44px;
    height: 28px;
    border-radius: var(--radius);
    border: 2px solid transparent;
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 7px;
    font-family: var(--font-mono);
    font-weight: 700;
    transition: border-color var(--transition-fast), transform var(--transition-fast);
    position: relative;
    overflow: hidden;
}
.theme-swatch.active { border-color: var(--accent); }
.theme-swatch:hover { border-color: var(--text-3); transform: scale(1.05); }

/* ─── Keyboard Shortcuts Reference ────────────────────────────────────── */

.shortcuts-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: var(--sp-2) var(--sp-5);
    padding: var(--sp-3);
}

.shortcut-item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: var(--sp-1) 0;
    font-size: var(--font-size-sm);
}

.shortcut-item .label { color: var(--text-1); }
.shortcut-item kbd {
    margin-left: var(--sp-2);
    padding: 2px 6px;
    background: var(--bg-2);
    border: 1px solid var(--border);
    border-radius: 3px;
    font-size: var(--font-size-xs);
    font-family: var(--font-mono);
    color: var(--text-2);
    box-shadow: 0 1px 0 var(--border);
}

/* ─── Go to Line bar ──────────────────────────────────────────────────── */

.goto-line-bar {
    display: flex;
    align-items: center;
    gap: var(--sp-3);
    padding: var(--sp-2) var(--sp-3);
    background: var(--bg-2);
    border-bottom: 1px solid var(--border-subtle);
    flex-shrink: 0;
}

.goto-line-bar input {
    width: 120px;
    padding: 2px var(--sp-2);
    background: var(--bg-1);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    color: var(--text-0);
    font-size: var(--font-size-xs);
    font-family: var(--font-mono);
    outline: none;
    transition: border-color var(--transition-fast);
}
.goto-line-bar input:focus { border-color: var(--accent); box-shadow: 0 0 0 2px var(--accent-subtle); }
.goto-line-bar label { font-size: var(--font-size-xs); color: var(--text-3); }

/* ─── Editor cursor info in status bar ────────────────────────────────── */

.editor-cursor-info {
    font-family: var(--font-mono);
    font-size: var(--font-size-xs);
    color: var(--text-3);
    display: flex;
    gap: var(--sp-3);
}
.editor-cursor-info span { white-space: nowrap; }

)_CSS_"; }

    static constexpr auto js() -> std::string_view { return R"_JS_(
// getgreSQL — SQL Editor Engine
// Syntax highlighting, autocomplete, multi-tab, history, find/replace

(function() {
'use strict';

// ─── SQL Token Types ─────────────────────────────────────────────────

var T = {
    KEYWORD: 'kw',
    TYPE: 'ty',
    BUILTIN: 'fn',
    STRING: 'st',
    NUMBER: 'nu',
    COMMENT: 'cm',
    OPERATOR: 'op',
    PUNCTUATION: 'pu',
    IDENTIFIER: 'id',
    WHITESPACE: 'ws',
    PARAM: 'pa',  // $1, $2
};

var KEYWORDS = new Set([
    'select','from','where','and','or','not','in','exists','between','like','ilike',
    'is','null','insert','into','values','update','set','delete','create','alter',
    'drop','table','index','view','function','trigger','schema','database','type',
    'enum','join','inner','left','right','full','outer','cross','on','using','order',
    'by','group','having','limit','offset','fetch','next','rows','only','as',
    'distinct','all','union','intersect','except','with','recursive','case','when',
    'then','else','end','if','elsif','elseif','loop','while','for','return','returns',
    'begin','commit','rollback','savepoint','transaction','primary','key','foreign',
    'references','unique','check','constraint','default','asc','desc','nulls','first',
    'last','grant','revoke','explain','analyze','verbose','cascade','restrict',
    'replace','language','volatile','stable','immutable','owner','to','do','declare',
    'raise','notice','exception','perform','execute','format','found','new','old',
    'row','statement','before','after','each','instead','of','deferrable','initially',
    'deferred','immediate','no','action','match','partial','simple','add','column',
    'rename','enable','disable','always','replica','identity','generated','stored',
    'overriding','system','value','partition','range','list','hash','include',
    'tablespace','concurrently','temporary','temp','unlogged','materialized',
    'refresh','security','definer','invoker','parallel','safe','unsafe','cost',
    'support','strict','called','input','true','false','lateral','tablesample',
    'bernoulli','window','over','filter','within','grouping','sets','cube',
    'rollup','extract','epoch','year','month','day','hour','minute','second',
    'zone','at','interval','similar','escape','collate','isnull','notnull',
    'normalize','overlay','placing','position','substring','trim','leading',
    'trailing','both','natural','preserve','preceding','following','unbounded',
    'current','exclude','ties','others','groups','abort','access','aggregate',
    'also','assertion','assignment','attribute','backward','cache','called',
    'catalog','chain','characteristics','checkpoint','class','close','cluster',
    'comment','comments','configuration','conflict','connection','conversion','copy',
    'csv','cursor','cycle','data','deallocate','depends','detach','dictionary',
    'discard','document','domain','double','encoding','encrypted','event',
    'extension','external','family','finalize','force','forward','freeze',
    'global','granted','handler','header','hold','import','increment',
    'indexes','inherit','inherits','inline','insensitive','instead','label',
    'large','leakproof','level','listen','load','local','location','lock','locked',
    'logged','mapping','maxvalue','method','minvalue','move','name','names',
    'nothing','notify','nowait','nullif','object','oids','operator','option',
    'options','ordinality','owned','parser','passing','password','plan','plans',
    'policy','preceding','prepare','prepared','preserve','prior','privileges',
    'procedural','procedure','program','publication','quote','reassign',
    'recheck','reindex','relative','release','replication','reset','restart',
    'returns','revoke','role','rule','savepoint','schemas','scroll','search',
    'sequence','sequences','serializable','server','session','share','show',
    'skip','snapshot','sql','stable','standalone','start','statistics',
    'stdin','stdout','storage','subscription','sysid','tables','temp',
    'template','text','transactions','transform','truncate','trusted',
    'types','uncommitted','unencrypted','unknown','unlisten','until','vacuum',
    'valid','validate','validator','varying','version','views','whitespace',
    'work','wrapper','write','xml','xmlattributes','xmlconcat','xmlelement',
    'xmlexists','xmlforest','xmlnamespaces','xmlparse','xmlpi','xmlroot',
    'xmlserialize','xmltable','yes','zone',
]);

var TYPES = new Set([
    'integer','int','int2','int4','int8','bigint','smallint','serial','bigserial',
    'smallserial','text','varchar','char','character','varying','boolean','bool',
    'date','time','timestamp','timestamptz','timetz','interval','numeric','decimal',
    'real','float','float4','float8','double','precision','money',
    'json','jsonb','uuid','bytea','xml','inet','cidr','macaddr','macaddr8',
    'bit','varbit','point','line','lseg','box','path','polygon','circle',
    'tsquery','tsvector','txid_snapshot','pg_lsn','pg_snapshot',
    'int4range','int8range','numrange','tsrange','tstzrange','daterange',
    'oid','regclass','regtype','regproc','regprocedure','regoper','regoperator',
    'regnamespace','regconfig','regdictionary','void','record','trigger',
    'event_trigger','array','setof','anyelement','anyarray','anynonarray',
    'anyenum','anyrange','anymultirange','cstring','internal','opaque',
    'name','pg_node_tree','pg_ndistinct','pg_dependencies','pg_mcv_list',
]);

var BUILTINS = new Set([
    'count','sum','avg','min','max','array_agg','string_agg','bool_and','bool_or',
    'every','json_agg','jsonb_agg','json_object_agg','jsonb_object_agg','xmlagg',
    'bit_and','bit_or','bit_xor','corr','covar_pop','covar_samp','regr_avgx',
    'regr_avgy','regr_count','regr_intercept','regr_r2','regr_slope','regr_sxx',
    'regr_sxy','regr_syy','stddev','stddev_pop','stddev_samp','variance',
    'var_pop','var_samp','mode','percentile_cont','percentile_disc',
    'rank','dense_rank','percent_rank','cume_dist','ntile','lag','lead',
    'first_value','last_value','nth_value','row_number',
    'coalesce','nullif','greatest','least','cast',
    'now','current_timestamp','current_date','current_time','localtime',
    'localtimestamp','clock_timestamp','statement_timestamp','transaction_timestamp',
    'timeofday','age','date_part','date_trunc','extract','make_date','make_time',
    'make_timestamp','make_timestamptz','make_interval','to_timestamp','to_date',
    'to_char','to_number',
    'abs','cbrt','ceil','ceiling','degrees','div','exp','factorial','floor',
    'gcd','lcm','ln','log','log10','mod','pi','power','radians','random',
    'round','scale','sign','sqrt','trunc','width_bucket','setseed',
    'length','lower','upper','initcap','left','right','lpad','rpad',
    'ltrim','rtrim','btrim','trim','repeat','replace','reverse','split_part',
    'strpos','substr','substring','translate','ascii','chr','concat','concat_ws',
    'encode','decode','format','md5','sha256','sha512',
    'regexp_match','regexp_matches','regexp_replace','regexp_split_to_array',
    'regexp_split_to_table','regexp_count','regexp_instr','regexp_like',
    'regexp_substr',
    'array_append','array_cat','array_dims','array_fill','array_length',
    'array_lower','array_upper','array_ndims','array_position','array_positions',
    'array_prepend','array_remove','array_replace','array_to_string',
    'string_to_array','unnest','cardinality',
    'json_build_array','json_build_object','json_extract_path',
    'json_extract_path_text','json_array_length','json_each','json_each_text',
    'json_object_keys','json_populate_record','json_populate_recordset',
    'json_array_elements','json_array_elements_text','json_typeof',
    'json_strip_nulls','json_to_record','json_to_recordset',
    'jsonb_build_array','jsonb_build_object','jsonb_extract_path',
    'jsonb_extract_path_text','jsonb_array_length','jsonb_each','jsonb_each_text',
    'jsonb_object_keys','jsonb_populate_record','jsonb_populate_recordset',
    'jsonb_array_elements','jsonb_array_elements_text','jsonb_typeof',
    'jsonb_strip_nulls','jsonb_to_record','jsonb_to_recordset',
    'jsonb_set','jsonb_insert','jsonb_pretty','jsonb_path_query',
    'jsonb_path_query_array','jsonb_path_query_first','jsonb_path_exists',
    'jsonb_path_match',
    'exists','row','overlaps','generate_series','generate_subscripts',
    'pg_typeof','pg_column_size','pg_database_size','pg_indexes_size',
    'pg_relation_size','pg_size_pretty','pg_table_size','pg_tablespace_size',
    'pg_total_relation_size','pg_get_constraintdef','pg_get_expr',
    'pg_get_functiondef','pg_get_indexdef','pg_get_ruledef','pg_get_serial_sequence',
    'pg_get_triggerdef','pg_get_userbyid','pg_get_viewdef',
    'pg_cancel_backend','pg_terminate_backend','pg_stat_get_activity',
    'current_user','current_schema','current_schemas','current_database',
    'session_user','user','version',
    'nextval','currval','setval','lastval',
    'txid_current','txid_current_if_assigned','txid_current_snapshot',
    'txid_snapshot_xip','txid_snapshot_xmax','txid_snapshot_xmin',
    'gen_random_uuid','gen_random_bytes',
]);

// ─── SQL Tokenizer ───────────────────────────────────────────────────

function tokenize(sql) {
    var tokens = [];
    var i = 0;
    var len = sql.length;

    while (i < len) {
        var ch = sql[i];

        // Whitespace
        if (ch === ' ' || ch === '\t' || ch === '\n' || ch === '\r') {
            var start = i;
            while (i < len && (sql[i] === ' ' || sql[i] === '\t' || sql[i] === '\n' || sql[i] === '\r')) i++;
            tokens.push({ type: T.WHITESPACE, value: sql.slice(start, i) });
            continue;
        }

        // Single-line comment
        if (ch === '-' && i + 1 < len && sql[i+1] === '-') {
            var start = i;
            i += 2;
            while (i < len && sql[i] !== '\n') i++;
            tokens.push({ type: T.COMMENT, value: sql.slice(start, i) });
            continue;
        }

        // Block comment
        if (ch === '/' && i + 1 < len && sql[i+1] === '*') {
            var start = i;
            i += 2;
            var depth = 1;
            while (i < len && depth > 0) {
                if (sql[i] === '/' && i + 1 < len && sql[i+1] === '*') { depth++; i += 2; }
                else if (sql[i] === '*' && i + 1 < len && sql[i+1] === '/') { depth--; i += 2; }
                else i++;
            }
            tokens.push({ type: T.COMMENT, value: sql.slice(start, i) });
            continue;
        }

        // String literal
        if (ch === "'") {
            var start = i;
            i++;
            while (i < len) {
                if (sql[i] === "'" && i + 1 < len && sql[i+1] === "'") { i += 2; continue; }
                if (sql[i] === "'") { i++; break; }
                i++;
            }
            tokens.push({ type: T.STRING, value: sql.slice(start, i) });
            continue;
        }

        // Dollar-quoted string
        if (ch === '$') {
            var start = i;
            var tagEnd = sql.indexOf('$', i + 1);
            if (tagEnd !== -1) {
                var tag = sql.slice(i, tagEnd + 1);
                i = tagEnd + 1;
                var closePos = sql.indexOf(tag, i);
                if (closePos !== -1) {
                    i = closePos + tag.length;
                } else {
                    i = len;
                }
                tokens.push({ type: T.STRING, value: sql.slice(start, i) });
                continue;
            }
            // Parameter $1, $2, etc.
            i++;
            var numStart = i;
            while (i < len && sql[i] >= '0' && sql[i] <= '9') i++;
            if (i > numStart) {
                tokens.push({ type: T.PARAM, value: sql.slice(start, i) });
            } else {
                tokens.push({ type: T.OPERATOR, value: '$' });
            }
            continue;
        }

        // Quoted identifier
        if (ch === '"') {
            var start = i;
            i++;
            while (i < len && sql[i] !== '"') i++;
            if (i < len) i++;
            tokens.push({ type: T.IDENTIFIER, value: sql.slice(start, i) });
            continue;
        }

        // Numbers
        if ((ch >= '0' && ch <= '9') || (ch === '.' && i + 1 < len && sql[i+1] >= '0' && sql[i+1] <= '9')) {
            var start = i;
            if (ch === '0' && i + 1 < len && (sql[i+1] === 'x' || sql[i+1] === 'X')) {
                i += 2;
                while (i < len && ((sql[i] >= '0' && sql[i] <= '9') || (sql[i] >= 'a' && sql[i] <= 'f') || (sql[i] >= 'A' && sql[i] <= 'F'))) i++;
            } else {
                while (i < len && sql[i] >= '0' && sql[i] <= '9') i++;
                if (i < len && sql[i] === '.') {
                    i++;
                    while (i < len && sql[i] >= '0' && sql[i] <= '9') i++;
                }
                if (i < len && (sql[i] === 'e' || sql[i] === 'E')) {
                    i++;
                    if (i < len && (sql[i] === '+' || sql[i] === '-')) i++;
                    while (i < len && sql[i] >= '0' && sql[i] <= '9') i++;
                }
            }
            tokens.push({ type: T.NUMBER, value: sql.slice(start, i) });
            continue;
        }

        // Identifiers and keywords
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch === '_') {
            var start = i;
            i++;
            while (i < len && ((sql[i] >= 'a' && sql[i] <= 'z') || (sql[i] >= 'A' && sql[i] <= 'Z') || (sql[i] >= '0' && sql[i] <= '9') || sql[i] === '_')) i++;
            var word = sql.slice(start, i);
            var lower = word.toLowerCase();
            if (KEYWORDS.has(lower)) {
                tokens.push({ type: T.KEYWORD, value: word });
            } else if (TYPES.has(lower)) {
                tokens.push({ type: T.TYPE, value: word });
            } else if (BUILTINS.has(lower)) {
                tokens.push({ type: T.BUILTIN, value: word });
            } else {
                tokens.push({ type: T.IDENTIFIER, value: word });
            }
            continue;
        }

        // Operators
        if ('=<>!+-*/%^|&~'.indexOf(ch) !== -1) {
            var start = i;
            i++;
            // Multi-char operators
            while (i < len && '=<>!+-*/%^|&~'.indexOf(sql[i]) !== -1) i++;
            tokens.push({ type: T.OPERATOR, value: sql.slice(start, i) });
            continue;
        }

        // Punctuation
        if ('(),;.[]{}:'.indexOf(ch) !== -1) {
            // :: cast operator
            if (ch === ':' && i + 1 < len && sql[i+1] === ':') {
                tokens.push({ type: T.OPERATOR, value: '::' });
                i += 2;
            } else {
                tokens.push({ type: T.PUNCTUATION, value: ch });
                i++;
            }
            continue;
        }

        // Unknown
        tokens.push({ type: T.IDENTIFIER, value: ch });
        i++;
    }

    return tokens;
}

// ─── HTML escaping ───────────────────────────────────────────────────

function esc(s) {
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

// ─── Syntax highlighting ─────────────────────────────────────────────

function highlight(sql, selectedWord) {
    if (!sql) return '\n';
    var tokens = tokenize(sql);
    var html = '';
    var bracketDepth = 0;
    var openBrackets = '([{';
    var closeBrackets = ')]}';

    for (var i = 0; i < tokens.length; i++) {
        var t = tokens[i];
        var val = esc(t.value);

        if (t.type === T.WHITESPACE) {
            html += val;
        } else if (t.type === T.PUNCTUATION && (openBrackets.indexOf(t.value) !== -1 || closeBrackets.indexOf(t.value) !== -1)) {
            // Bracket pair colorization
            if (closeBrackets.indexOf(t.value) !== -1 && bracketDepth > 0) bracketDepth--;
            var cls = 'bracket-' + ((bracketDepth % 3) + 1);
            html += '<span class="' + cls + '">' + val + '</span>';
            if (openBrackets.indexOf(t.value) !== -1) bracketDepth++;
        } else if (t.type === T.IDENTIFIER) {
            // Selection occurrence highlighting
            if (selectedWord && t.value.toLowerCase() === selectedWord.toLowerCase()) {
                html += '<span class="sel-match">' + val + '</span>';
            } else {
                html += val;
            }
        } else {
            html += '<span class="tok-' + t.type + '">' + val + '</span>';
        }
    }
    if (sql[sql.length - 1] === '\n') html += '\n';
    return html;
}

// ─── Cursor Position Measurement ─────────────────────────────────────

function getCursorCoords(textarea) {
    var mirror = textarea._mirror;
    if (!mirror) {
        mirror = document.createElement('div');
        mirror.className = 'editor-cursor-mirror';
        mirror.style.cssText =
            'position:absolute;top:0;left:0;visibility:hidden;white-space:pre-wrap;word-wrap:break-word;overflow:hidden;' +
            'font-family:inherit;font-size:inherit;line-height:inherit;padding:inherit;border:none;' +
            'pointer-events:none;box-sizing:border-box;';
        textarea.parentElement.appendChild(mirror);
        textarea._mirror = mirror;
    }

    var cs = getComputedStyle(textarea);
    mirror.style.width = cs.width;
    mirror.style.fontFamily = cs.fontFamily;
    mirror.style.fontSize = cs.fontSize;
    mirror.style.lineHeight = cs.lineHeight;
    mirror.style.padding = cs.padding;
    mirror.style.letterSpacing = cs.letterSpacing;
    mirror.style.tabSize = cs.tabSize;

    var text = textarea.value.substring(0, textarea.selectionStart);
    mirror.textContent = text;
    var span = document.createElement('span');
    span.textContent = '|';
    mirror.appendChild(span);

    var rect = textarea.getBoundingClientRect();
    var spanRect = span.getBoundingClientRect();
    var mirrorRect = mirror.getBoundingClientRect();

    return {
        top: spanRect.top - mirrorRect.top - textarea.scrollTop,
        left: spanRect.left - mirrorRect.left - textarea.scrollLeft,
        absTop: rect.top + (spanRect.top - mirrorRect.top) - textarea.scrollTop,
        absLeft: rect.left + (spanRect.left - mirrorRect.left) - textarea.scrollLeft,
        lineHeight: parseFloat(cs.lineHeight) || parseFloat(cs.fontSize) * 1.5,
    };
}

// ─── Bracket Matching ────────────────────────────────────────────────

var BRACKETS = { '(': ')', '[': ']', '{': '}' };
var CLOSE_BRACKETS = { ')': '(', ']': '[', '}': '{' };

function findMatchingBracket(text, pos) {
    var ch = text[pos];
    if (BRACKETS[ch]) {
        var close = BRACKETS[ch];
        var depth = 1;
        for (var i = pos + 1; i < text.length; i++) {
            if (text[i] === ch) depth++;
            else if (text[i] === close) { depth--; if (depth === 0) return i; }
        }
    } else if (CLOSE_BRACKETS[ch]) {
        var open = CLOSE_BRACKETS[ch];
        var depth = 1;
        for (var i = pos - 1; i >= 0; i--) {
            if (text[i] === ch) depth++;
            else if (text[i] === open) { depth--; if (depth === 0) return i; }
        }
    }
    return -1;
}

// ─── Autocomplete Context ────────────────────────────────────────────

function getCompletionContext(text, pos) {
    // Get word at cursor
    var before = text.substring(0, pos);
    var wordMatch = before.match(/[\w.]+$/);
    var word = wordMatch ? wordMatch[0] : '';
    var prefix = word.toLowerCase();

    // Check if there's a dot (table.column or schema.table)
    var dotIdx = prefix.lastIndexOf('.');
    if (dotIdx !== -1) {
        return {
            type: 'dot',
            qualifier: prefix.substring(0, dotIdx),
            prefix: prefix.substring(dotIdx + 1),
            fullWord: word,
            replaceFrom: pos - word.length,
        };
    }

    // Check preceding keyword for context
    var trimmed = before.replace(/[\w]+$/, '').replace(/\s+$/, '');
    var prevWord = (trimmed.match(/(\w+)\s*$/) || [])[1];
    prevWord = prevWord ? prevWord.toLowerCase() : '';

    var contextType = 'general';
    if (['from', 'join', 'into', 'update', 'table'].indexOf(prevWord) !== -1) {
        contextType = 'table';
    } else if (['schema'].indexOf(prevWord) !== -1) {
        contextType = 'schema';
    }

    return {
        type: contextType,
        prefix: prefix,
        fullWord: word,
        replaceFrom: pos - word.length,
    };
}

// ─── Fuzzy Match ─────────────────────────────────────────────────────

function fuzzyMatch(text, query) {
    if (!query) return { match: true, score: 0 };
    text = text.toLowerCase();
    query = query.toLowerCase();
    if (text.startsWith(query)) return { match: true, score: 100 - text.length };
    if (text.indexOf(query) !== -1) return { match: true, score: 50 - text.length };
    // Subsequence match
    var qi = 0;
    for (var ti = 0; ti < text.length && qi < query.length; ti++) {
        if (text[ti] === query[qi]) qi++;
    }
    if (qi === query.length) return { match: true, score: 25 - text.length };
    return { match: false, score: -1 };
}

// ─── SQL Editor Class ────────────────────────────────────────────────

// ─── Editor Settings ─────────────────────────────────────────────────

var EditorSettings = {
    KEY: 'getgresql_editor_settings',
    defaults: {
        theme: '',        // '' = auto (follows page dark/light)
        fontSize: 13,
        tabSize: 4,
        wordWrap: true,
        minimap: true,
        lineHighlight: true,
        bracketColors: true,
        indentGuides: true,
        blockCursor: false,
    },
    _cache: null,

    get: function() {
        if (this._cache) return this._cache;
        try {
            var saved = JSON.parse(localStorage.getItem(this.KEY) || '{}');
            this._cache = Object.assign({}, this.defaults, saved);
        } catch(e) { this._cache = Object.assign({}, this.defaults); }
        return this._cache;
    },

    set: function(key, value) {
        var s = this.get();
        s[key] = value;
        this._cache = s;
        try { localStorage.setItem(this.KEY, JSON.stringify(s)); } catch(e) {}
    }
};

function SQLEditorInstance(container, opts) {
    var self = this;
    opts = opts || {};
    this.container = container;
    this.completionData = null;
    this.tabs = [];
    this.activeTabId = null;
    this.tabIdCounter = 0;
    this.history = JSON.parse(localStorage.getItem('getgresql_history') || '[]');
    this.historyIdx = -1;
    this.acVisible = false;
    this.acItems = [];
    this.acSelected = 0;
    this.findVisible = false;
    this.selectedWord = '';
    this.settings = EditorSettings.get();

    this.build();
    this.applySettings();
    this.addTab('Query 1');
    this.loadCompletions();
    this.bindEvents();
}

SQLEditorInstance.prototype.build = function() {
    this.container.innerHTML = '';
    this.container.className = 'query-panel';

    // Tab bar
    this.tabBar = document.createElement('div');
    this.tabBar.className = 'editor-tab-bar';
    this.container.appendChild(this.tabBar);

    // Editor region
    this.editorRegion = document.createElement('div');
    this.editorRegion.className = 'query-editor';
    this.container.appendChild(this.editorRegion);

    // Gutter
    this.gutter = document.createElement('div');
    this.gutter.className = 'editor-gutter';

    // Editor area (holds highlight + textarea)
    this.editorArea = document.createElement('div');
    this.editorArea.className = 'editor-area';

    // Highlight overlay
    this.highlightPre = document.createElement('pre');
    this.highlightPre.className = 'editor-highlight';
    this.highlightPre.setAttribute('aria-hidden', 'true');

    // Textarea
    this.textarea = document.createElement('textarea');
    this.textarea.className = 'editor-input';
    this.textarea.setAttribute('spellcheck', 'false');
    this.textarea.setAttribute('autocapitalize', 'off');
    this.textarea.setAttribute('autocomplete', 'off');
    this.textarea.setAttribute('autocorrect', 'off');
    this.textarea.setAttribute('placeholder', 'SELECT * FROM ...');
    this.textarea.setAttribute('wrap', 'off');

    // Current line highlight (behind everything)
    this.currentLineEl = document.createElement('div');
    this.currentLineEl.className = 'editor-current-line';
    this.editorArea.appendChild(this.currentLineEl);

    // Indent guides layer
    this.indentGuidesEl = document.createElement('div');
    this.indentGuidesEl.className = 'editor-indent-guides';
    this.editorArea.appendChild(this.indentGuidesEl);

    this.editorArea.appendChild(this.highlightPre);
    this.editorArea.appendChild(this.textarea);

    // Minimap
    this.minimapEl = document.createElement('div');
    this.minimapEl.className = 'editor-minimap';
    this.minimapCanvas = document.createElement('canvas');
    this.minimapViewport = document.createElement('div');
    this.minimapViewport.className = 'minimap-viewport';
    this.minimapEl.appendChild(this.minimapCanvas);
    this.minimapEl.appendChild(this.minimapViewport);
    this.editorArea.appendChild(this.minimapEl);

    // Block cursor element
    this.blockCursorEl = document.createElement('div');
    this.blockCursorEl.className = 'editor-block-cursor';
    this.blockCursorEl.style.display = 'none';
    this.editorArea.appendChild(this.blockCursorEl);

    this.editorContainer = document.createElement('div');
    this.editorContainer.className = 'editor-container';
    this.editorContainer.appendChild(this.gutter);
    this.editorContainer.appendChild(this.editorArea);
    this.editorRegion.appendChild(this.editorContainer);

    // Toolbar
    this.toolbarEl = document.createElement('div');
    this.toolbarEl.className = 'editor-toolbar';
    this.toolbarEl.innerHTML =
        '<button class="btn btn-primary btn-sm" data-action="run">&#9654; Run <kbd>Ctrl+Enter</kbd></button>' +
        '<button class="btn btn-sm" data-action="explain">Explain</button>' +
        '<button class="btn btn-sm" data-action="explain-analyze">Explain Analyze</button>' +
        '<button class="btn btn-sm btn-warning" data-action="dry-run" title="Preview without executing">Dry Run</button>' +
        '<span class="toolbar-sep-v"></span>' +
        '<button class="btn btn-sm btn-ghost" data-action="save" title="Save Query (Ctrl+S)">Save</button>' +
        '<button class="btn btn-sm btn-ghost" data-action="saved" title="Saved Queries">Saved</button>' +
        '<button class="btn btn-sm btn-ghost" data-action="history" title="Query History">History</button>' +
        '<button class="btn btn-sm btn-ghost" data-action="diff" title="Compare with last result">Diff</button>' +
        '<span class="toolbar-spacer"></span>' +
        '<div class="btn-group">' +
        '<button class="btn btn-sm btn-ghost" data-action="export-csv" title="Export CSV">CSV</button>' +
        '<button class="btn btn-sm btn-ghost" data-action="export-json" title="Export JSON">JSON</button>' +
        '<button class="btn btn-sm btn-ghost" data-action="export-sql" title="Export SQL">SQL</button>' +
        '</div>' +
        '<span class="toolbar-sep-v"></span>' +
        '<button class="btn btn-sm btn-ghost" data-action="find" title="Find (Ctrl+F)">&#128269;</button>' +
        '<button class="btn btn-sm btn-ghost" data-action="format" title="Format SQL">Format</button>' +
        '<span class="editor-status"></span>';
    // Append common items to toolbar
    this.toolbarEl.innerHTML +=
        '<button class="btn btn-sm btn-ghost" data-action="shortcuts" title="Keyboard Shortcuts">?</button>' +
        '<button class="btn btn-sm btn-ghost" data-action="settings" title="Editor Settings">&#9881;</button>' +
        '<span class="editor-cursor-info"><span class="cursor-pos">Ln 1, Col 1</span></span>';
    this.editorRegion.appendChild(this.toolbarEl);

    // Go to Line bar
    this.gotoLineBar = document.createElement('div');
    this.gotoLineBar.className = 'goto-line-bar';
    this.gotoLineBar.style.display = 'none';
    this.gotoLineBar.innerHTML = '<label>Go to Line:</label><input type="number" min="1" placeholder="Line number">';
    this.editorRegion.insertBefore(this.gotoLineBar, this.editorContainer);

    // Find bar
    this.findBar = document.createElement('div');
    this.findBar.className = 'editor-find-bar';
    this.findBar.style.display = 'none';
    this.findBar.innerHTML =
        '<input type="text" class="find-input" placeholder="Find...">' +
        '<input type="text" class="replace-input" placeholder="Replace...">' +
        '<button class="btn btn-sm" data-find="next">Next</button>' +
        '<button class="btn btn-sm" data-find="prev">Prev</button>' +
        '<button class="btn btn-sm" data-find="replace">Replace</button>' +
        '<button class="btn btn-sm" data-find="replace-all">All</button>' +
        '<button class="btn btn-sm btn-ghost" data-find="close">&times;</button>' +
        '<span class="find-count"></span>';
    this.editorRegion.insertBefore(this.findBar, this.toolbarEl);

    // Resize handle
    this.resizeHandle = document.createElement('div');
    this.resizeHandle.className = 'editor-resize';
    this.container.appendChild(this.resizeHandle);

    // Results area
    this.resultsEl = document.createElement('div');
    this.resultsEl.className = 'query-results';
    this.resultsEl.innerHTML = '<div class="empty-state"><div class="empty-icon">&#9654;</div>Run a query with <kbd>Ctrl+Enter</kbd> or click Run</div>';
    this.container.appendChild(this.resultsEl);

    // Autocomplete popup
    this.acPopup = document.createElement('div');
    this.acPopup.className = 'ac-popup';
    this.acPopup.style.display = 'none';
    document.body.appendChild(this.acPopup);

    // Bracket highlight overlays
    this.bracketMarkers = [];
};

SQLEditorInstance.prototype.addTab = function(title, sql) {
    var id = ++this.tabIdCounter;
    this.tabs.push({ id: id, title: title, sql: sql || '', results: '', scrollTop: 0 });
    this.renderTabs();
    this.switchTab(id);
    return id;
};

SQLEditorInstance.prototype.closeTab = function(id) {
    if (this.tabs.length <= 1) return;
    var idx = this.tabs.findIndex(function(t) { return t.id === id; });
    if (idx === -1) return;
    this.tabs.splice(idx, 1);
    if (this.activeTabId === id) {
        var newIdx = Math.min(idx, this.tabs.length - 1);
        this.switchTab(this.tabs[newIdx].id);
    }
    this.renderTabs();
};

SQLEditorInstance.prototype.switchTab = function(id) {
    // Save current tab state
    if (this.activeTabId !== null) {
        var curr = this.tabs.find(function(t) { return t.id === this.activeTabId; }.bind(this));
        if (curr) {
            curr.sql = this.textarea.value;
            curr.results = this.resultsEl.innerHTML;
            curr.scrollTop = this.textarea.scrollTop;
        }
    }

    this.activeTabId = id;
    var tab = this.tabs.find(function(t) { return t.id === id; });
    if (tab) {
        this.textarea.value = tab.sql;
        this.resultsEl.innerHTML = tab.results;
        this.textarea.scrollTop = tab.scrollTop;
        this.syncHighlight();
        this.updateGutter();
    }
    this.renderTabs();
    this.textarea.focus();
};

SQLEditorInstance.prototype.renderTabs = function() {
    var self = this;
    var html = '';
    this.tabs.forEach(function(tab) {
        var cls = tab.id === self.activeTabId ? 'editor-tab active' : 'editor-tab';
        html += '<div class="' + cls + '" data-tab-id="' + tab.id + '">';
        html += '<span class="tab-icon">&#9654;</span>';
        html += '<span class="tab-label">' + esc(tab.title) + '</span>';
        if (self.tabs.length > 1) {
            html += '<span class="tab-close" data-close-tab="' + tab.id + '">&times;</span>';
        }
        html += '</div>';
    });
    html += '<div class="editor-tab-add" data-action="new-tab">+</div>';
    this.tabBar.innerHTML = html;
};

SQLEditorInstance.prototype.syncHighlight = function() {
    var text = this.textarea.value;
    this.highlightPre.innerHTML = highlight(text, this.selectedWord);
    this.highlightPre.scrollTop = this.textarea.scrollTop;
    this.highlightPre.scrollLeft = this.textarea.scrollLeft;
    this.updateCurrentLine();
    this.updateMinimap();
};

SQLEditorInstance.prototype.updateGutter = function() {
    var lines = this.textarea.value.split('\n').length;
    lines = Math.max(lines, 1);
    var html = '';
    for (var i = 1; i <= lines; i++) {
        html += '<span class="line-num">' + i + '</span>';
    }
    this.gutter.innerHTML = html;
    this.gutter.scrollTop = this.textarea.scrollTop;
};

SQLEditorInstance.prototype.updateStatus = function(msg) {
    var el = this.toolbarEl.querySelector('.editor-status');
    if (el) el.textContent = msg;
};

// ─── Completions ─────────────────────────────────────────────────────

SQLEditorInstance.prototype.loadCompletions = function() {
    var self = this;
    fetch('/api/completions')
        .then(function(r) { return r.json(); })
        .then(function(data) { self.completionData = data; })
        .catch(function() {});
};

SQLEditorInstance.prototype.showAutocomplete = function() {
    var ctx = getCompletionContext(this.textarea.value, this.textarea.selectionStart);
    if (!ctx.prefix && ctx.type === 'general') { this.hideAutocomplete(); return; }

    var items = [];
    var data = this.completionData;

    if (ctx.type === 'dot' && data) {
        // schema.table or table.column
        var qual = ctx.qualifier;
        // Check if qualifier is a schema
        var isSchema = data.schemas && data.schemas.indexOf(qual) !== -1;
        if (isSchema) {
            data.tables.forEach(function(t) {
                if (t.schema === qual) {
                    var m = fuzzyMatch(t.name, ctx.prefix);
                    if (m.match) items.push({ label: t.name, detail: t.type || 'table', kind: 'table', score: m.score });
                }
            });
        } else {
            // qualifier might be a table name — suggest columns
            data.tables.forEach(function(t) {
                if (t.name === qual || (t.schema + '.' + t.name) === qual) {
                    (t.columns || []).forEach(function(col) {
                        var m = fuzzyMatch(col.name || col, ctx.prefix);
                        if (m.match) {
                            var colName = col.name || col;
                            var colType = col.type || '';
                            items.push({ label: colName, detail: colType, kind: 'column', score: m.score });
                        }
                    });
                }
            });
        }
    } else {
        // General, table, or schema context
        if (data) {
            if (ctx.type !== 'schema') {
                data.tables.forEach(function(t) {
                    var m = fuzzyMatch(t.name, ctx.prefix);
                    if (m.match) items.push({ label: t.name, detail: t.schema, kind: 'table', score: m.score + 20 });
                });
            }
            if (data.schemas) {
                data.schemas.forEach(function(s) {
                    var m = fuzzyMatch(s, ctx.prefix);
                    if (m.match) items.push({ label: s, detail: 'schema', kind: 'schema', score: m.score + 10 });
                });
            }
        }

        // Keywords (lower priority in table context)
        if (ctx.type !== 'table') {
            KEYWORDS.forEach(function(kw) {
                var m = fuzzyMatch(kw, ctx.prefix);
                if (m.match && ctx.prefix.length >= 2) items.push({ label: kw.toUpperCase(), detail: 'keyword', kind: 'keyword', score: m.score });
            });
            BUILTINS.forEach(function(fn) {
                var m = fuzzyMatch(fn, ctx.prefix);
                if (m.match && ctx.prefix.length >= 2) items.push({ label: fn, detail: 'function', kind: 'function', score: m.score + 5 });
            });
            TYPES.forEach(function(ty) {
                var m = fuzzyMatch(ty, ctx.prefix);
                if (m.match && ctx.prefix.length >= 2) items.push({ label: ty, detail: 'type', kind: 'type', score: m.score + 3 });
            });
        }
    }

    // Sort by score descending
    items.sort(function(a, b) { return b.score - a.score; });
    items = items.slice(0, 30);

    if (items.length === 0) { this.hideAutocomplete(); return; }

    this.acItems = items;
    this.acSelected = 0;
    this.acContext = ctx;
    this.renderAutocomplete();
    this.positionAutocomplete();
    this.acPopup.style.display = 'block';
    this.acVisible = true;
};

SQLEditorInstance.prototype.renderAutocomplete = function() {
    var self = this;
    var html = '';
    var kindIcons = { table: 'T', column: 'C', schema: 'S', keyword: 'K', function: 'F', type: 'Y' };
    var kindColors = { table: 'ac-table', column: 'ac-column', schema: 'ac-schema', keyword: 'ac-keyword', function: 'ac-function', type: 'ac-type' };

    this.acItems.forEach(function(item, idx) {
        var cls = idx === self.acSelected ? 'ac-item selected' : 'ac-item';
        html += '<div class="' + cls + '" data-ac-idx="' + idx + '">';
        html += '<span class="ac-icon ' + (kindColors[item.kind] || '') + '">' + (kindIcons[item.kind] || '?') + '</span>';
        html += '<span class="ac-label">' + esc(item.label) + '</span>';
        html += '<span class="ac-detail">' + esc(item.detail) + '</span>';
        html += '</div>';
    });
    this.acPopup.innerHTML = html;
};

SQLEditorInstance.prototype.positionAutocomplete = function() {
    var coords = getCursorCoords(this.textarea);
    var editorRect = this.editorArea.getBoundingClientRect();
    this.acPopup.style.top = (editorRect.top + coords.top + coords.lineHeight + 2) + 'px';
    this.acPopup.style.left = (editorRect.left + coords.left) + 'px';
};

SQLEditorInstance.prototype.hideAutocomplete = function() {
    this.acPopup.style.display = 'none';
    this.acVisible = false;
};

SQLEditorInstance.prototype.acceptCompletion = function() {
    if (!this.acVisible || !this.acItems[this.acSelected]) return;
    var item = this.acItems[this.acSelected];
    var ctx = this.acContext;

    var ta = this.textarea;
    var before = ta.value.substring(0, ctx.replaceFrom);
    var after = ta.value.substring(ta.selectionStart);

    // For dot completions, keep the qualifier
    var insertText;
    if (ctx.type === 'dot') {
        insertText = ctx.qualifier + '.' + item.label;
    } else {
        insertText = item.label;
    }

    // Add parens for functions
    if (item.kind === 'function') {
        insertText += '(';
    }

    ta.value = before + insertText + after;
    var newPos = before.length + insertText.length;
    ta.selectionStart = ta.selectionEnd = newPos;

    this.hideAutocomplete();
    this.syncHighlight();
    this.updateGutter();
    ta.focus();
};

// ─── Query Execution ─────────────────────────────────────────────────

// Split SQL into individual statements, respecting strings and comments
SQLEditorInstance.prototype.splitStatements = function(sql) {
    var stmts = [];
    var current = '';
    var inSingle = false, inDouble = false, inDollar = false, dollarTag = '';
    var inLineComment = false, inBlockComment = false;
    var i = 0;
    while (i < sql.length) {
        var ch = sql[i], next = sql[i + 1] || '';
        if (inLineComment) {
            current += ch;
            if (ch === '\n') inLineComment = false;
            i++; continue;
        }
        if (inBlockComment) {
            current += ch;
            if (ch === '*' && next === '/') { current += '/'; inBlockComment = false; i += 2; continue; }
            i++; continue;
        }
        if (inDollar) {
            current += ch;
            // Check for closing dollar tag
            if (ch === '$') {
                var end = sql.indexOf('$', i + 1);
                if (end !== -1) {
                    var tag = sql.substring(i, end + 1);
                    if (tag === dollarTag) { current += sql.substring(i + 1, end + 1); i = end + 1; inDollar = false; continue; }
                }
            }
            i++; continue;
        }
        if (inSingle) {
            current += ch;
            if (ch === "'" && next === "'") { current += "'"; i += 2; continue; } // escaped
            if (ch === "'") inSingle = false;
            i++; continue;
        }
        if (inDouble) {
            current += ch;
            if (ch === '"') inDouble = false;
            i++; continue;
        }
        // Start of string/comment
        if (ch === '-' && next === '-') { inLineComment = true; current += ch; i++; continue; }
        if (ch === '/' && next === '*') { inBlockComment = true; current += '/*'; i += 2; continue; }
        if (ch === "'") { inSingle = true; current += ch; i++; continue; }
        if (ch === '"') { inDouble = true; current += ch; i++; continue; }
        if (ch === '$') {
            var dEnd = sql.indexOf('$', i + 1);
            if (dEnd !== -1 && dEnd - i < 64) {
                var dTag = sql.substring(i, dEnd + 1);
                if (/^\$[a-zA-Z0-9_]*\$$/.test(dTag)) { dollarTag = dTag; inDollar = true; current += ch; i++; continue; }
            }
        }
        if (ch === ';') {
            current += ';';
            var trimmed = current.trim();
            if (trimmed && trimmed !== ';') stmts.push(trimmed);
            current = '';
            i++; continue;
        }
        current += ch;
        i++;
    }
    var rest = current.trim();
    if (rest && rest !== ';') stmts.push(rest);
    return stmts;
};

SQLEditorInstance.prototype.runQuery = function(mode) {
    var sql = this.getSelectedOrAll();
    if (!sql.trim()) return;

    var self = this;

    // Detect if this is DML and mode is 'dry-run'
    if (mode === 'dry-run') {
        this.dryRun(sql);
        return;
    }

    var stmts = (mode === 'run') ? this.splitStatements(sql) : [sql];

    // For explain modes, always run as single statement
    if (mode === 'explain' || mode === 'explain-analyze') stmts = [sql];

    // Show loading
    this.resultsEl.innerHTML = '<div class="loading">Running ' + stmts.length + ' statement' + (stmts.length > 1 ? 's' : '') + '...</div>';
    this.updateStatus('Running...');

    var start = performance.now();
    var results = [];
    var completed = 0;

    function execStatement(idx) {
        if (idx >= stmts.length) {
            // All done — render results
            var elapsed = (performance.now() - start).toFixed(0);
            self.pushHistory(sql, { ms: parseInt(elapsed), rows: results.reduce(function(s, r) { return s + (r.rows || 0); }, 0) });
            self.renderMultiResults(results, elapsed);
            self.updateStatus(elapsed + ' ms');
            return;
        }

        var stmt = stmts[idx];
        var url, body;
        if (mode === 'explain') {
            url = '/query/explain';
            body = 'sql=' + encodeURIComponent(stmt) + '&analyze=false';
        } else if (mode === 'explain-analyze') {
            url = '/query/explain';
            body = 'sql=' + encodeURIComponent(stmt) + '&analyze=true';
        } else {
            url = '/query/exec';
            body = 'sql=' + encodeURIComponent(stmt);
        }

        fetch(url, {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded', 'HX-Request': 'true' },
            body: body,
        })
        .then(function(r) { return r.text(); })
        .then(function(html) {
            // Extract row count from the response HTML
            var rowMatch = html.match(/(\d+)\s*rows?/);
            results.push({ html: html, stmt: stmt, rows: rowMatch ? parseInt(rowMatch[1]) : 0, error: false });
            execStatement(idx + 1);
        })
        .catch(function(err) {
            results.push({ html: '<div class="query-error">' + esc(err.message) + '</div>', stmt: stmt, rows: 0, error: true });
            execStatement(idx + 1);
        });
    }

    execStatement(0);
};

SQLEditorInstance.prototype.renderMultiResults = function(results, elapsed) {
    var self = this;
    if (results.length === 1) {
        // Single statement — no tabs needed
        this.resultsEl.innerHTML = results[0].html;
        if (window.htmx) htmx.process(this.resultsEl);
        // Store for diff
        this._lastResults = results;
        return;
    }

    // Multiple statements — render result tabs
    var container = document.createElement('div');
    container.className = 'multi-result-container';

    var tabBar = document.createElement('div');
    tabBar.className = 'result-tab-bar';

    var panels = document.createElement('div');
    panels.className = 'result-panels';

    for (var i = 0; i < results.length; i++) {
        var r = results[i];
        var label = r.stmt.substring(0, 30).replace(/\s+/g, ' ');
        if (r.stmt.length > 30) label += '...';
        var errorCls = r.error ? ' result-tab-error' : '';

        var tab = document.createElement('div');
        tab.className = 'result-tab' + (i === 0 ? ' active' : '') + errorCls;
        tab.setAttribute('data-result-idx', i);
        tab.innerHTML = '<span class="result-tab-num">' + (i + 1) + '</span> ' + esc(label);
        if (r.rows > 0) tab.innerHTML += ' <span class="result-tab-rows">' + r.rows + '</span>';
        tabBar.appendChild(tab);

        var panel = document.createElement('div');
        panel.className = 'result-panel' + (i === 0 ? ' active' : '');
        panel.setAttribute('data-result-idx', i);
        panel.innerHTML = r.html;
        panels.appendChild(panel);
    }

    container.appendChild(tabBar);
    container.appendChild(panels);
    this.resultsEl.innerHTML = '';
    this.resultsEl.appendChild(container);

    // Tab switching
    tabBar.addEventListener('click', function(e) {
        var tab = e.target.closest('.result-tab');
        if (!tab) return;
        var idx = tab.getAttribute('data-result-idx');
        tabBar.querySelectorAll('.result-tab').forEach(function(t) { t.classList.remove('active'); });
        panels.querySelectorAll('.result-panel').forEach(function(p) { p.classList.remove('active'); });
        tab.classList.add('active');
        panels.querySelector('.result-panel[data-result-idx="' + idx + '"]').classList.add('active');
    });

    if (window.htmx) htmx.process(this.resultsEl);
    this._lastResults = results;
};

SQLEditorInstance.prototype.getSelectedOrAll = function() {
    var ta = this.textarea;
    // If there's a selection, use it
    if (ta.selectionStart !== ta.selectionEnd) {
        return ta.value.substring(ta.selectionStart, ta.selectionEnd);
    }
    // Auto-detect current statement: find the statement the cursor is in
    var text = ta.value;
    if (text.indexOf(';') === -1) return text; // single statement
    var pos = ta.selectionStart;
    var stmts = this.splitStatements(text);
    if (stmts.length <= 1) return text;
    // Map each statement back to its position in the source
    var offset = 0;
    for (var i = 0; i < stmts.length; i++) {
        var idx = text.indexOf(stmts[i].replace(/;$/, ''), offset);
        if (idx === -1) idx = offset;
        var end = idx + stmts[i].length;
        if (pos >= idx && pos <= end + 1) return stmts[i];
        offset = end;
    }
    return text;
};

// Dry-run: show EXPLAIN output + affected row estimate for DML
SQLEditorInstance.prototype.dryRun = function(sql) {
    var self = this;
    this.resultsEl.innerHTML = '<div class="loading">Dry run — analyzing...</div>';
    this.updateStatus('Dry run...');

    fetch('/query/explain', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded', 'HX-Request': 'true' },
        body: 'sql=' + encodeURIComponent(sql) + '&analyze=false',
    })
    .then(function(r) { return r.text(); })
    .then(function(html) {
        var wrapper = '<div class="dry-run-result">' +
            '<div class="dry-run-header">Dry Run — No changes made</div>' +
            '<div class="dry-run-sql"><code>' + esc(sql.substring(0, 300)) + '</code></div>' +
            html +
            '<div class="dry-run-actions">' +
            '<button class="btn btn-sm btn-primary" data-action="run-confirmed">Execute for real</button>' +
            '<button class="btn btn-sm" data-action="cancel-dry">Cancel</button>' +
            '</div></div>';
        self.resultsEl.innerHTML = wrapper;

        // Wire up the execute button
        self.resultsEl.querySelector('[data-action="run-confirmed"]').addEventListener('click', function() {
            self.runQuery('run');
        });
        self.resultsEl.querySelector('[data-action="cancel-dry"]').addEventListener('click', function() {
            self.resultsEl.innerHTML = '<div class="empty-state">Cancelled</div>';
        });
        self.updateStatus('Dry run complete');
    })
    .catch(function(err) {
        self.resultsEl.innerHTML = '<div class="query-error">' + esc(err.message) + '</div>';
        self.updateStatus('Error');
    });
};

// ─── History ─────────────────────────────────────────────────────────

SQLEditorInstance.prototype.pushHistory = function(sql, meta) {
    sql = sql.trim();
    if (!sql) return;
    meta = meta || {};
    // History entries are objects: { sql, ms, rows, ts }
    var entry = { sql: sql, ms: meta.ms || 0, rows: meta.rows || 0, ts: Date.now() };
    // Remove duplicate if most recent has same SQL
    if (this.history.length > 0 && this.history[this.history.length - 1].sql === sql) {
        this.history[this.history.length - 1] = entry; // update metadata
    } else {
        this.history.push(entry);
    }
    if (this.history.length > 200) this.history.shift();
    this.historyIdx = -1;
    try { localStorage.setItem('getgresql_history', JSON.stringify(this.history)); } catch (e) {}
};

// Migrate old string-only history to new object format
SQLEditorInstance.prototype.migrateHistory = function() {
    for (var i = 0; i < this.history.length; i++) {
        if (typeof this.history[i] === 'string') {
            this.history[i] = { sql: this.history[i], ms: 0, rows: 0, ts: 0 };
        }
    }
};

SQLEditorInstance.prototype.showHistory = function() {
    this.migrateHistory();
    if (this.history.length === 0) {
        this.resultsEl.innerHTML = '<div class="empty-state">No query history</div>';
        return;
    }
    var self = this;
    var html = '<div class="history-panel"><div class="history-header">' +
        '<h3>Query History</h3>' +
        '<input type="search" class="history-search" placeholder="Search history...">' +
        '</div>';
    html += '<div class="table-wrapper scrollable" style="max-height:calc(100vh - 340px)">' +
        '<table class="history-table"><thead><tr>' +
        '<th style="width:36px">#</th><th>Query</th>' +
        '<th style="width:70px">Time</th><th style="width:60px">Rows</th>' +
        '<th style="width:100px">When</th><th style="width:80px"></th>' +
        '</tr></thead><tbody>';
    for (var i = this.history.length - 1; i >= 0; i--) {
        var h = this.history[i];
        var preview = h.sql.length > 120 ? h.sql.substring(0, 120) + '...' : h.sql;
        var timeAgo = h.ts ? this.formatTimeAgo(h.ts) : '';
        html += '<tr data-history-row>';
        html += '<td class="num">' + (i + 1) + '</td>';
        html += '<td><code class="query-preview" style="white-space:pre-wrap;max-width:none">' + esc(preview) + '</code></td>';
        html += '<td class="mono" style="color:var(--text-3)">' + (h.ms ? h.ms + ' ms' : '') + '</td>';
        html += '<td class="mono" style="color:var(--text-3)">' + (h.rows || '') + '</td>';
        html += '<td style="color:var(--text-3);font-size:var(--font-size-xs)">' + esc(timeAgo) + '</td>';
        html += '<td><button class="btn btn-sm btn-primary" data-history-load="' + i + '">Load</button> ' +
                '<button class="btn btn-sm" data-history-run="' + i + '">Run</button></td>';
        html += '</tr>';
    }
    html += '</tbody></table></div></div>';
    this.resultsEl.innerHTML = html;

    // Search filter
    var search = this.resultsEl.querySelector('.history-search');
    if (search) {
        search.focus();
        search.addEventListener('input', function() {
            var q = search.value.toLowerCase();
            self.resultsEl.querySelectorAll('[data-history-row]').forEach(function(row) {
                row.style.display = row.textContent.toLowerCase().indexOf(q) !== -1 ? '' : 'none';
            });
        });
    }
};

SQLEditorInstance.prototype.formatTimeAgo = function(ts) {
    var diff = Date.now() - ts;
    if (diff < 60000) return 'just now';
    if (diff < 3600000) return Math.floor(diff / 60000) + 'm ago';
    if (diff < 86400000) return Math.floor(diff / 3600000) + 'h ago';
    if (diff < 604800000) return Math.floor(diff / 86400000) + 'd ago';
    return new Date(ts).toLocaleDateString();
};

// ─── Result Diff ────────────────────────────────────────────────────

SQLEditorInstance.prototype.showDiff = function() {
    var self = this;
    // Snapshot current grid data
    var currentData = this.extractGridData();
    if (!currentData) {
        this._diffSnapshot = null;
        // No visible results - take a snapshot for next time
        this.resultsEl.insertAdjacentHTML('afterbegin',
            '<div class="diff-notice">No results to diff. Run a query first, then click Diff to snapshot, run again, and click Diff to compare.</div>');
        return;
    }

    if (!this._diffSnapshot) {
        // First click: take snapshot
        this._diffSnapshot = currentData;
        this.updateStatus('Snapshot saved — run query again then click Diff');
        this.resultsEl.insertAdjacentHTML('afterbegin',
            '<div class="diff-notice">Snapshot taken (' + currentData.rows.length + ' rows). Run query again and click Diff to compare.</div>');
        return;
    }

    // Second click: compare
    var prev = this._diffSnapshot;
    var curr = currentData;
    this._diffSnapshot = currentData; // save for next diff

    // Build diff view
    var html = '<div class="diff-result"><div class="diff-header">Result Diff: ' +
        prev.rows.length + ' rows &rarr; ' + curr.rows.length + ' rows</div>';

    // Index rows by a key (all columns concatenated)
    var prevMap = {}, currMap = {};
    prev.rows.forEach(function(r, i) { var k = r.join('\x00'); prevMap[k] = i; });
    curr.rows.forEach(function(r, i) { var k = r.join('\x00'); currMap[k] = i; });

    var added = 0, removed = 0, unchanged = 0;
    curr.rows.forEach(function(r) { var k = r.join('\x00'); if (!(k in prevMap)) added++; else unchanged++; });
    prev.rows.forEach(function(r) { var k = r.join('\x00'); if (!(k in currMap)) removed++; });

    html += '<div class="diff-stats">' +
        '<span class="diff-stat diff-added">+' + added + ' added</span> ' +
        '<span class="diff-stat diff-removed">-' + removed + ' removed</span> ' +
        '<span class="diff-stat diff-same">' + unchanged + ' unchanged</span></div>';

    // Render table
    var cols = curr.headers.length > 0 ? curr.headers : prev.headers;
    html += '<div class="table-wrapper scrollable" style="max-height:calc(100vh - 340px)"><table class="dv-table"><thead><tr>';
    html += '<th style="width:24px"></th>';
    cols.forEach(function(c) { html += '<th>' + esc(c) + '</th>'; });
    html += '</tr></thead><tbody>';

    // Removed rows (in prev but not in curr)
    prev.rows.forEach(function(r) {
        var k = r.join('\x00');
        if (k in currMap) return;
        html += '<tr class="diff-row-removed"><td class="diff-marker">-</td>';
        r.forEach(function(v) { html += '<td>' + esc(v) + '</td>'; });
        html += '</tr>';
    });

    // Current rows — highlight added
    curr.rows.forEach(function(r) {
        var k = r.join('\x00');
        var isNew = !(k in prevMap);
        html += '<tr class="' + (isNew ? 'diff-row-added' : '') + '">';
        html += '<td class="diff-marker">' + (isNew ? '+' : '') + '</td>';
        // Cell-level diff for changed rows
        r.forEach(function(v, ci) {
            var prevRow = prevMap[k] !== undefined ? prev.rows[prevMap[k]] : null;
            if (!isNew && prevRow && ci < prevRow.length && prevRow[ci] !== v) {
                html += '<td class="diff-cell-changed">' + esc(v) + '</td>';
            } else {
                html += '<td>' + esc(v) + '</td>';
            }
        });
        html += '</tr>';
    });

    html += '</tbody></table></div></div>';
    this.resultsEl.innerHTML = html;
    this.updateStatus('Diff: +' + added + ' -' + removed);
};

SQLEditorInstance.prototype.extractGridData = function() {
    var table = this.resultsEl.querySelector('.dv-table');
    if (!table) return null;
    var headers = [];
    var headerCells = table.tHead ? table.tHead.rows[0].cells : [];
    for (var i = 0; i < headerCells.length; i++) {
        var text = headerCells[i].querySelector('.dv-th-text');
        if (text) headers.push(text.textContent.trim());
        else if (!headerCells[i].classList.contains('row-num-header') && !headerCells[i].classList.contains('dv-actions-header'))
            headers.push(headerCells[i].textContent.trim());
    }
    var rows = [];
    var tbody = table.tBodies[0];
    if (!tbody) return { headers: headers, rows: rows };
    var colStart = table.querySelector('.row-num-header') ? 1 : 0;
    for (var r = 0; r < tbody.rows.length; r++) {
        var row = [];
        for (var c = colStart; c < tbody.rows[r].cells.length; c++) {
            var td = tbody.rows[r].cells[c];
            if (td.classList.contains('dv-actions')) continue;
            var span = td.querySelector('.editable-cell, .dv-cell');
            if (span && span.getAttribute('data-full')) row.push(span.getAttribute('data-full'));
            else if (span) row.push(span.textContent);
            else row.push(td.textContent.trim());
        }
        rows.push(row);
    }
    return { headers: headers, rows: rows };
};

// ─── Find / Replace ──────────────────────────────────────────────────

SQLEditorInstance.prototype.toggleFind = function() {
    this.findVisible = !this.findVisible;
    this.findBar.style.display = this.findVisible ? 'flex' : 'none';
    if (this.findVisible) {
        var fi = this.findBar.querySelector('.find-input');
        // Pre-fill with selection
        var sel = this.textarea.value.substring(this.textarea.selectionStart, this.textarea.selectionEnd);
        if (sel && sel.indexOf('\n') === -1) fi.value = sel;
        fi.focus();
        fi.select();
    }
};

SQLEditorInstance.prototype.findNext = function(reverse) {
    var query = this.findBar.querySelector('.find-input').value;
    if (!query) return;
    var text = this.textarea.value.toLowerCase();
    query = query.toLowerCase();
    var from = reverse ? this.textarea.selectionStart - 1 : this.textarea.selectionEnd;
    var idx = reverse ? text.lastIndexOf(query, from - 1) : text.indexOf(query, from);
    if (idx === -1) {
        // Wrap around
        idx = reverse ? text.lastIndexOf(query) : text.indexOf(query);
    }
    if (idx !== -1) {
        this.textarea.selectionStart = idx;
        this.textarea.selectionEnd = idx + query.length;
        this.textarea.focus();
        // Count occurrences
        var count = 0, pos = 0;
        while ((pos = text.indexOf(query, pos)) !== -1) { count++; pos++; }
        this.findBar.querySelector('.find-count').textContent = count + ' found';
    } else {
        this.findBar.querySelector('.find-count').textContent = 'Not found';
    }
};

SQLEditorInstance.prototype.replaceOne = function() {
    var findStr = this.findBar.querySelector('.find-input').value;
    var replaceStr = this.findBar.querySelector('.replace-input').value;
    if (!findStr) return;
    var ta = this.textarea;
    var selected = ta.value.substring(ta.selectionStart, ta.selectionEnd);
    if (selected.toLowerCase() === findStr.toLowerCase()) {
        ta.value = ta.value.substring(0, ta.selectionStart) + replaceStr + ta.value.substring(ta.selectionEnd);
        ta.selectionStart = ta.selectionStart;
        ta.selectionEnd = ta.selectionStart + replaceStr.length;
        this.syncHighlight();
        this.updateGutter();
    }
    this.findNext(false);
};

SQLEditorInstance.prototype.replaceAll = function() {
    var findStr = this.findBar.querySelector('.find-input').value;
    var replaceStr = this.findBar.querySelector('.replace-input').value;
    if (!findStr) return;
    var ta = this.textarea;
    // Case-insensitive replace all
    var regex = new RegExp(findStr.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'gi');
    ta.value = ta.value.replace(regex, replaceStr);
    this.syncHighlight();
    this.updateGutter();
    this.findBar.querySelector('.find-count').textContent = 'Replaced all';
};

// ─── Simple SQL Formatter ────────────────────────────────────────────

SQLEditorInstance.prototype.formatSQL = function() {
    var sql = this.textarea.value;
    var tokens = tokenize(sql);
    var formatted = '';
    var indent = 0;
    var newlineKeywords = ['select', 'from', 'where', 'and', 'or', 'order', 'group',
        'having', 'limit', 'offset', 'join', 'inner', 'left', 'right', 'full',
        'cross', 'union', 'intersect', 'except', 'insert', 'update', 'delete',
        'set', 'values', 'returning', 'on', 'using', 'with'];
    var indentKeywords = ['select', 'from', 'where', 'set', 'values'];
    var needNewline = false;

    for (var i = 0; i < tokens.length; i++) {
        var t = tokens[i];
        if (t.type === T.WHITESPACE) {
            if (formatted.length > 0 && formatted[formatted.length - 1] !== ' ' && formatted[formatted.length - 1] !== '\n') {
                formatted += ' ';
            }
            continue;
        }
        if (t.type === T.KEYWORD && newlineKeywords.indexOf(t.value.toLowerCase()) !== -1) {
            if (formatted.length > 0) {
                formatted = formatted.replace(/\s+$/, '');
                formatted += '\n' + '    '.repeat(Math.max(0, indent));
            }
        }
        formatted += t.value;
        if (t.type === T.PUNCTUATION && t.value === ';') {
            formatted += '\n';
            indent = 0;
        }
    }

    this.textarea.value = formatted.trim();
    this.syncHighlight();
    this.updateGutter();
};

// ─── Event Binding ───────────────────────────────────────────────────

SQLEditorInstance.prototype.bindEvents = function() {
    var self = this;
    var ta = this.textarea;

    // Input -> highlight sync
    ta.addEventListener('input', function() {
        self.syncHighlight();
        self.updateGutter();
        // Trigger autocomplete on typing
        self.showAutocomplete();
    });

    // Scroll sync
    ta.addEventListener('scroll', function() {
        self.highlightPre.scrollTop = ta.scrollTop;
        self.highlightPre.scrollLeft = ta.scrollLeft;
        self.gutter.scrollTop = ta.scrollTop;
        self.updateCurrentLine();
        self.updateIndentGuides();
        self.updateMinimap();
        if (self.acVisible) self.positionAutocomplete();
    });

    // Keyboard shortcuts
    ta.addEventListener('keydown', function(e) {
        // Tab -> insert spaces
        if (e.key === 'Tab' && !self.acVisible) {
            e.preventDefault();
            if (e.shiftKey) {
                // Outdent current line
                var start = ta.selectionStart;
                var lineStart = ta.value.lastIndexOf('\n', start - 1) + 1;
                var line = ta.value.substring(lineStart, start);
                if (line.startsWith('    ')) {
                    ta.value = ta.value.substring(0, lineStart) + ta.value.substring(lineStart + 4);
                    ta.selectionStart = ta.selectionEnd = start - 4;
                }
            } else {
                var s = ta.selectionStart, en = ta.selectionEnd;
                ta.value = ta.value.substring(0, s) + '    ' + ta.value.substring(en);
                ta.selectionStart = ta.selectionEnd = s + 4;
            }
            self.syncHighlight();
            self.updateGutter();
            return;
        }

        // Tab/Enter when autocomplete is visible
        if (self.acVisible) {
            if (e.key === 'Tab' || e.key === 'Enter') {
                e.preventDefault();
                self.acceptCompletion();
                return;
            }
            if (e.key === 'ArrowDown') {
                e.preventDefault();
                self.acSelected = Math.min(self.acSelected + 1, self.acItems.length - 1);
                self.renderAutocomplete();
                // Scroll into view
                var sel = self.acPopup.querySelector('.selected');
                if (sel) sel.scrollIntoView({ block: 'nearest' });
                return;
            }
            if (e.key === 'ArrowUp') {
                e.preventDefault();
                self.acSelected = Math.max(self.acSelected - 1, 0);
                self.renderAutocomplete();
                var sel = self.acPopup.querySelector('.selected');
                if (sel) sel.scrollIntoView({ block: 'nearest' });
                return;
            }
            if (e.key === 'Escape') {
                e.preventDefault();
                self.hideAutocomplete();
                return;
            }
        }

        // Ctrl+Enter -> run query (or explain in explain mode)
        if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
            e.preventDefault();
            self.runQuery('run');
            return;
        }

        // Ctrl+Space -> force autocomplete
        if ((e.ctrlKey || e.metaKey) && e.key === ' ') {
            e.preventDefault();
            self.showAutocomplete();
            return;
        }

        // Ctrl+G -> go to line
        if ((e.ctrlKey || e.metaKey) && e.key === 'g') {
            e.preventDefault();
            self.goToLine();
            return;
        }

        // Ctrl+, -> settings
        if ((e.ctrlKey || e.metaKey) && e.key === ',') {
            e.preventDefault();
            self.openSettings();
            return;
        }

        // Ctrl+S -> save query
        if ((e.ctrlKey || e.metaKey) && e.key === 's') {
            e.preventDefault();
            self.saveQuery();
            return;
        }

        // Ctrl+F -> find
        if ((e.ctrlKey || e.metaKey) && e.key === 'f') {
            e.preventDefault();
            self.toggleFind();
            return;
        }

        // Ctrl+/ -> toggle comment
        if ((e.ctrlKey || e.metaKey) && e.key === '/') {
            e.preventDefault();
            self.toggleComment();
            return;
        }

        // Ctrl+D -> duplicate line
        if ((e.ctrlKey || e.metaKey) && e.key === 'd') {
            e.preventDefault();
            self.duplicateLine();
            return;
        }

        // Ctrl+Shift+K -> delete line
        if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'K') {
            e.preventDefault();
            self.deleteLine();
            return;
        }

        // Ctrl+L -> select line
        if ((e.ctrlKey || e.metaKey) && e.key === 'l') {
            e.preventDefault();
            var ls = ta.value.lastIndexOf('\n', ta.selectionStart - 1) + 1;
            var le = ta.value.indexOf('\n', ta.selectionEnd);
            if (le === -1) le = ta.value.length;
            ta.selectionStart = ls;
            ta.selectionEnd = le + 1;
            return;
        }

        // Auto-close brackets and quotes
        var pairs = { '(': ')', '[': ']', '{': '}', "'": "'", '"': '"' };
        if (pairs[e.key] && !e.ctrlKey && !e.metaKey) {
            var s = ta.selectionStart, en = ta.selectionEnd;
            if (s !== en) {
                // Wrap selection
                e.preventDefault();
                var selected = ta.value.substring(s, en);
                ta.value = ta.value.substring(0, s) + e.key + selected + pairs[e.key] + ta.value.substring(en);
                ta.selectionStart = s + 1;
                ta.selectionEnd = en + 1;
                self.syncHighlight();
                return;
            }
        }

        // Enter -> auto-indent
        if (e.key === 'Enter' && !self.acVisible) {
            var s = ta.selectionStart;
            var lineStart = ta.value.lastIndexOf('\n', s - 1) + 1;
            var line = ta.value.substring(lineStart, s);
            var match = line.match(/^(\s*)/);
            var indent = match ? match[1] : '';
            // Extra indent after certain keywords
            var trimmedLine = line.trim().toLowerCase();
            if (trimmedLine.endsWith('(') || trimmedLine === 'begin' || trimmedLine === 'then' ||
                trimmedLine === 'loop' || trimmedLine === 'else') {
                indent += '    ';
            }
            if (indent) {
                e.preventDefault();
                var before = ta.value.substring(0, s);
                var after = ta.value.substring(ta.selectionEnd);
                ta.value = before + '\n' + indent + after;
                ta.selectionStart = ta.selectionEnd = s + 1 + indent.length;
                self.syncHighlight();
                self.updateGutter();
            }
        }

        // Escape
        if (e.key === 'Escape') {
            if (self.findVisible) { self.toggleFind(); return; }
            self.hideAutocomplete();
        }
    });

    // Cursor position update on click and selection change
    ta.addEventListener('click', function() { self.updateCurrentLine(); });
    ta.addEventListener('select', function() { self.updateCurrentLine(); });
    document.addEventListener('selectionchange', function() {
        if (document.activeElement === ta) self.updateCurrentLine();
    });

    // Arrow keys update cursor position
    ta.addEventListener('keyup', function(e) {
        if (['ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Home','End','PageUp','PageDown'].indexOf(e.key) !== -1) {
            self.updateCurrentLine();
        }
    });

    // Minimap click to scroll
    if (this.minimapEl) {
        this.minimapEl.addEventListener('click', function(e) {
            var rect = self.minimapEl.getBoundingClientRect();
            var ratio = (e.clientY - rect.top) / rect.height;
            ta.scrollTop = ratio * (ta.scrollHeight - ta.clientHeight);
        });
    }

    // Click outside hides autocomplete
    document.addEventListener('mousedown', function(e) {
        if (self.acVisible && !self.acPopup.contains(e.target)) {
            self.hideAutocomplete();
        }
    });

    // Autocomplete item click
    this.acPopup.addEventListener('mousedown', function(e) {
        var item = e.target.closest('.ac-item');
        if (item) {
            e.preventDefault();
            self.acSelected = parseInt(item.getAttribute('data-ac-idx'));
            self.acceptCompletion();
        }
    });

    // Toolbar actions
    this.toolbarEl.addEventListener('click', function(e) {
        var btn = e.target.closest('[data-action]');
        if (!btn) return;
        var action = btn.getAttribute('data-action');
        if (action === 'run') self.runQuery('run');
        else if (action === 'explain') self.runQuery('explain');
        else if (action === 'explain-analyze') self.runQuery('explain-analyze');
        else if (action === 'dry-run') self.runQuery('dry-run');
        else if (action === 'diff') self.showDiff();
        else if (action === 'find') self.toggleFind();
        else if (action === 'format') self.formatSQL();
        else if (action === 'history') self.showHistory();
        else if (action === 'save') self.saveQuery();
        else if (action === 'saved') self.showSavedQueries();
        else if (action === 'export-csv' || action === 'export-json' || action === 'export-sql') {
            var fmt = action.replace('export-', '');
            // Find DataView in results area and click its export button
            var dvExport = self.resultsEl.querySelector('[data-dv-export="' + fmt + '"]');
            if (dvExport) dvExport.click();
            else if (typeof exportResults === 'function') exportResults(fmt);
        }
        else if (action === 'settings') self.openSettings();
        else if (action === 'shortcuts') self.showShortcuts();
    });

    // Tab bar clicks
    this.tabBar.addEventListener('click', function(e) {
        var closeBtn = e.target.closest('[data-close-tab]');
        if (closeBtn) {
            e.stopPropagation();
            self.closeTab(parseInt(closeBtn.getAttribute('data-close-tab')));
            return;
        }
        var addBtn = e.target.closest('[data-action="new-tab"]');
        if (addBtn) {
            self.addTab('Query ' + (self.tabs.length + 1));
            return;
        }
        var tab = e.target.closest('[data-tab-id]');
        if (tab) {
            self.switchTab(parseInt(tab.getAttribute('data-tab-id')));
        }
    });

    // Find bar
    this.findBar.addEventListener('click', function(e) {
        var btn = e.target.closest('[data-find]');
        if (!btn) return;
        var action = btn.getAttribute('data-find');
        if (action === 'next') self.findNext(false);
        else if (action === 'prev') self.findNext(true);
        else if (action === 'replace') self.replaceOne();
        else if (action === 'replace-all') self.replaceAll();
        else if (action === 'close') self.toggleFind();
    });
    this.findBar.addEventListener('keydown', function(e) {
        if (e.key === 'Enter') {
            e.preventDefault();
            self.findNext(e.shiftKey);
        }
        if (e.key === 'Escape') {
            self.toggleFind();
        }
    });

    // Results: load or run history item
    this.resultsEl.addEventListener('click', function(e) {
        var loadBtn = e.target.closest('[data-history-load]');
        if (loadBtn) {
            var idx = parseInt(loadBtn.getAttribute('data-history-load'));
            self.migrateHistory();
            if (self.history[idx]) {
                var entry = self.history[idx];
                ta.value = typeof entry === 'string' ? entry : entry.sql;
                self.syncHighlight();
                self.updateGutter();
                ta.focus();
            }
            return;
        }
        var runBtn = e.target.closest('[data-history-run]');
        if (runBtn) {
            var idx = parseInt(runBtn.getAttribute('data-history-run'));
            self.migrateHistory();
            if (self.history[idx]) {
                var entry = self.history[idx];
                ta.value = typeof entry === 'string' ? entry : entry.sql;
                self.syncHighlight();
                self.updateGutter();
                self.runQuery('run');
            }
            return;
        }
    });

    // Resize handle
    var resizing = false, startY = 0, startH = 0;
    this.resizeHandle.addEventListener('mousedown', function(e) {
        resizing = true;
        startY = e.clientY;
        startH = self.editorRegion.offsetHeight;
        document.body.style.cursor = 'ns-resize';
        document.body.style.userSelect = 'none';
        self.resizeHandle.classList.add('dragging');
        e.preventDefault();
    });
    document.addEventListener('mousemove', function(e) {
        if (!resizing) return;
        var h = startH + (e.clientY - startY);
        h = Math.max(120, Math.min(window.innerHeight * 0.75, h));
        self.editorRegion.style.height = h + 'px';
        self.editorRegion.style.minHeight = h + 'px';
        e.preventDefault();
    });
    document.addEventListener('mouseup', function() {
        if (!resizing) return;
        resizing = false;
        document.body.style.cursor = '';
        document.body.style.userSelect = '';
        self.resizeHandle.classList.remove('dragging');
    });

    // ─── File drag & drop ──────────────────────────────────────────────
    var dropOverlay = document.createElement('div');
    dropOverlay.className = 'editor-drop-overlay';
    dropOverlay.innerHTML = '<div class="editor-drop-message">Drop .sql file to load</div>';
    dropOverlay.style.display = 'none';
    this.editorRegion.appendChild(dropOverlay);

    var dragCounter = 0;
    this.editorRegion.addEventListener('dragenter', function(e) {
        e.preventDefault();
        dragCounter++;
        dropOverlay.style.display = 'flex';
    });
    this.editorRegion.addEventListener('dragleave', function(e) {
        e.preventDefault();
        dragCounter--;
        if (dragCounter <= 0) { dropOverlay.style.display = 'none'; dragCounter = 0; }
    });
    this.editorRegion.addEventListener('dragover', function(e) {
        e.preventDefault();
        e.dataTransfer.dropEffect = 'copy';
    });
    this.editorRegion.addEventListener('drop', function(e) {
        e.preventDefault();
        dragCounter = 0;
        dropOverlay.style.display = 'none';

        var files = e.dataTransfer.files;
        if (files.length === 0) return;

        // Load each file as a new tab
        Array.from(files).forEach(function(file) {
            var reader = new FileReader();
            reader.onload = function(ev) {
                var content = ev.target.result;
                var name = file.name.replace(/\.sql$/i, '');
                self.addTab(name, content);
            };
            reader.readAsText(file);
        });
    });

    // Initial sync
    this.syncHighlight();
    this.updateGutter();
    this.updateCurrentLine();
    this.updateIndentGuides();
};

// ─── Line Operations ─────────────────────────────────────────────────

SQLEditorInstance.prototype.toggleComment = function() {
    var ta = this.textarea;
    var start = ta.selectionStart;
    var end = ta.selectionEnd;
    var text = ta.value;

    var lineStart = text.lastIndexOf('\n', start - 1) + 1;
    var lineEnd = text.indexOf('\n', end);
    if (lineEnd === -1) lineEnd = text.length;

    var lines = text.substring(lineStart, lineEnd).split('\n');
    var allCommented = lines.every(function(l) { return l.trimStart().startsWith('--'); });

    var newLines;
    if (allCommented) {
        newLines = lines.map(function(l) { return l.replace(/^(\s*)-- ?/, '$1'); });
    } else {
        newLines = lines.map(function(l) { return l.replace(/^(\s*)/, '$1-- '); });
    }

    var newText = newLines.join('\n');
    ta.value = text.substring(0, lineStart) + newText + text.substring(lineEnd);
    ta.selectionStart = lineStart;
    ta.selectionEnd = lineStart + newText.length;
    this.syncHighlight();
    this.updateGutter();
};

SQLEditorInstance.prototype.duplicateLine = function() {
    var ta = this.textarea;
    var pos = ta.selectionStart;
    var lineStart = ta.value.lastIndexOf('\n', pos - 1) + 1;
    var lineEnd = ta.value.indexOf('\n', pos);
    if (lineEnd === -1) lineEnd = ta.value.length;
    var line = ta.value.substring(lineStart, lineEnd);
    ta.value = ta.value.substring(0, lineEnd) + '\n' + line + ta.value.substring(lineEnd);
    ta.selectionStart = ta.selectionEnd = pos + line.length + 1;
    this.syncHighlight();
    this.updateGutter();
};

SQLEditorInstance.prototype.deleteLine = function() {
    var ta = this.textarea;
    var pos = ta.selectionStart;
    var lineStart = ta.value.lastIndexOf('\n', pos - 1) + 1;
    var lineEnd = ta.value.indexOf('\n', pos);
    if (lineEnd === -1) lineEnd = ta.value.length;
    else lineEnd++;
    ta.value = ta.value.substring(0, lineStart) + ta.value.substring(lineEnd);
    ta.selectionStart = ta.selectionEnd = Math.min(lineStart, ta.value.length);
    this.syncHighlight();
    this.updateGutter();
};

// ─── Current Line Highlight + Cursor Info ────────────────────────

SQLEditorInstance.prototype.updateCurrentLine = function() {
    var ta = this.textarea;
    var text = ta.value.substring(0, ta.selectionStart);
    var line = (text.match(/\n/g) || []).length;
    var col = ta.selectionStart - text.lastIndexOf('\n') - 1;
    var totalLines = (ta.value.match(/\n/g) || []).length + 1;

    // Position current line highlight
    if (this.settings.lineHighlight && this.currentLineEl) {
        var lineH = parseFloat(getComputedStyle(ta).lineHeight) || (this.settings.fontSize * 1.6);
        var padTop = parseFloat(getComputedStyle(ta).paddingTop) || 12;
        this.currentLineEl.style.top = (padTop + line * lineH - ta.scrollTop) + 'px';
        this.currentLineEl.style.height = lineH + 'px';
        this.currentLineEl.style.display = '';
    } else if (this.currentLineEl) {
        this.currentLineEl.style.display = 'none';
    }

    // Active line number in gutter
    var lineNums = this.gutter.querySelectorAll('.line-num');
    lineNums.forEach(function(el, i) {
        el.classList.toggle('active', i === line);
    });

    // Cursor position display
    var cursorInfo = this.toolbarEl.querySelector('.cursor-pos');
    if (cursorInfo) {
        var selLen = Math.abs(ta.selectionEnd - ta.selectionStart);
        var info = 'Ln ' + (line + 1) + ', Col ' + (col + 1);
        if (selLen > 0) info += ' (' + selLen + ' selected)';
        info += '  |  ' + totalLines + ' lines';
        cursorInfo.textContent = info;
    }

    // Block cursor positioning
    if (this.settings.blockCursor && this.blockCursorEl) {
        this.editorArea.classList.add('block-cursor');
        var lineH = parseFloat(getComputedStyle(ta).lineHeight) || (this.settings.fontSize * 1.6);
        var padTop = parseFloat(getComputedStyle(ta).paddingTop) || 12;
        var padLeft = parseFloat(getComputedStyle(ta).paddingLeft) || 12;
        if (!this._charWidth) {
            var sp = document.createElement('span');
            sp.style.cssText = 'position:absolute;visibility:hidden;font-family:' + getComputedStyle(ta).fontFamily + ';font-size:' + getComputedStyle(ta).fontSize;
            sp.textContent = 'x';
            document.body.appendChild(sp);
            this._charWidth = sp.getBoundingClientRect().width;
            document.body.removeChild(sp);
        }
        this.blockCursorEl.style.display = (ta.selectionStart === ta.selectionEnd) ? '' : 'none';
        this.blockCursorEl.style.top = (padTop + line * lineH - ta.scrollTop) + 'px';
        this.blockCursorEl.style.left = (padLeft + col * this._charWidth - ta.scrollLeft) + 'px';
        this.blockCursorEl.style.height = lineH + 'px';
    } else if (this.blockCursorEl) {
        this.editorArea.classList.remove('block-cursor');
        this.blockCursorEl.style.display = 'none';
    }

    // Selection word highlighting
    var newWord = '';
    if (ta.selectionStart !== ta.selectionEnd) {
        var sel = ta.value.substring(ta.selectionStart, ta.selectionEnd);
        if (/^\w+$/.test(sel) && sel.length >= 2 && sel.length <= 60) {
            newWord = sel;
        }
    }
    if (newWord !== this.selectedWord) {
        this.selectedWord = newWord;
        this.highlightPre.innerHTML = highlight(ta.value, this.selectedWord);
    }
};

// ─── Minimap ─────────────────────────────────────────────────────

SQLEditorInstance.prototype.updateMinimap = function() {
    if (!this.settings.minimap || !this.minimapCanvas) return;
    var text = this.textarea.value;
    var lines = text.split('\n');
    var canvas = this.minimapCanvas;
    var ctx = canvas.getContext('2d');
    var dpr = window.devicePixelRatio || 1;
    var w = 64;
    var lineH = 2;
    var h = Math.max(this.editorArea.clientHeight, lines.length * lineH);

    canvas.width = w * dpr;
    canvas.height = h * dpr;
    canvas.style.height = h + 'px';
    ctx.scale(dpr, dpr);
    ctx.clearRect(0, 0, w, h);

    // Color map for tokens
    var colorMap = {};
    var cs = getComputedStyle(this.editorContainer);
    colorMap[T.KEYWORD] = cs.getPropertyValue('--ed-kw').trim() || '#7ee0ff';
    colorMap[T.STRING] = cs.getPropertyValue('--ed-st').trim() || '#a5d6a7';
    colorMap[T.COMMENT] = cs.getPropertyValue('--ed-cm').trim() || '#6e7681';
    colorMap[T.NUMBER] = cs.getPropertyValue('--ed-nu').trim() || '#ffab70';
    colorMap[T.BUILTIN] = cs.getPropertyValue('--ed-fn').trim() || '#dcbdfb';
    colorMap[T.TYPE] = cs.getPropertyValue('--ed-ty').trim() || '#56d4dd';
    var defaultColor = cs.getPropertyValue('--ed-fg').trim() || '#c9d1d9';

    // Simple render: draw colored rectangles per character
    var charW = 1;
    lines.forEach(function(line, lineIdx) {
        var y = lineIdx * lineH;
        if (y > h) return;
        var tokens = tokenize(line);
        var x = 4;
        tokens.forEach(function(tok) {
            if (tok.type === T.WHITESPACE) { x += tok.value.length * charW; return; }
            var color = colorMap[tok.type] || defaultColor;
            ctx.fillStyle = color;
            ctx.globalAlpha = 0.6;
            ctx.fillRect(x, y, tok.value.length * charW, lineH);
            x += tok.value.length * charW;
        });
    });
    ctx.globalAlpha = 1.0;

    // Viewport indicator
    var ta = this.textarea;
    var visibleRatio = ta.clientHeight / Math.max(ta.scrollHeight, 1);
    var scrollRatio = ta.scrollTop / Math.max(ta.scrollHeight - ta.clientHeight, 1);
    var vpH = Math.max(20, h * visibleRatio);
    var vpY = (h - vpH) * scrollRatio;
    this.minimapViewport.style.top = vpY + 'px';
    this.minimapViewport.style.height = vpH + 'px';
};

// ─── Indent Guides ───────────────────────────────────────────────

SQLEditorInstance.prototype.updateIndentGuides = function() {
    if (!this.settings.indentGuides || !this.indentGuidesEl) {
        if (this.indentGuidesEl) this.indentGuidesEl.innerHTML = '';
        return;
    }
    var ta = this.textarea;
    var text = ta.value;
    var lines = text.split('\n');
    var tabSize = this.settings.tabSize || 4;
    var lineH = parseFloat(getComputedStyle(ta).lineHeight) || (this.settings.fontSize * 1.6);
    var padTop = parseFloat(getComputedStyle(ta).paddingTop) || 12;
    var padLeft = parseFloat(getComputedStyle(ta).paddingLeft) || 12;

    // Measure char width
    if (!this._charWidth) {
        var span = document.createElement('span');
        span.style.cssText = 'position:absolute;visibility:hidden;font-family:' + getComputedStyle(ta).fontFamily + ';font-size:' + getComputedStyle(ta).fontSize;
        span.textContent = 'x';
        document.body.appendChild(span);
        this._charWidth = span.getBoundingClientRect().width;
        document.body.removeChild(span);
    }
    var charW = this._charWidth;

    var html = '';
    var maxGuides = 8;
    for (var level = 1; level <= maxGuides; level++) {
        var x = padLeft + level * tabSize * charW;
        // Find line ranges where indent >= this level
        var start = -1;
        for (var i = 0; i <= lines.length; i++) {
            var lineIndent = 0;
            if (i < lines.length) {
                var m = lines[i].match(/^(\s*)/);
                if (m) lineIndent = Math.floor(m[1].replace(/\t/g, '    ').length / tabSize);
            }
            if (lineIndent >= level) {
                if (start === -1) start = i;
            } else {
                if (start !== -1) {
                    var top = padTop + start * lineH - ta.scrollTop;
                    var height = (i - start) * lineH;
                    if (top + height > 0 && top < ta.clientHeight) {
                        html += '<div class="indent-guide" style="left:' + x.toFixed(1) + 'px;top:' + top.toFixed(1) + 'px;height:' + height.toFixed(1) + 'px"></div>';
                    }
                    start = -1;
                }
            }
        }
    }
    this.indentGuidesEl.innerHTML = html;
};

// ─── Apply Settings ──────────────────────────────────────────────

SQLEditorInstance.prototype.applySettings = function() {
    var s = this.settings;

    // Theme
    if (s.theme) {
        this.editorContainer.setAttribute('data-editor-theme', s.theme);
    } else {
        this.editorContainer.removeAttribute('data-editor-theme');
    }

    // Font size
    this.editorContainer.style.setProperty('--ed-font-size', s.fontSize + 'px');

    // Tab size
    this.editorContainer.style.setProperty('--ed-tab-size', s.tabSize);

    // Word wrap
    var wrapVal = s.wordWrap ? 'pre-wrap' : 'pre';
    if (this.textarea) this.textarea.style.whiteSpace = wrapVal;
    if (this.highlightPre) this.highlightPre.style.whiteSpace = wrapVal;
    if (this.textarea) this.textarea.style.overflowX = s.wordWrap ? 'hidden' : 'auto';
    if (this.highlightPre) this.highlightPre.style.overflowX = s.wordWrap ? 'hidden' : 'auto';

    // Minimap
    if (this.minimapEl) {
        this.minimapEl.style.display = s.minimap ? '' : 'none';
        this.editorArea.classList.toggle('has-minimap', s.minimap);
    }

    // Current line highlight
    if (this.currentLineEl) {
        this.currentLineEl.style.display = s.lineHighlight ? '' : 'none';
    }

    // Block cursor
    if (this.editorArea) {
        this.editorArea.classList.toggle('block-cursor', !!s.blockCursor);
    }

    // Reset char width cache on font size change
    this._charWidth = null;
};

// ─── Settings Panel ──────────────────────────────────────────────

SQLEditorInstance.prototype.openSettings = function() {
    var self = this;
    if (document.querySelector('.command-overlay')) return;

    var s = this.settings;
    var themes = [
        { id: '', name: 'Auto', bg: '#161b22', fg: '#7ee0ff' },
        { id: 'monokai', name: 'Monokai', bg: '#272822', fg: '#f92672' },
        { id: 'dracula', name: 'Dracula', bg: '#282a36', fg: '#ff79c6' },
        { id: 'nord', name: 'Nord', bg: '#2e3440', fg: '#81a1c1' },
        { id: 'solarized', name: 'Solarized', bg: '#002b36', fg: '#268bd2' },
        { id: 'one-dark', name: 'One Dark', bg: '#282c34', fg: '#c678dd' },
        { id: 'github-light', name: 'GitHub', bg: '#ffffff', fg: '#cf222e' },
        { id: 'high-contrast', name: 'HC', bg: '#000000', fg: '#6cf' },
    ];

    var overlay = document.createElement('div');
    overlay.className = 'command-overlay';
    var panel = document.createElement('div');
    panel.className = 'settings-panel';

    var themeSwatches = themes.map(function(t) {
        return '<div class="theme-swatch' + (s.theme === t.id ? ' active' : '') + '" ' +
            'data-theme-id="' + t.id + '" ' +
            'style="background:' + t.bg + ';color:' + t.fg + '" title="' + t.name + '">' +
            t.name.substring(0, 3) + '</div>';
    }).join('');

    panel.innerHTML =
        '<div class="settings-header"><span>Editor Settings</span><button data-close>&times;</button></div>' +
        '<div class="settings-body">' +
        '<div class="settings-group"><div class="settings-group-title">Appearance</div>' +
        '<div class="settings-row"><span class="settings-label">Theme</span></div>' +
        '<div class="theme-preview">' + themeSwatches + '</div>' +
        '<div class="settings-row"><span class="settings-label">Font Size</span>' +
        '<input type="number" class="settings-input-num" value="' + s.fontSize + '" min="10" max="24" data-setting="fontSize"></div>' +
        '</div>' +
        '<div class="settings-group"><div class="settings-group-title">Editor</div>' +
        '<div class="settings-row"><span class="settings-label">Tab Size</span>' +
        '<input type="number" class="settings-input-num" value="' + s.tabSize + '" min="2" max="8" data-setting="tabSize"></div>' +
        '<div class="settings-row"><span class="settings-label">Word Wrap</span>' +
        '<button class="settings-toggle' + (s.wordWrap ? ' on' : '') + '" data-toggle="wordWrap"></button></div>' +
        '<div class="settings-row"><span class="settings-label">Minimap</span>' +
        '<button class="settings-toggle' + (s.minimap ? ' on' : '') + '" data-toggle="minimap"></button></div>' +
        '<div class="settings-row"><span class="settings-label">Line Highlight</span>' +
        '<button class="settings-toggle' + (s.lineHighlight ? ' on' : '') + '" data-toggle="lineHighlight"></button></div>' +
        '<div class="settings-row"><span class="settings-label">Bracket Colors</span>' +
        '<button class="settings-toggle' + (s.bracketColors ? ' on' : '') + '" data-toggle="bracketColors"></button></div>' +
        '<div class="settings-row"><span class="settings-label">Indent Guides</span>' +
        '<button class="settings-toggle' + (s.indentGuides ? ' on' : '') + '" data-toggle="indentGuides"></button></div>' +
        '<div class="settings-row"><span class="settings-label">Block Cursor</span>' +
        '<button class="settings-toggle' + (s.blockCursor ? ' on' : '') + '" data-toggle="blockCursor"></button></div>' +
        '</div></div>';

    overlay.appendChild(panel);
    document.body.appendChild(overlay);

    // Event handlers
    overlay.addEventListener('click', function(e) {
        if (e.target === overlay || e.target.hasAttribute('data-close')) {
            overlay.remove();
            return;
        }
        var swatch = e.target.closest('.theme-swatch');
        if (swatch) {
            var themeId = swatch.getAttribute('data-theme-id');
            self.settings.theme = themeId;
            EditorSettings.set('theme', themeId);
            self.applySettings();
            self.syncHighlight();
            panel.querySelectorAll('.theme-swatch').forEach(function(s) { s.classList.remove('active'); });
            swatch.classList.add('active');
            return;
        }
        var toggle = e.target.closest('.settings-toggle');
        if (toggle) {
            var key = toggle.getAttribute('data-toggle');
            toggle.classList.toggle('on');
            self.settings[key] = toggle.classList.contains('on');
            EditorSettings.set(key, self.settings[key]);
            self.applySettings();
            self.syncHighlight();
            self.updateIndentGuides();
        }
    });

    panel.addEventListener('change', function(e) {
        var input = e.target;
        var key = input.getAttribute('data-setting');
        if (!key) return;
        var val = parseInt(input.value);
        if (isNaN(val)) return;
        self.settings[key] = val;
        EditorSettings.set(key, val);
        self.applySettings();
        self.syncHighlight();
        self.updateGutter();
    });
};

// ─── Go to Line ──────────────────────────────────────────────────

SQLEditorInstance.prototype.goToLine = function() {
    var self = this;
    this.gotoLineBar.style.display = 'flex';
    var input = this.gotoLineBar.querySelector('input');
    input.value = '';
    input.focus();

    var handler = function(e) {
        if (e.key === 'Enter') {
            var lineNum = parseInt(input.value);
            if (!isNaN(lineNum) && lineNum > 0) {
                var lines = self.textarea.value.split('\n');
                var offset = 0;
                for (var i = 0; i < Math.min(lineNum - 1, lines.length); i++) {
                    offset += lines[i].length + 1;
                }
                self.textarea.selectionStart = self.textarea.selectionEnd = offset;
                self.textarea.focus();
                self.updateCurrentLine();
            }
            self.gotoLineBar.style.display = 'none';
            input.removeEventListener('keydown', handler);
        } else if (e.key === 'Escape') {
            self.gotoLineBar.style.display = 'none';
            self.textarea.focus();
            input.removeEventListener('keydown', handler);
        }
    };
    input.addEventListener('keydown', handler);
};

// ─── Keyboard Shortcut Reference ─────────────────────────────────

SQLEditorInstance.prototype.showShortcuts = function() {
    if (document.querySelector('.command-overlay')) return;
    var overlay = document.createElement('div');
    overlay.className = 'command-overlay';
    overlay.innerHTML =
        '<div class="settings-panel">' +
        '<div class="settings-header"><span>Keyboard Shortcuts</span><button onclick="this.closest(\'.command-overlay\').remove()">&times;</button></div>' +
        '<div class="shortcuts-grid">' +
        '<div class="shortcut-item"><span class="label">Run Query</span><kbd>Ctrl+Enter</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Autocomplete</span><kbd>Ctrl+Space</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Find / Replace</span><kbd>Ctrl+F</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Go to Line</span><kbd>Ctrl+G</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Save Query</span><kbd>Ctrl+S</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Toggle Comment</span><kbd>Ctrl+/</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Duplicate Line</span><kbd>Ctrl+D</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Delete Line</span><kbd>Ctrl+Shift+K</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Select Line</span><kbd>Ctrl+L</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Indent</span><kbd>Tab</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Outdent</span><kbd>Shift+Tab</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Command Palette</span><kbd>Ctrl+K</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Settings</span><kbd>Ctrl+,</kbd></div>' +
        '<div class="shortcut-item"><span class="label">Close Panel</span><kbd>Escape</kbd></div>' +
        '</div></div>';
    document.body.appendChild(overlay);
    overlay.addEventListener('click', function(e) { if (e.target === overlay) overlay.remove(); });
};

// ─── Saved Queries ───────────────────────────────────────────────

SQLEditorInstance.prototype.saveQuery = function() {
    var sql = this.textarea.value.trim();
    if (!sql) return;
    var tab = this.tabs.find(function(t) { return t.id === this.activeTabId; }.bind(this));
    var defaultName = tab ? tab.title : 'Untitled';
    var name = prompt('Save query as:', defaultName);
    if (!name) return;
    SavedQueries.save(name, sql);
    // Update tab title
    if (tab) {
        tab.title = name;
        this.renderTabs();
    }
    this.updateStatus('Saved: ' + name);
};

SQLEditorInstance.prototype.showSavedQueries = function() {
    var self = this;
    this.resultsEl.innerHTML = '<div style="padding:var(--sp-4)"><h3>Saved Queries</h3>' + SavedQueries.render() + '</div>';

    // Bind load/delete buttons
    this.resultsEl.querySelectorAll('[data-load-query]').forEach(function(btn) {
        btn.addEventListener('click', function() {
            var name = this.getAttribute('data-load-query');
            var queries = SavedQueries.getAll();
            var q = queries.find(function(x) { return x.name === name; });
            if (q) {
                self.textarea.value = q.sql;
                self.syncHighlight();
                self.updateGutter();
                var tab = self.tabs.find(function(t) { return t.id === self.activeTabId; });
                if (tab) { tab.title = name; self.renderTabs(); }
                self.textarea.focus();
            }
        });
    });
    this.resultsEl.querySelectorAll('[data-delete-query]').forEach(function(btn) {
        btn.addEventListener('click', function() {
            var name = this.getAttribute('data-delete-query');
            if (confirm('Delete "' + name + '"?')) {
                SavedQueries.remove(name);
                self.showSavedQueries();
            }
        });
    });
};

// ─── Public API ──────────────────────────────────────────────────────

window.SQLEditor = {
    init: function(container, opts) {
        return new SQLEditorInstance(container, opts);
    }
};

// Auto-init on page load
function autoInit() {
    var ws = document.getElementById('query-workspace');
    if (ws) SQLEditor.init(ws);
}

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', autoInit);
} else {
    autoInit();
}

})();
)_JS_"; }
};

} // namespace getgresql::ssr
