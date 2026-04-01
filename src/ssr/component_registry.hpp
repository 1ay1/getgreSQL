#pragma once

// ─── Component Registry — All CSS/JS collected here ─────────────────
//
// Every type with css() or js() is listed in AllComponents.
// collect_css() and collect_js() iterate the list at startup and
// concatenate into /assets/css/components.css and /assets/js/components.js.
//
// ORDER MATTERS: CSS cascades top-to-bottom, JS executes top-to-bottom.
// Foundation types go first, feature components in the middle, polish last.

#include "core/type_list.hpp"
#include "ssr/engine.hpp"

// Vendor (must be first — htmx is a dependency for everything)
#include "ssr/components/htmx_vendor.hpp"

// Foundation (CSS only — design tokens, reset, layout)
#include "ssr/components/theme.hpp"
#include "ssr/components/layout_css.hpp"
#include "ssr/components/buttons.hpp"

// Core runtime (JS — theme toggle, SPA nav, tree, sidebar)
#include "ssr/components/core_runtime.hpp"
#include "ssr/components/ui_helpers.hpp"

// Simple components (CSS, no JS)
#include "ssr/components/alert.hpp"
#include "ssr/components/badge.hpp"
#include "ssr/components/breadcrumbs.hpp"
#include "ssr/components/health_card.hpp"
#include "ssr/components/progress_bar.hpp"
#include "ssr/components/search_input.hpp"
#include "ssr/components/section_tabs.hpp"
#include "ssr/components/size_bar.hpp"
#include "ssr/components/stat_card.hpp"
#include "ssr/components/table.hpp"
#include "ssr/components/tree_node.hpp"

// Feature components (CSS + JS)
#include "ssr/components/cell_editing.hpp"
#include "ssr/components/context_menu.hpp"
#include "ssr/components/dataview_css.hpp"
#include "ssr/components/dv_edit.hpp"
#include "ssr/components/explain_viz.hpp"
#include "ssr/components/erd_widget.hpp"

// Page-level styles
#include "ssr/components/editor_css.hpp"
#include "ssr/components/dashboard_css.hpp"

// Polish (last — animations, transitions, overrides)
#include "ssr/components/polish_css.hpp"

#include <string>

namespace getgresql::ssr {

using AllComponents = meta::TypeList<
    // Vendor (htmx — must be first JS)
    HtmxVendor,

    // Layer 0: Foundation
    Theme, LayoutCSS, Buttons,

    // Layer 1: Core runtime
    CoreRuntime, UIHelpers,

    // Layer 2: UI primitives
    Alert, Badge, Breadcrumbs, HealthCard,
    ProgressBar, SearchInput, SectionTabs,
    SizeBar, StatCard, Table, TreeNode,

    // Layer 3: Feature modules
    CellEditing, ContextMenu,
    DataViewCSS, DvLineagePanel,
    ExplainViz, ERDWidget,

    // Layer 4: Page styles
    EditorCSS, DashboardCSS,

    // Layer 5: Polish (last)
    PolishCSS
>;

inline auto collect_css() -> std::string {
    std::string out;
    out.reserve(32768);
    meta::for_each_type<AllComponents>([&]<typename C>() {
        if constexpr (HasCss<C>) {
            out += C::css();
            out += '\n';
        }
    });
    return out;
}

inline auto collect_js() -> std::string {
    std::string out;
    out.reserve(32768);
    meta::for_each_type<AllComponents>([&]<typename C>() {
        if constexpr (HasJs<C>) {
            out += C::js();
            out += '\n';
        }
    });
    return out;
}

} // namespace getgresql::ssr
