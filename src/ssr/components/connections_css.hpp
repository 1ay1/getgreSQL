#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ConnectionsCSS {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
/* ─── Connection Manager ─────────────────────────────────────────────── */

.conn-current {
    margin-bottom: var(--sp-5);
}

.conn-info-card {
    display: flex;
    align-items: center;
    gap: var(--sp-4);
    padding: var(--sp-4);
    background: var(--bg-2);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius);
}

.conn-dot-lg {
    width: 12px;
    height: 12px;
    background: var(--success);
    border-radius: 50%;
    box-shadow: 0 0 6px rgba(63, 185, 80, 0.4);
    flex-shrink: 0;
}

.conn-url {
    font-family: var(--font-mono);
    font-size: var(--font-size-xs);
    color: var(--text-3);
    word-break: break-all;
    margin-top: 2px;
}

.conn-add-section, .conn-saved-section {
    margin-bottom: var(--sp-5);
}

.conn-form-grid {
    display: grid;
    grid-template-columns: 1fr 2fr auto;
    gap: var(--sp-3);
    align-items: end;
    margin-bottom: var(--sp-3);
}

@media (max-width: 640px) {
    .conn-form-grid { grid-template-columns: 1fr; }
}

.form-field label {
    display: block;
    font-size: var(--font-size-xs);
    color: var(--text-2);
    margin-bottom: var(--sp-1);
    font-weight: 600;
}

.form-input {
    width: 100%;
    padding: var(--sp-2) var(--sp-3);
    background: var(--bg-1);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    color: var(--text-0);
    font-size: var(--font-size-sm);
    font-family: inherit;
    outline: none;
    transition: border-color var(--transition-fast);
}

.form-input:focus {
    border-color: var(--accent);
}

.form-input::placeholder { color: var(--text-4); }

select.form-input {
    cursor: pointer;
}

.conn-form-actions {
    display: flex;
    gap: var(--sp-2);
    margin-bottom: var(--sp-3);
}

.conn-item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: var(--sp-3) var(--sp-4);
    background: var(--bg-2);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius);
    margin-bottom: var(--sp-2);
    transition: border-color var(--transition-fast);
}

.conn-item:hover {
    border-color: var(--border);
}

.conn-item.conn-active {
    border-color: var(--success);
    background: rgba(63, 185, 80, 0.05);
}

.conn-item-info {
    flex: 1;
    min-width: 0;
}

.conn-item-actions {
    display: flex;
    gap: var(--sp-2);
    flex-shrink: 0;
    margin-left: var(--sp-3);
}

/* ─── Settings toolbar ────────────────────────────────────────────── */

.settings-toolbar {
    display: flex;
    align-items: center;
    gap: var(--sp-3);
    margin-bottom: var(--sp-4);
    flex-wrap: wrap;
}

/* ─── Toolbar connection indicator ────────────────────────────────── */

.toolbar-conn {
    display: flex;
    align-items: center;
    gap: var(--sp-2);
    padding: 2px var(--sp-3);
    border-radius: var(--radius);
    font-size: var(--font-size-xs);
    color: var(--text-2);
    cursor: pointer;
    transition: background var(--transition-fast);
    text-decoration: none;
    white-space: nowrap;
}

.toolbar-conn:hover {
    background: var(--bg-3);
    color: var(--text-0);
}

.toolbar-conn .conn-dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--success);
    flex-shrink: 0;
}

.toolbar-conn .conn-label {
    font-weight: 600;
}
)_CSS_"; }
};

} // namespace getgresql::ssr
