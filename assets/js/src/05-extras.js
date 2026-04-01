// ─── Right-click Context Menu ────────────────────────────────────────

(function() {
    var menu = null;

    document.addEventListener('contextmenu', function(e) {
        var cell = e.target.closest('td');
        var th = e.target.closest('th');
        if (!cell && !th) return;
        if (!cell && !th.closest('table')) return;
        e.preventDefault();
        closeContextMenu();

        var table = (cell || th).closest('table');
        if (!table) return;

        menu = document.createElement('div');
        menu.className = 'ctx-menu';
        menu.style.top = e.clientY + 'px';
        menu.style.left = e.clientX + 'px';

        var items = [];

        if (cell) {
            var text = cell.textContent.trim();
            var colIdx = Array.from(cell.parentElement.children).indexOf(cell);
            var colName = table.querySelector('thead th:nth-child(' + (colIdx + 1) + ')');
            var colLabel = colName ? colName.textContent.trim() : '';

            // Cell value actions
            if (text && text !== 'NULL' && text !== '—') {
                items.push({label: 'Filter: ' + colLabel + ' = "' + text.substring(0, 30) + '"', icon: '&#128269;', action: function() { filterByValue(table, colIdx, text); }});
                items.push({label: 'Exclude: ' + colLabel + ' ≠ "' + text.substring(0, 30) + '"', icon: '&#10005;', action: function() { excludeValue(table, colIdx, text); }});
                items.push({sep: true});
            }

            // Copy actions
            items.push({label: 'Copy Cell Value', icon: '&#128203;', action: function() { navigator.clipboard.writeText(text); }});
            items.push({label: 'Copy Row as JSON', icon: '{}', action: function() { copyRowAsJSON(table, cell.closest('tr')); }});
            items.push({label: 'Copy Row as INSERT', icon: 'SQL', action: function() { copyRowAsInsert(table, cell.closest('tr')); }});
            items.push({sep: true});

            // Row detail
            items.push({label: 'View Row Details', icon: '&#9776;', action: function() { showRowDetail(table, cell.closest('tr')); }});
        }

        if (th) {
            var thIdx = Array.from(th.parentElement.children).indexOf(th);
            items.push({label: 'Sort Ascending', icon: '&#9650;', action: function() { sortColumn(table, thIdx, 'asc'); }});
            items.push({label: 'Sort Descending', icon: '&#9660;', action: function() { sortColumn(table, thIdx, 'desc'); }});
            items.push({sep: true});
            items.push({label: 'Hide Column', icon: '&#128065;', action: function() { hideColumn(table, thIdx); }});
        }

        // Copy whole table
        items.push({sep: true});
        items.push({label: 'Copy All as CSV', icon: 'CSV', action: function() { copyTableAs(table, 'csv'); }});
        items.push({label: 'Copy All as JSON', icon: 'JSON', action: function() { copyTableAs(table, 'json'); }});

        items.forEach(function(item) {
            if (item.sep) { var s = document.createElement('div'); s.className = 'ctx-sep'; menu.appendChild(s); return; }
            var el = document.createElement('div');
            el.className = 'ctx-item';
            el.innerHTML = '<span class="ctx-icon">' + item.icon + '</span><span>' + item.label + '</span>';
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

    function closeContextMenu() {
        if (menu) { menu.remove(); menu = null; }
    }

    // ─── Filter by cell value (client-side) ──────────────────────
    function filterByValue(table, colIdx, value) {
        var rows = table.querySelectorAll('tbody tr');
        rows.forEach(function(row) {
            var cell = row.children[colIdx];
            if (!cell) return;
            row.style.display = cell.textContent.trim() === value ? '' : 'none';
        });
        showFilterBanner(table, 'Filtered: showing rows where column = "' + value + '"');
    }

    function excludeValue(table, colIdx, value) {
        var rows = table.querySelectorAll('tbody tr');
        rows.forEach(function(row) {
            var cell = row.children[colIdx];
            if (!cell) return;
            if (cell.textContent.trim() === value) row.style.display = 'none';
        });
        showFilterBanner(table, 'Excluded rows where column = "' + value + '"');
    }

    function showFilterBanner(table, msg) {
        var existing = table.parentElement.querySelector('.filter-banner');
        if (existing) existing.remove();
        var banner = document.createElement('div');
        banner.className = 'filter-banner';
        banner.innerHTML = '<span>' + msg + '</span><button class="btn btn-sm" onclick="clearFilters(this)">Clear Filter</button>';
        table.parentElement.insertBefore(banner, table);
    }

    // ─── Sort column (client-side) ───────────────────────────────
    function sortColumn(table, colIdx, dir) {
        var tbody = table.querySelector('tbody');
        var rows = Array.from(tbody.querySelectorAll('tr'));
        rows.sort(function(a, b) {
            var av = a.children[colIdx] ? a.children[colIdx].textContent.trim() : '';
            var bv = b.children[colIdx] ? b.children[colIdx].textContent.trim() : '';
            var an = parseFloat(av.replace(/[^0-9.\-]/g, ''));
            var bn = parseFloat(bv.replace(/[^0-9.\-]/g, ''));
            if (!isNaN(an) && !isNaN(bn)) return dir === 'asc' ? an - bn : bn - an;
            return dir === 'asc' ? av.localeCompare(bv) : bv.localeCompare(av);
        });
        rows.forEach(function(r) { tbody.appendChild(r); });
    }

    // ─── Hide column ─────────────────────────────────────────────
    function hideColumn(table, colIdx) {
        table.querySelectorAll('tr').forEach(function(row) {
            if (row.children[colIdx]) row.children[colIdx].style.display = 'none';
        });
    }

    // ─── Row detail panel (vertical view) ────────────────────────
    function showRowDetail(table, tr) {
        if (!tr) return;
        var headers = Array.from(table.querySelectorAll('thead th')).map(function(th) { return th.textContent.trim(); });
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
            if (i >= headers.length) return;
            var val = cell.innerHTML;
            html += '<tr><td style="font-weight:600;color:var(--text-2);white-space:nowrap;width:140px;vertical-align:top;padding:6px 10px">' + headers[i] + '</td>';
            html += '<td style="padding:6px 10px;word-break:break-all">' + val + '</td></tr>';
        });
        html += '</tbody></table></div>';

        panel.innerHTML = html;
        overlay.appendChild(panel);
        document.body.appendChild(overlay);
        overlay.addEventListener('click', function(e) { if (e.target === overlay) overlay.remove(); });
    }

    // ─── Copy row as JSON ────────────────────────────────────────
    function copyRowAsJSON(table, tr) {
        var headers = Array.from(table.querySelectorAll('thead th')).map(function(th) { return th.textContent.trim(); });
        var cells = Array.from(tr.children);
        var obj = {};
        cells.forEach(function(cell, i) {
            if (i < headers.length && headers[i]) {
                var val = cell.querySelector('.null-value') ? null : cell.textContent.trim();
                obj[headers[i]] = val;
            }
        });
        navigator.clipboard.writeText(JSON.stringify(obj, null, 2));
    }

    // ─── Copy row as INSERT ──────────────────────────────────────
    function copyRowAsInsert(table, tr) {
        var headers = Array.from(table.querySelectorAll('thead th')).map(function(th) { return th.textContent.trim(); });
        var cells = Array.from(tr.children);
        var cols = [], vals = [];
        cells.forEach(function(cell, i) {
            if (i < headers.length && headers[i] && headers[i] !== '#') {
                cols.push('"' + headers[i] + '"');
                var isNull = cell.querySelector('.null-value');
                if (isNull) vals.push('NULL');
                else vals.push("'" + cell.textContent.trim().replace(/'/g, "''") + "'");
            }
        });
        var sql = 'INSERT INTO table_name (' + cols.join(', ') + ') VALUES (' + vals.join(', ') + ');';
        navigator.clipboard.writeText(sql);
    }

    // ─── Copy table as CSV/JSON ──────────────────────────────────
    function copyTableAs(table, format) {
        var headers = Array.from(table.querySelectorAll('thead th')).map(function(th) { return th.textContent.trim(); });
        var rows = [];
        table.querySelectorAll('tbody tr').forEach(function(tr) {
            if (tr.style.display === 'none') return;
            var row = {};
            Array.from(tr.children).forEach(function(cell, i) {
                if (i < headers.length) row[headers[i]] = cell.querySelector('.null-value') ? null : cell.textContent.trim();
            });
            rows.push(row);
        });

        var text;
        if (format === 'csv') {
            text = headers.join(',') + '\n';
            rows.forEach(function(r) {
                text += headers.map(function(h) {
                    var v = r[h]; if (v === null) return '';
                    return v.indexOf(',') !== -1 || v.indexOf('"') !== -1 ? '"' + v.replace(/"/g, '""') + '"' : v;
                }).join(',') + '\n';
            });
        } else {
            text = JSON.stringify(rows, null, 2);
        }
        navigator.clipboard.writeText(text);
    }
})();

// ─── Clear client-side filters ───────────────────────────────────────

function clearFilters(btn) {
    var wrapper = btn.closest('.table-wrapper') || btn.closest('div');
    if (!wrapper) return;
    wrapper.querySelectorAll('tr[style]').forEach(function(r) { r.style.display = ''; });
    var banner = wrapper.querySelector('.filter-banner');
    if (banner) banner.remove();
}

// ─── Column Header Quick Filter ──────────────────────────────────────
// Double-click any column header to add an inline filter input

document.addEventListener('dblclick', function(e) {
    var th = e.target.closest('th');
    if (!th || th.querySelector('.col-filter-input')) return;
    var table = th.closest('table');
    if (!table) return;
    var colIdx = Array.from(th.parentElement.children).indexOf(th);

    var input = document.createElement('input');
    input.type = 'text';
    input.className = 'col-filter-input';
    input.placeholder = 'Filter...';
    th.appendChild(input);
    input.focus();

    input.addEventListener('input', function() {
        var query = input.value.toLowerCase();
        table.querySelectorAll('tbody tr').forEach(function(row) {
            var cell = row.children[colIdx];
            if (!cell) return;
            row.style.display = (!query || cell.textContent.toLowerCase().indexOf(query) !== -1) ? '' : 'none';
        });
    });

    input.addEventListener('keydown', function(e) {
        if (e.key === 'Escape') { input.remove(); clearFilters(th); }
    });

    input.addEventListener('blur', function() {
        if (!input.value) input.remove();
    });
});

// ─── Row Number Click → Row Detail Panel ─────────────────────────────

document.addEventListener('click', function(e) {
    var rowNum = e.target.closest('.row-num');
    if (!rowNum) return;
    var tr = rowNum.closest('tr');
    var table = tr ? tr.closest('table') : null;
    if (!table || !tr) return;

    var headers = Array.from(table.querySelectorAll('thead th')).map(function(th) { return th.textContent.trim(); });
    var cells = Array.from(tr.children);

    var overlay = document.createElement('div');
    overlay.className = 'command-overlay';
    var panel = document.createElement('div');
    panel.className = 'cell-modal';
    panel.style.maxHeight = '80vh';

    var html = '<div class="cell-modal-header"><span class="cell-modal-title">Row #' + rowNum.textContent.trim() + '</span>';
    html += '<button class="cell-modal-close" onclick="this.closest(\'.command-overlay\').remove()">&times;</button></div>';
    html += '<div style="overflow:auto;max-height:70vh;padding:0">';
    html += '<table style="width:100%"><tbody>';
    cells.forEach(function(cell, i) {
        if (i >= headers.length || !headers[i]) return;
        html += '<tr style="border-bottom:1px solid var(--border-subtle)">';
        html += '<td style="font-weight:600;color:var(--text-2);white-space:nowrap;width:160px;vertical-align:top;padding:8px 12px;background:var(--bg-2)">' + headers[i] + '</td>';
        html += '<td style="padding:8px 12px;word-break:break-all;font-family:var(--font-mono);font-size:var(--font-size-sm)">' + cell.innerHTML + '</td>';
        html += '</tr>';
    });
    html += '</tbody></table></div>';

    panel.innerHTML = html;
    overlay.appendChild(panel);
    document.body.appendChild(overlay);
    overlay.addEventListener('click', function(e) { if (e.target === overlay) overlay.remove(); });
});
