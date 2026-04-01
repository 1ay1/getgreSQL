// ─── Cell Editing & Explain — global functions ──────────────────────────
//
// These are called by the DataGrid engine (06-dataview.js) and the
// context menu (05-extras.js). Selection and keyboard navigation
// are handled by the grid engine; this file only provides:
//
//   startEdit(span, initialChar)  — inline editing
//   saveCellValue(...)            — POST to server
//   moveTo(span, direction)       — move cursor after edit commit
//   openCellModal(span)           — modal for long values
//   explainCell(cell)             — "Explain This" lineage panel
//   getColNameFor(span)           — utility
//   showEditToast(span, msg)      — utility

// ─── Explain This — fetch and show lineage panel ────────────────────────

function explainCell(cell) {
    if (!cell) return;
    var old = document.querySelector('.dv-lineage-popover');
    if (old) old.remove();

    var tableOid = cell.getAttribute('data-table-oid') || '0';
    var col = cell.getAttribute('data-col') || getColNameFor(cell);
    var val = cell.getAttribute('data-full') || cell.textContent;
    var tr = cell.closest('tr');
    var ctid = tr ? (tr.getAttribute('data-ctid') || '') : '';

    var url = '/dv/explain-cell?table_oid=' + encodeURIComponent(tableOid) +
              '&col=' + encodeURIComponent(col) +
              '&val=' + encodeURIComponent(val) +
              '&ctid=' + encodeURIComponent(ctid);

    var panel = document.createElement('div');
    panel.className = 'dv-lineage-popover';
    panel.innerHTML = '<div class="dv-lineage-panel"><div class="dv-lineage-header">Loading&hellip;</div></div>';
    panel.style.cssText = 'position:fixed;z-index:10001;top:50%;left:50%;transform:translate(-50%,-50%)';
    document.body.appendChild(panel);

    fetch(url).then(function(r) { return r.text(); }).then(function(html) {
        panel.innerHTML = html;
    }).catch(function() {
        panel.innerHTML = '<div class="dv-lineage-panel"><div class="dv-lineage-header">Failed to load</div></div>';
    });

    function closePanel(ev) {
        if (ev.type === 'keydown' && ev.key !== 'Escape') return;
        if (ev.type === 'mousedown' && panel.contains(ev.target)) return;
        panel.remove();
        document.removeEventListener('mousedown', closePanel);
        document.removeEventListener('keydown', closePanel);
    }
    setTimeout(function() {
        document.addEventListener('mousedown', closePanel);
        document.addEventListener('keydown', closePanel);
    }, 50);
}

// ─── Start Editing — works for both editable and read-only cells ─────

function startEdit(span, initialChar) {
    if (span.querySelector('.cell-edit-input') || span.querySelector('textarea')) return;
    window._dvEditing = true;

    // Long values → modal
    if ((span.classList.contains('dv-cell-long') || span.classList.contains('cell-long')) &&
        span.getAttribute('data-full') && !initialChar) {
        openCellModal(span);
        return;
    }

    var currentValue = span.getAttribute('data-full') || span.textContent;
    var directEdit = span.classList.contains('editable-cell') && (span.getAttribute('data-ctid') || span.closest('tr[data-ctid]'));

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
        window._dvEditing = false;
        var newValue = input.value;
        span.classList.remove('cell-editing');

        if (newValue === currentValue) {
            span.textContent = currentValue;
            if (moveDirection) moveTo(span, moveDirection);
            return;
        }

        // Update grid data store
        var dv = span.closest('.data-view');
        var grid = dv ? dv._grid : null;

        if (directEdit) {
            var db = dv ? dv.getAttribute('data-db') : '';
            var schema = dv ? dv.getAttribute('data-schema') : '';
            var table = dv ? dv.getAttribute('data-table') : '';
            var col = span.getAttribute('data-col');
            var tr = span.closest('tr');
            var ctid = span.getAttribute('data-ctid') || (tr ? tr.getAttribute('data-ctid') : '');

            span.textContent = newValue;
            span.classList.add('cell-saving');
            saveCellValue(db, schema, table, col, ctid, newValue, span, currentValue);

            if (grid) {
                var coords = grid._findCoords(span);
                if (coords) grid.updateCellValue(coords.di, coords.c, newValue, false);
            }
        } else {
            span.textContent = newValue;
            var colName = getColNameFor(span);
            var oldEsc = currentValue.replace(/'/g, "''");
            var newEsc = newValue.replace(/'/g, "''");
            var sql = "UPDATE table_name SET \"" + colName + "\" = '" + newEsc +
                      "' WHERE \"" + colName + "\" = '" + oldEsc + "'; -- review and run";
            navigator.clipboard.writeText(sql);
            showEditToast(span, 'UPDATE copied to clipboard');
            setTimeout(function() { span.textContent = currentValue; }, 2000);
        }
        if (moveDirection) moveTo(span, moveDirection);
    }

    input.addEventListener('blur', function() { if (!committed) commit(null); });
    input.addEventListener('keydown', function(e) {
        if (e.key === 'Enter') { e.preventDefault(); commit('down'); }
        else if (e.key === 'Tab') { e.preventDefault(); commit(e.shiftKey ? 'left' : 'right'); }
        else if (e.key === 'Escape') {
            committed = true;
            window._dvEditing = false;
            span.classList.remove('cell-editing');
            span.textContent = currentValue;
        }
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
    // Use grid engine if available
    var dv = fromSpan.closest('.data-view');
    var grid = dv ? dv._grid : null;
    if (grid) {
        grid.moveFromSpan(fromSpan, dir);
        return;
    }
    // Fallback for non-grid views
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
    window._dvEditing = true;
    var col = span.getAttribute('data-col') || getColNameFor(span);
    var ctid = span.getAttribute('data-ctid') || (span.closest('tr') ? span.closest('tr').getAttribute('data-ctid') : '');
    var dv = span.closest('.data-view');
    var schema = dv ? dv.getAttribute('data-schema') : '';
    var table = dv ? dv.getAttribute('data-table') : '';
    var db = dv ? dv.getAttribute('data-db') : '';
    var directEdit = span.classList.contains('editable-cell') && ctid;

    var fullValue = span.getAttribute('data-full') || span.textContent;

    // Try to pretty-print JSON
    var displayValue = fullValue;
    try {
        var parsed = JSON.parse(fullValue);
        displayValue = JSON.stringify(parsed, null, 2);
    } catch(e) {}

    var overlay = document.createElement('div');
    overlay.className = 'command-overlay';
    var modal = document.createElement('div');
    modal.className = 'cell-modal';
    modal.innerHTML =
        '<div class="cell-modal-header">' +
        '<span class="cell-modal-title">' + (col || 'Cell') + '</span>' +
        '<span class="cell-modal-chars">' + fullValue.length + ' chars</span>' +
        '<button class="cell-modal-close">&times;</button></div>' +
        '<textarea class="cell-modal-textarea">' + displayValue.replace(/&/g,'&amp;').replace(/</g,'&lt;') + '</textarea>' +
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

    function closeModal() { overlay.remove(); window._dvEditing = false; }

    modal.querySelector('.cell-modal-save').addEventListener('click', function() {
        var newValue = textarea.value;
        if (newValue === fullValue) { closeModal(); return; }

        // Update grid data store
        var grid = dv ? dv._grid : null;

        if (directEdit) {
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
            if (grid) {
                var coords = grid._findCoords(span);
                if (coords) grid.updateCellValue(coords.di, coords.c, newValue, false);
            }
        } else {
            var colName = col || 'column';
            var oldEsc = fullValue.replace(/'/g, "''");
            var newEsc = newValue.replace(/'/g, "''");
            var sql = "UPDATE table_name SET \"" + colName + "\" = '" + newEsc +
                      "' WHERE \"" + colName + "\" = '" + oldEsc + "';";
            navigator.clipboard.writeText(sql);
            showEditToast(span, 'UPDATE copied to clipboard');
        }
        closeModal();
    });

    modal.querySelector('.cell-modal-cancel').addEventListener('click', closeModal);
    modal.querySelector('.cell-modal-close').addEventListener('click', closeModal);
    overlay.addEventListener('click', function(e) { if (e.target === overlay) closeModal(); });
    overlay.addEventListener('keydown', function(e) {
        if (e.key === 'Escape') closeModal();
        if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') modal.querySelector('.cell-modal-save').click();
    });
}
