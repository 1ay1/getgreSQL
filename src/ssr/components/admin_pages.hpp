#pragma once

// ─── Admin Page SSR Components ───────────────────────────────────────
// Renders: Unused Indexes, Permissions, Explain Hints, Connection Pages
// All UI markup lives here. Handlers pass data, components render HTML.

#include "ssr/engine.hpp"
#include "ssr/components/table.hpp"
#include "ssr/components/alert.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr {

// ─── Unused Index entry ──────────────────────────────────────────────

struct UnusedIndex {
    std::string schema;
    std::string table;
    std::string index;
    std::string size;
    long long size_bytes;
    std::string definition;
};

struct UnusedIndexPage {
    struct Props {
        std::vector<UnusedIndex> indexes;
    };

    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div style=\"max-width:960px;margin:0 auto;padding:var(--sp-5)\">"
              "<div style=\"display:flex;align-items:center;gap:var(--sp-3);margin-bottom:var(--sp-4)\">"
              "<h3 style=\"margin:0\">Unused Indexes</h3>"
              "<span style=\"color:var(--text-3);font-size:var(--font-size-xs)\">"
              "Indexes with zero scans (excluding primary keys and unique constraints)</span>"
              "</div>");

        if (p.indexes.empty()) {
            h.raw("<div class=\"empty-state\"><div class=\"empty-icon\">&#9889;</div>"
                  "<p>No unused indexes found. Your database is well-optimized.</p></div>");
        } else {
            long long total_waste = 0;
            for (auto& idx : p.indexes) total_waste += idx.size_bytes;

            h.raw("<div class=\"query-info\" style=\"margin-bottom:var(--sp-3)\">"
                  "<span class=\"rows-badge\">").raw(std::to_string(p.indexes.size()))
             .raw(" unused indexes</span>"
                  "<span class=\"time-badge\">Wasting ");
            if (total_waste >= 1048576) h.raw(std::format("{:.1f} MB", total_waste / 1048576.0));
            else h.raw(std::format("{:.1f} KB", total_waste / 1024.0));
            h.raw("</span></div>");

            Table::begin(h, {
                {"Schema", "", true}, {"Table", "", true}, {"Index", "", true},
                {"Size", "num", true}, {"Definition", "", false}, {"", "", false}
            });
            for (auto& idx : p.indexes) {
                auto def_short = idx.definition.size() > 60
                    ? idx.definition.substr(0, 60) + "..." : idx.definition;
                Table::row(h, {
                    idx.schema, idx.table, idx.index, idx.size,
                    "<code title=\"" + idx.definition + "\">" + def_short + "</code>",
                    "<button class=\"btn btn-sm btn-danger\" "
                    "hx-post=\"/admin/drop-index\" "
                    "hx-vals='{\"schema\":\"" + idx.schema + "\",\"index\":\"" + idx.index + "\"}' "
                    "hx-target=\"closest tr\" hx-swap=\"outerHTML\" "
                    "hx-confirm=\"DROP INDEX " + idx.schema + "." + idx.index + "?\">Drop</button>"
                });
            }
            Table::end(h);
        }
        h.raw("</div>");
    }
};

// ─── Permission entry ────────────────────────────────────────────────

struct PermissionGrant {
    std::string grantee;
    std::string schema;
    std::string table;
    std::string privilege;
    bool is_grantable;
};

struct RoleMembership {
    std::string role;
    std::string member_of;
};

struct PermissionAuditPage {
    struct Props {
        std::vector<RoleMembership> memberships;
        std::vector<PermissionGrant> grants;
    };

    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div style=\"max-width:960px;margin:0 auto;padding:var(--sp-5)\">"
              "<h3>Permission Audit</h3>");

        if (!p.memberships.empty()) {
            h.raw("<div class=\"dashboard-section\" style=\"margin-bottom:var(--sp-4)\">"
                  "<div class=\"dashboard-section-header\">Role Memberships</div>"
                  "<div class=\"dashboard-section-body\">");
            Table::begin(h, {{"Role", "", true}, {"Member Of", "", true}});
            for (auto& m : p.memberships) {
                Table::row(h, {m.role, m.member_of});
            }
            Table::end(h);
            h.raw("</div></div>");
        }

        if (p.grants.empty()) {
            h.raw("<div class=\"empty-state\"><p>No custom table permissions found</p></div>");
        } else {
            h.raw("<div class=\"dashboard-section\">"
                  "<div class=\"dashboard-section-header\">Table Permissions"
                  "<span class=\"badge\" style=\"margin-left:var(--sp-2)\">")
             .raw(std::to_string(p.grants.size())).raw(" grants</span></div>"
                  "<div class=\"dashboard-section-body\">");
            Table::begin(h, {
                {"Role", "", true}, {"Schema", "", true}, {"Table", "", true},
                {"Privilege", "", true}, {"Grantable", "", true}
            });
            for (auto& g : p.grants) {
                Table::row(h, {
                    g.grantee, g.schema, g.table, g.privilege,
                    g.is_grantable ? "<span class=\"badge badge-success\">YES</span>" : "NO"
                });
            }
            Table::end(h);
            h.raw("</div></div>");
        }

        h.raw("</div>");
    }
};

// ─── Explain Index Hints ─────────────────────────────────────────────

struct IndexHint {
    std::string table;
    std::string column;  // empty = general seq scan warning
};

struct ExplainHints {
    struct Props {
        std::vector<IndexHint> hints;
    };

    static auto render(const Props& p, Html& h) -> void {
        if (p.hints.empty()) return;

        h.raw("<div class=\"explain-hints\">"
              "<div class=\"explain-hints-header\">&#128161; Index Suggestions</div>");
        for (auto& hint : p.hints) {
            h.raw("<div class=\"explain-hint-item\">");
            if (hint.column.empty()) {
                h.raw("<span class=\"explain-hint-text\">Sequential scan on <strong>")
                 .text(hint.table)
                 .raw("</strong> &mdash; consider adding an index if filtered frequently</span>");
            } else {
                auto idx_name = hint.table + "_" + hint.column + "_idx";
                auto create_sql = "CREATE INDEX CONCURRENTLY " + idx_name +
                    " ON " + hint.table + " (" + hint.column + ");";
                h.raw("<span class=\"explain-hint-text\">Sequential scan on <strong>")
                 .text(hint.table)
                 .raw("</strong> filtered by <code>").text(hint.column).raw("</code></span>");
                h.raw("<button class=\"btn btn-sm btn-ghost\" onclick=\"navigator.clipboard.writeText('")
                 .text(create_sql).raw("'); this.textContent='Copied!'\" title=\"")
                 .text(create_sql).raw("\">Copy CREATE INDEX</button>");
            }
            h.raw("</div>");
        }
        h.raw("</div>");
    }
};

// ─── Connection Page Components ──────────────────────────────────────

struct ConnectionInfo {
    std::string name;
    std::string url;
    std::string color;
    bool is_active;
};

struct ConnectionsPage {
    struct Props {
        std::string current_db;
        std::string current_url;
        std::vector<ConnectionInfo> connections;
    };

    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div style=\"max-width:720px;margin:0 auto;padding:var(--sp-5)\">");

        // Current connection
        h.raw("<div class=\"conn-current\">"
              "<h3>Current Connection</h3>"
              "<div class=\"conn-info-card\">"
              "<span class=\"conn-dot-lg\"></span>"
              "<div><strong>").text(p.current_db).raw("</strong>"
              "<div class=\"conn-url\">").text(p.current_url).raw("</div>"
              "</div></div></div>");

        // Add form
        h.raw("<div class=\"conn-add-section\">"
              "<h3>Add Connection</h3>"
              "<form hx-post=\"/connections/save\" hx-target=\"#conn-list\" hx-swap=\"innerHTML\">"
              "<div class=\"conn-form-grid\">"
              "<div class=\"form-field\"><label>Name</label>"
              "<input type=\"text\" name=\"name\" placeholder=\"Production DB\" required class=\"form-input\"></div>"
              "<div class=\"form-field\"><label>Connection URL</label>"
              "<input type=\"text\" name=\"url\" placeholder=\"postgresql://user:pass@host/db\" required class=\"form-input\"></div>"
              "<div class=\"form-field\"><label>Color</label>"
              "<select name=\"color\" class=\"form-input\">"
              "<option value=\"\">Default</option>"
              "<option value=\"#3b82f6\">Blue</option>"
              "<option value=\"#10b981\">Green</option>"
              "<option value=\"#f59e0b\">Amber</option>"
              "<option value=\"#ef4444\">Red</option>"
              "<option value=\"#8b5cf6\">Purple</option>"
              "<option value=\"#ec4899\">Pink</option>"
              "</select></div></div>"
              "<div class=\"conn-form-actions\">"
              "<button type=\"button\" class=\"btn btn-sm\" hx-post=\"/connections/test\" "
              "hx-include=\"closest form\" hx-target=\"#test-result\" hx-swap=\"innerHTML\">Test</button>"
              "<button type=\"submit\" class=\"btn btn-sm btn-primary\">Save Connection</button>"
              "</div><div id=\"test-result\"></div></form></div>");

        // Saved list
        h.raw("<div class=\"conn-saved-section\"><h3>Saved Connections</h3>"
              "<div id=\"conn-list\">");
        render_list(p.connections, p.current_url, h);
        h.raw("</div></div></div>");
    }

    static auto render_list(const std::vector<ConnectionInfo>& conns,
                            std::string_view current_url, Html& h) -> void {
        if (conns.empty()) {
            h.raw("<div class=\"empty-state\"><div class=\"empty-icon\">&#128268;</div>"
                  "<p>No saved connections. Add one above.</p></div>");
            return;
        }
        for (auto& c : conns) {
            bool active = (c.url == current_url);
            h.raw("<div class=\"conn-item");
            if (active) h.raw(" conn-active");
            h.raw("\"");
            if (!c.color.empty()) h.raw(" style=\"border-left:3px solid ").raw(c.color).raw("\"");
            h.raw("><div class=\"conn-item-info\"><strong>").text(c.name).raw("</strong>");
            if (active) h.raw(" <span class=\"badge badge-success\">active</span>");
            h.raw("<div class=\"conn-url\">").text(c.url).raw("</div></div>"
                  "<div class=\"conn-item-actions\">");
            if (!active) {
                h.raw("<button class=\"btn btn-sm btn-primary\" "
                      "hx-post=\"/connections/switch\" "
                      "hx-vals='{\"name\":\"").text(c.name).raw("\"}' "
                      "hx-confirm=\"Switch to ").text(c.name).raw("?\" "
                      "hx-target=\"body\">Switch</button>");
            }
            h.raw("<button class=\"btn btn-sm btn-danger\" "
                  "hx-post=\"/connections/delete\" "
                  "hx-vals='{\"name\":\"").text(c.name).raw("\"}' "
                  "hx-target=\"#conn-list\" hx-swap=\"innerHTML\" "
                  "hx-confirm=\"Delete ").text(c.name).raw("?\">Delete</button>"
                  "</div></div>\n");
        }
    }
};

// ─── Dashboard Health Component ──────────────────────────────────────

struct HealthCheckData {
    std::string name;
    std::string status;
    std::string value;
    std::string detail;
    std::string fix_action;
    std::string fix_label;
};

struct DashboardHealthRibbon {
    struct Props {
        std::vector<HealthCheckData> checks;
        std::string toast_message;  // non-empty after a fix action
    };

    static auto render(const Props& p, Html& h) -> void {
        if (!p.toast_message.empty()) {
            h.raw("<div class=\"dash-fix-toast\">").text(p.toast_message).raw("</div>");
        }

        int ok = 0, warn = 0, crit = 0;
        for (auto& c : p.checks) {
            if (c.status == "ok") ok++;
            else if (c.status == "warning") warn++;
            else crit++;
        }
        auto overall = crit > 0 ? "critical" : warn > 0 ? "degraded" : "healthy";
        auto overall_cls = crit > 0 ? "danger" : warn > 0 ? "warning" : "success";

        h.raw("<div class=\"dash-hero\">"
              "<div class=\"dash-hero-status dash-hero-").raw(overall_cls).raw("\">"
              "<div class=\"dash-hero-pulse\"></div>"
              "<span class=\"dash-hero-dot\"></span>"
              "<span class=\"dash-hero-label\">System ").raw(overall).raw("</span></div>"
              "<div class=\"dash-checks\">");

        for (auto& c : p.checks) {
            auto v = (c.status == "ok") ? "success" : (c.status == "warning") ? "warning" : "danger";
            auto icon = (c.status == "ok") ? "&#10003;" : (c.status == "warning") ? "&#9888;" : "&#10007;";
            h.raw("<div class=\"dash-check dash-check-").raw(v).raw("\" title=\"")
             .text(c.name).raw(": ").text(c.detail).raw("\">"
              "<span class=\"dash-check-icon\">").raw(icon).raw("</span>"
              "<span class=\"dash-check-name\">").text(c.name).raw("</span>"
              "<span class=\"dash-check-val\">").text(c.value).raw("</span>");
            if (!c.fix_action.empty() && c.status != "ok") {
                h.raw("<button class=\"dash-fix-btn\" "
                      "hx-post=\"/dashboard/fix\" "
                      "hx-vals='{\"action\":\"").raw(c.fix_action).raw("\"}' "
                      "hx-target=\"#dash-health\" hx-swap=\"innerHTML\" "
                      "hx-confirm=\"").text(c.fix_label).raw(" &mdash; are you sure?\">"
                      ).text(c.fix_label).raw("</button>");
            }
            h.raw("</div>");
        }
        h.raw("</div></div>");
    }
};

// ─── Query Row Batch Renderer ────────────────────────────────────────

struct QueryRowBatch {
    static auto render_row(Html& h, int col_start, int col_count,
                           const auto& result, int row_idx,
                           bool has_ctid) -> void {
        auto ctid = has_ctid ? std::string_view(result.get(row_idx, 0)) : std::string_view("");
        h.raw("<tr");
        if (!ctid.empty()) h.raw(" data-ctid=\"").text(ctid).raw("\"");
        h.raw(">");
        for (int c = col_start; c < col_count; ++c) {
            h.raw("<td>");
            if (result.is_null(row_idx, c)) {
                h.raw("<span class=\"null-value dv-cell\">NULL</span>");
            } else {
                auto val = result.get(row_idx, c);
                auto col_name = result.column_name(c);
                auto table_oid = result.column_table_oid(c);
                h.raw("<span class=\"dv-cell\"");
                if (!col_name.empty()) h.raw(" data-col=\"").text(col_name).raw("\"");
                if (table_oid != 0) h.raw(" data-table-oid=\"").raw(std::to_string(table_oid)).raw("\"");
                if (val.size() > 80) {
                    h.raw(" data-full=\"").text(val).raw("\">");
                    h.text(val.substr(0, 60)).raw("&hellip;");
                } else {
                    h.raw(">").text(val);
                }
                h.raw("</span>");
            }
            h.raw("</td>");
        }
        h.raw("</tr>\n");
    }
};

} // namespace getgresql::ssr
