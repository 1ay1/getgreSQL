#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct CoreRuntime {
    static constexpr auto js() -> std::string_view { return R"_JS_(
// getgreSQL — Database IDE client-side interactions

// ─── Theme ───────────────────────────────────────────────────────────────

var THEMES = [
    {id:'',                 name:'Dark',             bg:'#0d1117', fg:'#58a6ff'},
    {id:'light',            name:'Light',            bg:'#ffffff', fg:'#0969da'},
    {id:'monokai',          name:'Monokai',          bg:'#272822', fg:'#f92672'},
    {id:'dracula',          name:'Dracula',          bg:'#282a36', fg:'#bd93f9'},
    {id:'nord',             name:'Nord',             bg:'#2e3440', fg:'#88c0d0'},
    {id:'one-dark',         name:'One Dark',         bg:'#282c34', fg:'#61afef'},
    {id:'solarized-dark',   name:'Solarized Dark',   bg:'#002b36', fg:'#268bd2'},
    {id:'solarized-light',  name:'Solarized Light',  bg:'#fdf6e3', fg:'#268bd2'},
    {id:'catppuccin-mocha', name:'Catppuccin Mocha', bg:'#1e1e2e', fg:'#89b4fa'},
    {id:'catppuccin-latte', name:'Catppuccin Latte', bg:'#eff1f5', fg:'#1e66f5'},
    {id:'gruvbox-dark',     name:'Gruvbox Dark',     bg:'#282828', fg:'#83a598'},
    {id:'gruvbox-light',    name:'Gruvbox Light',    bg:'#fbf1c7', fg:'#076678'},
    {id:'tokyo-night',      name:'Tokyo Night',      bg:'#1a1b26', fg:'#7aa2f7'},
    {id:'kanagawa',         name:'Kanagawa',         bg:'#1f1f28', fg:'#7e9cd8'},
    {id:'rose-pine',        name:'Ros\u00e9 Pine',   bg:'#191724', fg:'#c4a7e7'},
    {id:'everforest',       name:'Everforest',       bg:'#2d353b', fg:'#7fbbb3'},
    {id:'ayu-dark',         name:'Ayu Dark',         bg:'#0b0e14', fg:'#e6b450'},
    {id:'ayu-mirage',       name:'Ayu Mirage',       bg:'#1f2430', fg:'#ffcc66'},
    {id:'ayu-light',        name:'Ayu Light',        bg:'#fafafa', fg:'#f2ae49'},
    {id:'palenight',        name:'Palenight',        bg:'#292d3e', fg:'#82aaff'},
    {id:'moonlight',        name:'Moonlight',        bg:'#1e2030', fg:'#82aaff'},
    {id:'night-owl',        name:'Night Owl',        bg:'#011627', fg:'#82aaff'},
    {id:'horizon',          name:'Horizon',          bg:'#1c1e26', fg:'#e95678'},
    {id:'poimandres',       name:'Poimandres',       bg:'#1b1e28', fg:'#add7ff'},
    {id:'cobalt2',          name:'Cobalt2',          bg:'#122738', fg:'#ffc600'},
    {id:'andromeda',        name:'Andromeda',        bg:'#23262e', fg:'#00e8c6'},
    {id:'shades-of-purple', name:'Shades of Purple', bg:'#1e1e3f', fg:'#fad000'},
    {id:'vitesse-dark',     name:'Vitesse Dark',     bg:'#121212', fg:'#4d9375'},
    {id:'midnight',         name:'Midnight',         bg:'#0f0f1e', fg:'#7b68ee'},
    {id:'github-dimmed',    name:'GitHub Dimmed',    bg:'#22272e', fg:'#539bf5'},
    {id:'slack-dark',       name:'Slack Dark',       bg:'#1a1d21', fg:'#1d9bd1'},
    {id:'synthwave',        name:'Synthwave \u201984',bg:'#1a1028', fg:'#ff7edb'},
    {id:'cyberpunk',        name:'Cyberpunk',        bg:'#0a0a12', fg:'#ff003c'},
    {id:'matrix',           name:'Matrix',           bg:'#000000', fg:'#00ff41'},
    {id:'amber',            name:'Amber Terminal',   bg:'#0c0800', fg:'#ffb000'},
    {id:'tron',             name:'Tron',             bg:'#0a0a0f', fg:'#6ee2ff'},
    {id:'high-contrast',    name:'High Contrast',    bg:'#000000', fg:'#71b7ff'},
    {id:'paper',            name:'Paper',            bg:'#f5f0eb', fg:'#8b5c34'}
];

function applyTheme(id) {
    if (id) document.documentElement.setAttribute('data-theme', id);
    else document.documentElement.removeAttribute('data-theme');
    localStorage.setItem('theme', id);
}

function toggleTheme() {
    // Open theme picker instead of simple toggle
    if (document.querySelector('.theme-picker-overlay')) { closeThemePicker(); return; }

    var overlay = document.createElement('div');
    overlay.className = 'theme-picker-overlay';
    var picker = document.createElement('div');
    picker.className = 'theme-picker';
    picker.innerHTML = '<div class="theme-picker-header"><span>Choose Theme</span><button class="theme-picker-close" onclick="closeThemePicker()">&times;</button></div>';

    var grid = document.createElement('div');
    grid.className = 'theme-picker-grid';

    var current = localStorage.getItem('theme') || '';
    THEMES.forEach(function(t) {
        var item = document.createElement('div');
        item.className = 'theme-picker-item' + (current === t.id ? ' active' : '');
        item.setAttribute('data-theme-id', t.id);
        item.innerHTML =
            '<div class="theme-swatch-preview" style="background:' + t.bg + '">' +
            '<span style="color:' + t.fg + '">Aa</span></div>' +
            '<span class="theme-name">' + t.name + '</span>';
        item.addEventListener('click', function() {
            applyTheme(t.id);
            grid.querySelectorAll('.theme-picker-item').forEach(function(el) { el.classList.remove('active'); });
            item.classList.add('active');
        });
        grid.appendChild(item);
    });

    picker.appendChild(grid);
    overlay.appendChild(picker);
    document.body.appendChild(overlay);

    overlay.addEventListener('click', function(e) { if (e.target === overlay) closeThemePicker(); });
    document.addEventListener('keydown', function handler(e) {
        if (e.key === 'Escape') { closeThemePicker(); document.removeEventListener('keydown', handler); }
    });
}

function closeThemePicker() {
    var o = document.querySelector('.theme-picker-overlay');
    if (o) o.remove();
}

(function() {
    var saved = localStorage.getItem('theme');
    if (saved) applyTheme(saved);
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

// ─── Sidebar: Ctrl+B toggle ──────────────────────────────────────────────

document.addEventListener('keydown', function(e) {
    if ((e.ctrlKey || e.metaKey) && e.key === 'b') {
        e.preventDefault();
        toggleSidebar();
    }
});

// ─── Sidebar: Filter, Refresh, Collapse ──────────────────────────────────

function sidebarCollapseAll() {
    document.querySelectorAll('.tree-children.loaded').forEach(function(c) {
        c.style.display = 'none';
        var chev = c.closest('.tree-item').querySelector('.tree-chevron');
        if (chev) chev.classList.remove('expanded');
    });
    treeSaveState();
}

function sidebarRefresh() {
    var tree = document.getElementById('sidebar-tree');
    if (tree) {
        tree.innerHTML = '<div class="loading">Loading...</div>';
        htmx.ajax('GET', '/tree', { target: '#sidebar-tree', swap: 'innerHTML' });
    }
}

// Live filter — hides tree items that don't match the search
(function() {
    document.addEventListener('input', function(e) {
        if (e.target.id !== 'sidebar-filter') return;
        var q = e.target.value.toLowerCase().trim();
        var items = document.querySelectorAll('#sidebar-tree .tree-item');
        if (!q) {
            items.forEach(function(li) { li.style.display = ''; });
            return;
        }
        items.forEach(function(li) {
            var text = li.querySelector('.tree-text');
            if (!text) { li.style.display = ''; return; }
            var match = text.textContent.toLowerCase().indexOf(q) !== -1;
            li.style.display = match ? '' : 'none';
            // If match, also show all parent tree-items
            if (match) {
                var parent = li.parentElement;
                while (parent) {
                    if (parent.classList && parent.classList.contains('tree-item')) parent.style.display = '';
                    if (parent.classList && parent.classList.contains('tree-children')) {
                        parent.style.display = '';
                        var chev = parent.closest('.tree-item');
                        if (chev) { chev.style.display = ''; var c = chev.querySelector('.tree-chevron'); if (c) c.classList.add('expanded'); }
                    }
                    parent = parent.parentElement;
                }
            }
        });
    });
})();

// ─── Sidebar: Keyboard Navigation ────────────────────────────────────────

document.addEventListener('keydown', function(e) {
    var sidebar = document.querySelector('.sidebar-tree');
    if (!sidebar || !sidebar.contains(document.activeElement)) return;
    var rows = Array.from(sidebar.querySelectorAll('.tree-row'));
    var visible = rows.filter(function(r) {
        var li = r.closest('.tree-item');
        if (!li) return true;
        var parent = li.parentElement;
        if (parent && parent.classList.contains('tree-children') && parent.style.display === 'none') return false;
        return r.offsetParent !== null;
    });
    var idx = visible.indexOf(document.activeElement);
    if (idx === -1) return;

    if (e.key === 'ArrowDown') {
        e.preventDefault();
        if (idx + 1 < visible.length) visible[idx + 1].focus();
    } else if (e.key === 'ArrowUp') {
        e.preventDefault();
        if (idx > 0) visible[idx - 1].focus();
    } else if (e.key === 'ArrowRight') {
        // Expand
        var children = document.activeElement.closest('.tree-item');
        if (children) {
            var ch = children.querySelector('.tree-children');
            if (ch && ch.style.display === 'none') {
                document.activeElement.click();
            }
        }
    } else if (e.key === 'ArrowLeft') {
        // Collapse
        var item = document.activeElement.closest('.tree-item');
        if (item) {
            var ch = item.querySelector('.tree-children');
            if (ch && ch.style.display !== 'none' && ch.classList.contains('loaded')) {
                document.activeElement.click();
            }
        }
    } else if (e.key === 'Enter') {
        e.preventDefault();
        document.activeElement.click();
    }
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

// ─── Toolbar connection info ──────────────────────────────────────────

(function() {
    function updateConnInfo() {
        fetch('/api/connection-info')
            .then(function(r) { return r.json(); })
            .then(function(data) {
                var el = document.getElementById('toolbar-db');
                if (el) el.textContent = data.db;
                var sb = document.getElementById('status-db');
                if (sb) sb.textContent = data.db;
            })
            .catch(function() {});
    }
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', updateConnInfo);
    } else {
        updateConnInfo();
    }
})();

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

)_JS_"; }
};

} // namespace getgresql::ssr
