#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ERDWidget {
    static constexpr auto js() -> std::string_view { return R"_JS_(
// ─── ERD SVG Renderer ────────────────────────────────────────────────────

function initERD(container) {
    var url = container.getAttribute('data-url');
    if (!url) return;

    fetch(url).then(function(r) { return r.json(); }).then(function(data) {
        if (!data.tables || data.tables.length === 0) {
            container.innerHTML = '<div class="empty-state">No tables found in this schema</div>';
            return;
        }
        renderERD(container, data);
    }).catch(function(err) {
        container.innerHTML = '<div class="alert alert-error">' + err.message + '</div>';
    });
}

function renderERD(container, data) {
    var tables = data.tables;
    var rels = data.relationships || [];

    // Layout: grid arrangement
    var cols = Math.ceil(Math.sqrt(tables.length));
    var boxW = 220, boxH = 0, padX = 40, padY = 40, headerH = 28, rowH = 20, maxRows = 12;

    // Calculate positions
    var positions = {};
    tables.forEach(function(t, i) {
        var col = i % cols;
        var row = Math.floor(i / cols);
        var h = headerH + Math.min(t.columns.length, maxRows) * rowH + 8;
        if (t.columns.length > maxRows) h += rowH; // "... more" row
        positions[t.name] = { x: col * (boxW + padX) + 20, y: row * (200 + padY) + 20, w: boxW, h: h };
    });

    // SVG dimensions
    var svgW = (cols) * (boxW + padX) + 40;
    var svgH = (Math.ceil(tables.length / cols)) * (200 + padY) + 40;

    var svg = '<svg class="erd-svg" viewBox="0 0 ' + svgW + ' ' + svgH + '" xmlns="http://www.w3.org/2000/svg">';

    // Draw relationship lines first (behind boxes)
    rels.forEach(function(rel) {
        var s = positions[rel.source];
        var t = positions[rel.target];
        if (!s || !t) return;
        var sx = s.x + s.w, sy = s.y + s.h / 2;
        var tx = t.x, ty = t.y + t.h / 2;
        if (sx > tx) { sx = s.x; tx = t.x + t.w; }
        var mx = (sx + tx) / 2;
        svg += '<path d="M' + sx + ',' + sy + ' C' + mx + ',' + sy + ' ' + mx + ',' + ty + ' ' + tx + ',' + ty + '" class="erd-line" />';
        // Arrow
        var angle = Math.atan2(ty - sy, tx - sx);
        svg += '<circle cx="' + tx + '" cy="' + ty + '" r="4" class="erd-arrow" />';
    });

    // Draw table boxes
    tables.forEach(function(t) {
        var pos = positions[t.name];
        if (!pos) return;

        svg += '<g class="erd-table" transform="translate(' + pos.x + ',' + pos.y + ')">';
        svg += '<rect x="0" y="0" width="' + pos.w + '" height="' + pos.h + '" rx="4" class="erd-box" />';
        svg += '<rect x="0" y="0" width="' + pos.w + '" height="' + headerH + '" rx="4" class="erd-header" />';
        svg += '<rect x="0" y="' + (headerH - 4) + '" width="' + pos.w + '" height="4" class="erd-header" />'; // square bottom corners
        svg += '<text x="10" y="' + (headerH - 8) + '" class="erd-title">' + t.name + '</text>';

        var shown = Math.min(t.columns.length, maxRows);
        for (var i = 0; i < shown; i++) {
            var col = t.columns[i];
            var y = headerH + 4 + i * rowH;
            svg += '<text x="10" y="' + (y + 14) + '" class="erd-col">' + col.name + '</text>';
            svg += '<text x="' + (pos.w - 10) + '" y="' + (y + 14) + '" class="erd-col-type" text-anchor="end">' + col.type + '</text>';
        }
        if (t.columns.length > maxRows) {
            var y = headerH + 4 + shown * rowH;
            svg += '<text x="10" y="' + (y + 14) + '" class="erd-col" style="fill:var(--text-4)">... ' + (t.columns.length - maxRows) + ' more</text>';
        }

        svg += '</g>';
    });

    svg += '</svg>';

    container.innerHTML = '<div class="erd-toolbar">' +
        '<span class="erd-info">' + tables.length + ' tables, ' + rels.length + ' relationships</span>' +
        '</div>' + svg;
}

// Auto-init ERD
document.addEventListener('DOMContentLoaded', function() {
    var c = document.getElementById('erd-container');
    if (c) initERD(c);
});
document.addEventListener('htmx:afterSettle', function() {
    var c = document.getElementById('erd-container');
    if (c && !c.querySelector('.erd-svg')) initERD(c);
});

// ─── Saved Queries ───────────────────────────────────────────────────────

var SavedQueries = {
    KEY: 'getgresql_saved_queries',

    getAll: function() {
        try { return JSON.parse(localStorage.getItem(this.KEY) || '[]'); }
        catch(e) { return []; }
    },

    save: function(name, sql) {
        var queries = this.getAll();
        // Update if name exists
        var existing = queries.findIndex(function(q) { return q.name === name; });
        if (existing !== -1) {
            queries[existing].sql = sql;
            queries[existing].updated = Date.now();
        } else {
            queries.push({ name: name, sql: sql, created: Date.now(), updated: Date.now() });
        }
        localStorage.setItem(this.KEY, JSON.stringify(queries));
    },

    remove: function(name) {
        var queries = this.getAll().filter(function(q) { return q.name !== name; });
        localStorage.setItem(this.KEY, JSON.stringify(queries));
    },

    render: function() {
        var queries = this.getAll();
        if (queries.length === 0) {
            return '<div class="empty-state">No saved queries. Use the Save button in the query editor toolbar.</div>';
        }
        var html = '<div class="saved-queries-list">';
        queries.sort(function(a, b) { return b.updated - a.updated; });
        queries.forEach(function(q) {
            var preview = q.sql.length > 100 ? q.sql.substring(0, 100) + '...' : q.sql;
            var date = new Date(q.updated).toLocaleDateString();
            html += '<div class="saved-query-item">';
            html += '<div class="saved-query-header">';
            html += '<strong>' + q.name.replace(/</g, '&lt;') + '</strong>';
            html += '<span class="saved-query-date">' + date + '</span>';
            html += '</div>';
            html += '<code class="saved-query-preview">' + preview.replace(/</g, '&lt;') + '</code>';
            html += '<div class="saved-query-actions">';
            html += '<button class="btn btn-sm btn-primary" data-load-query="' + q.name.replace(/"/g, '&quot;') + '">Load</button>';
            html += '<button class="btn btn-sm btn-danger" data-delete-query="' + q.name.replace(/"/g, '&quot;') + '">Delete</button>';
            html += '</div></div>';
        });
        html += '</div>';
        return html;
    }
};

// ═══════════════════════════════════════════════════════════════════════
// Data Grid Power Features — makes DBeaver users switch
// ═══════════════════════════════════════════════════════════════════════

)_JS_"; }
};

} // namespace getgresql::ssr
