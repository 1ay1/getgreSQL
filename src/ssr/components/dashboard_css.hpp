#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct DashboardCSS {
    static constexpr auto css() -> std::string_view { return R"_CSS_(
/* HealthCard: co-located in health_card.hpp */

/* ═══════════════════════════════════════════════════════════════════════
   UX Polish — Final Pass
   ═══════════════════════════════════════════════════════════════════════ */


/* ─── Dashboard Hero ──────────────────────────────────────────────────── */

.dash-hero {
    display: flex;
    align-items: center;
    gap: var(--sp-5);
    padding: var(--sp-4) var(--sp-5);
    background: linear-gradient(135deg, var(--bg-1) 0%, var(--bg-2) 100%);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
    margin-bottom: var(--sp-4);
    flex-wrap: wrap;
}

.dash-hero-status {
    display: flex;
    align-items: center;
    gap: var(--sp-3);
    position: relative;
    padding: var(--sp-2) var(--sp-4);
    border-radius: 100px;
    font-weight: 700;
    font-size: var(--font-size-sm);
    letter-spacing: 0.02em;
    text-transform: uppercase;
    flex-shrink: 0;
}
.dash-hero-success {
    background: rgba(63, 185, 80, 0.1);
    color: var(--success);
    border: 1px solid rgba(63, 185, 80, 0.2);
}
.dash-hero-warning {
    background: rgba(210, 153, 34, 0.1);
    color: var(--warning);
    border: 1px solid rgba(210, 153, 34, 0.2);
}
.dash-hero-danger {
    background: rgba(248, 81, 73, 0.1);
    color: var(--danger);
    border: 1px solid rgba(248, 81, 73, 0.2);
}

.dash-hero-dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: currentColor;
    flex-shrink: 0;
    position: relative;
    z-index: 1;
}
.dash-hero-pulse {
    position: absolute;
    left: 16px;
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: currentColor;
    animation: dash-pulse 2s ease-out infinite;
}
@keyframes dash-pulse {
    0% { transform: scale(1); opacity: 0.6; }
    100% { transform: scale(3); opacity: 0; }
}

.dash-checks {
    display: flex;
    gap: var(--sp-3);
    flex-wrap: wrap;
    flex: 1;
}
.dash-check {
    display: flex;
    align-items: center;
    gap: var(--sp-2);
    padding: var(--sp-1) var(--sp-3);
    border-radius: var(--radius);
    font-size: var(--font-size-xs);
    font-family: var(--font-mono);
    background: var(--bg-1);
    border: 1px solid var(--border-subtle);
    transition: all var(--transition-fast);
    cursor: default;
}
.dash-check:hover {
    border-color: var(--border);
    transform: translateY(-1px);
    box-shadow: var(--shadow-sm);
}
.dash-check-icon { font-size: 0.85rem; }
.dash-check-success .dash-check-icon { color: var(--success); }
.dash-check-warning .dash-check-icon { color: var(--warning); }
.dash-check-danger .dash-check-icon { color: var(--danger); }
.dash-check-name { color: var(--text-2); }
.dash-check-val { color: var(--text-0); font-weight: 600; }

/* ─── Dashboard Server Bar ───────────────────────────────────────────── */

/* ─── Dashboard Two-Column Row ───────────────────────────────────────── */

.dash-row {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: var(--sp-4);
    margin-bottom: var(--sp-4);
}
@media (max-width: 900px) {
    .dash-row { grid-template-columns: 1fr; }
}

.dash-server {
    padding: var(--sp-3) var(--sp-5);
    background: var(--bg-1);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
    margin-bottom: var(--sp-4);
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: var(--sp-4);
    flex-wrap: wrap;
}
.dash-server-info {
    display: flex;
    align-items: center;
    gap: var(--sp-4);
    flex-wrap: wrap;
}
.dash-server-actions {
    display: flex;
    gap: var(--sp-2);
}
.dash-server-version {
    font-family: var(--font-mono);
    font-size: var(--font-size-xs);
    color: var(--text-2);
    background: var(--bg-2);
    padding: var(--sp-1) var(--sp-3);
    border-radius: var(--radius);
    border: 1px solid var(--border-subtle);
}
.dash-server-uptime {
    font-size: var(--font-size-xs);
    color: var(--text-3);
}
.dash-server-uptime strong { color: var(--text-1); }
.dash-server-pid {
    font-family: var(--font-mono);
    font-size: var(--font-size-xs);
    color: var(--text-4);
}

/* ─── Dashboard Metric Cards ─────────────────────────────────────────── */

.dash-metrics {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
    gap: var(--sp-4);
    margin-bottom: var(--sp-4);
}

.dash-metric-card {
    background: var(--bg-1);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
    padding: var(--sp-4);
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: var(--sp-2);
    transition: border-color var(--transition-normal), transform var(--transition-normal), box-shadow var(--transition-normal);
    box-shadow: var(--shadow-sm);
    animation: dash-card-in 0.4s ease-out both;
}
.dash-metric-card:nth-child(1) { animation-delay: 0s; }
.dash-metric-card:nth-child(2) { animation-delay: 0.08s; }
.dash-metric-card:nth-child(3) { animation-delay: 0.16s; }
@keyframes dash-card-in {
    from { opacity: 0; transform: translateY(12px); }
    to { opacity: 1; transform: translateY(0); }
}
.dash-metric-card:hover {
    border-color: color-mix(in srgb, var(--accent) 40%, transparent);
    transform: translateY(-2px);
    box-shadow: var(--shadow-md), 0 0 20px rgba(56, 139, 253, 0.06);
}

.dash-ring-container {
    position: relative;
    width: 120px;
    height: 120px;
}
.dash-ring {
    width: 100%;
    height: 100%;
    transform: rotate(-90deg);
}
.dash-ring-fill {
    transition: stroke-dasharray 1s ease-out;
}
.dash-ring-label {
    position: absolute;
    inset: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
}
.dash-ring-value {
    font-size: 1.5rem;
    font-weight: 800;
    font-family: var(--font-mono);
    color: var(--text-0);
    line-height: 1;
}
.dash-ring-sub {
    font-size: var(--font-size-xs);
    color: var(--text-3);
    font-family: var(--font-mono);
}

.dash-metric-title {
    font-size: var(--font-size-sm);
    font-weight: 600;
    color: var(--text-1);
    text-align: center;
}
.dash-metric-breakdown {
    display: flex;
    flex-wrap: wrap;
    gap: var(--sp-2) var(--sp-3);
    justify-content: center;
    font-size: var(--font-size-xs);
    color: var(--text-3);
    font-family: var(--font-mono);
}
.dash-metric-warn { color: var(--warning); }

.dash-dot {
    display: inline-block;
    width: 7px;
    height: 7px;
    border-radius: 50%;
    margin-right: 2px;
}
.dash-dot-active { background: var(--success); }
.dash-dot-idle { background: var(--text-4); }
.dash-dot-warning { background: var(--warning); }

.dash-metric-big {
    padding: var(--sp-3) 0;
    text-align: center;
}
.dash-big-num {
    font-size: 2.2rem;
    font-weight: 800;
    font-family: var(--font-mono);
    color: var(--text-0);
    line-height: 1;
    letter-spacing: -0.03em;
}
.dash-big-label {
    display: block;
    font-size: var(--font-size-xs);
    color: var(--text-3);
    margin-top: var(--sp-1);
    text-transform: uppercase;
    letter-spacing: 0.08em;
}

/* ─── Dashboard Activity Summary ──────────────────────────────────────── */

.dash-activity-summary {
    display: flex;
    gap: var(--sp-2);
    margin-bottom: var(--sp-3);
}
.dash-activity-badge {
    font-size: var(--font-size-xs);
    font-family: var(--font-mono);
    padding: 2px 8px;
    border-radius: 100px;
    font-weight: 600;
}
.dash-ab-active { background: rgba(63, 185, 80, 0.12); color: var(--success); }
.dash-ab-idle { background: var(--bg-2); color: var(--text-3); }
.dash-ab-warn { background: rgba(210, 153, 34, 0.12); color: var(--warning); }

/* ─── Dashboard Activity Feed ────────────────────────────────────────── */

.dash-activity-list {
    display: flex;
    flex-direction: column;
    gap: 1px;
}
.dash-activity-row {
    display: grid;
    grid-template-columns: 16px auto auto 1fr;
    grid-template-rows: auto;
    gap: var(--sp-2);
    align-items: center;
    padding: var(--sp-2) var(--sp-3);
    font-size: var(--font-size-xs);
    background: var(--bg-0);
    border-radius: var(--radius);
    transition: background var(--transition-fast);
}
.dash-activity-row:hover { background: var(--bg-2); }

.dash-activity-state { font-size: 8px; line-height: 1; }
.dash-activity-active { color: var(--success); animation: dash-blink 1.5s ease-in-out infinite; }
.dash-activity-warning { color: var(--warning); }
.dash-activity-idle { color: var(--text-4); }
@keyframes dash-blink {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.3; }
}

.dash-activity-db {
    font-weight: 600;
    color: var(--text-1);
    font-family: var(--font-mono);
}
.dash-activity-user {
    color: var(--text-3);
    font-family: var(--font-mono);
}
.dash-activity-duration {
    font-family: var(--font-mono);
    color: var(--accent);
    grid-column: 2;
}
.dash-activity-query {
    grid-column: 1 / -1;
    font-family: var(--font-mono);
    color: var(--text-2);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    max-width: 100%;
}

/* ─── Dashboard Bar Chart ────────────────────────────────────────────── */

/* ─── Dashboard Bar Chart — table layout for perfect alignment ────────── */

.dash-bar-chart {
    display: table;
    width: 100%;
    border-spacing: 0 2px;
}
.dash-bar-row {
    display: table-row;
    font-size: var(--font-size-xs);
    animation: dash-bar-in 0.4s ease-out both;
}
.dash-bar-row:nth-child(1) { animation-delay: 0s; }
.dash-bar-row:nth-child(2) { animation-delay: 0.03s; }
.dash-bar-row:nth-child(3) { animation-delay: 0.06s; }
.dash-bar-row:nth-child(4) { animation-delay: 0.09s; }
.dash-bar-row:nth-child(5) { animation-delay: 0.12s; }
.dash-bar-row:nth-child(6) { animation-delay: 0.15s; }
.dash-bar-row:nth-child(7) { animation-delay: 0.18s; }
.dash-bar-row:nth-child(8) { animation-delay: 0.21s; }
.dash-bar-row:nth-child(9) { animation-delay: 0.24s; }
.dash-bar-row:nth-child(10) { animation-delay: 0.27s; }
@keyframes dash-bar-in {
    from { opacity: 0; transform: translateX(-6px); }
    to { opacity: 1; transform: translateX(0); }
}

.dash-bar-row > * {
    display: table-cell;
    vertical-align: middle;
    padding: var(--sp-2) var(--sp-2);
}

.dash-bar-name {
    font-family: var(--font-mono);
    white-space: nowrap;
    width: 1%; /* shrink-to-fit */
    padding-right: var(--sp-4);
}
.dash-bar-name a {
    color: var(--text-1);
    text-decoration: none;
    transition: color var(--transition-fast);
}
.dash-bar-name a:hover { color: var(--accent); }

.dash-bar-track {
    width: 100%; /* takes remaining space */
    padding: 0 var(--sp-3);
}
.dash-bar-track > div { /* the inner track container */
    height: 6px;
    background: var(--bg-3);
    border-radius: 3px;
    overflow: hidden;
}
.dash-bar-fill {
    height: 100%;
    background: var(--accent);
    border-radius: 3px;
    transition: width 0.8s ease-out;
    min-width: 2px;
}
.dash-bar-fill.dash-bar-warn { background: var(--warning); }

.dash-bar-value {
    font-family: var(--font-mono);
    font-weight: 600;
    color: var(--text-1);
    white-space: nowrap;
    text-align: right;
    width: 1%;
    padding-left: var(--sp-3);
}
.dash-bar-meta {
    color: var(--text-3);
    font-family: var(--font-mono);
    white-space: nowrap;
    width: 1%;
    padding-left: var(--sp-2);
}
.dash-bar-dead { color: var(--warning); font-weight: 600; }

/* ─── Dashboard Database Cards ────────────────────────────────────────── */

.dash-db-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
    gap: var(--sp-3);
}

.dash-db-card {
    display: block;
    background: var(--bg-1);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
    padding: var(--sp-3) var(--sp-4);
    text-decoration: none;
    color: inherit;
    transition: all var(--transition-normal);
    box-shadow: var(--shadow-sm);
    animation: dash-card-in 0.4s ease-out both;
}
.dash-db-card:nth-child(1) { animation-delay: 0s; }
.dash-db-card:nth-child(2) { animation-delay: 0.06s; }
.dash-db-card:nth-child(3) { animation-delay: 0.12s; }
.dash-db-card:nth-child(4) { animation-delay: 0.18s; }
.dash-db-card:hover {
    border-color: var(--accent);
    transform: translateY(-2px);
    box-shadow: var(--shadow-md), 0 0 0 1px var(--accent-subtle);
}

.dash-db-header {
    display: flex;
    justify-content: space-between;
    align-items: baseline;
    margin-bottom: var(--sp-2);
}
.dash-db-name {
    font-weight: 700;
    font-size: var(--font-size-sm);
    color: var(--text-0);
}
.dash-db-size {
    font-family: var(--font-mono);
    font-size: var(--font-size-xs);
    color: var(--accent);
    font-weight: 600;
}

.dash-db-bar {
    height: 4px;
    background: var(--bg-3);
    border-radius: 2px;
    overflow: hidden;
    margin-bottom: var(--sp-2);
}
.dash-db-bar-fill {
    height: 100%;
    background: linear-gradient(90deg, var(--accent), color-mix(in srgb, var(--accent) 50%, var(--success)));
    border-radius: 2px;
    transition: width 0.8s ease-out;
}

.dash-db-stats {
    display: flex;
    gap: var(--sp-3);
    font-size: var(--font-size-xs);
    font-family: var(--font-mono);
    color: var(--text-3);
    flex-wrap: wrap;
}
.dash-db-stat-icon {
    font-size: 6px;
    vertical-align: middle;
    margin-right: 2px;
    color: var(--text-4);
}
.dash-db-cache-success { color: var(--success); }
.dash-db-cache-warning { color: var(--warning); }
.dash-db-cache-danger { color: var(--danger); }
.dash-db-deadlocks { color: var(--danger); font-weight: 600; }

/* ─── Dashboard Empty State ──────────────────────────────────────────── */

.dash-empty {
    padding: var(--sp-5);
    text-align: center;
    color: var(--text-4);
    font-size: var(--font-size-sm);
    font-style: italic;
}

/* ─── Dashboard Grid Layout ──────────────────────────────────────────── */
)_CSS_"; }

};

} // namespace getgresql::ssr
