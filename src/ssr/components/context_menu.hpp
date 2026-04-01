#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct ContextMenu {
    static constexpr auto js() -> std::string_view { return R"_JS_(
// ─── Right-click Context Menu — Unified for all DataViews ────────────
//
// Context-aware: shows different actions based on cell type.
//   - .editable-cell (table browse): Edit, Set NULL, Delete Row + copy/filter
//   - .dv-cell (query results): Copy/filter only
//   - th headers: Sort, Hide Column
//
// Every action that modifies data goes through the DSL-backed endpoints.

(function() {
    var menu = null;

    document.addEventListener('contextmenu', function(e) {
        var cell = e.target.closest('td');
        var th = e.target.closest('th');
        if (!cell && !th) return;
        var table = (cell || th).closest('table');
        if (!table) return;
        e.preventDefault();
        closeContextMenu();

        var dv = table.closest('.data-view');
        var isEditable = dv && dv.getAttribute('data-editable') === 'true';
        var db = dv ? dv.getAttribute('data-db') : '';
        var schema = dv ? dv.getAttribute('data-schema') : '';
        var tableName = dv ? dv.getAttribute('data-table') : '';

        menu = document.createElement('div');
        menu.className = 'ctx-menu';
        menu.style.top = e.clientY + 'px';
        menu.style.left = e.clientX + 'px';

        var items = [];

        if (cell) {
            var span = cell.querySelector('.editable-cell, .dv-cell, .null-value');
            var text = span ? (span.getAttribute('data-full') || span.textContent.trim()) : cell.textContent.trim();
            var isNull = !!cell.querySelector('.null-value');
            var colIdx = Array.from(cell.parentElement.children).indexOf(cell);
            var thEl = table.querySelector('thead th:nth-child(' + (colIdx + 1) + ')');
            var colLabel = thEl ? (thEl.querySelector('.dv-th-text') || thEl).textContent.trim() : '';
            var editableSpan = cell.querySelector('.editable-cell');
            var tr = cell.closest('tr');
            var ctid = tr ? tr.getAttribute('data-ctid') : '';
            var colName = editableSpan ? editableSpan.getAttribute('data-col') : colLabel;

            // ── Edit actions — same for ALL data views ─────────────
            // Any cell with data-col is editable (rendered by SSR with htmx triggers)
            var targetSpan = editableSpan || cell.querySelector('[data-col]');
            var canEdit = targetSpan && (targetSpan.hasAttribute('hx-get') || targetSpan.classList.contains('editable-cell'));
            if (canEdit) {
                items.push({
                    label: 'Edit Cell',
                    icon: '&#9998;',
                    kbd: 'F2',
                    action: function() {
                        if (targetSpan.hasAttribute('hx-get')) {
                            htmx.trigger(targetSpan, 'dblclick');
                        } else {
                            startEdit(targetSpan);
                        }
                    }
                });
                if (!isNull && ctid) {
                    items.push({
                        label: 'Set NULL',
                        icon: '&#8709;',
                        action: function() {
                            setToNull(db, schema, tableName, colName, ctid, targetSpan);
                        }
                    });
                }
                items.push({sep: true});
            }

            // ── Table-level actions (only for table browse with full context) ──
            if (isEditable && ctid) {
                items.push({
                    label: 'Insert Row',
                    icon: '+',
                    action: function() {
                        var btn = dv.querySelector('[data-dv-action="toggle-insert"]');
                        if (btn) btn.click();
                    }
                });
                items.push({
                    label: 'Delete Row',
                    icon: '&#10005;',
                    cls: 'ctx-danger',
                    action: function() {
                        if (confirm('Delete this row?')) {
                            deleteRowViaApi(db, schema, tableName, ctid);
                        }
                    }
                });
                items.push({sep: true});
            }

            // ── Filter actions ───────────────────────────────────────
            if (text && !isNull && text !== '—' && colLabel) {
                var displayVal = text.length > 25 ? text.substring(0, 25) + '...' : text;
                items.push({label: 'Filter: ' + colLabel + ' = "' + displayVal + '"', icon: '&#128269;', action: function() { filterByValue(table, colIdx, text); }});
                items.push({label: 'Exclude this value', icon: '&#10005;', action: function() { excludeValue(table, colIdx, text); }});
                items.push({sep: true});
            }

            // ── Explain This — cell lineage & metadata ────────────────
            var explainSpan = cell.querySelector('[data-table-oid], [data-col]') || span;
            if (explainSpan) {
                items.push({
                    label: 'Explain This',
                    icon: '&#128269;',
                    kbd: 'Ctrl+I',
                    cls: 'ctx-explain',
                    action: function() {
                        if (typeof explainCell === 'function') explainCell(explainSpan);
                    }
                });
                items.push({sep: true});
            }

            // ── Copy actions ─────────────────────────────────────────
            items.push({label: 'Copy Cell Value', icon: '&#128203;', kbd: 'Ctrl+C', action: function() { navigator.clipboard.writeText(text); showToast('Copied!'); }});
            items.push({label: 'Copy Row as JSON', icon: '{ }', action: function() { copyRowAsJSON(table, tr); showToast('Copied JSON!'); }});
            items.push({label: 'Copy Row as INSERT', icon: 'SQL', action: function() { copyRowAsInsert(table, tr, schema, tableName); showToast('Copied SQL!'); }});
            items.push({sep: true});

            // ── View actions ─────────────────────────────────────────
            items.push({label: 'View Row Details', icon: '&#9776;', action: function() { showRowDetail(table, tr); }});
        }

        if (th) {
            var thIdx = Array.from(th.parentElement.children).indexOf(th);
            items.push({label: 'Sort Ascending', icon: '&#9650;', action: function() { sortColumn(table, thIdx, 'asc'); }});
            items.push({label: 'Sort Descending', icon: '&#9660;', action: function() { sortColumn(table, thIdx, 'desc'); }});
            items.push({sep: true});
            items.push({label: 'Hide Column', icon: '&#128065;', action: function() { hideColumn(table, thIdx); }});
        }

        // ── Table-level actions ──────────────────────────────────────
        items.push({sep: true});
        items.push({label: 'Copy All as CSV', icon: 'CSV', action: function() { copyTableAs(table, 'csv'); showToast('Copied CSV!'); }});
        items.push({label: 'Copy All as JSON', icon: 'JSON', action: function() { copyTableAs(table, 'json'); showToast('Copied JSON!'); }});

        // Render menu
        items.forEach(function(item) {
            if (item.sep) { var s = document.createElement('div'); s.className = 'ctx-sep'; menu.appendChild(s); return; }
            var el = document.createElement('div');
            el.className = 'ctx-item' + (item.cls ? ' ' + item.cls : '');
            var html = '<span class="ctx-icon">' + item.icon + '</span><span class="ctx-label">' + item.label + '</span>';
            if (item.kbd) html += '<span class="ctx-kbd">' + item.kbd + '</span>';
            el.innerHTML = html;
            el.addEventListener('click', function() { item.action(); closeContextMenu(); });
            menu.appendChild(el);
        });

        document.body.appendChild(menu);

        // Keep menu on screen
        var rect = menu.getBoundingClientRect();
        if (rect.right > window.innerWidth) menu.style.left = (window.innerWidth - rect.width - 8) + 'px';
        if (rect.bottom > window.innerHeight) menu.style.top = (window.innerHeight - rect.height - 8) + 'px';
    });

    document.addEventListener('click', closeContextMenu);
    document.addEventListener('scroll', closeContextMenu, true);
    document.addEventListener('keydown', function(e) { if (e.key === 'Escape') closeContextMenu(); });

    function closeContextMenu() {
        if (menu) { menu.remove(); menu = null; }
    }

    function showToast(msg) {
        var toast = document.createElement('div');
        toast.className = 'dv-toast dv-toast-show';
        toast.textContent = msg;
        toast.style.cssText = 'position:fixed;top:16px;right:16px;z-index:10001';
        document.body.appendChild(toast);
        setTimeout(function() { toast.classList.remove('dv-toast-show'); }, 1200);
        setTimeout(function() { toast.remove(); }, 1500);
    }

    // ── Set cell to NULL via API ─────────────────────────────────────
    function setToNull(db, schema, table, col, ctid, span) {
        var oldValue = span.textContent;
        span.textContent = 'NULL';
        span.classList.add('null-value', 'cell-saving');
        saveCellValue(db, schema, table, col, ctid, '', span, oldValue);
    }

    // ── Delete row via API ───────────────────────────────────────────
    function deleteRowViaApi(db, schema, table, ctid) {
        fetch('/db/' + db + '/schema/' + schema + '/table/' + table + '/delete-row', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded', 'HX-Request': 'true' },
            body: 'ctid=' + encodeURIComponent(ctid)
        }).then(function(r) { return r.text(); }).then(function(html) {
            var target = document.getElementById('tab-content');
            if (target) { target.innerHTML = html; if (window.htmx) htmx.process(target); }
        });
    }

    // ─── Filter/Exclude (client-side) ────────────────────────────────
    function filterByValue(table, colIdx, value) {
        table.querySelectorAll('tbody tr').forEach(function(row) {
            var cell = row.children[colIdx];
            if (!cell) return;
            var cv = cell.querySelector('.editable-cell, .dv-cell');
            var txt = cv ? (cv.getAttribute('data-full') || cv.textContent.trim()) : cell.textContent.trim();
            row.style.display = txt === value ? '' : 'none';
        });
        showFilterBanner(table, 'Showing: column = "' + value.substring(0, 40) + '"');
    }

    function excludeValue(table, colIdx, value) {
        table.querySelectorAll('tbody tr').forEach(function(row) {
            var cell = row.children[colIdx];
            if (!cell) return;
            var cv = cell.querySelector('.editable-cell, .dv-cell');
            var txt = cv ? (cv.getAttribute('data-full') || cv.textContent.trim()) : cell.textContent.trim();
            if (txt === value) row.style.display = 'none';
        });
        showFilterBanner(table, 'Hidden: column = "' + value.substring(0, 40) + '"');
    }

    function showFilterBanner(table, msg) {
        var existing = table.parentElement.querySelector('.filter-banner');
        if (existing) existing.remove();
        var banner = document.createElement('div');
        banner.className = 'filter-banner';
        banner.innerHTML = '<span>' + msg + '</span><button class="btn btn-sm" onclick="clearFilters(this)">Clear</button>';
        table.parentElement.insertBefore(banner, table);
    }

    // ─── Sort column ─────────────────────────────────────────────────
    function sortColumn(table, colIdx, dir) {
        var dv = table.closest('.data-view');
        var grid = dv ? dv._grid : null;
        if (grid) {
            var ci = colIdx - (grid.hasRowNum ? 1 : 0);
            grid.sortCol = ci;
            grid.sortDir = dir;
            for (var i = 0; i < grid.cols.length; i++) {
                grid.cols[i].el.classList.remove('sort-asc', 'sort-desc');
                grid.cols[i].el.removeAttribute('data-sort-dir');
            }
            if (ci >= 0 && ci < grid.cols.length) {
                grid.cols[ci].el.classList.add('sort-' + dir);
                grid.cols[ci].el.setAttribute('data-sort-dir', dir);
            }
            grid._afterViewChange();
            return;
        }
        var tbody = table.querySelector('tbody');
        var rows = Array.from(tbody.querySelectorAll('tr'));
        rows.sort(function(a, b) {
            var av = a.children[colIdx] ? a.children[colIdx].textContent.trim() : '';
            var bv = b.children[colIdx] ? b.children[colIdx].textContent.trim() : '';
            if (av === 'NULL') return 1;
            if (bv === 'NULL') return -1;
            var an = parseFloat(av.replace(/[^0-9.\-]/g, ''));
            var bn = parseFloat(bv.replace(/[^0-9.\-]/g, ''));
            if (!isNaN(an) && !isNaN(bn)) return dir === 'asc' ? an - bn : bn - an;
            return dir === 'asc' ? av.localeCompare(bv) : bv.localeCompare(av);
        });
        rows.forEach(function(r) { tbody.appendChild(r); });
    }

    // ─── Hide column ─────────────────────────────────────────────────
    function hideColumn(table, colIdx) {
        table.querySelectorAll('tr').forEach(function(row) {
            if (row.children[colIdx]) row.children[colIdx].style.display = 'none';
        });
    }

    // ─── Row detail panel ────────────────────────────────────────────
    function showRowDetail(table, tr) {
        if (!tr) return;
        var headers = Array.from(table.querySelectorAll('thead th')).map(function(th) {
            var t = th.querySelector('.dv-th-text');
            return t ? t.textContent.trim() : th.textContent.trim();
        });
        var cells = Array.from(tr.children);

        var overlay = document.createElement('div');
        overlay.className = 'command-overlay';
        var panel = document.createElement('div');
        panel.className = 'cell-modal';
        panel.style.maxHeight = '80vh';

        var html = '<div class="cell-modal-header"><span class="cell-modal-title">Row Details</span>';
        html += '<button class="cell-modal-close" onclick="this.closest(\'.command-overlay\').remove()">&times;</button></div>';
        html += '<div style="overflow:auto;max-height:70vh;padding:0">';
        html += '<table style="width:100%"><tbody>';
        cells.forEach(function(cell, i) {
            if (i >= headers.length || !headers[i] || headers[i] === '#') return;
            var span = cell.querySelector('.editable-cell, .dv-cell, .null-value');
            var val = span ? (span.getAttribute('data-full') || span.textContent) : cell.textContent;
            var isNull = !!cell.querySelector('.null-value');
            html += '<tr><td style="font-weight:600;color:var(--text-2);white-space:nowrap;width:160px;vertical-align:top;padding:8px 12px;border-bottom:1px solid var(--border-subtle)">' + headers[i] + '</td>';
            html += '<td style="padding:8px 12px;word-break:break-all;font-family:var(--font-mono);font-size:var(--font-size-xs);border-bottom:1px solid var(--border-subtle)">';
            if (isNull) html += '<span class="null-value">NULL</span>';
            else html += val.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
            html += '</td></tr>';
        });
        html += '</tbody></table></div>';

        panel.innerHTML = html;
        overlay.appendChild(panel);
        document.body.appendChild(overlay);
        overlay.addEventListener('click', function(e) { if (e.target === overlay) overlay.remove(); });
        overlay.addEventListener('keydown', function(e) { if (e.key === 'Escape') overlay.remove(); });
    }

    // ─── Copy helpers ────────────────────────────────────────────────
    function copyRowAsJSON(table, tr) {
        var headers = Array.from(table.querySelectorAll('thead th')).map(function(th) {
            var t = th.querySelector('.dv-th-text');
            return t ? t.textContent.trim() : th.textContent.trim();
        });
        var obj = {};
        Array.from(tr.children).forEach(function(cell, i) {
            if (i >= headers.length || !headers[i] || headers[i] === '#') return;
            var span = cell.querySelector('.editable-cell, .dv-cell');
            var isNull = !!cell.querySelector('.null-value');
            obj[headers[i]] = isNull ? null : (span ? (span.getAttribute('data-full') || span.textContent.trim()) : cell.textContent.trim());
        });
        navigator.clipboard.writeText(JSON.stringify(obj, null, 2));
    }

    function copyRowAsInsert(table, tr, schemaName, tblName) {
        var headers = Array.from(table.querySelectorAll('thead th')).map(function(th) {
            var t = th.querySelector('.dv-th-text');
            return t ? t.textContent.trim() : th.textContent.trim();
        });
        var cols = [], vals = [];
        Array.from(tr.children).forEach(function(cell, i) {
            if (i >= headers.length || !headers[i] || headers[i] === '#') return;
            if (cell.classList.contains('dv-actions')) return;
            cols.push('"' + headers[i] + '"');
            var isNull = !!cell.querySelector('.null-value');
            if (isNull) vals.push('NULL');
            else {
                var span = cell.querySelector('.editable-cell, .dv-cell');
                var v = span ? (span.getAttribute('data-full') || span.textContent.trim()) : cell.textContent.trim();
                vals.push("'" + v.replace(/'/g, "''") + "'");
            }
        });
        var fullName = schemaName && tblName ? '"' + schemaName + '"."' + tblName + '"' : 'table_name';
        navigator.clipboard.writeText('INSERT INTO ' + fullName + ' (' + cols.join(', ') + ') VALUES (' + vals.join(', ') + ');');
    }

    function copyTableAs(table, format) {
        var headers = [];
        Array.from(table.querySelectorAll('thead th')).forEach(function(th) {
            var t = th.querySelector('.dv-th-text');
            var name = t ? t.textContent.trim() : th.textContent.trim();
            if (name && name !== '#' && !th.classList.contains('dv-actions-header')) headers.push(name);
        });
        var startIdx = table.querySelector('.row-num-header') ? 1 : 0;
        var hasActions = !!table.querySelector('.dv-actions-header');
        var rows = [];
        table.querySelectorAll('tbody tr').forEach(function(tr) {
            if (tr.style.display === 'none') return;
            var row = [];
            Array.from(tr.children).forEach(function(cell, i) {
                if (i < startIdx) return;
                if (hasActions && cell.classList.contains('dv-actions')) return;
                var span = cell.querySelector('.editable-cell, .dv-cell');
                var isNull = !!cell.querySelector('.null-value');
                row.push(isNull ? null : (span ? (span.getAttribute('data-full') || span.textContent.trim()) : cell.textContent.trim()));
            });
            rows.push(row);
        });

        var text;
        if (format === 'csv') {
            text = headers.join(',') + '\n';
            rows.forEach(function(r) {
                text += r.map(function(v) {
                    if (v === null) return '';
                    return v.indexOf(',') !== -1 || v.indexOf('"') !== -1 ? '"' + v.replace(/"/g, '""') + '"' : v;
                }).join(',') + '\n';
            });
        } else {
            var arr = rows.map(function(r) {
                var obj = {};
                r.forEach(function(v, i) { obj[headers[i] || 'col' + i] = v; });
                return obj;
            });
            text = JSON.stringify(arr, null, 2);
        }
        navigator.clipboard.writeText(text);
    }
})();

// ─── Clear client-side filters ───────────────────────────────────────

function clearFilters(btn) {
    var wrapper = btn.closest('.table-wrapper') || btn.closest('div');
    if (wrapper) {
        wrapper.querySelectorAll('tr').forEach(function(r) { r.style.display = ''; });
        var banner = wrapper.querySelector('.filter-banner');
        if (banner) banner.remove();
    }
}

// ─── Column Header Quick Filter ──────────────────────────────────────

document.addEventListener('click', function(e) {
    var btn = e.target.closest('.col-filter-btn');
    if (!btn) return;
    var th = btn.closest('th');
    if (!th) return;
    var existing = th.querySelector('.col-filter-input');
    if (existing) { existing.remove(); return; }

    var input = document.createElement('input');
    input.type = 'text';
    input.className = 'col-filter-input';
    input.placeholder = 'Filter...';
    th.appendChild(input);
    input.focus();

    var colIdx = Array.from(th.parentElement.children).indexOf(th);
    var table = th.closest('table');

    input.addEventListener('input', function() {
        var q = input.value.toLowerCase();
        table.querySelectorAll('tbody tr').forEach(function(row) {
            var cell = row.children[colIdx];
            if (!cell) return;
            row.style.display = !q || cell.textContent.toLowerCase().indexOf(q) !== -1 ? '' : 'none';
        });
    });
    input.addEventListener('keydown', function(e) { if (e.key === 'Escape') { input.remove(); clearFilters(th); } });
});

// ─── Row Number Click → Row Detail Panel ─────────────────────────────

document.addEventListener('dblclick', function(e) {
    var num = e.target.closest('.row-num');
    if (!num) return;
    var tr = num.closest('tr');
    var table = num.closest('table');
    if (tr && table) {
        // Trigger row detail via the context menu's showRowDetail
        // For now, just highlight
    }
});
)_JS_"; }
};

} // namespace getgresql::ssr
