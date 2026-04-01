#pragma once
#include "ssr/engine.hpp"
#include <concepts>
#include <string_view>

namespace getgresql::ssr {

struct PageLayout {
    struct Props {
        std::string_view title;
        std::string_view active_nav;
    };

    template<std::invocable<Html&> F>
    static auto render(const Props& p, Html& h, F&& content_fn) -> void {
        render_head(p, h);
        h.raw("<div class=\"ide\">\n");
        render_toolbar(p, h);
        render_sidebar(h);
        h.raw("<div class=\"workspace\">\n  <div class=\"tab-bar\">\n    <div class=\"tab active\"><span class=\"tab-label\">");
        h.text(p.title);
        h.raw("</span></div>\n    <div class=\"tab-bar-end\"></div>\n  </div>\n  <div class=\"content\">\n");
        content_fn(h);
        h.raw("  </div>\n</div>\n");
        render_status_bar(h);
        h.raw("</div>\n</body>\n</html>\n");
    }

    template<std::invocable<Html&> F>
    static auto render_full(const Props& p, Html& h, F&& content_fn) -> void {
        render_head(p, h);
        h.raw("<div class=\"ide\">\n");
        render_toolbar(p, h);
        render_sidebar(h);
        h.raw("<div class=\"workspace\">\n  <div class=\"tab-bar\">\n    <div class=\"tab active\"><span class=\"tab-label\">");
        h.text(p.title);
        h.raw("</span></div>\n    <div class=\"tab-bar-end\"></div>\n  </div>\n");
        content_fn(h);
        h.raw("</div>\n");
        render_status_bar(h);
        h.raw("</div>\n</body>\n</html>\n");
    }

private:
    static auto render_head(const Props& p, Html& h) -> void {
        h.raw("<!DOCTYPE html>\n<html lang=\"en\" data-theme=\"dark\">\n<head>\n"
              "    <meta charset=\"utf-8\">\n"
              "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
              "    <title>");
        h.text(p.title);
        h.raw(" - getgreSQL</title>\n"
              "    <link rel=\"stylesheet\" href=\"/assets/css/components.css\">\n"
              "    <script src=\"/assets/js/components.js\" defer></script>\n"
              "</head>\n<body>\n");
    }

    static auto render_toolbar(const Props& p, Html& h) -> void {
        h.raw("<header class=\"toolbar\">\n"
              "  <span class=\"toolbar-brand\">getgreSQL<span class=\"version\">v0.1</span></span>\n"
              "  <span class=\"toolbar-sep\"></span>\n"
              "  <nav class=\"toolbar-nav\">\n");
        constexpr struct { const char* href; const char* icon; const char* label; } navs[] = {
            {"/", "&#9635;", "Dashboard"}, {"/query", "&#9654;", "Query"},
            {"/monitor", "&#9673;", "Monitor"},
        };
        for (auto& n : navs) {
            h.raw("    <a href=\"").raw(n.href).raw("\" class=\"toolbar-btn");
            if (p.active_nav == n.label) h.raw(" active");
            h.raw("\" data-spa><span class=\"icon\">").raw(n.icon).raw("</span> ").raw(n.label).raw("</a>");
        }
        h.raw("  </nav>\n  <span class=\"toolbar-spacer\"></span>\n"
              "  <a href=\"/connections\" class=\"toolbar-conn\" title=\"Manage Connections\">"
              "<span class=\"conn-dot\"></span>"
              "<span class=\"conn-label\" id=\"toolbar-db\">Connected</span></a>\n"
              "  <div class=\"toolbar-actions\">\n"
              "    <button onclick=\"openCommandPalette()\" class=\"toolbar-icon-btn\" title=\"Command Palette (Ctrl+K)\">&#8984;</button>\n"
              "    <button onclick=\"toggleSidebar()\" class=\"toolbar-icon-btn\" title=\"Toggle Sidebar\">&#9776;</button>\n"
              "    <button onclick=\"toggleTheme()\" class=\"toolbar-icon-btn\" title=\"Theme\">&#9680;</button>\n"
              "  </div>\n</header>\n");
    }

    static auto render_sidebar(Html& h) -> void {
        h.raw("<aside class=\"sidebar\">\n"
              "  <div class=\"sidebar-header\">\n"
              "    <span class=\"sidebar-title\">Explorer</span>\n"
              "    <div class=\"sidebar-actions\">\n"
              "      <button class=\"sidebar-icon\" onclick=\"sidebarRefresh()\" title=\"Refresh\">&#8635;</button>\n"
              "      <button class=\"sidebar-icon\" onclick=\"sidebarCollapseAll()\" title=\"Collapse All\">&#8722;</button>\n"
              "      <button class=\"sidebar-icon\" onclick=\"toggleSidebar()\" title=\"Hide Sidebar (Ctrl+B)\">&#10005;</button>\n"
              "    </div>\n  </div>\n"
              "  <div class=\"sidebar-search\">\n"
              "    <input type=\"text\" class=\"sidebar-search-input\" id=\"sidebar-filter\" "
              "placeholder=\"Filter... (type to search)\" autocomplete=\"off\">\n"
              "  </div>\n"
              "  <div class=\"sidebar-tree\" id=\"sidebar-tree\" hx-get=\"/tree\" hx-trigger=\"load\" hx-swap=\"innerHTML\">\n"
              "    <div class=\"loading\">Loading...</div>\n"
              "  </div>\n  <div class=\"resize-handle\"></div>\n</aside>\n");
    }

    static auto render_status_bar(Html& h) -> void {
        h.raw("<footer class=\"status-bar\">\n"
              "  <span class=\"status-item\"><span class=\"conn-dot\"></span> <span id=\"status-db\">Connected</span></span>\n"
              "  <span class=\"status-sep\"></span>\n"
              "  <span class=\"status-item\" id=\"status-info\"></span>\n"
              "  <span class=\"status-spacer\"></span>\n"
              "  <span class=\"status-item\"><a href=\"/connections\" style=\"color:inherit;text-decoration:none\">&#128268; Connections</a></span>\n"
              "  <span class=\"status-item\"><kbd>Ctrl+B</kbd> Sidebar</span>\n"
              "  <span class=\"status-item\"><kbd>Ctrl+K</kbd> Command Palette</span>\n"
              "</footer>\n");
    }
};

} // namespace getgresql::ssr
