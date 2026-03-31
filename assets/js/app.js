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

// ─── Section Tab Clicks ──────────────────────────────────────────────────
// Handles data-tab-url tabs via fetch — more reliable than hx-* attributes
// which can break after SPA navigation swaps the DOM.

document.addEventListener('click', function(e) {
    var tab = e.target.closest('.section-tab[data-tab-url]');
    if (!tab) return;

    var url = tab.getAttribute('data-tab-url');
    var tabsContainer = tab.closest('.section-tabs');
    var targetId = tabsContainer ? tabsContainer.getAttribute('data-target') : null;
    var target = targetId ? document.getElementById(targetId) : null;
    if (!url || !target) return;

    // Update active state
    tabsContainer.querySelectorAll('.section-tab').forEach(function(t) {
        t.classList.remove('active');
    });
    tab.classList.add('active');

    // Fetch and swap
    target.innerHTML = '<div class="loading">Loading...</div>';
    fetch(url, { headers: { 'HX-Request': 'true' } })
        .then(function(r) { return r.text(); })
        .then(function(html) {
            target.innerHTML = html;
            if (window.htmx) htmx.process(target);
        })
        .catch(function(err) {
            target.innerHTML = '<div class="alert alert-error">' + err.message + '</div>';
        });
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

// ─── SPA Router ──────────────────────────────────────────────────────────
// Intercepts [data-spa] link clicks, fetches the full page, swaps only
// the .workspace div. Sidebar tree and toolbar stay intact.

(function() {
    // Intercept clicks on [data-spa] links AND internal content links
    document.addEventListener('click', function(e) {
        var link = e.target.closest('a[href]');
        if (!link) return;
        if (e.ctrlKey || e.metaKey || e.shiftKey || e.altKey) return;

        var href = link.getAttribute('href');
        if (!href || href.startsWith('http') || href.startsWith('#') || href.startsWith('javascript')) return;

        // Skip links with htmx attributes (they handle their own swapping)
        if (link.hasAttribute('hx-get') || link.hasAttribute('hx-post')) return;

        // Skip download links
        if (link.hasAttribute('download') || href.endsWith('/export')) return;

        e.preventDefault();
        e.stopPropagation();
        spaNavigate(href);
    });

    // Browser back/forward
    window.addEventListener('popstate', function() {
        spaNavigate(window.location.pathname, true);
    });

    function spaNavigate(url, skipPush) {
        var workspace = document.querySelector('.workspace');
        if (!workspace) { window.location = url; return; }

        // Phase 1: fade out current content
        var content = workspace.querySelector('.content') || workspace;
        content.classList.add('spa-exit');

        // Fetch while animating out
        var fetchPromise = fetch(url, { headers: { 'X-SPA': '1' } })
            .then(function(r) {
                if (!r.ok) throw new Error(r.status);
                return r.text();
            });

        // Wait for both animation and fetch
        var animDone = new Promise(function(resolve) { setTimeout(resolve, 120); });

        Promise.all([fetchPromise, animDone])
            .then(function(results) {
                var html = results[0];
                var parser = new DOMParser();
                var doc = parser.parseFromString(html, 'text/html');

                var newWorkspace = doc.querySelector('.workspace');
                if (!newWorkspace) { window.location = url; return; }

                // Phase 2: swap and fade in
                workspace.outerHTML = newWorkspace.outerHTML;

                var ws = document.querySelector('.workspace');
                var newContent = ws.querySelector('.content') || ws;
                newContent.classList.add('spa-enter');
                requestAnimationFrame(function() {
                    requestAnimationFrame(function() {
                        newContent.classList.remove('spa-enter');
                    });
                });

                // Update title
                var newTitle = doc.querySelector('title');
                if (newTitle) document.title = newTitle.textContent;

                // Update toolbar
                var newToolbarNav = doc.querySelector('.toolbar-nav');
                var currentToolbarNav = document.querySelector('.toolbar-nav');
                if (newToolbarNav && currentToolbarNav) {
                    currentToolbarNav.innerHTML = newToolbarNav.innerHTML;
                }

                if (!skipPush) history.pushState(null, '', url);

                if (ws && window.htmx) htmx.process(ws);

                var queryWs = document.getElementById('query-workspace');
                if (queryWs && window.SQLEditor) SQLEditor.init(queryWs);

                treeHighlightCurrent();

                var c = ws.querySelector('.content');
                if (c) c.scrollTop = 0;
            })
            .catch(function() {
                window.location = url;
            });
    }

    // Expose for programmatic navigation
    window.spaNavigate = spaNavigate;
})();

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
