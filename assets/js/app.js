// getgreSQL — minimal client-side JS

// Theme toggle
function toggleTheme() {
    const html = document.documentElement;
    const current = html.getAttribute('data-theme');
    const next = current === 'dark' ? 'light' : 'dark';
    html.setAttribute('data-theme', next);
    localStorage.setItem('theme', next);
}

// Restore saved theme
(function() {
    const saved = localStorage.getItem('theme');
    if (saved) document.documentElement.setAttribute('data-theme', saved);
})();

// Ctrl+Enter to submit query
document.addEventListener('keydown', function(e) {
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
        const editor = document.getElementById('sql-editor');
        if (editor && document.activeElement === editor) {
            const form = editor.closest('form');
            if (form) {
                // Trigger htmx submission
                htmx.trigger(form, 'submit');
            }
        }
    }
});

// Tab key inserts spaces in the SQL editor
document.addEventListener('keydown', function(e) {
    if (e.key === 'Tab') {
        const editor = document.getElementById('sql-editor');
        if (editor && document.activeElement === editor) {
            e.preventDefault();
            const start = editor.selectionStart;
            const end = editor.selectionEnd;
            editor.value = editor.value.substring(0, start) + '    ' + editor.value.substring(end);
            editor.selectionStart = editor.selectionEnd = start + 4;
        }
    }
});
