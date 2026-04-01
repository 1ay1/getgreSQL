#pragma once

// ─── getgreSQL Component Type System ────────────────────────────────
//
// Type-theoretic component architecture:
//
//   1. Components are types satisfying the Component concept
//   2. Props are value types — the compiler checks every field
//   3. Phantom types track state (Escaped/Trusted HTML, ReadOnly/Editable views)
//   4. RAII scopes enforce matching open/close tags
//   5. Concepts constrain which operations are valid in which context
//
// Bad states that are compile errors:
//   - Rendering a component with wrong props type
//   - Emitting unescaped user content via emit()
//   - Calling editable_row() on a ReadOnly DataView
//   - Forgetting to close a scoped element (RAII)
//   - Using a moved-from component builder

#include "ssr/engine.hpp"

#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr {

// ── Re-export core concepts from engine.hpp ─────────────────────────
// Component, Renderable, ContainerComponent, SafeHtml are defined there.
// This header adds higher-level composition utilities.

// ── render_to_string: component → std::string ───────────────────────

template<Component C>
auto render_to_string(const typename C::Props& props, std::size_t cap = 4096) -> std::string {
    auto h = Html::with_capacity(cap);
    C::render(props, h);
    return std::move(h).finish();
}

// ── render_partial: lambda → std::string ────────────────────────────

template<std::invocable<Html&> F>
auto render_partial(F&& fn, std::size_t cap = 4096) -> std::string {
    auto h = Html::with_capacity(cap);
    fn(h);
    return std::move(h).finish();
}

// ── render_page / render_page_full: declared here, defined after PageLayout ──

// Forward-declared; actual definitions in page_layout.hpp after PageLayout is defined.

} // namespace getgresql::ssr
