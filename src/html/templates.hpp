#pragma once

#include "html/escape.hpp"

#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace getgresql::html {

// ─── Page head (shared by all pages) ─────────────────────────────────

inline auto page_head(std::string_view title) -> std::string {
    return std::format(
        "<!DOCTYPE html>\n"
        "<html lang=\"en\" data-theme=\"dark\">\n"
        "<head>\n"
        "    <meta charset=\"utf-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "    <title>{} - getgreSQL</title>\n"
        "    <link rel=\"stylesheet\" href=\"/assets/css/style.css\">\n"
        "    <script src=\"/assets/js/htmx.min.js\" defer></script>\n"
        "    <script src=\"/assets/js/app.js\" defer></script>\n"
        "    <script src=\"/assets/js/editor.js\" defer></script>\n"
        "</head>\n"
        "<body>\n", title);
}

inline constexpr auto PAGE_TAIL = "\n</body>\n</html>\n";

// ─── Toolbar ─────────────────────────────────────────────────────────

inline auto toolbar(std::string_view active_page = "") -> std::string {
    auto nav_btn = [&](std::string_view href, std::string_view icon,
                       std::string_view label) -> std::string {
        auto cls = (active_page == label) ? "toolbar-btn active" : "toolbar-btn";
        return std::format(
            "<a href=\"{}\" class=\"{}\" data-spa>"
            "<span class=\"icon\">{}</span> {}</a>",
            href, cls, icon, label
        );
    };

    std::string out;
    out += "<header class=\"toolbar\">\n";
    out += "  <span class=\"toolbar-brand\">getgreSQL<span class=\"version\">v0.1</span></span>\n";
    out += "  <span class=\"toolbar-sep\"></span>\n";
    out += "  <nav class=\"toolbar-nav\">\n";
    out += "    " + nav_btn("/", "&#9635;", "Dashboard");
    out += "    " + nav_btn("/query", "&#9654;", "Query");
    out += "    " + nav_btn("/explain", "&#9881;", "Explain");
    out += "    " + nav_btn("/monitor", "&#9673;", "Monitor");
    out += "  </nav>\n";
    out += "  <span class=\"toolbar-spacer\"></span>\n";
    out += "  <div class=\"toolbar-actions\">\n";
    out += "    <button onclick=\"openCommandPalette()\" class=\"toolbar-icon-btn\" title=\"Command Palette (Ctrl+K)\">&#8984;</button>\n";
    out += "    <button onclick=\"toggleSidebar()\" class=\"toolbar-icon-btn\" title=\"Toggle Sidebar\">&#9776;</button>\n";
    out += "    <button onclick=\"toggleTheme()\" class=\"toolbar-icon-btn\" title=\"Toggle Theme\">&#9680;</button>\n";
    out += "  </div>\n";
    out += "</header>\n";
    return out;
}

// ─── Object explorer sidebar ─────────────────────────────────────────

inline auto sidebar() -> std::string {
    std::string out;
    out += "<aside class=\"sidebar\">\n";
    out += "  <div class=\"sidebar-header\">\n";
    out += "    <span class=\"sidebar-title\">Explorer</span>\n";
    out += "    <div class=\"sidebar-actions\">\n";
    out += "      <button class=\"sidebar-icon\" onclick=\"document.getElementById('sidebar-tree').querySelectorAll('.tree-children.loaded').forEach(c => { c.style.display='none'; c.closest('.tree-item').querySelector('.tree-chevron').classList.remove('expanded'); })\" title=\"Collapse All\">&#8722;</button>\n";
    out += "    </div>\n";
    out += "  </div>\n";
    out += "  <div class=\"sidebar-tree\" id=\"sidebar-tree\" hx-get=\"/tree\" hx-trigger=\"load\" hx-swap=\"innerHTML\">\n";
    out += "    <div class=\"loading\">Loading...</div>\n";
    out += "  </div>\n";
    out += "  <div class=\"resize-handle\"></div>\n";
    out += "</aside>\n";
    return out;
}

// ─── Status bar ──────────────────────────────────────────────────────

inline auto status_bar() -> std::string {
    std::string out;
    out += "<footer class=\"status-bar\">\n";
    out += "  <span class=\"status-item\"><span class=\"conn-dot\"></span> Connected</span>\n";
    out += "  <span class=\"status-sep\"></span>\n";
    out += "  <span class=\"status-item\" id=\"status-info\"></span>\n";
    out += "  <span class=\"status-spacer\"></span>\n";
    out += "  <span class=\"status-item\"><kbd>Ctrl+K</kbd> Command Palette</span>\n";
    out += "</footer>\n";
    return out;
}

// ─── IDE page wrapper ────────────────────────────────────────────────

inline auto ide_page(std::string_view title, std::string_view active_nav,
                     std::string content) -> std::string {
    std::string out = page_head(title);
    out += "<div class=\"ide\">\n";
    out += toolbar(active_nav);
    out += sidebar();
    out += "<div class=\"workspace\">\n";
    out += "  <div class=\"tab-bar\">\n";
    out += std::format("    <div class=\"tab active\"><span class=\"tab-label\">{}</span></div>\n", title);
    out += "    <div class=\"tab-bar-end\"></div>\n";
    out += "  </div>\n";
    out += "  <div class=\"content\">\n";
    out += std::move(content);
    out += "  </div>\n";
    out += "</div>\n";
    out += status_bar();
    out += "</div>\n";
    out += PAGE_TAIL;
    return out;
}

// Query page uses full-height layout (no padding/scrollbar on content)
inline auto ide_page_full(std::string_view title, std::string_view active_nav,
                          std::string content) -> std::string {
    std::string out = page_head(title);
    out += "<div class=\"ide\">\n";
    out += toolbar(active_nav);
    out += sidebar();
    out += "<div class=\"workspace\">\n";
    out += "  <div class=\"tab-bar\">\n";
    out += std::format("    <div class=\"tab active\"><span class=\"tab-label\">{}</span></div>\n", title);
    out += "    <div class=\"tab-bar-end\"></div>\n";
    out += "  </div>\n";
    out += std::move(content);
    out += "</div>\n";
    out += status_bar();
    out += "</div>\n";
    out += PAGE_TAIL;
    return out;
}

// Backward-compatible page() — now wraps ide_page
inline auto page(std::string_view title, std::string_view active_nav,
                  std::string content) -> std::string {
    return ide_page(title, active_nav, std::move(content));
}

// htmx partial -- just the content, no page shell
inline auto partial(std::string content) -> std::string {
    return content;
}

// ─── Tree node helpers ───────────────────────────────────────────────

inline auto tree_node_expandable(std::string_view icon_class, std::string_view label,
                                  std::string_view children_url, int depth,
                                  std::string_view badge_text = "") -> std::string {
    std::string out;
    out += "<li class=\"tree-item\">\n";
    out += std::format(
        "  <div class=\"tree-row\" style=\"--tree-depth:{}\" onclick=\"treeToggle(this)\">\n"
        "    <span class=\"tree-chevron\">&#9656;</span>\n"
        "    <span class=\"tree-icon {}\">&#9679;</span>\n"
        "    <span class=\"tree-text\">{}</span>\n",
        depth, icon_class, escape(label)
    );
    if (!badge_text.empty()) {
        out += std::format("    <span class=\"tree-badge\">{}</span>\n", escape(badge_text));
    }
    out += "  </div>\n";
    out += std::format(
        "  <ul class=\"tree-children\" hx-get=\"{}\" hx-trigger=\"click from:closest .tree-item > .tree-row once\" hx-swap=\"innerHTML\">\n"
        "  </ul>\n",
        children_url
    );
    out += "</li>\n";
    return out;
}

inline auto tree_node_leaf(std::string_view icon_class, std::string_view label,
                            std::string_view href, int depth,
                            std::string_view extra = "") -> std::string {
    return std::format(
        "<li class=\"tree-item\">"
        "<a class=\"tree-row\" style=\"--tree-depth:{}\" href=\"{}\" data-spa>"
        "<span class=\"tree-chevron empty\"></span>"
        "<span class=\"tree-icon {}\">&#9679;</span>"
        "<span class=\"tree-text\">{}</span>"
        "{}"
        "</a></li>\n",
        depth, href, icon_class, escape(label), extra
    );
}

inline auto tree_group(std::string_view label, int depth) -> std::string {
    return std::format(
        "<li class=\"tree-group-label\" style=\"--tree-depth:{}\">{}</li>\n",
        depth, escape(label)
    );
}

inline auto tree_separator() -> std::string {
    return "<li class=\"tree-separator\"></li>\n";
}

// ─── Table rendering ─────────────────────────────────────────────────

struct TableColumn {
    std::string header;
    std::string css_class;  // optional
    bool sortable = false;
};

inline auto table_begin(const std::vector<TableColumn>& columns,
                         std::string_view id = "",
                         std::string_view extra_attrs = "") -> std::string {
    std::string out = "<div class=\"table-wrapper scrollable\"><table";
    if (!id.empty()) out += std::format(" id=\"{}\"", id);
    if (!extra_attrs.empty()) out += std::format(" {}", extra_attrs);
    out += ">";
    out += "<thead><tr>";
    for (auto& col : columns) {
        std::string cls;
        if (!col.css_class.empty() && col.sortable) {
            cls = col.css_class + " sortable";
        } else if (!col.css_class.empty()) {
            cls = col.css_class;
        } else if (col.sortable) {
            cls = "sortable";
        }

        if (cls.empty()) {
            out += std::format("<th>{}</th>", escape(col.header));
        } else {
            out += std::format("<th class=\"{}\">{}</th>", cls, escape(col.header));
        }
    }
    out += "</tr></thead><tbody>";
    return out;
}

inline auto table_end() -> std::string {
    return "</tbody></table></div>";
}

inline auto table_row(const std::vector<std::string>& cells,
                       std::string_view extra_attrs = "") -> std::string {
    std::string out = extra_attrs.empty()
        ? "<tr>"
        : std::format("<tr {}>", extra_attrs);
    for (auto& cell : cells) {
        out += std::format("<td>{}</td>", cell);
    }
    out += "</tr>";
    return out;
}

// ─── Common UI components ────────────────────────────────────────────

inline auto badge(std::string_view text, std::string_view variant = "default") -> std::string {
    return std::format("<span class=\"badge badge-{}\">{}</span>", variant, escape(text));
}

inline auto stat_card(std::string_view label, std::string_view value,
                       std::string_view variant = "") -> std::string {
    auto cls = variant.empty() ? std::string("stat-card") : std::format("stat-card stat-{}", variant);
    return std::format(
        "<div class=\"{}\">"
        "<div class=\"stat-value\">{}</div>"
        "<div class=\"stat-label\">{}</div>"
        "</div>\n",
        cls, escape(value), escape(label)
    );
}

inline auto alert(std::string_view message, std::string_view type = "info") -> std::string {
    return std::format("<div class=\"alert alert-{}\">{}</div>", type, escape(message));
}

inline auto breadcrumbs(const std::vector<std::pair<std::string, std::string>>& items) -> std::string {
    std::string out = "<nav class=\"breadcrumbs\">";
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i > 0) out += "<span class=\"bc-sep\">&#8250;</span>";
        if (i == items.size() - 1 || items[i].second.empty()) {
            out += std::format("<span class=\"bc-current\">{}</span>", escape(items[i].first));
        } else {
            out += std::format("<a href=\"{}\" class=\"bc-link\">{}</a>",
                               items[i].second, escape(items[i].first));
        }
    }
    out += "</nav>";
    return out;
}

inline auto progress_bar(double percent, std::string_view variant = "") -> std::string {
    auto cls = variant.empty() ? std::string("progress-bar") : std::format("progress-bar progress-{}", variant);
    return std::format(
        "<div class=\"progress\"><div class=\"{}\" style=\"width: {:.1f}%\"></div></div>",
        cls, percent
    );
}

inline auto search_input(std::string_view target, std::string_view url,
                          std::string_view placeholder = "Search...") -> std::string {
    return std::format(
        "<div class=\"search-box\">"
        "<input type=\"search\" name=\"q\" placeholder=\"{}\" "
        "hx-get=\"{}\" hx-trigger=\"input changed delay:300ms, search\" "
        "hx-target=\"#{}\" class=\"search-input\">"
        "</div>",
        placeholder, url, target
    );
}

} // namespace getgresql::html
