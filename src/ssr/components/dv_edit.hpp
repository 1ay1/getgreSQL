#pragma once

// ─── DataView SSR Edit Components ───────────────────────────────────
// Pure render functions for htmx-driven cell editing.
// Handlers call these — no raw HTML in handler code.

#include "ssr/engine.hpp"
#include "ssr/components/badge.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr {

// ─── Inline edit form (replaces cell content on double-click) ────────

struct DvEditForm {
    struct Props {
        std::string_view db;
        std::string_view schema;
        std::string_view table;
        std::string_view col;
        std::string_view ctid;
        std::string_view val;
        std::string_view table_oid;  // for query result cells
    };
    static auto render(const Props& p, Html& h) -> void {
        h.raw("<form class=\"dv-edit-form\" hx-post=\"/dv/save-cell\" hx-swap=\"outerHTML\" hx-target=\"closest td\">");
        hidden(h, "db", p.db);
        hidden(h, "schema", p.schema);
        hidden(h, "table", p.table);
        hidden(h, "col", p.col);
        hidden(h, "ctid", p.ctid);
        hidden(h, "table_oid", p.table_oid);
        h.raw("<input type=\"text\" name=\"val\" value=\"").text(p.val)
         .raw("\" class=\"cell-edit-input\" autofocus onfocus=\"this.select()\" "
              "onkeydown=\"if(event.key==='Escape'){this.closest('form').outerHTML=this.closest('form').dataset.orig; event.preventDefault();}\"");
        // Store original cell HTML for escape restore
        h.raw(" data-restore=\"").text(p.val).raw("\">");
        h.raw("</form>");
    }
private:
    static auto hidden(Html& h, std::string_view name, std::string_view val) -> void {
        h.raw("<input type=\"hidden\" name=\"").raw(name).raw("\" value=\"").text(val).raw("\">");
    }
};

// ─── Saved cell (returned after successful update) ──────────────────

struct DvSavedCell {
    struct Props {
        std::string_view db;
        std::string_view schema;
        std::string_view table;
        std::string_view col;
        std::string_view ctid;
        std::string_view val;
        bool is_null = false;
        bool is_error = false;
        std::string_view error_msg;
    };
    static auto render(const Props& p, Html& h) -> void {
        auto edit_url = edit_url_for(p);

        h.raw("<td>");
        if (p.is_null) {
            h.raw("<span class=\"null-value editable-cell");
            if (p.is_error) h.raw(" cell-error");
            else h.raw(" cell-saved");
            h.raw("\" data-col=\"").text(p.col).raw("\" data-ctid=\"").text(p.ctid)
             .raw("\" hx-get=\"").raw(edit_url)
             .raw("\" hx-trigger=\"dblclick\" hx-target=\"this\" hx-swap=\"outerHTML\">"
                  "NULL</span>");
        } else if (p.is_error) {
            h.raw("<span class=\"editable-cell cell-error\" data-col=\"").text(p.col)
             .raw("\" data-ctid=\"").text(p.ctid)
             .raw("\" title=\"").text(p.error_msg)
             .raw("\" hx-get=\"").raw(edit_url)
             .raw("\" hx-trigger=\"dblclick\" hx-target=\"this\" hx-swap=\"outerHTML\">");
            h.text(p.val);
            h.raw("</span>");
        } else {
            bool is_long = p.val.size() > 80;
            h.raw("<span class=\"editable-cell");
            if (is_long) h.raw(" dv-cell-long");
            h.raw(" cell-saved\" data-col=\"").text(p.col)
             .raw("\" data-ctid=\"").text(p.ctid).raw("\"");
            if (is_long) h.raw(" data-full=\"").text(p.val).raw("\"");
            h.raw(" hx-get=\"").raw(edit_url)
             .raw("\" hx-trigger=\"dblclick\" hx-target=\"this\" hx-swap=\"outerHTML\">");
            if (is_long) {
                h.text(p.val.substr(0, 60)).raw("&hellip;");
            } else {
                h.text(p.val);
            }
            h.raw("</span>");
        }
        h.raw("</td>");
    }

private:
    static auto edit_url_for(const Props& p) -> std::string {
        auto h = Html::with_capacity(256);
        h.raw("/dv/edit-cell?db=").text(p.db).raw("&schema=").text(p.schema)
         .raw("&table=").text(p.table).raw("&col=").text(p.col)
         .raw("&ctid=").text(p.ctid).raw("&val=").text(p.val);
        return std::move(h).finish();
    }
};

// ─── Read-only cell with explain-this trigger ───────────────────────

struct DvReadOnlyCell {
    struct Props {
        std::string_view val;
        std::string_view col;
        std::string_view table_oid;  // "0" or "" = computed
        bool is_null = false;
    };
    static auto render(const Props& p, Html& h) -> void {
        if (p.is_null) {
            h.raw("<span class=\"null-value dv-cell\">NULL</span>");
        } else {
            bool editable_source = !p.table_oid.empty() && p.table_oid != "0";
            bool is_long = p.val.size() > 200;

            h.raw("<span class=\"dv-cell");
            if (is_long) h.raw(" dv-cell-long");
            h.raw("\"");
            if (is_long) h.raw(" data-full=\"").text(p.val).raw("\"");
            // Explain-this: htmx popover on right-click via JS trigger
            if (editable_source) {
                h.raw(" data-table-oid=\"").raw(p.table_oid)
                 .raw("\" data-col=\"").text(p.col).raw("\"");
            }
            h.raw(">");
            if (is_long) {
                h.text(p.val.substr(0, 200)).raw("&hellip;");
            } else {
                h.text(p.val);
            }
            h.raw("</span>");
        }
    }
};

// ─── Cell lineage panel (Explain This) ──────────────────────────────

struct DvLineagePanel {
    struct SourceInfo {
        std::string schema;
        std::string table;
        std::string column;
        std::string type;
        std::string table_size;
        std::string approx_rows;
        std::string col_comment;
        std::string table_comment;
        bool not_null = false;
        std::string col_default;
    };

    struct ForeignKey { std::string name; std::string definition; };
    struct Index { std::string definition; };

    struct Props {
        std::string_view col;
        std::string_view val;
        bool has_source = false;
        SourceInfo source;
        std::vector<ForeignKey> fks;
        std::vector<Index> indexes;
    };

    static auto render(const Props& p, Html& h) -> void {
        h.raw("<div class=\"dv-lineage-panel\">");
        h.raw("<div class=\"dv-lineage-header\">Cell Lineage"
              "<button onclick=\"this.closest('.dv-lineage-panel').remove()\" "
              "class=\"dv-lineage-close\">&times;</button></div>");

        if (!p.has_source) {
            section(h, "Source", "Computed expression (no source table)");
            section(h, "Column", std::string("<code>") + std::string(p.col) + "</code>");
        } else {
            auto& s = p.source;
            // Source table
            auto table_link = std::string("<a href=\"/db/postgres/schema/") + s.schema +
                "/table/" + s.table + "\" data-spa><code>" + s.schema + "." + s.table + "</code></a>";
            if (!s.table_comment.empty()) table_link += " <span class=\"dv-lineage-comment\">" + s.table_comment + "</span>";
            section(h, "Source Table", table_link);

            // Column — render badges inline
            {
                auto ch = Html::with_capacity(512);
                ch.raw("<code>").raw(s.column).raw("</code> ");
                Badge::render({s.type, "secondary"}, ch);
                if (s.not_null) { ch.raw(" "); Badge::render({"NOT NULL", "warning"}, ch); }
                if (!s.col_default.empty()) ch.raw(" <span class=\"dv-lineage-default\">default: <code>").raw(s.col_default).raw("</code></span>");
                if (!s.col_comment.empty()) ch.raw("<br><span class=\"dv-lineage-comment\">").raw(s.col_comment).raw("</span>");
                section(h, "Column", std::move(ch).finish());
            }

            // Stats
            section(h, "Table Stats", s.table_size + " &middot; ~" + s.approx_rows + " rows");

            // Foreign keys
            if (!p.fks.empty()) {
                auto fk_html = std::string();
                for (auto& fk : p.fks) fk_html += "<div><code>" + fk.name + "</code>: " + fk.definition + "</div>";
                section(h, "Foreign Keys", fk_html);
            }

            // Indexes
            if (!p.indexes.empty()) {
                auto idx_html = std::string();
                for (auto& idx : p.indexes) idx_html += "<div><code>" + idx.definition + "</code></div>";
                section(h, "Indexes", idx_html);
            }
        }

        // Current value
        {
            auto val_h = Html::with_capacity(p.val.size() + 32);
            val_h.raw("<code>").text(p.val).raw("</code>");
            section(h, "Value", std::move(val_h).finish());
        }

        h.raw("</div>");
    }

private:
    static auto section(Html& h, std::string_view label, const std::string& content) -> void {
        h.raw("<div class=\"dv-lineage-section\">"
              "<div class=\"dv-lineage-label\">").raw(label).raw("</div>"
              "<div class=\"dv-lineage-value\">").raw(content).raw("</div></div>");
    }
};

} // namespace getgresql::ssr
