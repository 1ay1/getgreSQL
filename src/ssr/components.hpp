#pragma once

// ─── getgreSQL Component System ─────────────────────────────────────
//
// Every UI element is a component. Components are C++ types with:
//
//   struct MyComponent {
//       struct Props { ... };                              // Data
//       static constexpr auto css() -> std::string_view;  // Styles
//       static constexpr auto js()  -> std::string_view;  // Behavior
//       static auto render(const Props&, Html&) -> void;  // Template
//   };
//
// All CSS/JS is co-located — no external asset files.
// The registry collects assets at startup into /assets/css/components.css
// and /assets/js/components.js, served as single bundles.
//
// Order in AllComponents determines CSS cascade and JS execution order:
//   Vendor → Foundation → Runtime → Primitives → Features → Pages → Polish

#include "core/type_list.hpp"
#include "ssr/engine.hpp"

// ─── Vendor ─────────────────────────────────────────────────────────
#include "ssr/components/htmx_vendor.hpp"

// ─── Foundation (CSS — design tokens, layout grid, buttons) ─────────
#include "ssr/components/theme.hpp"
#include "ssr/components/layout_css.hpp"
#include "ssr/components/buttons.hpp"

// ─── Runtime (JS — core behaviors, no render method) ────────────────
#include "ssr/components/core_runtime.hpp"
#include "ssr/components/ui_helpers.hpp"

// ─── UI Primitives ──────────────────────────────────────────────────
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

// ─── Feature Components (CSS + JS) ──────────────────────────────────
#include "ssr/components/cell_editing.hpp"
#include "ssr/components/context_menu.hpp"
#include "ssr/components/data_view.hpp"
#include "ssr/components/dataview_css.hpp"
#include "ssr/components/dv_edit.hpp"
#include "ssr/components/explain_viz.hpp"
#include "ssr/components/erd_widget.hpp"

// ─── Page Components ────────────────────────────────────────────────
#include "ssr/components/page_layout.hpp"
#include "ssr/components/editor_css.hpp"
#include "ssr/components/dashboard_css.hpp"
#include "ssr/components/connections_css.hpp"

// ─── Polish (last — animations, transitions, print) ─────────────────
#include "ssr/components/polish_css.hpp"

#include <string>
#include <string_view>

namespace getgresql::ssr {

// ─── Component Registry ─────────────────────────────────────────────

using AllComponents = meta::TypeList<
    HtmxVendor,
    Theme, LayoutCSS, Buttons,
    CoreRuntime, UIHelpers,
    Alert, Badge, Breadcrumbs, HealthCard,
    ProgressBar, SearchInput, SectionTabs,
    SizeBar, StatCard, Table, TreeNode,
    CellEditing, ContextMenu,
    DataViewCSS, DvLineagePanel,
    ExplainViz, ERDWidget,
    EditorCSS, DashboardCSS, ConnectionsCSS,
    PolishCSS
>;

// Collect all component CSS/JS into single bundle strings.
// Called once at startup, cached for process lifetime.
inline auto collect_css() -> std::string {
    std::string out;
    out.reserve(32768);
    meta::for_each_type<AllComponents>([&]<typename C>() {
        if constexpr (HasCss<C>) { out += C::css(); out += '\n'; }
    });
    return out;
}

inline auto collect_js() -> std::string {
    std::string out;
    out.reserve(65536);
    meta::for_each_type<AllComponents>([&]<typename C>() {
        if constexpr (HasJs<C>) { out += C::js(); out += '\n'; }
    });
    return out;
}

// ─── Render Helpers ─────────────────────────────────────────────────

template<Component C>
auto render_to_string(const typename C::Props& props, std::size_t cap = 4096) -> std::string {
    auto h = Html::with_capacity(cap);
    C::render(props, h);
    return std::move(h).finish();
}

template<std::invocable<Html&> F>
auto render_partial(F&& fn, std::size_t cap = 4096) -> std::string {
    auto h = Html::with_capacity(cap);
    fn(h);
    return std::move(h).finish();
}

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
