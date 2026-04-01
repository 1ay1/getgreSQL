#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ExplainViz {
    static constexpr auto js() -> std::string_view { return R"_JS_(
// ─── Smart EXPLAIN Analyzer ──────────────────────────────────────────────
// Parses EXPLAIN output, renders visual tree, and generates intelligent
// optimization suggestions based on DBA best practices.

function parseExplainPlan(text) {
    if (!text) return null;
    var lines = text.split('\n').filter(function(l) { return l.trim(); });
    var nodes = [];
    var stack = [{ children: nodes, indent: -1 }];

    lines.forEach(function(line) {
        var indent = 0;
        var m = line.match(/^(\s*)(->)?\s*/);
        if (m) { indent = m[0].length; line = line.substring(m[0].length); }

        var node = { text: line, indent: indent, children: [], type: '', table: '', cost: 0, startup_cost: 0, rows: 0, actual_time: 0, actual_rows: 0, width: 0, loops: 1, filter: '', sort_key: '', join_type: '', index: '', buffers: '' };

        var costM = line.match(/\(cost=([0-9.]+)\.\.([0-9.]+)\s+rows=(\d+)\s+width=(\d+)\)/);
        if (costM) { node.startup_cost = parseFloat(costM[1]); node.cost = parseFloat(costM[2]); node.rows = parseInt(costM[3]); node.width = parseInt(costM[4]); }

        var actM = line.match(/\(actual time=([0-9.]+)\.\.([0-9.]+)\s+rows=(\d+)\s+loops=(\d+)\)/);
        if (actM) { node.actual_time = parseFloat(actM[2]); node.actual_rows = parseInt(actM[3]); node.loops = parseInt(actM[4]); }

        var typeM = line.match(/^(\S[\w\s]*?)(?:\s+on\s+(\S+)|\s+using\s+(\S+))?\s*\(/);
        if (typeM) { node.type = typeM[1].trim(); node.table = typeM[2] || typeM[3] || ''; }
        else { node.type = line.replace(/\(.*$/, '').trim(); }

        // Extract extra info
        var filterM = line.match(/Filter:\s*(.*)/); if (filterM) node.filter = filterM[1];
        var sortM = line.match(/Sort Key:\s*(.*)/); if (sortM) node.sort_key = sortM[1];
        var idxM = line.match(/Index.*:\s*(\S+)/); if (idxM) node.index = idxM[1];
        var bufM = line.match(/Buffers:\s*(.*)/); if (bufM) node.buffers = bufM[1];

        while (stack.length > 1 && stack[stack.length - 1].indent >= indent) stack.pop();
        stack[stack.length - 1].children.push(node);
        stack.push(node);
    });
    return nodes;
}

// ─── Performance Score (A/B/C/D/F) ──────────────────────────────────

function scoreQuery(nodes, totalTime) {
    if (!totalTime) {
        // Use cost-based scoring
        var totalCost = nodes[0] ? nodes[0].cost : 0;
        if (totalCost < 10) return {grade:'A', label:'Excellent', color:'var(--success)'};
        if (totalCost < 100) return {grade:'B', label:'Good', color:'var(--success)'};
        if (totalCost < 1000) return {grade:'C', label:'Moderate', color:'var(--warning)'};
        if (totalCost < 10000) return {grade:'D', label:'Slow', color:'var(--danger)'};
        return {grade:'F', label:'Critical', color:'var(--danger)'};
    }
    if (totalTime < 1) return {grade:'A', label:'Excellent', color:'var(--success)'};
    if (totalTime < 10) return {grade:'B', label:'Good', color:'var(--success)'};
    if (totalTime < 100) return {grade:'C', label:'Moderate', color:'var(--warning)'};
    if (totalTime < 1000) return {grade:'D', label:'Slow', color:'var(--danger)'};
    return {grade:'F', label:'Critical', color:'var(--danger)'};
}

// ─── Smart Optimization Suggestions ─────────────────────────────────

function analyzeExplainPlan(nodes) {
    var suggestions = [];
    var stats = { seqScans: 0, indexScans: 0, sorts: 0, hashJoins: 0, nestedLoops: 0, filters: 0, totalNodes: 0, bottleneck: null, maxCost: 0 };

    function walk(ns) {
        ns.forEach(function(n) {
            stats.totalNodes++;
            if (n.cost > stats.maxCost) { stats.maxCost = n.cost; stats.bottleneck = n; }

            var t = n.type.toLowerCase();

            // Sequential scan detection
            if (t.indexOf('seq scan') !== -1) {
                stats.seqScans++;
                if (n.rows > 1000) {
                    suggestions.push({
                        severity: 'warning',
                        icon: '&#9888;',
                        title: 'Sequential Scan on ' + (n.table || 'table'),
                        detail: 'Scanning ' + n.rows.toLocaleString() + ' rows sequentially. ' +
                            (n.filter ? 'Filter: ' + n.filter + '. ' : '') +
                            'Consider adding an index on the filtered/joined columns.',
                        sql: n.table ? 'CREATE INDEX idx_' + n.table + '_... ON ' + n.table + ' (...);' : ''
                    });
                }
                if (n.rows > 100000) {
                    suggestions[suggestions.length - 1].severity = 'critical';
                    suggestions[suggestions.length - 1].icon = '&#10007;';
                }
            }

            // Index scan — good
            if (t.indexOf('index') !== -1 && t.indexOf('scan') !== -1) stats.indexScans++;

            // Sort without index
            if (t.indexOf('sort') !== -1) {
                stats.sorts++;
                if (n.cost > 100) {
                    suggestions.push({
                        severity: 'info',
                        icon: '&#8645;',
                        title: 'Sort Operation (cost: ' + n.cost.toFixed(0) + ')',
                        detail: (n.sort_key ? 'Sort Key: ' + n.sort_key + '. ' : '') +
                            'If this sort happens frequently, consider adding an index that matches the sort order.',
                        sql: ''
                    });
                }
            }

            // Nested loop with many iterations
            if (t.indexOf('nested loop') !== -1) {
                stats.nestedLoops++;
                if (n.loops > 100 || n.rows > 10000) {
                    suggestions.push({
                        severity: 'warning',
                        icon: '&#128260;',
                        title: 'Nested Loop (' + n.rows + ' rows, ' + n.loops + ' loops)',
                        detail: 'Nested loops are slow for large datasets. PostgreSQL may benefit from a Hash Join or Merge Join. Check if join columns have indexes, or increase work_mem.',
                        sql: 'SET work_mem = \'256MB\'; -- try increasing for this session'
                    });
                }
            }

            // Hash join — check memory
            if (t.indexOf('hash') !== -1) stats.hashJoins++;

            // Estimated vs actual row mismatch
            if (n.actual_rows > 0 && n.rows > 0) {
                var ratio = n.actual_rows / n.rows;
                if (ratio > 10 || ratio < 0.1) {
                    suggestions.push({
                        severity: 'warning',
                        icon: '&#128202;',
                        title: 'Row Estimate Mismatch on ' + (n.table || n.type),
                        detail: 'Estimated ' + n.rows.toLocaleString() + ' rows but got ' + n.actual_rows.toLocaleString() +
                            ' (' + (ratio > 1 ? ratio.toFixed(0) + 'x more' : (1/ratio).toFixed(0) + 'x fewer') +
                            '). Run ANALYZE on the table to update statistics.',
                        sql: n.table ? 'ANALYZE ' + n.table + ';' : 'ANALYZE;'
                    });
                }
            }

            // Filter removing many rows
            if (n.filter && n.actual_rows > 0 && n.rows > 0 && n.actual_rows < n.rows * 0.1) {
                suggestions.push({
                    severity: 'info',
                    icon: '&#128269;',
                    title: 'Filter Removes >90% of Rows',
                    detail: 'The filter "' + n.filter.substring(0, 80) + '" keeps only ' +
                        n.actual_rows + ' of ' + n.rows + ' rows. An index on the filter column would avoid reading discarded rows.',
                    sql: ''
                });
            }

            walk(n.children);
        });
    }
    walk(nodes);

    // Overall suggestions
    if (stats.seqScans > 0 && stats.indexScans === 0) {
        suggestions.unshift({
            severity: 'critical',
            icon: '&#9888;',
            title: 'No Index Scans Used',
            detail: 'The entire query uses sequential scans. This means no indexes are being utilized. Ensure the WHERE/JOIN columns have appropriate indexes.',
            sql: ''
        });
    }

    if (suggestions.length === 0) {
        suggestions.push({
            severity: 'ok',
            icon: '&#10003;',
            title: 'Query Looks Good',
            detail: 'No obvious optimization issues detected. ' + stats.indexScans + ' index scan(s), ' +
                stats.totalNodes + ' plan node(s). The query planner chose an efficient path.',
            sql: ''
        });
    }

    return { suggestions: suggestions, stats: stats };
}

// ─── Render Visual Plan + Analysis ───────────────────────────────────

function renderSmartExplain(nodes, totalTime) {
    var maxCost = 0;
    (function findMax(ns) { ns.forEach(function(n) { if (n.cost > maxCost) maxCost = n.cost; findMax(n.children); }); })(nodes);
    if (maxCost === 0) maxCost = 1;

    var score = scoreQuery(nodes, totalTime);
    var analysis = analyzeExplainPlan(nodes);

    var html = '';

    // Score badge
    html += '<div class="explain-score">';
    html += '<div class="explain-grade" style="background:' + score.color + '">' + score.grade + '</div>';
    html += '<div class="explain-grade-info"><strong>' + score.label + '</strong>';
    if (totalTime) html += '<span>' + totalTime.toFixed(1) + 'ms execution</span>';
    else html += '<span>Cost: ' + maxCost.toFixed(0) + '</span>';
    html += '<span>' + analysis.stats.totalNodes + ' nodes, ' + analysis.stats.indexScans + ' index scans, ' + analysis.stats.seqScans + ' seq scans</span>';
    html += '</div></div>';

    // Suggestions
    if (analysis.suggestions.length > 0) {
        html += '<div class="explain-suggestions">';
        analysis.suggestions.forEach(function(s) {
            var cls = s.severity === 'critical' ? 'danger' : s.severity === 'warning' ? 'warning' : s.severity === 'ok' ? 'success' : 'info';
            html += '<div class="explain-suggestion explain-sug-' + cls + '">';
            html += '<span class="explain-sug-icon">' + s.icon + '</span>';
            html += '<div class="explain-sug-body">';
            html += '<div class="explain-sug-title">' + s.title + '</div>';
            html += '<div class="explain-sug-detail">' + s.detail + '</div>';
            if (s.sql) html += '<code class="explain-sug-sql">' + s.sql + '</code>';
            html += '</div></div>';
        });
        html += '</div>';
    }

    // Visual tree
    html += '<div class="explain-visual">';
    html += renderExplainTree(nodes, maxCost);
    html += '</div>';

    return html;
}

function renderExplainTree(nodes, maxCost) {
    var html = '';
    nodes.forEach(function(node) {
        if (!node.type) return;
        var costPct = Math.min(100, (node.cost / maxCost) * 100);
        var costColor = costPct > 80 ? 'var(--danger)' : costPct > 40 ? 'var(--warning)' : 'var(--success)';
        var isBottleneck = costPct > 80;

        html += '<div class="explain-node' + (isBottleneck ? ' explain-bottleneck' : '') + '">';
        html += '<div class="explain-node-header">';
        html += '<span class="explain-node-type">' + node.type + '</span>';
        if (node.table) html += ' <span class="explain-node-table">on ' + node.table + '</span>';
        if (isBottleneck) html += ' <span class="badge badge-danger" style="font-size:9px">BOTTLENECK</span>';
        html += '</div>';
        html += '<div class="explain-node-stats">';
        if (node.cost > 0) {
            html += '<div class="explain-cost-bar"><div class="explain-cost-fill" style="width:' + costPct.toFixed(1) + '%;background:' + costColor + '"></div></div>';
            html += '<span class="explain-stat">Cost: ' + node.cost.toFixed(1) + '</span>';
        }
        if (node.rows > 0) html += '<span class="explain-stat">Rows: ' + node.rows.toLocaleString() + '</span>';
        if (node.actual_time > 0) html += '<span class="explain-stat">Time: ' + node.actual_time.toFixed(3) + 'ms</span>';
        if (node.actual_rows > 0) html += '<span class="explain-stat">Actual: ' + node.actual_rows.toLocaleString() + '</span>';
        if (node.loops > 1) html += '<span class="explain-stat">Loops: ' + node.loops + '</span>';
        html += '</div>';
        if (node.filter) html += '<div class="explain-node-extra">Filter: ' + node.filter + '</div>';
        if (node.sort_key) html += '<div class="explain-node-extra">Sort Key: ' + node.sort_key + '</div>';
        if (node.buffers) html += '<div class="explain-node-extra">Buffers: ' + node.buffers + '</div>';
        if (node.children.length > 0) {
            html += '<div class="explain-children">' + renderExplainTree(node.children, maxCost) + '</div>';
        }
        html += '</div>';
    });
    return html;
}

// Auto-render EXPLAIN results with smart analysis
document.addEventListener('htmx:afterSettle', renderExplainResults);
document.addEventListener('DOMContentLoaded', renderExplainResults);

function renderExplainResults() {
    document.querySelectorAll('.explain-plan').forEach(function(el) {
        if (el.dataset.rendered) return;
        var text = el.textContent;
        var nodes = parseExplainPlan(text);
        if (!nodes || !nodes.length || !nodes[0].cost) return;

        // Extract timing from nearby query-info
        var totalTime = 0;
        var infoEl = el.closest('div').querySelector('.query-info');
        if (infoEl) {
            var timeM = infoEl.textContent.match(/Execution:\s*([0-9.]+)/);
            if (timeM) totalTime = parseFloat(timeM[1]);
        }

        var container = document.createElement('div');
        container.className = 'explain-analysis';
        container.innerHTML = renderSmartExplain(nodes, totalTime);

        el.parentElement.insertBefore(container, el);
        el.style.display = 'none';

        var toggle = document.createElement('button');
        toggle.className = 'btn btn-sm btn-ghost';
        toggle.style.marginTop = '8px';
        toggle.textContent = 'Show Raw Plan';
        toggle.onclick = function() {
            el.style.display = el.style.display === 'none' ? '' : 'none';
            toggle.textContent = el.style.display === 'none' ? 'Show Raw Plan' : 'Hide Raw Plan';
        };
        el.parentElement.insertBefore(toggle, el);
        el.dataset.rendered = '1';
    });
}

)_JS_"; }
};

} // namespace getgresql::ssr
