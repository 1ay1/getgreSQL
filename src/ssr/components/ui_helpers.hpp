#pragma once
#include "ssr/engine.hpp"
#include <string_view>

namespace getgresql::ssr {

struct UIHelpers {
    static constexpr auto js() -> std::string_view { return R"_JS_(
// ─── Table Column Sorting ────────────────────────────────────────────────

document.addEventListener('click', function(e) {
    var th = e.target.closest('th.sortable');
    if (!th) return;

    var table = th.closest('table');
    if (!table) return;

    // DataView tables have their own sort handler in dataview.js
    if (table.classList.contains('dv-table')) return;

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
    { label: 'Connections', icon: '\u{1F50C}', href: '/connections' },
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

)_JS_"; }
};

} // namespace getgresql::ssr
