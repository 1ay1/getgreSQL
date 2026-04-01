// ─── Data Grid: Cell Selection & Inline Editing ─────────────────────────

(function() {
    var selectedCell = null;

    // Click on editable cell to select it
    document.addEventListener('click', function(e) {
        var cell = e.target.closest('.editable-cell');
        if (cell) {
            selectCell(cell);
            return;
        }
        // Click outside deselects
        if (selectedCell && !e.target.closest('.cell-edit-input')) {
            deselectCell();
        }
    });

    // Double-click to edit
    document.addEventListener('dblclick', function(e) {
        var cell = e.target.closest('.editable-cell');
        if (cell) editCell(cell);
    });

    // Keyboard navigation on selected cell
    document.addEventListener('keydown', function(e) {
        if (!selectedCell || selectedCell.querySelector('.cell-edit-input')) return;
        var td = selectedCell.closest('td');
        if (!td) return;
        var tr = td.closest('tr');
        if (!tr) return;

        if (e.key === 'Enter' || e.key === 'F2') {
            e.preventDefault();
            editCell(selectedCell);
        } else if (e.key === 'Tab') {
            e.preventDefault();
            var next = e.shiftKey ? getPrevCell(td, tr) : getNextCell(td, tr);
            if (next) selectCell(next);
        } else if (e.key === 'ArrowRight') {
            var next = getNextCell(td, tr);
            if (next) { e.preventDefault(); selectCell(next); }
        } else if (e.key === 'ArrowLeft') {
            var prev = getPrevCell(td, tr);
            if (prev) { e.preventDefault(); selectCell(prev); }
        } else if (e.key === 'ArrowDown') {
            var below = getCellBelow(td, tr);
            if (below) { e.preventDefault(); selectCell(below); }
        } else if (e.key === 'ArrowUp') {
            var above = getCellAbove(td, tr);
            if (above) { e.preventDefault(); selectCell(above); }
        } else if (e.key === 'Delete' || e.key === 'Backspace') {
            // Could implement set-to-null here
        } else if (e.key.length === 1 && !e.ctrlKey && !e.metaKey) {
            // Start typing to edit
            editCell(selectedCell, e.key);
            e.preventDefault();
        }
    });

    function selectCell(cell) {
        if (selectedCell) selectedCell.classList.remove('cell-selected');
        selectedCell = cell;
        cell.classList.add('cell-selected');
        // Highlight the row
        document.querySelectorAll('tr.row-active').forEach(function(r) { r.classList.remove('row-active'); });
        var tr = cell.closest('tr');
        if (tr) tr.classList.add('row-active');
    }

    function deselectCell() {
        if (selectedCell) {
            selectedCell.classList.remove('cell-selected');
            selectedCell = null;
        }
        document.querySelectorAll('tr.row-active').forEach(function(r) { r.classList.remove('row-active'); });
    }

    function getNextCell(td, tr) {
        var next = td.nextElementSibling;
        while (next) {
            var cell = next.querySelector('.editable-cell');
            if (cell) return cell;
            next = next.nextElementSibling;
        }
        // Move to next row
        var nextRow = tr.nextElementSibling;
        if (nextRow) {
            var firstCell = nextRow.querySelector('.editable-cell');
            if (firstCell) return firstCell;
        }
        return null;
    }

    function getPrevCell(td, tr) {
        var prev = td.previousElementSibling;
        while (prev) {
            var cell = prev.querySelector('.editable-cell');
            if (cell) return cell;
            prev = prev.previousElementSibling;
        }
        var prevRow = tr.previousElementSibling;
        if (prevRow) {
            var cells = prevRow.querySelectorAll('.editable-cell');
            if (cells.length) return cells[cells.length - 1];
        }
        return null;
    }

    function getCellBelow(td, tr) {
        var idx = Array.from(tr.children).indexOf(td);
        var nextRow = tr.nextElementSibling;
        if (nextRow && nextRow.children[idx]) {
            return nextRow.children[idx].querySelector('.editable-cell');
        }
        return null;
    }

    function getCellAbove(td, tr) {
        var idx = Array.from(tr.children).indexOf(td);
        var prevRow = tr.previousElementSibling;
        if (prevRow && prevRow.children[idx]) {
            return prevRow.children[idx].querySelector('.editable-cell');
        }
        return null;
    }
})();

function editCell(span, initialChar) {
    if (span.querySelector('.cell-edit-input') || span.querySelector('textarea')) return;

    // For long values, open a modal instead
    if (span.classList.contains('cell-long') || (span.getAttribute('data-full') && !initialChar)) {
        openCellModal(span);
        return;
    }

    var currentValue = span.textContent;
    var col = span.getAttribute('data-col');
    var ctid = span.getAttribute('data-ctid');
    var tr = span.closest('tr');
    var schema = tr ? tr.getAttribute('data-schema') : '';
    var table = tr ? tr.getAttribute('data-table') : '';
    var db = tr ? tr.getAttribute('data-db') : '';

    span.classList.add('cell-editing');
    var input = document.createElement('input');
    input.type = 'text';
    input.value = initialChar || currentValue;
    input.className = 'cell-edit-input';
    span.textContent = '';
    span.appendChild(input);
    input.focus();
    if (!initialChar) input.select();
    else input.selectionStart = input.selectionEnd = input.value.length;

    var committed = false;

    function commit(moveDirection) {
        if (committed) return;
        committed = true;
        var newValue = input.value;
        span.classList.remove('cell-editing');

        if (newValue === currentValue) {
            span.textContent = currentValue;
            if (moveDirection) moveTo(span, moveDirection);
            return;
        }

        span.textContent = newValue;
        span.classList.add('cell-saving');
        saveCellValue(db, schema, table, col, ctid, newValue, span, currentValue);
        if (moveDirection) moveTo(span, moveDirection);
    }

    input.addEventListener('blur', function() { if (!committed) commit(null); });
    input.addEventListener('keydown', function(e) {
        if (e.key === 'Enter') { e.preventDefault(); commit('down'); }
        else if (e.key === 'Tab') { e.preventDefault(); commit(e.shiftKey ? 'left' : 'right'); }
        else if (e.key === 'Escape') { committed = true; span.classList.remove('cell-editing'); span.textContent = currentValue; }
    });
}

function saveCellValue(db, schema, table, col, ctid, newValue, span, oldValue) {
    fetch('/db/' + db + '/schema/' + schema + '/table/' + table + '/update-cell', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'col=' + encodeURIComponent(col) + '&val=' + encodeURIComponent(newValue) + '&ctid=' + encodeURIComponent(ctid)
    }).then(function(r) { return r.json(); }).then(function(data) {
        span.classList.remove('cell-saving');
        if (data.error) {
            span.textContent = oldValue;
            span.classList.add('cell-error');
            setTimeout(function() { span.classList.remove('cell-error'); }, 2000);
        } else {
            span.classList.add('cell-saved');
            setTimeout(function() { span.classList.remove('cell-saved'); }, 1500);
        }
    }).catch(function() {
        span.classList.remove('cell-saving');
        span.textContent = oldValue;
    });
}

function moveTo(fromSpan, dir) {
    var td = fromSpan.closest('td');
    var tr = td ? td.closest('tr') : null;
    if (!td || !tr) return;
    var target = null;
    if (dir === 'right') {
        var next = td.nextElementSibling;
        while (next) { target = next.querySelector('.editable-cell'); if (target) break; next = next.nextElementSibling; }
    } else if (dir === 'left') {
        var prev = td.previousElementSibling;
        while (prev) { target = prev.querySelector('.editable-cell'); if (target) break; prev = prev.previousElementSibling; }
    } else if (dir === 'down') {
        var idx = Array.from(tr.children).indexOf(td);
        var nextRow = tr.nextElementSibling;
        if (nextRow && nextRow.children[idx]) target = nextRow.children[idx].querySelector('.editable-cell');
    }
    if (target) {
        target.classList.add('cell-selected');
        var newTr = target.closest('tr');
        if (newTr) newTr.classList.add('row-active');
    }
}

// ─── Cell Modal (for long values like JSON, text) ────────────────────────

function openCellModal(span) {
    var col = span.getAttribute('data-col');
    var ctid = span.getAttribute('data-ctid');
    var tr = span.closest('tr');
    var schema = tr ? tr.getAttribute('data-schema') : '';
    var table = tr ? tr.getAttribute('data-table') : '';
    var db = tr ? tr.getAttribute('data-db') : '';
    var fullValue = span.getAttribute('data-full') || span.textContent;

    var overlay = document.createElement('div');
    overlay.className = 'command-overlay';

    var modal = document.createElement('div');
    modal.className = 'cell-modal';
    modal.innerHTML =
        '<div class="cell-modal-header">' +
        '<span class="cell-modal-title">Edit: <code>' + col.replace(/</g, '&lt;') + '</code></span>' +
        '<button class="cell-modal-close" onclick="this.closest(\'.command-overlay\').remove()">&times;</button>' +
        '</div>' +
        '<textarea class="cell-modal-textarea" spellcheck="false">' + fullValue.replace(/</g, '&lt;').replace(/>/g, '&gt;') + '</textarea>' +
        '<div class="cell-modal-footer">' +
        '<span class="cell-modal-info">' + fullValue.length + ' chars</span>' +
        '<div class="btn-group">' +
        '<button class="btn btn-sm" data-action="cancel">Cancel</button>' +
        '<button class="btn btn-sm btn-primary" data-action="save">Save</button>' +
        '</div></div>';

    overlay.appendChild(modal);
    document.body.appendChild(overlay);

    var textarea = modal.querySelector('textarea');
    // Auto-size to content
    textarea.style.height = 'auto';
    textarea.style.height = Math.min(Math.max(80, textarea.scrollHeight), window.innerHeight * 0.6) + 'px';
    textarea.focus();
    textarea.selectionStart = textarea.selectionEnd = textarea.value.length;

    // Update char count
    textarea.addEventListener('input', function() {
        modal.querySelector('.cell-modal-info').textContent = textarea.value.length + ' chars';
    });

    overlay.addEventListener('click', function(e) {
        if (e.target === overlay) overlay.remove();
        var action = e.target.getAttribute('data-action');
        if (action === 'cancel') overlay.remove();
        if (action === 'save') {
            var newValue = textarea.value;
            var oldValue = fullValue;
            // Update display
            if (newValue.length <= 80) {
                span.textContent = newValue;
                span.classList.remove('cell-long');
                span.removeAttribute('data-full');
            } else {
                span.textContent = newValue.substring(0, 60);
                span.innerHTML = span.textContent.replace(/</g, '&lt;') + '&hellip;';
                span.setAttribute('data-full', newValue);
            }
            span.classList.add('cell-saving');
            saveCellValue(db, schema, table, col, ctid, newValue, span, oldValue);
            overlay.remove();
        }
    });

    // Ctrl+Enter to save
    textarea.addEventListener('keydown', function(e) {
        if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
            modal.querySelector('[data-action="save"]').click();
        }
        if (e.key === 'Escape') overlay.remove();
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

