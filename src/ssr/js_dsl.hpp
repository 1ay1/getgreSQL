#pragma once

// ─── Compile-Time Typed JS Event & htmx DSL ─────────────────────────
//
// ZERO raw JS strings needed for inline handlers:
//
//   on_click(call("toggleTheme"))
//   on_click(confirm_then("Delete?", call("deleteItem", id)))
//   on_keydown(if_key("Escape", call("closePanel")))
//   on_click(then(copy_to_clipboard(sql), set_text("Copied!")))
//
// htmx:
//   hx::post("/api/save").target("#r").swap("innerHTML").confirm("Sure?")
//
// Large JS blocks stay as constexpr string_view in js() methods —
// the DSL covers inline event handlers, not application logic.
//

#include "ssr/html_dsl.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr::js {

// ─── JS Expression (typed wrapper) ───────────────────────────────────

struct Expr {
    std::string code;
};

// ── Constructors ─────────────────────────────────────────────────────

inline auto call(std::string_view fn) -> Expr {
    return {std::string(fn) + "()"};
}
inline auto call(std::string_view fn, std::string_view a1) -> Expr {
    return {std::string(fn) + "('" + std::string(a1) + "')"};
}
inline auto call(std::string_view fn, std::string_view a1, std::string_view a2) -> Expr {
    return {std::string(fn) + "('" + std::string(a1) + "','" + std::string(a2) + "')"};
}
inline auto call(std::string_view fn, std::string_view a1, std::string_view a2, std::string_view a3) -> Expr {
    return {std::string(fn) + "('" + std::string(a1) + "','" + std::string(a2) + "','" + std::string(a3) + "')"};
}
inline auto call_n(std::string_view fn, int arg) -> Expr {
    return {std::string(fn) + "(" + std::to_string(arg) + ")"};
}

// ── Combinators ──────────────────────────────────────────────────────

inline auto then(Expr a, Expr b) -> Expr { return {a.code + ";" + b.code}; }
inline auto confirm_then(std::string_view msg, Expr action) -> Expr {
    return {"if(confirm('" + std::string(msg) + "')){" + action.code + "}"};
}
inline auto prevent_default(Expr action) -> Expr {
    return {"event.preventDefault();" + action.code};
}
inline auto if_key(std::string_view key, Expr action) -> Expr {
    return {"if(event.key==='" + std::string(key) + "'){" + action.code + "}"};
}
inline auto if_ctrl_key(std::string_view key, Expr action) -> Expr {
    return {"if((event.ctrlKey||event.metaKey)&&event.key==='" + std::string(key) + "'){event.preventDefault();" + action.code + "}"};
}

// ── Common actions ───────────────────────────────────────────────────

inline auto copy_to_clipboard(std::string_view text) -> Expr {
    return {"navigator.clipboard.writeText('" + std::string(text) + "')"};
}
inline auto set_text(std::string_view text) -> Expr {
    return {"this.textContent='" + std::string(text) + "'"};
}
inline auto select_this() -> Expr { return {"this.select()"}; }
inline auto remove_closest(std::string_view selector) -> Expr {
    return {"this.closest('" + std::string(selector) + "').remove()"};
}
inline auto navigate(std::string_view url) -> Expr {
    return {"window.spaNavigate?spaNavigate('" + std::string(url) + "'):window.location='" + std::string(url) + "'"};
}
inline auto raw_js(std::string_view code) -> Expr { return {std::string(code)}; }

// ── ALL Event Handlers → html::Attr ──────────────────────────────────

inline auto on_click(Expr e) -> html::Attr { return {"onclick", std::move(e.code)}; }
inline auto on_dblclick(Expr e) -> html::Attr { return {"ondblclick", std::move(e.code)}; }
inline auto on_change(Expr e) -> html::Attr { return {"onchange", std::move(e.code)}; }
inline auto on_submit(Expr e) -> html::Attr { return {"onsubmit", std::move(e.code)}; }
inline auto on_input(Expr e) -> html::Attr { return {"oninput", std::move(e.code)}; }
inline auto on_keydown(Expr e) -> html::Attr { return {"onkeydown", std::move(e.code)}; }
inline auto on_keyup(Expr e) -> html::Attr { return {"onkeyup", std::move(e.code)}; }
inline auto on_focus(Expr e) -> html::Attr { return {"onfocus", std::move(e.code)}; }
inline auto on_blur(Expr e) -> html::Attr { return {"onblur", std::move(e.code)}; }
inline auto on_mouseenter(Expr e) -> html::Attr { return {"onmouseenter", std::move(e.code)}; }
inline auto on_mouseleave(Expr e) -> html::Attr { return {"onmouseleave", std::move(e.code)}; }
inline auto on_mousedown(Expr e) -> html::Attr { return {"onmousedown", std::move(e.code)}; }
inline auto on_mouseup(Expr e) -> html::Attr { return {"onmouseup", std::move(e.code)}; }
inline auto on_contextmenu(Expr e) -> html::Attr { return {"oncontextmenu", std::move(e.code)}; }
inline auto on_paste(Expr e) -> html::Attr { return {"onpaste", std::move(e.code)}; }
inline auto on_scroll(Expr e) -> html::Attr { return {"onscroll", std::move(e.code)}; }
inline auto on_load(Expr e) -> html::Attr { return {"onload", std::move(e.code)}; }

// ── Keyboard shortcut presets ─────────────────────────────────────────

inline auto if_enter(Expr action) -> Expr { return if_key("Enter", action); }
inline auto if_escape(Expr action) -> Expr { return if_key("Escape", action); }
inline auto if_tab(Expr action) -> Expr { return if_key("Tab", action); }
inline auto if_ctrl_enter(Expr action) -> Expr { return if_ctrl_key("Enter", action); }
inline auto if_ctrl_s(Expr action) -> Expr { return if_ctrl_key("s", action); }
inline auto if_ctrl_k(Expr action) -> Expr { return if_ctrl_key("k", action); }

// ── Stop propagation combinators ─────────────────────────────────────

inline auto stop_propagation() -> Expr { return {"event.stopPropagation()"}; }
inline auto stop_then(Expr action) -> Expr {
    return {"event.stopPropagation();" + action.code};
}

} // namespace getgresql::ssr::js

// ─── htmx Behavior Builder ──────────────────────────────────────────

namespace getgresql::ssr::hx {

class Behavior {
    std::vector<html::Attr> attrs_;
public:
    // HTTP methods
    auto get(std::string_view url) -> Behavior& { attrs_.push_back({"hx-get", std::string(url)}); return *this; }
    auto post(std::string_view url) -> Behavior& { attrs_.push_back({"hx-post", std::string(url)}); return *this; }
    auto put(std::string_view url) -> Behavior& { attrs_.push_back({"hx-put", std::string(url)}); return *this; }
    auto del(std::string_view url) -> Behavior& { attrs_.push_back({"hx-delete", std::string(url)}); return *this; }
    auto patch(std::string_view url) -> Behavior& { attrs_.push_back({"hx-patch", std::string(url)}); return *this; }

    // Targeting
    auto target(std::string_view sel) -> Behavior& { attrs_.push_back({"hx-target", std::string(sel)}); return *this; }
    auto swap(std::string_view s) -> Behavior& { attrs_.push_back({"hx-swap", std::string(s)}); return *this; }
    auto select(std::string_view sel) -> Behavior& { attrs_.push_back({"hx-select", std::string(sel)}); return *this; }

    // Triggering
    auto trigger(std::string_view t) -> Behavior& { attrs_.push_back({"hx-trigger", std::string(t)}); return *this; }
    auto confirm(std::string_view msg) -> Behavior& { attrs_.push_back({"hx-confirm", std::string(msg)}); return *this; }

    // Data
    auto vals(std::string_view json) -> Behavior& { attrs_.push_back({"hx-vals", std::string(json)}); return *this; }
    auto include(std::string_view sel) -> Behavior& { attrs_.push_back({"hx-include", std::string(sel)}); return *this; }
    auto params(std::string_view p) -> Behavior& { attrs_.push_back({"hx-params", std::string(p)}); return *this; }
    auto headers(std::string_view h) -> Behavior& { attrs_.push_back({"hx-headers", std::string(h)}); return *this; }
    auto encoding(std::string_view e) -> Behavior& { attrs_.push_back({"hx-encoding", std::string(e)}); return *this; }

    // Behavior
    auto push_url(bool v = true) -> Behavior& { attrs_.push_back({"hx-push-url", v ? "true" : "false"}); return *this; }
    auto boost(bool v = true) -> Behavior& { attrs_.push_back({"hx-boost", v ? "true" : "false"}); return *this; }
    auto indicator(std::string_view sel) -> Behavior& { attrs_.push_back({"hx-indicator", std::string(sel)}); return *this; }
    auto sync(std::string_view s) -> Behavior& { attrs_.push_back({"hx-sync", std::string(s)}); return *this; }
    auto preserve() -> Behavior& { attrs_.push_back({"hx-preserve", "true"}); return *this; }
    auto disable() -> Behavior& { attrs_.push_back({"hx-disable", ""}); return *this; }
    auto disinherit(std::string_view d) -> Behavior& { attrs_.push_back({"hx-disinherit", std::string(d)}); return *this; }
    auto ext(std::string_view e) -> Behavior& { attrs_.push_back({"hx-ext", std::string(e)}); return *this; }
    auto on(std::string_view event, std::string_view handler) -> Behavior& {
        attrs_.push_back({"hx-on:" + std::string(event), std::string(handler)}); return *this;
    }

    auto attrs() const -> const std::vector<html::Attr>& { return attrs_; }
};

inline auto get(std::string_view url) -> Behavior { return Behavior{}.get(url); }
inline auto post(std::string_view url) -> Behavior { return Behavior{}.post(url); }
inline auto put(std::string_view url) -> Behavior { return Behavior{}.put(url); }
inline auto del(std::string_view url) -> Behavior { return Behavior{}.del(url); }

// Preset: debounced search input that targets a results container
inline auto search(std::string_view url, std::string_view target_id) -> Behavior {
    return Behavior{}.get(url)
        .trigger("input changed delay:300ms, search")
        .target("#" + std::string(target_id));
}

// Preset: load once (htmx lazy section)
inline auto load_once(std::string_view url) -> Behavior {
    return Behavior{}.get(url).trigger("load").swap("innerHTML");
}

// Preset: polling section
inline auto poll(std::string_view url, std::string_view interval) -> Behavior {
    return Behavior{}.get(url).trigger("load, every " + std::string(interval)).swap("innerHTML");
}

} // namespace getgresql::ssr::hx
