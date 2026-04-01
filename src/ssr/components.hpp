#pragma once

// ─── getgreSQL SSR Components — Barrel Header ────────────────────────
//
// Each component is in its own file under ssr/components/.
// This header re-exports them all for backward compatibility.
//
// Type-theoretic guarantees:
//   - Each component satisfies the Component or ContainerComponent concept
//   - static_assert in each file verifies this at compile time
//   - Props use aggregate init: missing required fields = compile error
//   - SafeHtml types (HtmlString<Escaped>, HtmlString<Trusted>) prevent XSS
//   - Table::row() accepts HtmlString<Trusted> for type-safe cell content
//
// Component concepts (defined in engine.hpp):
//   Component          — has Props, render(Props, Html&) -> void
//   ContainerComponent — has Props, render(Props, Html&, F) -> void
//   Renderable         — has render(Html&) -> void

#include "ssr/engine.hpp"

// Individual components
#include "ssr/components/badge.hpp"
#include "ssr/components/stat_card.hpp"
#include "ssr/components/alert.hpp"
#include "ssr/components/health_card.hpp"
#include "ssr/components/size_bar.hpp"
#include "ssr/components/progress_bar.hpp"
#include "ssr/components/breadcrumbs.hpp"
#include "ssr/components/table.hpp"
#include "ssr/components/data_view.hpp"
#include "ssr/components/search_input.hpp"
#include "ssr/components/section_tabs.hpp"
#include "ssr/components/tree_node.hpp"
#include "ssr/components/page_layout.hpp"
#include "ssr/components/dv_edit.hpp"

#include <format>
#include <string>
#include <string_view>
#include <vector>
#include <span>

namespace getgresql::ssr {

// ─── Convenience: render component into string ───────────────────────

template<Component C>
auto render_to_string(const typename C::Props& props, std::size_t cap = 4096) -> std::string {
    auto h = Html::with_capacity(cap);
    C::render(props, h);
    return std::move(h).finish();
}

// Render a partial (no page shell) into string
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
