#pragma once

// ─── getgreSQL SSR Components ────────────────────────────────────────
// Pure functions: (Props, Html&) → void
// Each component satisfies the Component concept.
// Props are value types. The compiler verifies every call site.
// No side effects — rendering is a pure transformation from data to bytes.

#include "ssr/engine.hpp"

#include <format>
#include <string>
#include <string_view>
#include <vector>
#include <span>

namespace getgresql::ssr {

// ─── Badge ───────────────────────────────────────────────────────────

struct Badge {
    struct Props {
        std::string_view text;
        std::string_view variant = "default";
    };
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<span class=\"badge badge-").raw(p.variant).raw("\">").text(p.text).raw("</span>");
    }
};

// ─── Stat Card ───────────────────────────────────────────────────────

struct StatCard {
    struct Props {
        std::string_view label;
        std::string_view value;
        std::string_view variant = "";
    };
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"stat-card");
        if (!p.variant.empty()) h.raw(" stat-").raw(p.variant);
        h.raw("\"><div class=\"stat-value\">").text(p.value).raw("</div>");
        h.raw("<div class=\"stat-label\">").text(p.label).raw("</div></div>\n");
    }
};

// ─── Alert ───────────────────────────────────────────────────────────

struct Alert {
    struct Props {
        std::string_view message;
        std::string_view type = "info";
    };
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"alert alert-").raw(p.type).raw("\">").text(p.message).raw("</div>");
    }
};

// ─── Health Card ─────────────────────────────────────────────────────

struct HealthCard {
    struct Props {
        std::string_view name;
        std::string_view status;
        std::string_view value;
        std::string_view detail;
    };
    static auto render(const Props& p, Html& h) -> void {
        auto v = (p.status == "ok") ? "success" : (p.status == "warning") ? "warning" : "danger";
        auto icon = (p.status == "ok") ? "&#10003;" : (p.status == "warning") ? "&#9888;" : "&#10007;";
        h.raw("<div class=\"health-card health-").raw(v).raw("\">");
        h.raw("<div class=\"health-icon\">").raw(icon).raw("</div>");
        h.raw("<div class=\"health-info\">");
        h.raw("<div class=\"health-name\">").text(p.name).raw("</div>");
        h.raw("<div class=\"health-value\">").text(p.value).raw("</div>");
        h.raw("<div class=\"health-detail\">").text(p.detail).raw("</div>");
        h.raw("</div></div>");
    }
};

// ─── Size Bar ────────────────────────────────────────────────────────

struct SizeBar {
    struct Props { double percent; };
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"size-bar\"><div class=\"size-bar-fill\" style=\"width:")
         .raw(std::format("{:.0f}", p.percent)).raw("%\"></div></div>");
    }
};

// ─── Progress Bar ────────────────────────────────────────────────────

struct ProgressBar {
    struct Props { double percent; std::string_view variant = ""; };
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"progress\"><div class=\"progress-bar");
        if (!p.variant.empty()) h.raw(" progress-").raw(p.variant);
        h.raw("\" style=\"width:").raw(std::format("{:.1f}", p.percent)).raw("%\"></div></div>");
    }
};

// ─── Breadcrumbs ─────────────────────────────────────────────────────

struct Crumb { std::string label; std::string href; };

struct Breadcrumbs {
    static auto render(std::initializer_list<Crumb> il, Html& h) -> void {
        std::vector<Crumb> items(il); render(items, h);
    }
    static auto render(const std::vector<Crumb>& items, Html& h) -> void {
        h.raw("<nav class=\"breadcrumbs\">");
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i > 0) h.raw("<span class=\"bc-sep\">&#8250;</span>");
            if (i == items.size() - 1 || items[i].href.empty()) {
                h.raw("<span class=\"bc-current\">").text(items[i].label).raw("</span>");
            } else {
                h.raw("<a href=\"").text(items[i].href).raw("\" class=\"bc-link\">").text(items[i].label).raw("</a>");
            }
        }
        h.raw("</nav>");
    }
};

// ─── Table ───────────────────────────────────────────────────────────
// Type-safe table builder. Usage:
//   Table::begin(h, cols, "my-id");
//   Table::row(h, cells);
//   Table::end(h);

struct Col { std::string_view header; std::string_view cls = ""; bool sortable = false; };

struct Table {
    static auto begin(Html& h, std::initializer_list<Col> columns, std::string_view id = "") -> void {
        begin_impl(h, columns, id);
    }
    static auto begin(Html& h, const std::vector<Col>& columns, std::string_view id = "") -> void {
        begin_impl(h, columns, id);
    }
    template<typename Cols>
    static auto begin_impl(Html& h, const Cols& columns, std::string_view id) -> void {
        h.raw("<div class=\"table-wrapper scrollable\"><table");
        if (!id.empty()) h.raw(" id=\"").text(id).raw("\"");
        h.raw("><thead><tr>");
        for (auto& c : columns) {
            h.raw("<th");
            if (!c.cls.empty() || c.sortable) {
                h.raw(" class=\"");
                if (!c.cls.empty()) h.raw(c.cls);
                if (c.sortable) { if (!c.cls.empty()) h.raw(' '); h.raw("sortable"); }
                h.raw("\"");
            }
            h.raw(">").text(c.header).raw("</th>");
        }
        h.raw("</tr></thead><tbody>");
    }

    static auto row(Html& h, std::initializer_list<std::string> cells, std::string_view attrs = "") -> void {
        if (attrs.empty()) h.raw("<tr>"); else h.raw("<tr ").raw(attrs).raw(">");
        for (auto& cell : cells) h.raw("<td>").raw(cell).raw("</td>");
        h.raw("</tr>");
    }

    static auto row(Html& h, const std::vector<std::string>& cells, std::string_view attrs = "") -> void {
        if (attrs.empty()) h.raw("<tr>"); else h.raw("<tr ").raw(attrs).raw(">");
        for (auto& cell : cells) h.raw("<td>").raw(cell).raw("</td>");
        h.raw("</tr>");
    }

    static auto end(Html& h) -> void {
        h.raw("</tbody></table></div>");
    }
};

// ─── DataView ───────────────────────────────────────────────────────
// Unified data grid component used by both query results and table browse.
// Renders a consistent HTML structure that dataview.js enhances with
// client-side sorting, filtering, copy, export, column resize, and
// keyboard navigation.
//
// Usage (read-only, e.g. query results):
//   DataView::begin(h, {.row_count=42, .exec_ms=15});
//   DataView::columns(h, cols);
//   DataView::row(h, cells);
//   DataView::end(h);
//
// Usage (editable, e.g. table browse):
//   DataView::begin(h, {.row_count=42, .editable=true, .db="mydb",
//       .schema="public", .table="users", .page=1, .limit=50,
//       .total_rows=1200, .base_url="/db/mydb/schema/public/table/users/browse"});
//   DataView::columns(h, cols);
//   DataView::editable_row(h, ctid, row_num, cells);
//   DataView::end(h);

struct DataView {
    struct Options {
        int row_count = 0;
        int exec_ms = 0;
        std::string_view command_tag;  // e.g. "INSERT" for non-SELECT

        // Editable context (table browse)
        bool editable = false;
        std::string_view db;
        std::string_view schema;
        std::string_view table;

        // Pagination (only for table browse)
        int page = 0;       // 0 = no pagination
        int limit = 0;
        long long total_rows = 0;
        std::string_view base_url;
    };

    struct DCol {
        std::string_view name;
        std::string_view type_hint;  // "numeric", "text", "boolean", "json", "date", ""
        bool sortable = true;
    };

    struct Cell {
        std::string value;
        bool is_null = false;
        bool is_raw = false;  // if true, value is already HTML-escaped
    };

    // ── begin: renders info bar + toolbar + opens table ──
    static auto begin(Html& h, const Options& o) -> void {
        h.raw("<div class=\"data-view\"");
        if (o.editable) {
            h.raw(" data-editable=\"true\" data-db=\"").text(o.db)
             .raw("\" data-schema=\"").text(o.schema)
             .raw("\" data-table=\"").text(o.table).raw("\"");
        }
        h.raw(">\n");

        // Info bar
        if (o.command_tag.empty()) {
            h.raw("<div class=\"query-info\"><span class=\"rows-badge\">")
             .raw(std::to_string(o.row_count)).raw(" rows</span>");
            if (o.exec_ms > 0) {
                h.raw(" <span class=\"time-badge\">").raw(std::to_string(o.exec_ms)).raw(" ms</span>");
            }
            if (o.page > 0) {
                auto total_pages = std::max(1LL, (o.total_rows + o.limit - 1) / o.limit);
                h.raw(" <span class=\"time-badge\">~").raw(std::to_string(o.total_rows))
                 .raw(" total &middot; page ").raw(std::to_string(o.page))
                 .raw(" of ~").raw(std::to_string(total_pages)).raw("</span>");
            }
            h.raw("</div>\n");
        } else {
            h.raw("<div class=\"query-info\"><span class=\"rows-badge\">").text(o.command_tag)
             .raw(" &mdash; ").raw(std::to_string(o.row_count)).raw(" affected</span>");
            if (o.exec_ms > 0) {
                h.raw(" <span class=\"time-badge\">").raw(std::to_string(o.exec_ms)).raw(" ms</span>");
            }
            h.raw("</div>\n");
        }

        // Toolbar
        h.raw("<div class=\"dv-toolbar\">");
        // Filter input
        h.raw("<input type=\"search\" class=\"dv-filter-input\" placeholder=\"Filter rows...\" title=\"Type to filter visible rows\">");

        if (o.page > 0) {
            // Pagination
            auto total_pages = std::max(1LL, (o.total_rows + o.limit - 1) / o.limit);
            h.raw("<div class=\"btn-group dv-pagination\">");
            if (o.page > 1) {
                h.raw("<button class=\"btn btn-sm\" hx-get=\"").raw(o.base_url)
                 .raw("?page=1&limit=").raw(std::to_string(o.limit))
                 .raw("\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">&laquo;</button>");
                h.raw("<button class=\"btn btn-sm\" hx-get=\"").raw(o.base_url)
                 .raw("?page=").raw(std::to_string(o.page - 1)).raw("&limit=").raw(std::to_string(o.limit))
                 .raw("\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">&lsaquo; Prev</button>");
            }
            if (o.page < static_cast<int>(total_pages)) {
                h.raw("<button class=\"btn btn-sm\" hx-get=\"").raw(o.base_url)
                 .raw("?page=").raw(std::to_string(o.page + 1)).raw("&limit=").raw(std::to_string(o.limit))
                 .raw("\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">Next &rsaquo;</button>");
            }
            h.raw("</div>");

            // Page size
            h.raw("<div class=\"btn-group dv-page-size\">");
            for (auto sz : {25, 50, 100, 250}) {
                auto cls = (sz == o.limit) ? "btn btn-sm btn-primary" : "btn btn-sm";
                h.raw("<button class=\"").raw(cls).raw("\" hx-get=\"").raw(o.base_url)
                 .raw("?page=1&limit=").raw(std::to_string(sz))
                 .raw("\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">").raw(std::to_string(sz)).raw("</button>");
            }
            h.raw("</div>");
        }

        // Export buttons
        h.raw("<div class=\"btn-group dv-export\">"
              "<button class=\"btn btn-sm btn-ghost\" data-dv-export=\"csv\" title=\"Export CSV\">CSV</button>"
              "<button class=\"btn btn-sm btn-ghost\" data-dv-export=\"json\" title=\"Export JSON\">JSON</button>"
              "<button class=\"btn btn-sm btn-ghost\" data-dv-export=\"sql\" title=\"Copy as INSERT\">SQL</button>"
              "</div>");

        // Copy button
        h.raw("<button class=\"btn btn-sm btn-ghost\" data-dv-action=\"copy\" title=\"Copy selection (Ctrl+C)\">Copy</button>");

        if (o.editable) {
            // CSV export (server-side)
            h.raw("<a href=\"/db/").raw(o.db).raw("/schema/").raw(o.schema).raw("/table/").raw(o.table)
             .raw("/export\" class=\"btn btn-sm btn-ghost\" title=\"Download full CSV\">&#8615; Full CSV</a>");
            // Insert button
            h.raw("<button class=\"btn btn-sm btn-success\" data-dv-action=\"toggle-insert\">+ Insert</button>");
        }

        h.raw("</div>\n"); // end toolbar

        // Insert form (hidden, editable only)
        if (o.editable) {
            h.raw("<div class=\"dv-insert-form\" style=\"display:none\">"
                  "<form hx-post=\"/db/").raw(o.db).raw("/schema/").raw(o.schema)
             .raw("/table/").raw(o.table).raw("/insert-row\" hx-target=\"#tab-content\" hx-swap=\"innerHTML\">"
                  "<div class=\"insert-form-grid\" id=\"dv-insert-fields\"></div>"
                  "<div class=\"insert-actions\">"
                  "<button type=\"submit\" class=\"btn btn-sm btn-primary\">Insert</button>"
                  "<button type=\"button\" class=\"btn btn-sm\" data-dv-action=\"toggle-insert\">Cancel</button>"
                  "</div></form></div>\n");
        }
    }

    // ── columns: renders thead ──
    static auto columns(Html& h, const std::vector<DCol>& cols, bool with_row_num = false, bool with_actions = false) -> void {
        h.raw("<div class=\"table-wrapper scrollable\"><table class=\"dv-table\"><thead><tr>");
        if (with_row_num) h.raw("<th class=\"row-num-header\">#</th>");
        for (auto& c : cols) {
            h.raw("<th class=\"sortable\" data-col=\"").text(c.name).raw("\"");
            if (!c.type_hint.empty()) h.raw(" data-type=\"").raw(c.type_hint).raw("\"");
            h.raw("><span class=\"dv-th-text\">").text(c.name).raw("</span>"
                  "<span class=\"dv-resize-handle\"></span></th>");
        }
        if (with_actions) h.raw("<th class=\"dv-actions-header\"></th>");
        h.raw("</tr></thead><tbody>");
    }

    // ── row: renders a read-only row ──
    static auto row(Html& h, const std::vector<Cell>& cells) -> void {
        h.raw("<tr>");
        for (auto& cell : cells) {
            h.raw("<td>");
            if (cell.is_null) {
                h.raw("<span class=\"null-value\">NULL</span>");
            } else if (cell.is_raw) {
                h.raw("<span class=\"dv-cell\">").raw(cell.value).raw("</span>");
            } else if (cell.value.size() > 200) {
                h.raw("<span class=\"dv-cell dv-cell-long\">");
                auto tmp = Html::with_capacity(220);
                tmp.text(std::string_view(cell.value).substr(0, 200));
                h.raw(tmp.view()).raw("&hellip;</span>");
            } else {
                h.raw("<span class=\"dv-cell\">");
                h.text(cell.value);
                h.raw("</span>");
            }
            h.raw("</td>");
        }
        h.raw("</tr>\n");
    }

    // ── editable_row: renders a row with editing support ──
    static auto editable_row(Html& h, std::string_view ctid, int row_num,
                             const std::vector<DCol>& cols,
                             const std::vector<Cell>& cells,
                             std::string_view db, std::string_view schema, std::string_view table) -> void {
        h.raw("<tr data-ctid=\"").text(ctid).raw("\">");
        h.raw("<td class=\"row-num\">").raw(std::to_string(row_num)).raw("</td>");

        for (std::size_t i = 0; i < cells.size(); ++i) {
            auto& cell = cells[i];
            auto col_name = (i < cols.size()) ? cols[i].name : std::string_view("");
            h.raw("<td>");
            if (cell.is_null) {
                h.raw("<span class=\"null-value editable-cell\" data-col=\"").text(col_name)
                 .raw("\" data-ctid=\"").text(ctid).raw("\">NULL</span>");
            } else if (cell.value.size() > 80) {
                h.raw("<span class=\"editable-cell dv-cell-long\" data-col=\"").text(col_name)
                 .raw("\" data-ctid=\"").text(ctid).raw("\" data-full=\"");
                h.text(cell.value);
                h.raw("\">");
                auto tmp = Html::with_capacity(80);
                tmp.text(std::string_view(cell.value).substr(0, 60));
                h.raw(tmp.view()).raw("&hellip;</span>");
            } else {
                h.raw("<span class=\"editable-cell\" data-col=\"").text(col_name)
                 .raw("\" data-ctid=\"").text(ctid).raw("\">");
                h.text(cell.value);
                h.raw("</span>");
            }
            h.raw("</td>");
        }

        // Delete button
        h.raw("<td><button class=\"btn btn-sm btn-danger\" "
              "hx-post=\"/db/").raw(db).raw("/schema/").raw(schema).raw("/table/").raw(table)
         .raw("/delete-row\" hx-vals='{\"ctid\":\"");
        h.text(ctid);
        h.raw("\"}' hx-target=\"#tab-content\" hx-swap=\"innerHTML\" "
              "hx-confirm=\"Delete this row?\">&#10005;</button></td>");
        h.raw("</tr>\n");
    }

    // ── end: closes table + container ──
    static auto end(Html& h) -> void {
        h.raw("</tbody></table></div>\n"); // close table-wrapper
        h.raw("</div>\n"); // close data-view
    }
};

// ─── Search Input ────────────────────────────────────────────────────

struct SearchInput {
    struct Props {
        std::string_view target;
        std::string_view url;
        std::string_view placeholder = "Search...";
    };
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"search-box\"><input type=\"search\" name=\"q\" placeholder=\"")
         .text(p.placeholder)
         .raw("\" hx-get=\"").raw(p.url)
         .raw("\" hx-trigger=\"input changed delay:300ms, search\" hx-target=\"#")
         .raw(p.target).raw("\" class=\"search-input\"></div>");
    }
};

// ─── Section Tabs ────────────────────────────────────────────────────

struct SectionTab { std::string label; std::string url; bool active = false; };

struct SectionTabs {
    static auto render(std::initializer_list<SectionTab> il, std::string_view target, Html& h) -> void {
        std::vector<SectionTab> tabs(il); render(tabs, target, h);
    }
    static auto render(const std::vector<SectionTab>& tabs, std::string_view target, Html& h) -> void {
        h.raw("<div class=\"section-tabs\" data-target=\"").raw(target).raw("\">");
        for (auto& t : tabs) {
            h.raw("<button class=\"section-tab");
            if (t.active) h.raw(" active");
            h.raw("\"");
            if (!t.url.empty()) {
                h.raw(" data-tab-url=\"").raw(t.url).raw("\"");
            }
            h.raw(">").text(t.label).raw("</button>");
        }
        h.raw("</div>");
    }
};

// ─── Tree Nodes ──────────────────────────────────────────────────────

struct TreeNode {
    static auto expandable(Html& h, std::string_view icon_class, std::string_view label,
                           std::string_view children_url, int depth, std::string_view badge_text = "") -> void {
        h.raw("<li class=\"tree-item\"><div class=\"tree-row\" style=\"--tree-depth:")
         .raw(std::to_string(depth)).raw("\" onclick=\"treeToggle(this)\">");
        h.raw("<span class=\"tree-chevron\">&#9656;</span>");
        h.raw("<span class=\"tree-icon ").raw(icon_class).raw("\">&#9679;</span>");
        h.raw("<span class=\"tree-text\">").text(label).raw("</span>");
        if (!badge_text.empty()) h.raw("<span class=\"tree-badge\">").text(badge_text).raw("</span>");
        h.raw("</div><ul class=\"tree-children\" hx-get=\"").raw(children_url)
         .raw("\" hx-trigger=\"click from:closest .tree-item > .tree-row once\" hx-swap=\"innerHTML\"></ul></li>\n");
    }

    static auto leaf(Html& h, std::string_view icon_class, std::string_view label,
                     std::string_view href, int depth, std::string_view extra = "") -> void {
        h.raw("<li class=\"tree-item\"><a class=\"tree-row\" style=\"--tree-depth:")
         .raw(std::to_string(depth)).raw("\" href=\"").raw(href).raw("\" data-spa>");
        h.raw("<span class=\"tree-chevron empty\"></span>");
        h.raw("<span class=\"tree-icon ").raw(icon_class).raw("\">&#9679;</span>");
        h.raw("<span class=\"tree-text\">").text(label).raw("</span>");
        if (!extra.empty()) h.raw(extra);
        h.raw("</a></li>\n");
    }

    static auto separator(Html& h) -> void {
        h.raw("<li class=\"tree-separator\"></li>\n");
    }

    static auto empty_leaf(Html& h, std::string_view message, int depth) -> void {
        h.raw("<li class=\"tree-item\"><span class=\"tree-row\" style=\"--tree-depth:")
         .raw(std::to_string(depth)).raw("\"><span class=\"tree-chevron empty\"></span>")
         .raw("<span class=\"tree-text\" style=\"color:var(--text-4);font-style:italic\">")
         .text(message).raw("</span></span></li>");
    }
};

// ─── Page Layout ─────────────────────────────────────────────────────

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
              "    <link rel=\"stylesheet\" href=\"/assets/css/style.css\">\n"
              "    <script src=\"/assets/js/htmx.min.js\" defer></script>\n"
              "    <script src=\"/assets/js/app.js\" defer></script>\n"
              "    <script src=\"/assets/js/dataview.js\" defer></script>\n"
              "    <script src=\"/assets/js/editor.js\" defer></script>\n"
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
              "  <div class=\"toolbar-actions\">\n"
              "    <button onclick=\"openCommandPalette()\" class=\"toolbar-icon-btn\" title=\"Command Palette (Ctrl+K)\">&#8984;</button>\n"
              "    <button onclick=\"toggleSidebar()\" class=\"toolbar-icon-btn\" title=\"Toggle Sidebar\">&#9776;</button>\n"
              "    <button onclick=\"toggleTheme()\" class=\"toolbar-icon-btn\" title=\"Toggle Theme\">&#9680;</button>\n"
              "  </div>\n</header>\n");
    }

    static auto render_sidebar(Html& h) -> void {
        h.raw("<aside class=\"sidebar\">\n"
              "  <div class=\"sidebar-header\">\n"
              "    <span class=\"sidebar-title\">Explorer</span>\n"
              "    <div class=\"sidebar-actions\">\n"
              "      <button class=\"sidebar-icon\" onclick=\"document.getElementById('sidebar-tree')"
              ".querySelectorAll('.tree-children.loaded').forEach(c => { c.style.display='none'; "
              "c.closest('.tree-item').querySelector('.tree-chevron').classList.remove('expanded'); })\" "
              "title=\"Collapse All\">&#8722;</button>\n"
              "    </div>\n  </div>\n"
              "  <div class=\"sidebar-tree\" id=\"sidebar-tree\" hx-get=\"/tree\" hx-trigger=\"load\" hx-swap=\"innerHTML\">\n"
              "    <div class=\"loading\">Loading...</div>\n"
              "  </div>\n  <div class=\"resize-handle\"></div>\n</aside>\n");
    }

    static auto render_status_bar(Html& h) -> void {
        h.raw("<footer class=\"status-bar\">\n"
              "  <span class=\"status-item\"><span class=\"conn-dot\"></span> Connected</span>\n"
              "  <span class=\"status-sep\"></span>\n"
              "  <span class=\"status-item\" id=\"status-info\"></span>\n"
              "  <span class=\"status-spacer\"></span>\n"
              "  <span class=\"status-item\"><kbd>Ctrl+K</kbd> Command Palette</span>\n"
              "</footer>\n");
    }
};

// ─── Convenience: render component into string ───────────────────────

template<Component C>
auto render_to_string(const typename C::Props& props, std::size_t cap = 4096) -> std::string {
    auto h = Html::with_capacity(cap);
    C::render(props, h);
    return std::move(h).finish();
}

// Render a partial (no page shell) into Html
template<std::invocable<Html&> F>
auto render_partial(F&& fn, std::size_t cap = 4096) -> std::string {
    auto h = Html::with_capacity(cap);
    fn(h);
    return std::move(h).finish();
}

// Render a full page into string
template<std::invocable<Html&> F>
auto render_page(std::string_view title, std::string_view nav, F&& fn, std::size_t cap = 16384) -> std::string {
    auto h = Html::with_capacity(cap);
    PageLayout::render({title, nav}, h, std::forward<F>(fn));
    return std::move(h).finish();
}

template<std::invocable<Html&> F>
auto render_page_full(std::string_view title, std::string_view nav, F&& fn, std::size_t cap = 16384) -> std::string {
    auto h = Html::with_capacity(cap);
    PageLayout::render_full({title, nav}, h, std::forward<F>(fn));
    return std::move(h).finish();
}

} // namespace getgresql::ssr
