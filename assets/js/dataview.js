// getgreSQL — DataView: unified data grid behaviors
// Client-side sorting, filtering, copy, export, column resize, keyboard nav
// Works on any .data-view container rendered by the C++ DataView component.

(function() {
'use strict';

// ─── Helpers ─────────────────────────────────────────────────────────

function esc(s) {
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

function cellText(td) {
    // Get the text content of a cell, preferring data-full for truncated values
    var span = td.querySelector('.editable-cell, .dv-cell');
    if (span && span.getAttribute('data-full')) return span.getAttribute('data-full');
    if (span) return span.textContent;
    return td.textContent.trim();
}

function getDataView(el) {
    return el.closest('.data-view');
}

function getTable(dv) {
    return dv ? dv.querySelector('.dv-table') : null;
}

function getBodyRows(table) {
    if (!table || !table.tBodies[0]) return [];
    return Array.from(table.tBodies[0].rows);
}

function getHeaderCells(table) {
    if (!table || !table.tHead) return [];
    return Array.from(table.tHead.rows[0].cells);
}

// ─── Client-side Sorting ─────────────────────────────────────────────

function sortTable(th) {
    var table = th.closest('table');
    if (!table) return;
    var colIdx = Array.from(th.parentNode.children).indexOf(th);
    var rows = getBodyRows(table);
    if (rows.length === 0) return;

    var isNumeric = th.getAttribute('data-type') === 'numeric';
    // Auto-detect numeric if not explicitly typed
    if (!isNumeric && rows.length > 0) {
        var sample = 0, numeric = 0;
        for (var i = 0; i < Math.min(rows.length, 20); i++) {
            var v = cellText(rows[i].cells[colIdx]);
            if (v && v !== 'NULL') {
                sample++;
                if (!isNaN(parseFloat(v)) && isFinite(v)) numeric++;
            }
        }
        if (sample > 0 && numeric / sample > 0.8) isNumeric = true;
    }

    // Determine sort direction
    var currentDir = th.getAttribute('data-sort-dir');
    var dir = currentDir === 'asc' ? 'desc' : 'asc';

    // Clear other sort indicators
    getHeaderCells(table).forEach(function(h) {
        h.removeAttribute('data-sort-dir');
        h.classList.remove('sort-asc', 'sort-desc');
    });
    th.setAttribute('data-sort-dir', dir);
    th.classList.add('sort-' + dir);

    rows.sort(function(a, b) {
        var av = cellText(a.cells[colIdx]);
        var bv = cellText(b.cells[colIdx]);

        // NULLs always last
        if (av === 'NULL' && bv !== 'NULL') return 1;
        if (bv === 'NULL' && av !== 'NULL') return -1;
        if (av === 'NULL' && bv === 'NULL') return 0;

        if (isNumeric) {
            var an = parseFloat(av) || 0;
            var bn = parseFloat(bv) || 0;
            return dir === 'asc' ? an - bn : bn - an;
        }
        var cmp = av.localeCompare(bv, undefined, { sensitivity: 'base' });
        return dir === 'asc' ? cmp : -cmp;
    });

    var tbody = table.tBodies[0];
    rows.forEach(function(row) { tbody.appendChild(row); });
}

// ─── Client-side Filtering ───────────────────────────────────────────

function filterRows(dv, query) {
    var table = getTable(dv);
    if (!table) return;
    var rows = getBodyRows(table);
    var q = query.toLowerCase().trim();
    var visible = 0;

    rows.forEach(function(row) {
        if (!q) {
            row.style.display = '';
            visible++;
            return;
        }
        var text = '';
        for (var i = 0; i < row.cells.length; i++) {
            text += ' ' + cellText(row.cells[i]).toLowerCase();
        }
        if (text.indexOf(q) !== -1) {
            row.style.display = '';
            visible++;
        } else {
            row.style.display = 'none';
        }
    });

    // Update filter count display
    var badge = dv.querySelector('.dv-filter-count');
    if (!badge) {
        badge = document.createElement('span');
        badge.className = 'dv-filter-count';
        var input = dv.querySelector('.dv-filter-input');
        if (input) input.parentNode.insertBefore(badge, input.nextSibling);
    }
    badge.textContent = q ? visible + ' / ' + rows.length : '';
}

// ─── Export ──────────────────────────────────────────────────────────

function exportData(dv, format) {
    var table = getTable(dv);
    if (!table) return;
    var headers = [];
    getHeaderCells(table).forEach(function(th) {
        var text = th.querySelector('.dv-th-text');
        if (text) headers.push(text.textContent);
        else if (!th.classList.contains('row-num-header') && !th.classList.contains('dv-actions-header'))
            headers.push(th.textContent.trim());
    });

    // Filter out # and actions columns
    var colStart = table.querySelector('.row-num-header') ? 1 : 0;
    var colEnd = table.querySelector('.dv-actions-header') ? headers.length : headers.length;
    headers = [];
    getHeaderCells(table).forEach(function(th, idx) {
        if (idx < colStart) return;
        if (th.classList.contains('dv-actions-header')) return;
        var text = th.querySelector('.dv-th-text');
        headers.push(text ? text.textContent : th.textContent.trim());
    });

    var rows = [];
    getBodyRows(table).forEach(function(row) {
        if (row.style.display === 'none') return; // respect filter
        var cells = [];
        Array.from(row.cells).forEach(function(td, idx) {
            if (idx < colStart) return;
            if (idx >= row.cells.length - (table.querySelector('.dv-actions-header') ? 1 : 0)) return;
            cells.push(cellText(td));
        });
        rows.push(cells);
    });

    var output = '';
    var mime = 'text/plain';
    var ext = 'txt';

    if (format === 'csv') {
        mime = 'text/csv';
        ext = 'csv';
        output = headers.map(function(h) { return '"' + h.replace(/"/g, '""') + '"'; }).join(',') + '\n';
        rows.forEach(function(r) {
            output += r.map(function(v) { return '"' + v.replace(/"/g, '""') + '"'; }).join(',') + '\n';
        });
    } else if (format === 'json') {
        mime = 'application/json';
        ext = 'json';
        var arr = rows.map(function(r) {
            var obj = {};
            r.forEach(function(v, i) { obj[headers[i] || 'col' + i] = v === 'NULL' ? null : v; });
            return obj;
        });
        output = JSON.stringify(arr, null, 2);
    } else if (format === 'sql') {
        ext = 'sql';
        // Try to get table name from data-view context
        var tableName = dv.getAttribute('data-table') || 'table_name';
        var schemaName = dv.getAttribute('data-schema');
        var fullName = schemaName ? '"' + schemaName + '"."' + tableName + '"' : '"' + tableName + '"';
        rows.forEach(function(r) {
            var vals = r.map(function(v) {
                if (v === 'NULL') return 'NULL';
                return "'" + v.replace(/'/g, "''") + "'";
            });
            output += 'INSERT INTO ' + fullName + ' (' + headers.map(function(h) { return '"' + h + '"'; }).join(', ') +
                ') VALUES (' + vals.join(', ') + ');\n';
        });
    }

    // Download
    var blob = new Blob([output], { type: mime });
    var url = URL.createObjectURL(blob);
    var a = document.createElement('a');
    a.href = url;
    a.download = 'export.' + ext;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

// ─── Copy to Clipboard ───────────────────────────────────────────────

function copySelection(dv) {
    var table = getTable(dv);
    if (!table) return;

    // If a cell is selected, copy just that cell
    var selected = dv.querySelector('.cell-selected, .dv-cell-selected');
    if (selected) {
        var text = selected.getAttribute('data-full') || selected.textContent;
        navigator.clipboard.writeText(text).then(function() { showToast(dv, 'Copied!'); });
        return;
    }

    // If rows are selected, copy them as TSV
    var selectedRows = dv.querySelectorAll('tr.dv-row-selected');
    if (selectedRows.length > 0) {
        var lines = [];
        selectedRows.forEach(function(row) {
            var cells = [];
            var colStart = table.querySelector('.row-num-header') ? 1 : 0;
            Array.from(row.cells).forEach(function(td, idx) {
                if (idx < colStart) return;
                if (td.querySelector('.btn-danger')) return; // skip delete button col
                cells.push(cellText(td));
            });
            lines.push(cells.join('\t'));
        });
        navigator.clipboard.writeText(lines.join('\n')).then(function() {
            showToast(dv, 'Copied ' + selectedRows.length + ' rows!');
        });
        return;
    }

    // Otherwise copy all visible rows
    var headers = [];
    var colStart = table.querySelector('.row-num-header') ? 1 : 0;
    getHeaderCells(table).forEach(function(th, idx) {
        if (idx < colStart) return;
        if (th.classList.contains('dv-actions-header')) return;
        var text = th.querySelector('.dv-th-text');
        headers.push(text ? text.textContent : th.textContent.trim());
    });
    var lines = [headers.join('\t')];
    getBodyRows(table).forEach(function(row) {
        if (row.style.display === 'none') return;
        var cells = [];
        Array.from(row.cells).forEach(function(td, idx) {
            if (idx < colStart) return;
            if (td.querySelector('.btn-danger')) return;
            cells.push(cellText(td));
        });
        lines.push(cells.join('\t'));
    });
    navigator.clipboard.writeText(lines.join('\n')).then(function() {
        showToast(dv, 'Copied all visible rows!');
    });
}

function showToast(dv, msg) {
    var toast = document.createElement('div');
    toast.className = 'dv-toast';
    toast.textContent = msg;
    dv.appendChild(toast);
    setTimeout(function() { toast.classList.add('dv-toast-show'); }, 10);
    setTimeout(function() { toast.classList.remove('dv-toast-show'); }, 1200);
    setTimeout(function() { toast.remove(); }, 1500);
}

// ─── Column Resize ───────────────────────────────────────────────────

function initColumnResize(dv) {
    var handles = dv.querySelectorAll('.dv-resize-handle');
    handles.forEach(function(handle) {
        handle.addEventListener('mousedown', function(e) {
            e.preventDefault();
            e.stopPropagation();
            var th = handle.closest('th');
            var startX = e.clientX;
            var startWidth = th.offsetWidth;

            function onMove(ev) {
                var newWidth = Math.max(40, startWidth + (ev.clientX - startX));
                th.style.width = newWidth + 'px';
                th.style.minWidth = newWidth + 'px';
            }
            function onUp() {
                document.removeEventListener('mousemove', onMove);
                document.removeEventListener('mouseup', onUp);
                document.body.style.cursor = '';
                document.body.style.userSelect = '';
            }
            document.addEventListener('mousemove', onMove);
            document.addEventListener('mouseup', onUp);
            document.body.style.cursor = 'col-resize';
            document.body.style.userSelect = 'none';
        });
    });
}

// ─── Row Selection ───────────────────────────────────────────────────

var lastClickedRow = null;

function selectRow(row, dv, e) {
    if (!row || !dv) return;
    var table = getTable(dv);
    if (!table) return;

    if (e && e.shiftKey && lastClickedRow) {
        // Range select
        var rows = getBodyRows(table);
        var from = rows.indexOf(lastClickedRow);
        var to = rows.indexOf(row);
        if (from === -1 || to === -1) return;
        var start = Math.min(from, to);
        var end = Math.max(from, to);
        for (var i = start; i <= end; i++) {
            rows[i].classList.add('dv-row-selected');
        }
    } else if (e && (e.ctrlKey || e.metaKey)) {
        // Toggle select
        row.classList.toggle('dv-row-selected');
    } else {
        // Single select
        dv.querySelectorAll('tr.dv-row-selected').forEach(function(r) { r.classList.remove('dv-row-selected'); });
        row.classList.add('dv-row-selected');
    }
    lastClickedRow = row;
}

// ─── Read-only Cell Selection (for query results) ────────────────────

var selectedDvCell = null;

function selectDvCell(cell) {
    if (selectedDvCell) selectedDvCell.classList.remove('dv-cell-selected');
    selectedDvCell = cell;
    if (cell) cell.classList.add('dv-cell-selected');
}

// ─── Event Delegation ────────────────────────────────────────────────

document.addEventListener('click', function(e) {
    // Column sort
    var th = e.target.closest('.dv-table th.sortable');
    if (th && !e.target.closest('.dv-resize-handle')) {
        sortTable(th);
        return;
    }

    // Export buttons
    var exportBtn = e.target.closest('[data-dv-export]');
    if (exportBtn) {
        var dv = getDataView(exportBtn);
        if (dv) exportData(dv, exportBtn.getAttribute('data-dv-export'));
        return;
    }

    // Action buttons
    var actionBtn = e.target.closest('[data-dv-action]');
    if (actionBtn) {
        var action = actionBtn.getAttribute('data-dv-action');
        var dv = getDataView(actionBtn);
        if (action === 'copy' && dv) { copySelection(dv); return; }
        if (action === 'toggle-insert' && dv) {
            var form = dv.querySelector('.dv-insert-form');
            if (form) form.style.display = form.style.display === 'none' ? '' : 'none';
            return;
        }
    }

    // Read-only cell click
    var dvCell = e.target.closest('.dv-cell');
    if (dvCell) {
        selectDvCell(dvCell);
        return;
    }

    // Row number click -> row selection
    var rowNum = e.target.closest('.row-num');
    if (rowNum) {
        var row = rowNum.closest('tr');
        var dv = getDataView(rowNum);
        if (row && dv) selectRow(row, dv, e);
        return;
    }

    // Click outside deselects dv-cell
    if (selectedDvCell && !e.target.closest('.data-view')) {
        selectDvCell(null);
    }
});

// Filter input
document.addEventListener('input', function(e) {
    if (!e.target.classList.contains('dv-filter-input')) return;
    var dv = getDataView(e.target);
    if (dv) filterRows(dv, e.target.value);
});

// Keyboard shortcuts
document.addEventListener('keydown', function(e) {
    // Ctrl+C in a data-view
    if ((e.ctrlKey || e.metaKey) && e.key === 'c') {
        // Only intercept if focus is within a data-view and not in an input
        var active = document.activeElement;
        if (active && (active.tagName === 'INPUT' || active.tagName === 'TEXTAREA')) return;
        if (selectedDvCell || document.querySelector('.dv-row-selected')) {
            var dv = selectedDvCell ? getDataView(selectedDvCell) : document.querySelector('.dv-row-selected').closest('.data-view');
            if (dv) {
                e.preventDefault();
                copySelection(dv);
            }
        }
    }

    // Arrow keys for dv-cell navigation
    if (selectedDvCell && ['ArrowUp','ArrowDown','ArrowLeft','ArrowRight'].indexOf(e.key) !== -1) {
        var td = selectedDvCell.closest('td');
        var tr = td ? td.closest('tr') : null;
        if (!td || !tr) return;
        e.preventDefault();

        var target = null;
        var idx = Array.from(tr.children).indexOf(td);

        if (e.key === 'ArrowRight') {
            var next = td.nextElementSibling;
            while (next) { target = next.querySelector('.dv-cell'); if (target) break; next = next.nextElementSibling; }
        } else if (e.key === 'ArrowLeft') {
            var prev = td.previousElementSibling;
            while (prev) { target = prev.querySelector('.dv-cell'); if (target) break; prev = prev.previousElementSibling; }
        } else if (e.key === 'ArrowDown') {
            var nextRow = tr.nextElementSibling;
            if (nextRow && nextRow.cells[idx]) target = nextRow.cells[idx].querySelector('.dv-cell');
        } else if (e.key === 'ArrowUp') {
            var prevRow = tr.previousElementSibling;
            if (prevRow && prevRow.cells[idx]) target = prevRow.cells[idx].querySelector('.dv-cell');
        }

        if (target) selectDvCell(target);
    }
});

// ─── Auto-init on DOM changes (htmx swaps) ──────────────────────────

function initDataViews() {
    document.querySelectorAll('.data-view').forEach(function(dv) {
        if (dv._dvInit) return;
        dv._dvInit = true;
        initColumnResize(dv);
    });
}

// Run on page load
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initDataViews);
} else {
    initDataViews();
}

// Run after htmx swaps
document.addEventListener('htmx:afterSwap', initDataViews);
document.addEventListener('htmx:afterSettle', initDataViews);

// Also observe for dynamic content (e.g. query results injected by fetch)
var observer = new MutationObserver(function(mutations) {
    var needsInit = false;
    mutations.forEach(function(m) {
        if (m.addedNodes.length > 0) needsInit = true;
    });
    if (needsInit) initDataViews();
});
observer.observe(document.body, { childList: true, subtree: true });

})();
