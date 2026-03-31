#pragma once

#include "html/escape.hpp"

#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace getgresql::html {

// ─── Compile-time page layout ───────────────────────────────────────

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
        "</head>\n"
        "<body>\n", title);
}

inline constexpr auto PAGE_TAIL = "\n</body>\n</html>\n";

// ─── Navigation sidebar ─────────────────────────────────────────────

inline auto nav_sidebar(std::string_view active_page = "") -> std::string {
    auto nav_item = [&](std::string_view href, std::string_view icon,
                        std::string_view label) -> std::string {
        auto cls = (active_page == label) ? "nav-item active" : "nav-item";
        return std::format(
            "<a href=\"{}\" class=\"{}\">{} <span>{}</span></a>\n",
            href, cls, icon, label
        );
    };

    std::string out;
    out += "<nav class=\"sidebar\">\n";
    out += "  <div class=\"sidebar-brand\"><h1 class=\"logo\">getgreSQL</h1></div>\n";
    out += "  <div class=\"sidebar-nav\">\n";
    out += "    " + nav_item("/", "&#8862;", "Dashboard");
    out += "    " + nav_item("/databases", "&#9921;", "Databases");
    out += "    " + nav_item("/query", "&#8984;", "Query");
    out += "    " + nav_item("/monitor", "&#9673;", "Monitor");
    out += "    " + nav_item("/monitor/locks", "&#9919;", "Locks");
    out += "  </div>\n";
    out += "  <div class=\"sidebar-footer\">\n";
    out += "    <button onclick=\"toggleTheme()\" class=\"theme-toggle\" title=\"Toggle theme\">\n";
    out += "      <span class=\"theme-icon\">&#9680;</span>\n";
    out += "    </button>\n";
    out += "  </div>\n";
    out += "</nav>\n";
    return out;
}

// ─── Page wrapper ───────────────────────────────────────────────────

inline auto page(std::string_view title, std::string_view active_nav,
                  std::string content) -> std::string {
    return page_head(title) +
           nav_sidebar(active_nav) +
           std::format("<main class=\"content\"><div class=\"content-header\"><h2>{}</h2></div>", title) +
           std::move(content) +
           "</main>" +
           PAGE_TAIL;
}

// htmx partial -- just the content, no page shell
inline auto partial(std::string content) -> std::string {
    return content;
}

// ─── Table rendering ────────────────────────────────────────────────

struct TableColumn {
    std::string header;
    std::string css_class;  // optional
};

inline auto table_begin(const std::vector<TableColumn>& columns,
                         std::string_view id = "",
                         std::string_view extra_attrs = "") -> std::string {
    std::string out = "<div class=\"table-wrapper\"><table";
    if (!id.empty()) out += std::format(" id=\"{}\"", id);
    if (!extra_attrs.empty()) out += std::format(" {}", extra_attrs);
    out += ">";
    out += "<thead><tr>";
    for (auto& col : columns) {
        if (col.css_class.empty()) {
            out += std::format("<th>{}</th>", escape(col.header));
        } else {
            out += std::format("<th class=\"{}\">{}</th>", col.css_class, escape(col.header));
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

// ─── Common UI components ───────────────────────────────────────────

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
        if (i > 0) out += "<span class=\"bc-sep\">&rsaquo;</span>";
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

} // namespace getgresql::html
