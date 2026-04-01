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

