// ─── Unified Cell Selection & Editing ────────────────────────────────────
//
// Works on BOTH .editable-cell (table browse, direct save) and
// .dv-cell (query results, generates UPDATE for review).
//
// Interactions:
//   Click         → select cell
//   Right-click   → "Explain This" lineage popover
//   Double-click  → start editing
//   F2 / Enter    → start editing
//   Type a key    → start editing with that character
//   Tab           → commit + move right (Shift+Tab = left)
//   Enter         → commit + move down
//   Escape        → cancel edit
//   Arrow keys    → navigate cells
//   Delete        → set to NULL (editable only)

(function() {
    var selectedCell = null;
    var CELL_SEL = '.editable-cell, .dv-cell';

    // ── Click to select ──────────────────────────────────────────────
    document.addEventListener('click', function(e) {
        var cell = e.target.closest(CELL_SEL);
        if (cell) {
            selectCell(cell);
            return;
        }
        if (selectedCell && !e.target.closest('.cell-edit-input')) {
            deselectCell();
        }
    });

    // ── Right-click → Explain This ───────────────────────────────────
    document.addEventListener('contextmenu', function(e) {
        var cell = e.target.closest(CELL_SEL);
        if (!cell) return;

        e.preventDefault();
        selectCell(cell);

        // Remove any existing lineage panel
        var old = document.querySelector('.dv-lineage-popover');
        if (old) old.remove();

        // Gather cell metadata
        var tableOid = cell.getAttribute('data-table-oid') || '0';
        var col = cell.getAttribute('data-col') || getColNameFor(cell);
        var val = cell.getAttribute('data-full') || cell.textContent;
        var tr = cell.closest('tr');
        var ctid = tr ? (tr.getAttribute('data-ctid') || '') : '';

        // Fetch lineage panel from server
        var url = '/dv/explain-cell?table_oid=' + encodeURIComponent(tableOid) +
                  '&col=' + encodeURIComponent(col) +
                  '&val=' + encodeURIComponent(val) +
                  '&ctid=' + encodeURIComponent(ctid);

        var popover = document.createElement('div');
        popover.className = 'dv-lineage-popover';
        popover.innerHTML = '<div class="dv-lineage-panel"><div class="dv-lineage-header">Loading&hellip;</div></div>';

        // Position near the click
        popover.style.position = 'fixed';
        popover.style.zIndex = '10000';
        positionPopover(popover, e.clientX, e.clientY);
        document.body.appendChild(popover);

        fetch(url).then(function(r) { return r.text(); }).then(function(html) {
            popover.innerHTML = html;
            positionPopover(popover, e.clientX, e.clientY);
        }).catch(function() {
            popover.innerHTML = '<div class="dv-lineage-panel"><div class="dv-lineage-header">Failed to load</div></div>';
        });

        // Close on click outside or Escape
        function closePopover(ev) {
            if (ev.type === 'keydown' && ev.key !== 'Escape') return;
            if (ev.type === 'mousedown' && popover.contains(ev.target)) return;
            popover.remove();
            document.removeEventListener('mousedown', closePopover);
            document.removeEventListener('keydown', closePopover);
        }
        // Delay so this click doesn't immediately close it
        setTimeout(function() {
            document.addEventListener('mousedown', closePopover);
            document.addEventListener('keydown', closePopover);
        }, 50);
    });

    function positionPopover(el, x, y) {
        var pad = 8;
        el.style.left = x + 'px';
        el.style.top = y + 'px';
        // Adjust if overflowing viewport
        requestAnimationFrame(function() {
            var rect = el.getBoundingClientRect();
            if (rect.right > window.innerWidth - pad) {
                el.style.left = Math.max(pad, x - rect.width) + 'px';
            }
            if (rect.bottom > window.innerHeight - pad) {
                el.style.top = Math.max(pad, y - rect.height) + 'px';
            }
        });
    }

    // ── Double-click to edit ─────────────────────────────────────────
    // Skip cells with hx-get — those are handled by htmx SSR editing
    document.addEventListener('dblclick', function(e) {
        var cell = e.target.closest(CELL_SEL);
        if (cell && !cell.hasAttribute('hx-get')) startEdit(cell);
    });

    // ── Keyboard ─────────────────────────────────────────────────────
    document.addEventListener('keydown', function(e) {
        if (!selectedCell || selectedCell.querySelector('.cell-edit-input')) return;
        // Don't intercept if focus is in an input/textarea elsewhere
        var active = document.activeElement;
        if (active && (active.tagName === 'INPUT' || active.tagName === 'TEXTAREA') && !active.classList.contains('cell-edit-input')) return;

        var td = selectedCell.closest('td');
        if (!td) return;
        var tr = td.closest('tr');
        if (!tr) return;

        if (e.key === 'Enter' || e.key === 'F2') {
            e.preventDefault();
            // htmx-managed cells: trigger htmx click to load edit form
            if (selectedCell.hasAttribute('hx-get')) {
                htmx.trigger(selectedCell, 'dblclick');
            } else {
                startEdit(selectedCell);
            }
        } else if (e.key === 'Tab') {
            e.preventDefault();
            var next = e.shiftKey ? getPrev(td, tr) : getNext(td, tr);
            if (next) selectCell(next);
        } else if (e.key === 'ArrowRight') {
            var n = getNext(td, tr);
            if (n) { e.preventDefault(); selectCell(n); }
        } else if (e.key === 'ArrowLeft') {
            var p = getPrev(td, tr);
            if (p) { e.preventDefault(); selectCell(p); }
        } else if (e.key === 'ArrowDown') {
            var b = getBelow(td, tr);
            if (b) { e.preventDefault(); selectCell(b); }
        } else if (e.key === 'ArrowUp') {
            var a = getAbove(td, tr);
            if (a) { e.preventDefault(); selectCell(a); }
        } else if (e.key === 'Delete' || e.key === 'Backspace') {
            if (isDirectEditable(selectedCell)) {
                e.preventDefault();
                setToNull(selectedCell);
            }
        } else if (e.key === 'Escape') {
            deselectCell();
        } else if (e.key.length === 1 && !e.ctrlKey && !e.metaKey) {
            e.preventDefault();
            if (selectedCell.hasAttribute('hx-get')) {
                htmx.trigger(selectedCell, 'dblclick');
            } else {
                startEdit(selectedCell, e.key);
            }
        }
    });

    // ── Selection helpers ────────────────────────────────────────────

    function selectCell(cell) {
        if (selectedCell) selectedCell.classList.remove('cell-selected');
        selectedCell = cell;
        cell.classList.add('cell-selected');
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

    // ── Navigation ──────────────────────────────────────────────────

    function getNext(td, tr) {
        var n = td.nextElementSibling;
        while (n) { var c = n.querySelector(CELL_SEL); if (c) return c; n = n.nextElementSibling; }
        var nr = tr.nextElementSibling;
        if (nr) { var c = nr.querySelector(CELL_SEL); if (c) return c; }
        return null;
    }
    function getPrev(td, tr) {
        var p = td.previousElementSibling;
        while (p) { var c = p.querySelector(CELL_SEL); if (c) return c; p = p.previousElementSibling; }
        var pr = tr.previousElementSibling;
        if (pr) { var all = pr.querySelectorAll(CELL_SEL); if (all.length) return all[all.length - 1]; }
        return null;
    }
    function getBelow(td, tr) {
        var idx = Array.from(tr.children).indexOf(td);
        var nr = tr.nextElementSibling;
        return nr && nr.children[idx] ? nr.children[idx].querySelector(CELL_SEL) : null;
    }
    function getAbove(td, tr) {
        var idx = Array.from(tr.children).indexOf(td);
        var pr = tr.previousElementSibling;
        return pr && pr.children[idx] ? pr.children[idx].querySelector(CELL_SEL) : null;
    }

    // ── Is this cell directly editable (has table context)? ─────────

    function isDirectEditable(span) {
        return span.classList.contains('editable-cell') && span.getAttribute('data-ctid');
    }

    // ── Get column name for a cell ──────────────────────────────────

    function getColName(span) {
        if (span.getAttribute('data-col')) return span.getAttribute('data-col');
        var td = span.closest('td');
        var tr = td ? td.closest('tr') : null;
        if (!td || !tr) return '';
        var idx = Array.from(tr.children).indexOf(td);
        var table = tr.closest('table');
        if (!table || !table.tHead) return '';
        var th = table.tHead.rows[0].children[idx];
        if (!th) return '';
        var text = th.querySelector('.dv-th-text');
        return text ? text.textContent.trim() : th.textContent.trim();
    }

    // ── Set to NULL (editable cells only) ───────────────────────────

    function setToNull(span) {
        if (!isDirectEditable(span)) return;
        var dv = span.closest('.data-view');
        var db = dv ? dv.getAttribute('data-db') : '';
        var schema = dv ? dv.getAttribute('data-schema') : '';
        var table = dv ? dv.getAttribute('data-table') : '';
        var col = span.getAttribute('data-col');
        var ctid = span.getAttribute('data-ctid');
        var oldValue = span.textContent;

        span.textContent = 'NULL';
        span.classList.add('null-value', 'cell-saving');
        saveCellValue(db, schema, table, col, ctid, '', span, oldValue);
    }
})();

// ─── Start Editing — works for both editable and read-only cells ─────

function startEdit(span, initialChar) {
    if (span.querySelector('.cell-edit-input') || span.querySelector('textarea')) return;

    // Long values → modal
    if ((span.classList.contains('dv-cell-long') || span.classList.contains('cell-long')) &&
        span.getAttribute('data-full') && !initialChar) {
        openCellModal(span);
        return;
    }

    var currentValue = span.getAttribute('data-full') || span.textContent;
    var directEdit = span.classList.contains('editable-cell') && span.getAttribute('data-ctid');

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

        if (directEdit) {
            // ── Editable mode: save directly to DB ──────────────
            var dv = span.closest('.data-view');
            var db = dv ? dv.getAttribute('data-db') : '';
            var schema = dv ? dv.getAttribute('data-schema') : '';
            var table = dv ? dv.getAttribute('data-table') : '';
            var col = span.getAttribute('data-col');
            var ctid = span.getAttribute('data-ctid');

            span.textContent = newValue;
            span.classList.add('cell-saving');
            saveCellValue(db, schema, table, col, ctid, newValue, span, currentValue);
        } else {
            // ── Read-only mode: generate UPDATE and copy to clipboard ──
            span.textContent = newValue;
            var colName = getColNameFor(span);
            var oldEsc = currentValue.replace(/'/g, "''");
            var newEsc = newValue.replace(/'/g, "''");
            var sql = "UPDATE table_name SET \"" + colName + "\" = '" + newEsc +
                      "' WHERE \"" + colName + "\" = '" + oldEsc + "'; -- review and run";
            navigator.clipboard.writeText(sql);
            showEditToast(span, 'UPDATE copied to clipboard');
            // Revert the visual — this was just for generating SQL
            setTimeout(function() { span.textContent = currentValue; }, 2000);
        }
        if (moveDirection) moveTo(span, moveDirection);
    }

    input.addEventListener('blur', function() { if (!committed) commit(null); });
    input.addEventListener('keydown', function(e) {
        if (e.key === 'Enter') { e.preventDefault(); commit('down'); }
        else if (e.key === 'Tab') { e.preventDefault(); commit(e.shiftKey ? 'left' : 'right'); }
        else if (e.key === 'Escape') { committed = true; span.classList.remove('cell-editing'); span.textContent = currentValue; }
    });
}

function getColNameFor(span) {
    if (span.getAttribute('data-col')) return span.getAttribute('data-col');
    var td = span.closest('td');
    var tr = td ? td.closest('tr') : null;
    if (!td || !tr) return 'column';
    var idx = Array.from(tr.children).indexOf(td);
    var table = tr.closest('table');
    if (!table || !table.tHead) return 'column';
    var th = table.tHead.rows[0].children[idx];
    if (!th) return 'column';
    var text = th.querySelector('.dv-th-text');
    return text ? text.textContent.trim() : th.textContent.trim();
}

function showEditToast(span, msg) {
    var dv = span.closest('.data-view');
    var container = dv || document.body;
    var toast = document.createElement('div');
    toast.className = 'dv-toast dv-toast-show';
    toast.textContent = msg;
    if (!dv) toast.style.cssText = 'position:fixed;top:16px;right:16px;z-index:10001';
    container.appendChild(toast);
    setTimeout(function() { toast.classList.remove('dv-toast-show'); }, 1500);
    setTimeout(function() { toast.remove(); }, 1800);
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
    var SEL = '.editable-cell, .dv-cell';
    var td = fromSpan.closest('td');
    var tr = td ? td.closest('tr') : null;
    if (!td || !tr) return;
    var target = null;
    var idx = Array.from(tr.children).indexOf(td);
    if (dir === 'right') {
        var n = td.nextElementSibling;
        while (n) { target = n.querySelector(SEL); if (target) break; n = n.nextElementSibling; }
    } else if (dir === 'left') {
        var p = td.previousElementSibling;
        while (p) { target = p.querySelector(SEL); if (target) break; p = p.previousElementSibling; }
    } else if (dir === 'down') {
        var nr = tr.nextElementSibling;
        if (nr && nr.children[idx]) target = nr.children[idx].querySelector(SEL);
    }
    if (target) {
        target.classList.add('cell-selected');
        var newTr = target.closest('tr');
        if (newTr) newTr.classList.add('row-active');
    }
}

// ─── Cell Modal (for long values like JSON, text) ────────────────────────

function openCellModal(span) {
    var col = span.getAttribute('data-col') || getColNameFor(span);
    var ctid = span.getAttribute('data-ctid');
    var dv = span.closest('.data-view');
    var schema = dv ? dv.getAttribute('data-schema') : '';
    var table = dv ? dv.getAttribute('data-table') : '';
    var db = dv ? dv.getAttribute('data-db') : '';
    var directEdit = span.classList.contains('editable-cell') && ctid;

    var fullValue = span.getAttribute('data-full') || span.textContent;

    var overlay = document.createElement('div');
    overlay.className = 'command-overlay';
    var modal = document.createElement('div');
    modal.className = 'cell-modal';
    modal.innerHTML =
        '<div class="cell-modal-header">' +
        '<span class="cell-modal-title">' + (col || 'Cell') + '</span>' +
        '<span class="cell-modal-chars">' + fullValue.length + ' chars</span>' +
        '<button class="cell-modal-close">&times;</button></div>' +
        '<textarea class="cell-modal-textarea">' + fullValue.replace(/&/g,'&amp;').replace(/</g,'&lt;') + '</textarea>' +
        '<div class="cell-modal-footer">' +
        (directEdit ? '<button class="btn btn-sm btn-primary cell-modal-save">Save</button>' :
                       '<button class="btn btn-sm btn-primary cell-modal-save">Copy UPDATE</button>') +
        '<button class="btn btn-sm cell-modal-cancel">Cancel</button></div>';

    overlay.appendChild(modal);
    document.body.appendChild(overlay);

    var textarea = modal.querySelector('textarea');
    var charCount = modal.querySelector('.cell-modal-chars');
    textarea.style.height = Math.min(window.innerHeight * 0.6, Math.max(80, textarea.scrollHeight)) + 'px';
    textarea.focus();

    textarea.addEventListener('input', function() {
        charCount.textContent = textarea.value.length + ' chars';
    });

    modal.querySelector('.cell-modal-save').addEventListener('click', function() {
        var newValue = textarea.value;
        if (newValue === fullValue) { overlay.remove(); return; }

        if (directEdit) {
            // Direct save
            if (newValue.length <= 80) {
                span.textContent = newValue;
                span.classList.remove('dv-cell-long', 'cell-long');
                span.removeAttribute('data-full');
            } else {
                span.textContent = newValue.substring(0, 60) + '\u2026';
                span.setAttribute('data-full', newValue);
            }
            span.classList.add('cell-saving');
            saveCellValue(db, schema, table, col, ctid, newValue, span, fullValue);
        } else {
            // Generate UPDATE
            var colName = col || 'column';
            var oldEsc = fullValue.replace(/'/g, "''");
            var newEsc = newValue.replace(/'/g, "''");
            var sql = "UPDATE table_name SET \"" + colName + "\" = '" + newEsc +
                      "' WHERE \"" + colName + "\" = '" + oldEsc + "';";
            navigator.clipboard.writeText(sql);
            showEditToast(span, 'UPDATE copied to clipboard');
        }
        overlay.remove();
    });

    modal.querySelector('.cell-modal-cancel').addEventListener('click', function() { overlay.remove(); });
    modal.querySelector('.cell-modal-close').addEventListener('click', function() { overlay.remove(); });
    overlay.addEventListener('click', function(e) { if (e.target === overlay) overlay.remove(); });
    overlay.addEventListener('keydown', function(e) {
        if (e.key === 'Escape') overlay.remove();
        if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') modal.querySelector('.cell-modal-save').click();
    });
}
