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

function treeToggle(el) {
    const item = el.closest('.tree-item');
    if (!item) return;
    const children = item.querySelector('.tree-children');
    const chevron = el.querySelector('.tree-chevron');
    if (!children || !chevron) return;

    if (children.classList.contains('loaded')) {
        // Already loaded, just toggle visibility
        const isHidden = children.style.display === 'none';
        children.style.display = isHidden ? '' : 'none';
        chevron.classList.toggle('expanded', isHidden);
    }
    // If not loaded, htmx will handle loading and we just need to expand
}

// After htmx loads tree children, mark as loaded and expand
document.addEventListener('htmx:afterSwap', function(e) {
    if (e.detail.target.classList && e.detail.target.classList.contains('tree-children')) {
        e.detail.target.classList.add('loaded');
        e.detail.target.style.display = '';
        // Expand the chevron
        const item = e.detail.target.closest('.tree-item');
        if (item) {
            const chevron = item.querySelector('.tree-chevron');
            if (chevron) chevron.classList.add('expanded');
        }
    }
});

// Tree row selection highlight
document.addEventListener('click', function(e) {
    var row = e.target.closest('.tree-row');
    if (!row) return;
    // Remove previous selection
    document.querySelectorAll('.tree-row.selected').forEach(function(el) {
        el.classList.remove('selected');
    });
    row.classList.add('selected');
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
