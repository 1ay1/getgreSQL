#pragma once

// ─── Declarative UI Composition DSL ──────────────────────────────────
//
// Compose components declaratively with typed props:
//
//   using namespace getgresql::ssr::ui;
//
//   // Render a component:
//   render<StatCard>(h, {.label = "Cache", .value = "99.1%"});
//
//   // Conditional rendering:
//   render_if<Alert>(h, has_error, {error_msg, "error"});
//
//   // List rendering:
//   each(h, items, [](Html& h, auto& item) {
//       render<StatCard>(h, {.label = item.name, .value = item.val});
//   });
//
//   // Container with children:
//   Card::render({"Health"}, h, [&](Html& h) {
//       render<StatCard>(h, health_props);
//   });
//
//   // Element with htmx behavior:
//   auto _ = ui::open<Div>(h, {cls("grid")},
//       hx::get("/refresh").trigger("every 5s").swap("innerHTML"));
//

#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/js_dsl.hpp"

#include <concepts>

namespace getgresql::ssr::ui {

// ─── Component Rendering ─────────────────────────────────────────────

// Render a component with props
template<Component C>
inline void render(Html& h, const typename C::Props& props) {
    C::render(props, h);
}

// Render a container component with children
template<typename C, std::invocable<Html&> F>
    requires requires(typename C::Props p, Html& h, F f) { C::render(p, h, f); }
inline void render(Html& h, const typename C::Props& props, F&& children) {
    C::render(props, h, std::forward<F>(children));
}

// ─── Conditional Rendering ───────────────────────────────────────────

// Render component only if condition is true
template<Component C>
inline void render_if(Html& h, bool condition, const typename C::Props& props) {
    if (condition) C::render(props, h);
}

// Render arbitrary content only if condition is true
template<std::invocable<Html&> F>
inline void when(Html& h, bool condition, F&& fn) {
    if (condition) fn(h);
}

// Render one of two branches
template<std::invocable<Html&> T, std::invocable<Html&> F>
inline void if_else(Html& h, bool condition, T&& then_fn, F&& else_fn) {
    if (condition) then_fn(h);
    else else_fn(h);
}

// ─── List Rendering ─────────────────────────────────────────────────

// Render each item via a lambda
template<typename Range, typename F>
    requires std::invocable<F, Html&, const typename Range::value_type&>
inline void each(Html& h, const Range& items, F&& fn) {
    for (auto& item : items) {
        fn(h, item);
    }
}

// Render each item as a component via a transform function
template<Component C, typename Range, typename Transform>
    requires std::invocable<Transform, const typename Range::value_type&>
inline void render_each(Html& h, const Range& items, Transform&& to_props) {
    for (auto& item : items) {
        C::render(to_props(item), h);
    }
}

// ─── htmx-Enhanced Elements ─────────────────────────────────────────

// Write html attrs + htmx behavior attrs together
inline void write_merged_attrs(Html& h,
                                std::initializer_list<html::Attr> attrs,
                                const hx::Behavior& behavior) {
    html::write_attrs(h, attrs);
    for (auto& a : behavior.attrs()) {
        h.raw(" ").raw(a.key).raw("=\"").text(a.value).raw("\"");
    }
}

// Open a scoped element with both regular attrs and htmx behavior
template<typename Tag>
[[nodiscard]] inline auto open(Html& h,
                                std::initializer_list<html::Attr> attrs,
                                const hx::Behavior& behavior) -> html::Scope {
    h.raw("<").raw(Tag::name);
    write_merged_attrs(h, attrs, behavior);
    h.raw(">");
    return html::Scope{h, Tag::name};
}

// Complete element with htmx behavior and text content
template<typename Tag>
inline void el(Html& h,
               std::initializer_list<html::Attr> attrs,
               const hx::Behavior& behavior,
               std::string_view content) {
    h.raw("<").raw(Tag::name);
    write_merged_attrs(h, attrs, behavior);
    h.raw(">").text(content).raw("</").raw(Tag::name).raw(">");
}

// Void element with htmx behavior
template<typename Tag>
inline void void_el(Html& h,
                    std::initializer_list<html::Attr> attrs,
                    const hx::Behavior& behavior) {
    h.raw("<").raw(Tag::name);
    write_merged_attrs(h, attrs, behavior);
    h.raw(">");
}

// ─── Text Helpers ────────────────────────────────────────────────────

// Escaped text node (safe)
inline void text(Html& h, std::string_view content) {
    h.text(content);
}

// Raw HTML node (trusted only)
inline void raw(Html& h, std::string_view content) {
    h.raw(content);
}

// ─── Empty State ─────────────────────────────────────────────────────
// Consistent "nothing here" message across the app.

inline void empty_state(Html& h, std::string_view message) {
    html::el<html::Div>(h, {html::cls("empty-state")}, message);
}

inline void empty_state(Html& h, std::string_view icon, std::string_view message) {
    auto _ = html::open<html::Div>(h, {html::cls("empty-state")});
    html::el_raw<html::Div>(h, {html::cls("empty-icon")}, icon);
    html::el<html::Div>(h, {}, message);
}

// ─── Stat Grid ──────────────────────────────────────────────────────
// Wraps children in .stat-grid layout. Eliminates raw h.open("div","class=\"stat-grid\"").

template<std::invocable<Html&> F>
inline void stat_grid(Html& h, F&& fn) {
    auto _ = html::open<html::Div>(h, {html::cls("stat-grid")});
    fn(h);
}

// ─── Health Grid ────────────────────────────────────────────────────

template<std::invocable<Html&> F>
inline void health_grid(Html& h, F&& fn) {
    auto _ = html::open<html::Div>(h, {html::cls("health-grid")});
    fn(h);
}

// ─── Loading placeholder ────────────────────────────────────────────

inline void loading(Html& h, std::string_view message = "Loading...") {
    html::el<html::Div>(h, {html::cls("loading")}, message);
}

// ─── Deferred section (htmx lazy-load placeholder) ──────────────────

inline void deferred(Html& h, std::string_view elem_id, std::string_view url,
                     std::string_view trigger = "load", std::string_view loading_text = "Loading...") {
    auto _ = html::open<html::Div>(h, {html::id(elem_id),
        html::hx_get(url), html::hx_trigger(trigger), html::hx_swap("innerHTML")});
    if (!loading_text.empty()) loading(h, loading_text);
}

// ─── Live section (htmx polling placeholder) ────────────────────────

inline void live_section(Html& h, std::string_view elem_id, std::string_view url,
                         std::string_view poll_interval, std::string_view loading_text = "Loading...") {
    auto trigger = "load, every " + std::string(poll_interval);
    deferred(h, elem_id, url, trigger, loading_text);
}

} // namespace getgresql::ssr::ui
