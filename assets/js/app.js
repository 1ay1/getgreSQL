// getgreSQL — Database IDE client-side interactions

// ─── Theme ───────────────────────────────────────────────────────────────

function toggleTheme() {
    const html = document.documentElement;
    const next = html.getAttribute('data-theme') === 'dark' ? 'light' : 'dark';
    html.setAttribute('data-theme', next);
    localStorage.setItem('theme', next);
}

(function() {
    const saved = localStorage.getItem('theme');
    if (saved) document.documentElement.setAttribute('data-theme', saved);
})();

// ─── Tree View ───────────────────────────────────────────────────────────

var TREE_STATE_KEY = 'getgresql_tree_state';

function treeGetSavedState() {
    try { return JSON.parse(sessionStorage.getItem(TREE_STATE_KEY) || '[]'); }
    catch(e) { return []; }
}

function treeSaveState() {
    var expanded = [];
    document.querySelectorAll('.tree-children.loaded').forEach(function(el) {
        if (el.style.display !== 'none') {
            var url = el.getAttribute('hx-get');
            if (url) expanded.push(url);
        }
    });
    sessionStorage.setItem(TREE_STATE_KEY, JSON.stringify(expanded));
}

function treeToggle(el) {
    var item = el.closest('.tree-item');
    if (!item) return;
    var children = item.querySelector('.tree-children');
    var chevron = el.querySelector('.tree-chevron');
    if (!children || !chevron) return;

    if (children.classList.contains('loaded')) {
        var isHidden = children.style.display === 'none';
        children.style.display = isHidden ? '' : 'none';
        chevron.classList.toggle('expanded', isHidden);
        treeSaveState();
    }
    // If not loaded, htmx will handle loading — state saved in afterSwap
}

// After htmx loads tree children, mark as loaded, expand, and auto-restore nested state
document.addEventListener('htmx:afterSwap', function(e) {
    if (e.detail.target.classList && e.detail.target.classList.contains('tree-children')) {
        e.detail.target.classList.add('loaded');
        e.detail.target.style.display = '';
        var item = e.detail.target.closest('.tree-item');
        if (item) {
            var chevron = item.querySelector('.tree-chevron');
            if (chevron) chevron.classList.add('expanded');
        }
        treeSaveState();

        // Auto-expand child nodes that were previously open
        var saved = treeGetSavedState();
        if (saved.length > 0) {
            e.detail.target.querySelectorAll('.tree-children[hx-get]').forEach(function(child) {
                var url = child.getAttribute('hx-get');
                if (saved.indexOf(url) !== -1) {
                    // Trigger expansion by clicking the parent row
                    var parentRow = child.closest('.tree-item');
                    if (parentRow) {
                        var row = parentRow.querySelector(':scope > .tree-row');
                        if (row) row.click();
                    }
                }
            });
        }
    }
});

// Highlight the tree row matching the current URL
function treeHighlightCurrent() {
    var path = window.location.pathname;
    document.querySelectorAll('.tree-row.selected').forEach(function(el) {
        el.classList.remove('selected');
    });
    document.querySelectorAll('.tree-row[href]').forEach(function(el) {
        if (el.getAttribute('href') === path) {
            el.classList.add('selected');
        }
    });
}

// Tree row selection highlight on click
document.addEventListener('click', function(e) {
    var row = e.target.closest('.tree-row');
    if (!row) return;
    document.querySelectorAll('.tree-row.selected').forEach(function(el) {
        el.classList.remove('selected');
    });
    row.classList.add('selected');
});

// Highlight current page in tree after any htmx swap
document.addEventListener('htmx:afterSettle', function() {
    treeHighlightCurrent();
});

// ─── Sidebar Resize ──────────────────────────────────────────────────────

(function() {
    var handle = null;
    var startX = 0;
    var startWidth = 0;

    document.addEventListener('mousedown', function(e) {
        if (!e.target.classList.contains('resize-handle')) return;
        handle = e.target;
        handle.classList.add('dragging');
        var sidebar = document.querySelector('.sidebar');
        startX = e.clientX;
        startWidth = sidebar.offsetWidth;
        document.body.style.cursor = 'col-resize';
        document.body.style.userSelect = 'none';
        e.preventDefault();
    });

    document.addEventListener('mousemove', function(e) {
        if (!handle) return;
        var newWidth = startWidth + (e.clientX - startX);
        newWidth = Math.max(180, Math.min(420, newWidth));
        document.querySelector('.ide').style.gridTemplateColumns = newWidth + 'px 1fr';
        e.preventDefault();
    });

    document.addEventListener('mouseup', function() {
        if (!handle) return;
        handle.classList.remove('dragging');
        handle = null;
        document.body.style.cursor = '';
        document.body.style.userSelect = '';
    });
})();

// ─── Query Editor ────────────────────────────────────────────────────────

// Ctrl+Enter to submit query
document.addEventListener('keydown', function(e) {
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
        var editor = document.getElementById('sql-editor');
        if (editor && document.activeElement === editor) {
            var form = editor.closest('form');
            if (form) htmx.trigger(form, 'submit');
        }
    }
});

// Tab inserts spaces in SQL editors
document.addEventListener('keydown', function(e) {
    if (e.key !== 'Tab') return;
    var editor = e.target;
    if (!editor.classList || !editor.classList.contains('sql-input')) return;
    e.preventDefault();
    var start = editor.selectionStart;
    var end = editor.selectionEnd;
    editor.value = editor.value.substring(0, start) + '    ' + editor.value.substring(end);
    editor.selectionStart = editor.selectionEnd = start + 4;
    updateGutter(editor);
});

// Line number gutter sync
function updateGutter(textarea) {
    var gutter = textarea.parentElement.querySelector('.editor-gutter');
    if (!gutter) return;
    var lines = textarea.value.split('\n').length;
    lines = Math.max(lines, 8);
    var html = '';
    for (var i = 1; i <= lines; i++) {
        html += '<span class="line-num">' + i + '</span>';
    }
    gutter.innerHTML = html;
}

// Sync gutter on input
document.addEventListener('input', function(e) {
    if (e.target.classList && e.target.classList.contains('sql-input')) {
        updateGutter(e.target);
    }
});

// Sync gutter scroll
document.addEventListener('scroll', function(e) {
    if (e.target.classList && e.target.classList.contains('sql-input')) {
        var gutter = e.target.parentElement.querySelector('.editor-gutter');
        if (gutter) gutter.scrollTop = e.target.scrollTop;
    }
}, true);

// Initialize gutter on page load
document.addEventListener('htmx:afterSettle', function() {
    document.querySelectorAll('.sql-input').forEach(updateGutter);
});
document.addEventListener('DOMContentLoaded', function() {
    document.querySelectorAll('.sql-input').forEach(updateGutter);
});

// ─── EXPLAIN handling ────────────────────────────────────────────────────

document.addEventListener('click', function(e) {
    var btn = e.target.closest('[data-analyze]');
    if (!btn) return;
    var input = document.getElementById('analyze-flag');
    if (input) input.value = btn.getAttribute('data-analyze');
});

// ─── Query Editor Resize (vertical split) ────────────────────────────────

(function() {
    var handle = null;
    var startY = 0;
    var startHeight = 0;
    var editor = null;

    document.addEventListener('mousedown', function(e) {
        if (!e.target.classList.contains('editor-resize')) return;
        handle = e.target;
        handle.classList.add('dragging');
        editor = handle.previousElementSibling;
        if (!editor) return;
        startY = e.clientY;
        startHeight = editor.offsetHeight;
        document.body.style.cursor = 'ns-resize';
        document.body.style.userSelect = 'none';
        e.preventDefault();
    });

    document.addEventListener('mousemove', function(e) {
        if (!handle || !editor) return;
        var newHeight = startHeight + (e.clientY - startY);
        newHeight = Math.max(100, Math.min(window.innerHeight * 0.7, newHeight));
        editor.style.height = newHeight + 'px';
        editor.style.minHeight = newHeight + 'px';
        e.preventDefault();
    });

    document.addEventListener('mouseup', function() {
        if (!handle) return;
        handle.classList.remove('dragging');
        handle = null;
        editor = null;
        document.body.style.cursor = '';
        document.body.style.userSelect = '';
    });
})();

// ─── Table Column Sorting ────────────────────────────────────────────────

document.addEventListener('click', function(e) {
    var th = e.target.closest('th.sortable');
    if (!th) return;

    var table = th.closest('table');
    if (!table) return;

    var tbody = table.querySelector('tbody');
    if (!tbody) return;

    var colIdx = Array.from(th.parentElement.children).indexOf(th);
    var isAsc = th.classList.contains('sorted-asc');

    // Remove sort classes from all headers
    th.parentElement.querySelectorAll('th').forEach(function(h) {
        h.classList.remove('sorted-asc', 'sorted-desc');
    });

    // Toggle direction
    var dir = isAsc ? 'desc' : 'asc';
    th.classList.add('sorted-' + dir);

    // Sort rows
    var rows = Array.from(tbody.querySelectorAll('tr'));
    rows.sort(function(a, b) {
        var aVal = a.children[colIdx] ? a.children[colIdx].textContent.trim() : '';
        var bVal = b.children[colIdx] ? b.children[colIdx].textContent.trim() : '';

        // Try numeric comparison
        var aNum = parseFloat(aVal.replace(/[^0-9.\-]/g, ''));
        var bNum = parseFloat(bVal.replace(/[^0-9.\-]/g, ''));
        if (!isNaN(aNum) && !isNaN(bNum)) {
            return dir === 'asc' ? aNum - bNum : bNum - aNum;
        }
        // String comparison
        return dir === 'asc' ? aVal.localeCompare(bVal) : bVal.localeCompare(aVal);
    });

    rows.forEach(function(row) { tbody.appendChild(row); });
});

// ─── Command Palette ─────────────────────────────────────────────────────

var commandPaletteItems = [
    { label: 'Dashboard', icon: '\u25A3', href: '/' },
    { label: 'Databases', icon: '\u2689', href: '/databases' },
    { label: 'Query Editor', icon: '\u2318', href: '/query' },
    { label: 'Explain Analyze', icon: '\u2699', href: '/explain' },
    { label: 'Monitor', icon: '\u25C9', href: '/monitor' },
    { label: 'Active Queries', icon: '\u25B6', href: '/monitor' },
    { label: 'Locks', icon: '\u26BF', href: '/monitor/locks' },
    { label: 'Table Statistics', icon: '\u2637', href: '/monitor/tablestats' },
    { label: 'Replication', icon: '\u21C4', href: '/monitor/replication' },
    { label: 'Roles', icon: '\u265F', href: '/roles' },
    { label: 'Extensions', icon: '\u2756', href: '/extensions' },
    { label: 'Settings', icon: '\u2699', href: '/settings' },
];

function openCommandPalette() {
    if (document.querySelector('.command-overlay')) return;

    var overlay = document.createElement('div');
    overlay.className = 'command-overlay';
    overlay.innerHTML =
        '<div class="command-palette">' +
        '<input class="command-input" placeholder="Go to..." autofocus>' +
        '<div class="command-list"></div>' +
        '</div>';

    document.body.appendChild(overlay);

    var input = overlay.querySelector('.command-input');
    var list = overlay.querySelector('.command-list');
    var selectedIdx = 0;

    function render(filter) {
        var items = commandPaletteItems.filter(function(item) {
            return !filter || item.label.toLowerCase().includes(filter.toLowerCase());
        });
        selectedIdx = 0;
        list.innerHTML = items.map(function(item, i) {
            return '<a href="' + item.href + '" class="command-item' + (i === 0 ? ' selected' : '') + '">' +
                '<span class="icon">' + item.icon + '</span>' +
                '<span>' + item.label + '</span>' +
                '</a>';
        }).join('');
    }

    render('');

    input.addEventListener('input', function() {
        render(input.value);
    });

    input.addEventListener('keydown', function(e) {
        var items = list.querySelectorAll('.command-item');
        if (e.key === 'ArrowDown') {
            e.preventDefault();
            selectedIdx = Math.min(selectedIdx + 1, items.length - 1);
        } else if (e.key === 'ArrowUp') {
            e.preventDefault();
            selectedIdx = Math.max(selectedIdx - 1, 0);
        } else if (e.key === 'Enter') {
            e.preventDefault();
            if (items[selectedIdx]) items[selectedIdx].click();
            return;
        } else if (e.key === 'Escape') {
            closeCommandPalette();
            return;
        } else {
            return;
        }
        items.forEach(function(el, i) {
            el.classList.toggle('selected', i === selectedIdx);
        });
    });

    overlay.addEventListener('click', function(e) {
        if (e.target === overlay) closeCommandPalette();
    });

    input.focus();
}

function closeCommandPalette() {
    var overlay = document.querySelector('.command-overlay');
    if (overlay) overlay.remove();
}

// Ctrl+K to open command palette
document.addEventListener('keydown', function(e) {
    if ((e.ctrlKey || e.metaKey) && e.key === 'k') {
        e.preventDefault();
        if (document.querySelector('.command-overlay')) {
            closeCommandPalette();
        } else {
            openCommandPalette();
        }
    }
    if (e.key === 'Escape') {
        closeCommandPalette();
    }
});

// ─── Sidebar toggle ──────────────────────────────────────────────────────

function toggleSidebar() {
    document.querySelector('.ide').classList.toggle('sidebar-collapsed');
}

// ─── Inline Cell Editing ─────────────────────────────────────────────────

function editCell(span) {
    if (span.querySelector('input')) return; // already editing

    var currentValue = span.textContent;
    var col = span.getAttribute('data-col');
    var schema = span.getAttribute('data-schema');
    var table = span.getAttribute('data-table');
    var db = span.getAttribute('data-db');
    var where = span.getAttribute('data-where');

    var input = document.createElement('input');
    input.type = 'text';
    input.value = currentValue;
    input.className = 'cell-edit-input';
    span.textContent = '';
    span.appendChild(input);
    input.focus();
    input.select();

    function commit() {
        var newValue = input.value;
        if (newValue === currentValue) {
            span.textContent = currentValue;
            return;
        }
        // Send update
        fetch('/db/' + db + '/schema/' + schema + '/table/' + table + '/update-cell', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: 'col=' + encodeURIComponent(col) + '&val=' + encodeURIComponent(newValue) + '&where=' + encodeURIComponent(where)
        }).then(function(r) { return r.json(); }).then(function(data) {
            if (data.error) {
                span.textContent = currentValue;
                alert('Update failed: ' + data.error);
            } else {
                span.textContent = newValue;
                span.classList.add('cell-updated');
                setTimeout(function() { span.classList.remove('cell-updated'); }, 1500);
            }
        }).catch(function() {
            span.textContent = currentValue;
        });
    }

    input.addEventListener('blur', commit);
    input.addEventListener('keydown', function(e) {
        if (e.key === 'Enter') { e.preventDefault(); input.blur(); }
        if (e.key === 'Escape') { span.textContent = currentValue; }
    });
}

// ─── Export Results (CSV / JSON / SQL INSERT) ────────────────────────────

function exportResults(format) {
    var table = document.querySelector('.query-results table');
    if (!table) { alert('No results to export'); return; }

    var headers = [];
    table.querySelectorAll('thead th').forEach(function(th) {
        headers.push(th.textContent.trim());
    });

    var rows = [];
    table.querySelectorAll('tbody tr').forEach(function(tr) {
        var row = [];
        tr.querySelectorAll('td').forEach(function(td) {
            var nullEl = td.querySelector('.null-value');
            row.push(nullEl ? null : td.textContent.trim());
        });
        rows.push(row);
    });

    var content, filename, mime;

    if (format === 'csv') {
        var csvEsc = function(v) {
            if (v === null) return '';
            if (v.indexOf(',') !== -1 || v.indexOf('"') !== -1 || v.indexOf('\n') !== -1) {
                return '"' + v.replace(/"/g, '""') + '"';
            }
            return v;
        };
        content = headers.map(csvEsc).join(',') + '\n';
        rows.forEach(function(r) { content += r.map(csvEsc).join(',') + '\n'; });
        filename = 'export.csv';
        mime = 'text/csv';
    } else if (format === 'json') {
        var data = rows.map(function(r) {
            var obj = {};
            headers.forEach(function(h, i) { obj[h] = r[i]; });
            return obj;
        });
        content = JSON.stringify(data, null, 2);
        filename = 'export.json';
        mime = 'application/json';
    } else if (format === 'sql') {
        content = '';
        rows.forEach(function(r) {
            var vals = r.map(function(v) {
                if (v === null) return 'NULL';
                return "'" + v.replace(/'/g, "''") + "'";
            });
            content += 'INSERT INTO table_name (' + headers.join(', ') + ') VALUES (' + vals.join(', ') + ');\n';
        });
        filename = 'export.sql';
        mime = 'text/sql';
    }

    var blob = new Blob([content], { type: mime });
    var a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = filename;
    a.click();
    URL.revokeObjectURL(a.href);
}

// ─── Visual EXPLAIN Renderer ─────────────────────────────────────────────

function parseExplainPlan(text) {
    if (!text) return null;
    var lines = text.split('\n').filter(function(l) { return l.trim(); });
    var nodes = [];
    var stack = [{ children: nodes, indent: -1 }];

    lines.forEach(function(line) {
        var indent = 0;
        var trimmed = line;
        // Count leading spaces and ->
        var m = line.match(/^(\s*)(->)?\s*/);
        if (m) {
            indent = m[0].length;
            trimmed = line.substring(m[0].length);
        }

        // Parse node: "Type on table  (cost=X..Y rows=N width=W) (actual time=A..B rows=R loops=L)"
        var node = { text: trimmed, indent: indent, children: [], type: '', table: '', cost: 0, rows: 0, actual_time: 0, actual_rows: 0, width: 0, loops: 1, extra: '' };

        var costMatch = trimmed.match(/\(cost=([0-9.]+)\.\.([0-9.]+)\s+rows=(\d+)\s+width=(\d+)\)/);
        if (costMatch) {
            node.cost = parseFloat(costMatch[2]);
            node.rows = parseInt(costMatch[3]);
            node.width = parseInt(costMatch[4]);
        }
        var actualMatch = trimmed.match(/\(actual time=([0-9.]+)\.\.([0-9.]+)\s+rows=(\d+)\s+loops=(\d+)\)/);
        if (actualMatch) {
            node.actual_time = parseFloat(actualMatch[2]);
            node.actual_rows = parseInt(actualMatch[3]);
            node.loops = parseInt(actualMatch[4]);
        }
        var typeMatch = trimmed.match(/^(\S[\w\s]*?)(?:\s+on\s+(\S+)|\s+using\s+(\S+))?\s*\(/);
        if (typeMatch) {
            node.type = typeMatch[1].trim();
            node.table = typeMatch[2] || typeMatch[3] || '';
        } else {
            // Sub-plan info like "Filter:", "Sort Key:", etc.
            node.type = trimmed.replace(/\(.*$/, '').trim();
        }

        // Place in tree based on indent
        while (stack.length > 1 && stack[stack.length - 1].indent >= indent) {
            stack.pop();
        }
        stack[stack.length - 1].children.push(node);
        stack.push(node);
    });

    return nodes;
}

function renderExplainTree(nodes, maxCost) {
    if (!nodes || !nodes.length) return '';
    if (!maxCost) {
        maxCost = 0;
        (function findMax(ns) { ns.forEach(function(n) { if (n.cost > maxCost) maxCost = n.cost; findMax(n.children); }); })(nodes);
        if (maxCost === 0) maxCost = 1;
    }

    var html = '';
    nodes.forEach(function(node) {
        if (!node.type) return;
        var costPct = Math.min(100, (node.cost / maxCost) * 100);
        var costColor = costPct > 80 ? 'var(--danger)' : costPct > 40 ? 'var(--warning)' : 'var(--success)';

        html += '<div class="explain-node">';
        html += '<div class="explain-node-header">';
        html += '<span class="explain-node-type">' + node.type + '</span>';
        if (node.table) html += ' <span class="explain-node-table">on ' + node.table + '</span>';
        html += '</div>';
        html += '<div class="explain-node-stats">';
        if (node.cost > 0) {
            html += '<div class="explain-cost-bar"><div class="explain-cost-fill" style="width:' + costPct.toFixed(1) + '%;background:' + costColor + '"></div></div>';
            html += '<span class="explain-stat">Cost: ' + node.cost.toFixed(1) + '</span>';
        }
        if (node.rows > 0) html += '<span class="explain-stat">Rows: ' + node.rows + '</span>';
        if (node.actual_time > 0) html += '<span class="explain-stat">Time: ' + node.actual_time.toFixed(3) + 'ms</span>';
        if (node.actual_rows > 0) html += '<span class="explain-stat">Actual: ' + node.actual_rows + '</span>';
        if (node.loops > 1) html += '<span class="explain-stat">Loops: ' + node.loops + '</span>';
        html += '</div>';
        if (node.children.length > 0) {
            html += '<div class="explain-children">' + renderExplainTree(node.children, maxCost) + '</div>';
        }
        html += '</div>';
    });
    return html;
}

// Auto-render EXPLAIN results
document.addEventListener('htmx:afterSettle', function() {
    var plans = document.querySelectorAll('.explain-plan');
    plans.forEach(function(el) {
        if (el.dataset.rendered) return;
        var text = el.textContent;
        var nodes = parseExplainPlan(text);
        if (nodes && nodes.length > 0 && nodes[0].cost > 0) {
            var visual = document.createElement('div');
            visual.className = 'explain-visual';
            visual.innerHTML = renderExplainTree(nodes);
            el.parentElement.insertBefore(visual, el);
            // Keep original as collapsible
            el.style.display = 'none';
            var toggle = document.createElement('button');
            toggle.className = 'btn btn-sm btn-ghost';
            toggle.textContent = 'Show Raw Plan';
            toggle.onclick = function() {
                el.style.display = el.style.display === 'none' ? '' : 'none';
                toggle.textContent = el.style.display === 'none' ? 'Show Raw Plan' : 'Hide Raw Plan';
            };
            el.parentElement.insertBefore(toggle, el);
            el.dataset.rendered = '1';
        }
    });
});

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
