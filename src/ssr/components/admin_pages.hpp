#pragma once

// ─── Admin Page SSR Components ───────────────────────────────────────
// All UI markup uses the compile-time HTML+CSS+JS DSL. Zero raw HTML.

#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/css_dsl.hpp"
#include "ssr/js_dsl.hpp"
#include "ssr/ui.hpp"
#include "ssr/components/table.hpp"
#include "ssr/components/alert.hpp"

#include <format>
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
        using namespace html;
        {
            auto wrap = open<Div>(h, {style("max-width:960px;margin:0 auto;padding:var(--sp-5)")});
            {
                auto header = open<Div>(h, {style("display:flex;align-items:center;gap:var(--sp-3);margin-bottom:var(--sp-4)")});
                el<H3>(h, {style("margin:0")}, "Unused Indexes");
                el<Span>(h, {style("color:var(--text-3);font-size:var(--font-size-xs)")},
                    "Indexes with zero scans (excluding primary keys and unique constraints)");
            }

            if (p.indexes.empty()) {
                {
                    auto empty = open<Div>(h, {cls("empty-state")});
                    el_raw<Div>(h, {cls("empty-icon")}, "&#9889;");
                    el<P>(h, {}, "No unused indexes found. Your database is well-optimized.");
                }
            } else {
                long long total_waste = 0;
                for (auto& idx : p.indexes) total_waste += idx.size_bytes;

                auto waste_str = total_waste >= 1048576
                    ? std::format("{:.1f} MB", total_waste / 1048576.0)
                    : std::format("{:.1f} KB", total_waste / 1024.0);
                {
                    auto info = open<Div>(h, {cls("query-info"), style("margin-bottom:var(--sp-3)")});
                    el<Span>(h, {cls("rows-badge")}, std::to_string(p.indexes.size()) + " unused indexes");
                    el<Span>(h, {cls("time-badge")}, "Wasting " + waste_str);
                }

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
        }
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
        using namespace html;
        {
            auto wrap = open<Div>(h, {style("max-width:960px;margin:0 auto;padding:var(--sp-5)")});
            el<H3>(h, {}, "Permission Audit");

            if (!p.memberships.empty()) {
                {
                    auto section = open<Div>(h, {cls("dashboard-section"), style("margin-bottom:var(--sp-4)")});
                    el<Div>(h, {cls("dashboard-section-header")}, "Role Memberships");
                    {
                        auto body = open<Div>(h, {cls("dashboard-section-body")});
                        Table::begin(h, {{"Role", "", true}, {"Member Of", "", true}});
                        for (auto& m : p.memberships) Table::row(h, {m.role, m.member_of});
                        Table::end(h);
                    }
                }
            }

            if (p.grants.empty()) {
                {
                    auto empty = open<Div>(h, {cls("empty-state")});
                    el<P>(h, {}, "No custom table permissions found");
                }
            } else {
                {
                    auto section = open<Div>(h, {cls("dashboard-section")});
                    {
                        auto hdr = open<Div>(h, {cls("dashboard-section-header")});
                        h.raw("Table Permissions");
                        el<Span>(h, {cls("badge"), style("margin-left:var(--sp-2)")},
                            std::to_string(p.grants.size()) + " grants");
                    }
                    {
                        auto body = open<Div>(h, {cls("dashboard-section-body")});
                        Table::begin(h, {
                            {"Role", "", true}, {"Schema", "", true}, {"Table", "", true},
                            {"Privilege", "", true}, {"Grantable", "", true}
                        });
                        for (auto& g : p.grants) {
                            Table::row(h, {
                                g.grantee, g.schema, g.table, g.privilege,
                                g.is_grantable ? "<span class=\"badge success\">YES</span>" : "NO"
                            });
                        }
                        Table::end(h);
                    }
                }
            }
        }
    }
};

// ─── Explain Index Hints ─────────────────────────────────────────────

struct IndexHint {
    std::string table;
    std::string column;
};

struct ExplainHints {
    struct Props {
        std::vector<IndexHint> hints;
    };

    static auto render(const Props& p, Html& h) -> void {
        if (p.hints.empty()) return;
        using namespace html;
        {
            auto wrap = open<Div>(h, {cls("explain-hints")});
            el_raw<Div>(h, {cls("explain-hints-header")}, "&#128161; Index Suggestions");
            for (auto& hint : p.hints) {
                {
                    auto item = open<Div>(h, {cls("explain-hint-item")});
                    if (hint.column.empty()) {
                        {
                            auto text = open<Span>(h, {cls("explain-hint-text")});
                            h.raw("Sequential scan on ");
                            el<Strong>(h, {}, hint.table);
                            h.raw(" &mdash; consider adding an index if filtered frequently");
                        }
                    } else {
                        auto idx_name = hint.table + "_" + hint.column + "_idx";
                        auto create_sql = "CREATE INDEX CONCURRENTLY " + idx_name +
                            " ON " + hint.table + " (" + hint.column + ");";
                        {
                            auto text = open<Span>(h, {cls("explain-hint-text")});
                            h.raw("Sequential scan on ");
                            el<Strong>(h, {}, hint.table);
                            h.raw(" filtered by ");
                            el<Code>(h, {}, hint.column);
                        }
                        el<Button>(h, {
                            cls("btn btn-sm btn-ghost"),
                            js::on_click(js::then(
                                js::copy_to_clipboard(create_sql),
                                js::set_text("Copied!")
                            )),
                            title(create_sql),
                        }, "Copy CREATE INDEX");
                    }
                }
            }
        }
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
        using namespace html;
        {
            auto wrap = open<Div>(h, {style("max-width:720px;margin:0 auto;padding:var(--sp-5)")});

            // Current connection
            {
                auto section = open<Div>(h, {cls("conn-current")});
                el<H3>(h, {}, "Current Connection");
                {
                    auto card = open<Div>(h, {cls("conn-info-card")});
                    el<Span>(h, {cls("conn-dot-lg")});
                    {
                        auto info = open<Div>(h);
                        el<Strong>(h, {}, p.current_db);
                        el<Div>(h, {cls("conn-url")}, p.current_url);
                    }
                }
            }

            // Add form
            {
                auto section = open<Div>(h, {cls("conn-add-section")});
                el<H3>(h, {}, "Add Connection");
                {
                    auto form = open<Form>(h, {hx_post("/connections/save"),
                                               hx_target("#conn-list"), hx_swap("innerHTML")});
                    {
                        auto grid = open<Div>(h, {cls("conn-form-grid")});
                        {
                            auto f = open<Div>(h, {cls("form-field")});
                            el<Label>(h, {}, "Name");
                            void_el<Input>(h, {type("text"), name("name"),
                                              placeholder("Production DB"), required(), cls("form-input")});
                        }
                        {
                            auto f = open<Div>(h, {cls("form-field")});
                            el<Label>(h, {}, "Connection URL");
                            void_el<Input>(h, {type("text"), name("url"),
                                              placeholder("postgresql://user:pass@host/db"), required(), cls("form-input")});
                        }
                        {
                            auto f = open<Div>(h, {cls("form-field")});
                            el<Label>(h, {}, "Color");
                            {
                                auto sel = open<Select>(h, {name("color"), cls("form-input")});
                                el<Option>(h, {value("")}, "Default");
                                el<Option>(h, {value("#3b82f6")}, "Blue");
                                el<Option>(h, {value("#10b981")}, "Green");
                                el<Option>(h, {value("#f59e0b")}, "Amber");
                                el<Option>(h, {value("#ef4444")}, "Red");
                                el<Option>(h, {value("#8b5cf6")}, "Purple");
                                el<Option>(h, {value("#ec4899")}, "Pink");
                            }
                        }
                    }
                    {
                        auto actions = open<Div>(h, {cls("conn-form-actions")});
                        el<Button>(h, {type("button"), cls("btn btn-sm"),
                                      hx_post("/connections/test"),
                                      hx_include("closest form"),
                                      hx_target("#test-result"), hx_swap("innerHTML")}, "Test");
                        el<Button>(h, {type("submit"), cls("btn btn-sm btn-primary")}, "Save Connection");
                    }
                    el<Div>(h, {id("test-result")});
                }
            }

            // Saved list
            {
                auto section = open<Div>(h, {cls("conn-saved-section")});
                el<H3>(h, {}, "Saved Connections");
                {
                    auto list = open<Div>(h, {id("conn-list")});
                    render_list(p.connections, p.current_url, h);
                }
            }
        }
    }

    static auto render_list(const std::vector<ConnectionInfo>& conns,
                            std::string_view current_url, Html& h) -> void {
        using namespace html;
        if (conns.empty()) {
            {
                auto empty = open<Div>(h, {cls("empty-state")});
                el_raw<Div>(h, {cls("empty-icon")}, "&#128268;");
                el<P>(h, {}, "No saved connections. Add one above.");
            }
            return;
        }
        for (auto& c : conns) {
            bool active = (c.url == current_url);
            auto item_cls = std::string("conn-item");
            if (active) item_cls += " conn-active";
            auto item_style = c.color.empty() ? std::string{} : "border-left:3px solid " + c.color;
            {
                auto item = open<Div>(h, item_style.empty()
                    ? std::initializer_list<Attr>{cls(item_cls)}
                    : std::initializer_list<Attr>{cls(item_cls), style(item_style)});
                {
                    auto info = open<Div>(h, {cls("conn-item-info")});
                    el<Strong>(h, {}, c.name);
                    if (active) el<Span>(h, {cls("badge success")}, "active");
                    el<Div>(h, {cls("conn-url")}, c.url);
                }
                {
                    auto actions = open<Div>(h, {cls("conn-item-actions")});
                    if (!active) {
                        el<Button>(h, {
                            cls("btn btn-sm btn-primary"),
                            hx_post("/connections/switch"),
                            hx_vals("{\"name\":\"" + c.name + "\"}"),
                            hx_confirm("Switch to " + c.name + "?"),
                            hx_target("body"),
                        }, "Switch");
                    }
                    el<Button>(h, {
                        cls("btn btn-sm btn-danger"),
                        hx_post("/connections/delete"),
                        hx_vals("{\"name\":\"" + c.name + "\"}"),
                        hx_target("#conn-list"), hx_swap("innerHTML"),
                        hx_confirm("Delete " + c.name + "?"),
                    }, "Delete");
                }
            }
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
        std::string toast_message;
    };

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;

        if (!p.toast_message.empty()) {
            el<Div>(h, {cls("dash-fix-toast")}, p.toast_message);
        }

        int ok = 0, warn = 0, crit = 0;
        for (auto& c : p.checks) {
            if (c.status == "ok") ok++;
            else if (c.status == "warning") warn++;
            else crit++;
        }
        auto overall = crit > 0 ? "critical" : warn > 0 ? "degraded" : "healthy";
        auto overall_cls = crit > 0 ? "danger" : warn > 0 ? "warning" : "success";

        {
            auto hero = open<Div>(h, {cls("dash-hero")});
            {
                auto status = open<Div>(h, {cls(join("dash-hero-status", "dash-hero-" + std::string(overall_cls)))});
                el<Div>(h, {cls("dash-hero-pulse")});
                el<Span>(h, {cls("dash-hero-dot")});
                el_raw<Span>(h, {cls("dash-hero-label")}, "System " + std::string(overall));
            }
            {
                auto checks = open<Div>(h, {cls("dash-checks")});
                for (auto& c : p.checks) {
                    auto v = (c.status == "ok") ? "success" : (c.status == "warning") ? "warning" : "danger";
                    auto icon_str = (c.status == "ok") ? "&#10003;" : (c.status == "warning") ? "&#9888;" : "&#10007;";
                    {
                        auto check = open<Div>(h, {
                            cls(join("dash-check", "dash-check-" + std::string(v))),
                            title(c.name + ": " + c.detail),
                        });
                        el_raw<Span>(h, {cls("dash-check-icon")}, icon_str);
                        el<Span>(h, {cls("dash-check-name")}, c.name);
                        el<Span>(h, {cls("dash-check-val")}, c.value);
                        if (!c.fix_action.empty() && c.status != "ok") {
                            el<Button>(h, {
                                cls("dash-fix-btn"),
                                hx_post("/dashboard/fix"),
                                hx_vals("{\"action\":\"" + c.fix_action + "\"}"),
                                hx_target("#dash-health"),
                                hx_swap("innerHTML"),
                                hx_confirm(c.fix_label + " — are you sure?"),
                            }, c.fix_label);
                        }
                    }
                }
            }
        }
    }
};

// ─── Query Row Batch Renderer ────────────────────────────────────────

struct QueryRowBatch {
    static auto render_row(Html& h, int col_start, int col_count,
                           const auto& result, int row_idx,
                           bool has_ctid) -> void {
        using namespace html;
        auto ctid = has_ctid ? std::string_view(result.get(row_idx, 0)) : std::string_view("");
        {
            auto tr = ctid.empty()
                ? open<Tr>(h)
                : open<Tr>(h, {data("ctid", ctid)});
            for (int c = col_start; c < col_count; ++c) {
                auto td_scope = open<Td>(h);
                if (result.is_null(row_idx, c)) {
                    el<Span>(h, {cls("null-value dv-cell")}, "NULL");
                } else {
                    auto val = result.get(row_idx, c);
                    auto col_name = result.column_name(c);
                    auto table_oid = result.column_table_oid(c);
                    std::initializer_list<Attr> attrs = (val.size() > 80)
                        ? std::initializer_list<Attr>{cls("dv-cell dv-cell-long"),
                            data("col", col_name),
                            data("table-oid", std::to_string(table_oid)),
                            data("full", val)}
                        : std::initializer_list<Attr>{cls("dv-cell"),
                            data("col", col_name),
                            data("table-oid", std::to_string(table_oid))};
                    if (val.size() > 80) {
                        auto span = open<Span>(h, {cls("dv-cell dv-cell-long"),
                            data("col", col_name),
                            data("table-oid", std::to_string(table_oid)),
                            data("full", val)});
                        h.text(val.substr(0, 60));
                        h.raw("&hellip;");
                    } else {
                        el<Span>(h, {cls("dv-cell"),
                            data("col", col_name),
                            data("table-oid", std::to_string(table_oid))}, val);
                    }
                }
            }
        }
    }
};

} // namespace getgresql::ssr
