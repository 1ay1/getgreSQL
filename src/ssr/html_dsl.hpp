#pragma once

// ─── Compile-Time Type-Theoretic HTML DSL ────────────────────────────
//
// Design principles:
//   1. Tag names are TYPES — typos are compile errors
//   2. Scoped elements use RAII — unclosed tags are impossible
//   3. Text content is escaped by default — XSS impossible by construction
//   4. Attributes are escaped — injection impossible
//   5. Zero overhead — compiles to the same raw() / text() calls
//
// Usage:
//
//   using namespace getgresql::ssr::html;
//
//   // Scoped element (RAII closes </div> automatically)
//   {
//       auto _ = open<Div>(h, {cls("container"), id("main")});
//       el<H3>(h, {cls("title")}, "Hello");
//       el<Span>(h, {cls("badge")}, count_str);
//       {
//           auto _ = open<P>(h, {cls("desc")});
//           h.text(user_input);  // safely escaped
//       }  // </p>
//   }  // </div>
//
//   // Void elements (no closing tag)
//   void_el<Input>(h, {type("text"), name("q"), placeholder("Search...")});
//   void_el<Br>(h);
//
//   // Element with raw HTML content (explicit opt-in)
//   el_raw<Td>(h, {}, "&#9654; <strong>bold</strong>");
//
//   // HTMX attributes
//   el<Button>(h, {cls("btn"), hx_post("/api/do"), hx_confirm("Sure?")}, "Go");
//

#include "ssr/engine.hpp"

#include <initializer_list>
#include <string>
#include <string_view>

namespace getgresql::ssr::html {

// ─── Attribute ───────────────────────────────────────────────────────
// Key is trusted (compile-time string literal), value is escaped on render.

struct Attr {
    std::string key;
    std::string value;
};

// Write attributes to buffer. Values are HTML-escaped via text().
inline void write_attrs(Html& h, std::initializer_list<Attr> attrs) {
    for (auto& a : attrs) {
        h.raw(" ").raw(a.key).raw("=\"").text(a.value).raw("\"");
    }
}

// ─── Tag Types ───────────────────────────────────────────────────────
// Each HTML tag is a distinct type. Using a non-existent tag = compile error.
// The `name` member is constexpr — zero runtime cost.

#define HTML_TAG(Name, str) struct Name { static constexpr const char* name = str; }

// Document structure
HTML_TAG(Html_,  "html");
HTML_TAG(Head,   "head");
HTML_TAG(Body,   "body");
HTML_TAG(Main,   "main");

// Sections
HTML_TAG(Div,     "div");
HTML_TAG(Span,    "span");
HTML_TAG(Section, "section");
HTML_TAG(Article, "article");
HTML_TAG(Aside,   "aside");
HTML_TAG(Header,  "header");
HTML_TAG(Footer,  "footer");
HTML_TAG(Nav,     "nav");

// Headings
HTML_TAG(H1, "h1");
HTML_TAG(H2, "h2");
HTML_TAG(H3, "h3");
HTML_TAG(H4, "h4");
HTML_TAG(H5, "h5");
HTML_TAG(H6, "h6");

// Text
HTML_TAG(P,      "p");
HTML_TAG(A,      "a");
HTML_TAG(Strong, "strong");
HTML_TAG(Em,     "em");
HTML_TAG(Code,   "code");
HTML_TAG(Pre,    "pre");
HTML_TAG(Small,  "small");
HTML_TAG(Label,  "label");
HTML_TAG(Kbd,    "kbd");

// Lists
HTML_TAG(Ul, "ul");
HTML_TAG(Ol, "ol");
HTML_TAG(Li, "li");

// Table
HTML_TAG(Table_, "table");
HTML_TAG(Thead,  "thead");
HTML_TAG(Tbody,  "tbody");
HTML_TAG(Tr,     "tr");
HTML_TAG(Th,     "th");
HTML_TAG(Td,     "td");

// Forms
HTML_TAG(Form,     "form");
HTML_TAG(Button,   "button");
HTML_TAG(Select,   "select");
HTML_TAG(Option,   "option");
HTML_TAG(Textarea, "textarea");

// Void elements (no closing tag)
HTML_TAG(Input, "input");
HTML_TAG(Br,    "br");
HTML_TAG(Hr,    "hr");
HTML_TAG(Img,   "img");
HTML_TAG(Meta,  "meta");
HTML_TAG(Link,  "link");

// Void elements (more)
HTML_TAG(Source, "source");
HTML_TAG(Track,  "track");

// Media / misc
HTML_TAG(Svg,      "svg");
HTML_TAG(Circle,   "circle");
HTML_TAG(Path,     "path");
HTML_TAG(Line,     "line");
HTML_TAG(Rect,     "rect");
HTML_TAG(Title,    "title");
HTML_TAG(Script,   "script");
HTML_TAG(Style,    "style");
HTML_TAG(Datalist, "datalist");
HTML_TAG(Fieldset, "fieldset");
HTML_TAG(Legend,   "legend");
HTML_TAG(Details,  "details");
HTML_TAG(Summary,  "summary");
HTML_TAG(Dialog,   "dialog");
HTML_TAG(Canvas,   "canvas");
HTML_TAG(Video,    "video");
HTML_TAG(Audio,    "audio");
HTML_TAG(Iframe,   "iframe");
HTML_TAG(Template_,"template");

#undef HTML_TAG

// ─── Scoped Element (RAII) ───────────────────────────────────────────
// Opening a scoped element returns a Scope. When the Scope is destroyed,
// the closing tag is emitted. Move-only to prevent double-close.
//
// Invalid states are unrepresentable:
//   - Cannot forget to close (destructor handles it)
//   - Cannot close twice (move-only)
//   - Cannot use after move (moved_ flag)

class Scope {
    Html& h_;
    const char* tag_;
    bool moved_ = false;

public:
    Scope(Html& h, const char* tag) : h_(h), tag_(tag) {}
    ~Scope() { if (!moved_) { h_.raw("</").raw(tag_).raw(">"); } }

    Scope(Scope&& o) noexcept : h_(o.h_), tag_(o.tag_) { o.moved_ = true; }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    Scope& operator=(Scope&&) = delete;
};

// ─── Core DSL Functions ──────────────────────────────────────────────

// open<Tag>(h, {attrs...}) → Scope
// Opens a tag, returns RAII scope that closes it on destruction.
template<typename Tag>
[[nodiscard]] auto open(Html& h, std::initializer_list<Attr> attrs = {}) -> Scope {
    h.raw("<").raw(Tag::name);
    write_attrs(h, attrs);
    h.raw(">");
    return Scope{h, Tag::name};
}

// el<Tag>(h, {attrs...}, text_content)
// Writes a complete element with ESCAPED text content.
template<typename Tag>
void el(Html& h, std::initializer_list<Attr> attrs, std::string_view content) {
    h.raw("<").raw(Tag::name);
    write_attrs(h, attrs);
    h.raw(">").text(content).raw("</").raw(Tag::name).raw(">");
}

// el<Tag>(h, {attrs...})
// Writes a complete element with no content.
template<typename Tag>
void el(Html& h, std::initializer_list<Attr> attrs = {}) {
    h.raw("<").raw(Tag::name);
    write_attrs(h, attrs);
    h.raw("></").raw(Tag::name).raw(">");
}

// el_raw<Tag>(h, {attrs...}, raw_html)
// Writes a complete element with RAW (unescaped) HTML content.
// Use only for trusted content (icons, nested components).
template<typename Tag>
void el_raw(Html& h, std::initializer_list<Attr> attrs, std::string_view raw_content) {
    h.raw("<").raw(Tag::name);
    write_attrs(h, attrs);
    h.raw(">").raw(raw_content).raw("</").raw(Tag::name).raw(">");
}

// void_el<Tag>(h, {attrs...})
// Writes a void element (no closing tag): <input ...>, <br>, <hr>, etc.
template<typename Tag>
void void_el(Html& h, std::initializer_list<Attr> attrs = {}) {
    h.raw("<").raw(Tag::name);
    write_attrs(h, attrs);
    h.raw(">");
}

// ─── Attribute Constructors ──────────────────────────────────────────
// Each returns an Attr. Values are escaped when written to the buffer.

// Standard HTML attributes
inline auto cls(std::string_view v) -> Attr { return {"class", std::string(v)}; }
inline auto id(std::string_view v) -> Attr { return {"id", std::string(v)}; }
inline auto style(std::string_view v) -> Attr { return {"style", std::string(v)}; }
inline auto href(std::string_view v) -> Attr { return {"href", std::string(v)}; }
inline auto src(std::string_view v) -> Attr { return {"src", std::string(v)}; }
inline auto title(std::string_view v) -> Attr { return {"title", std::string(v)}; }
inline auto type(std::string_view v) -> Attr { return {"type", std::string(v)}; }
inline auto name(std::string_view v) -> Attr { return {"name", std::string(v)}; }
inline auto value(std::string_view v) -> Attr { return {"value", std::string(v)}; }
inline auto placeholder(std::string_view v) -> Attr { return {"placeholder", std::string(v)}; }
inline auto onclick(std::string_view v) -> Attr { return {"onclick", std::string(v)}; }
inline auto tabindex(std::string_view v) -> Attr { return {"tabindex", std::string(v)}; }
inline auto autocomplete(std::string_view v) -> Attr { return {"autocomplete", std::string(v)}; }

// Data attributes
inline auto data(std::string_view key, std::string_view v) -> Attr {
    return {"data-" + std::string(key), std::string(v)};
}

// Form attributes
inline auto action(std::string_view v) -> Attr { return {"action", std::string(v)}; }
inline auto method(std::string_view v) -> Attr { return {"method", std::string(v)}; }
inline auto for_(std::string_view v) -> Attr { return {"for", std::string(v)}; }
inline auto min(std::string_view v) -> Attr { return {"min", std::string(v)}; }
inline auto max(std::string_view v) -> Attr { return {"max", std::string(v)}; }
inline auto step(std::string_view v) -> Attr { return {"step", std::string(v)}; }
inline auto rows(std::string_view v) -> Attr { return {"rows", std::string(v)}; }
inline auto cols(std::string_view v) -> Attr { return {"cols", std::string(v)}; }
inline auto spellcheck(std::string_view v) -> Attr { return {"spellcheck", std::string(v)}; }
inline auto autocapitalize(std::string_view v) -> Attr { return {"autocapitalize", std::string(v)}; }
inline auto autocorrect(std::string_view v) -> Attr { return {"autocorrect", std::string(v)}; }
inline auto wrap(std::string_view v) -> Attr { return {"wrap", std::string(v)}; }
inline auto list(std::string_view v) -> Attr { return {"list", std::string(v)}; }
inline auto pattern(std::string_view v) -> Attr { return {"pattern", std::string(v)}; }
inline auto maxlength(int n) -> Attr { return {"maxlength", std::to_string(n)}; }
inline auto download(std::string_view v = "") -> Attr { return {"download", std::string(v)}; }
inline auto target(std::string_view v) -> Attr { return {"target", std::string(v)}; }
inline auto rel(std::string_view v) -> Attr { return {"rel", std::string(v)}; }

// Accessibility
inline auto role(std::string_view v) -> Attr { return {"role", std::string(v)}; }
inline auto aria(std::string_view key, std::string_view v) -> Attr {
    return {"aria-" + std::string(key), std::string(v)};
}
inline auto aria_hidden() -> Attr { return {"aria-hidden", "true"}; }
inline auto aria_label(std::string_view v) -> Attr { return {"aria-label", std::string(v)}; }

// Boolean attributes
inline auto required() -> Attr { return {"required", ""}; }
inline auto disabled() -> Attr { return {"disabled", ""}; }
inline auto checked() -> Attr { return {"checked", ""}; }
inline auto autofocus() -> Attr { return {"autofocus", ""}; }
inline auto readonly() -> Attr { return {"readonly", ""}; }
inline auto multiple() -> Attr { return {"multiple", ""}; }
inline auto selected() -> Attr { return {"selected", ""}; }
inline auto open_attr() -> Attr { return {"open", ""}; }

// Generic attribute escape hatch
inline auto attr(std::string_view key, std::string_view v) -> Attr {
    return {std::string(key), std::string(v)};
}

// HTMX attributes (kept for backward compat, prefer hx:: builder)
inline auto hx_get(std::string_view v) -> Attr { return {"hx-get", std::string(v)}; }
inline auto hx_post(std::string_view v) -> Attr { return {"hx-post", std::string(v)}; }
inline auto hx_put(std::string_view v) -> Attr { return {"hx-put", std::string(v)}; }
inline auto hx_delete(std::string_view v) -> Attr { return {"hx-delete", std::string(v)}; }
inline auto hx_target(std::string_view v) -> Attr { return {"hx-target", std::string(v)}; }
inline auto hx_swap(std::string_view v) -> Attr { return {"hx-swap", std::string(v)}; }
inline auto hx_trigger(std::string_view v) -> Attr { return {"hx-trigger", std::string(v)}; }
inline auto hx_confirm(std::string_view v) -> Attr { return {"hx-confirm", std::string(v)}; }
inline auto hx_vals(std::string_view v) -> Attr { return {"hx-vals", std::string(v)}; }
inline auto hx_include(std::string_view v) -> Attr { return {"hx-include", std::string(v)}; }

// ─── Attribute concatenation helper ──────────────────────────────────
// For dynamic class names: cls("btn " + variant)
// For joining: cls(join("btn", variant, size))

inline auto join(auto&&... parts) -> std::string {
    std::string result;
    ((result += std::string(parts), result += ' '), ...);
    if (!result.empty()) result.pop_back(); // remove trailing space
    return result;
}

} // namespace getgresql::ssr::html

// ─── Icon Constants ──────────────────────────────────────────────────
// Named constants for HTML entities used as icons throughout the UI.
// Usage: el_raw<Span>(h, {cls("icon")}, icon::check);

namespace getgresql::ssr::icon {

inline constexpr auto check       = "&#10003;";
inline constexpr auto cross       = "&#10007;";
inline constexpr auto warning     = "&#9888;";
inline constexpr auto info        = "&#8505;";
inline constexpr auto play        = "&#9654;";
inline constexpr auto circle      = "&#9679;";
inline constexpr auto circle_o    = "&#9673;";
inline constexpr auto diamond     = "&#9671;";
inline constexpr auto chevron_r   = "&#9656;";
inline constexpr auto arrow_r     = "&rsaquo;";
inline constexpr auto arrow_lr    = "&laquo;";
inline constexpr auto arrow_l     = "&lsaquo;";
inline constexpr auto dash        = "&mdash;";
inline constexpr auto ellipsis    = "&hellip;";
inline constexpr auto times       = "&times;";
inline constexpr auto reload      = "&#8635;";
inline constexpr auto minus       = "&#8722;";
inline constexpr auto hamburger   = "&#9776;";
inline constexpr auto contrast    = "&#9680;";
inline constexpr auto command     = "&#8984;";
inline constexpr auto download    = "&#8615;";
inline constexpr auto lightning   = "&#9889;";
inline constexpr auto gear        = "&#9881;";
inline constexpr auto bulb        = "&#128161;";
inline constexpr auto link_icon   = "&#128268;";
inline constexpr auto search      = "&#128269;";
inline constexpr auto dot_bullet  = "&#8226;";
inline constexpr auto hash        = "#";
inline constexpr auto fn          = "&#402;";
inline constexpr auto pawn        = "&#9823;";
inline constexpr auto ornament    = "&#10070;";

} // namespace getgresql::ssr::icon

// ─── Markup Helpers (pre-rendered HTML strings for table cells) ───────
// These return std::string containing safe HTML for use in Table::row,
// tooltips, or any context that needs a pre-rendered HTML fragment.
//
// All text content is HTML-escaped. Raw HTML is only used for tags.
//
// Usage:
//   Table::row(h, {
//       markup::strong("Alice"),
//       markup::code("SELECT 1"),
//       markup::link("/users/1", "View"),
//       markup::btn("Delete").danger().hx_post("/del/1").confirm("Sure?"),
//   });

namespace getgresql::ssr::markup {

// Internal: escape text to string
namespace detail {
inline auto esc(std::string_view s) -> std::string {
    auto h = Html::with_capacity(s.size() + 32);
    h.text(s);
    return std::move(h).finish();
}
} // namespace detail

// ── Simple element wrappers (all text-escaped) ──────────────────────

inline auto strong(std::string_view text) -> std::string {
    return "<strong>" + detail::esc(text) + "</strong>";
}

inline auto em(std::string_view text) -> std::string {
    return "<em>" + detail::esc(text) + "</em>";
}

inline auto code(std::string_view text) -> std::string {
    return "<code>" + detail::esc(text) + "</code>";
}

inline auto code_cls(std::string_view cls, std::string_view text) -> std::string {
    return "<code class=\"" + std::string(cls) + "\">" + detail::esc(text) + "</code>";
}

inline auto kbd(std::string_view text) -> std::string {
    return "<kbd>" + detail::esc(text) + "</kbd>";
}

inline auto small(std::string_view text) -> std::string {
    return "<small>" + detail::esc(text) + "</small>";
}

// ── Links (href + text both escaped) ────────────────────────────────

inline auto link(std::string_view url, std::string_view text) -> std::string {
    return "<a href=\"" + detail::esc(url) + "\">" + detail::esc(text) + "</a>";
}

inline auto spa_link(std::string_view url, std::string_view text) -> std::string {
    return "<a href=\"" + detail::esc(url) + "\" data-spa>" + detail::esc(text) + "</a>";
}

inline auto link_cls(std::string_view cls, std::string_view url, std::string_view text) -> std::string {
    return "<a href=\"" + detail::esc(url) + "\" class=\"" + std::string(cls) + "\">" + detail::esc(text) + "</a>";
}

inline auto spa_link_raw(std::string_view cls, std::string_view url, std::string_view raw_html) -> std::string {
    return "<a href=\"" + detail::esc(url) + "\" class=\"" + std::string(cls) + "\" data-spa>" + std::string(raw_html) + "</a>";
}

// ── Boolean display ─────────────────────────────────────────────────

inline auto bool_check(bool v) -> std::string {
    return v ? std::string(icon::check) : "";
}

inline auto bool_yes_no(bool v) -> std::string {
    return v ? "<span class=\"badge success\">YES</span>"
             : "<span class=\"badge secondary\">NO</span>";
}

// ── Special values ──────────────────────────────────────────────────

inline auto mdash() -> std::string { return "&mdash;"; }
inline auto empty() -> std::string { return ""; }

// ── Composable: join multiple fragments ─────────────────────────────

inline auto cat(std::initializer_list<std::string_view> parts) -> std::string {
    std::string out;
    for (auto& p : parts) out += p;
    return out;
}

// ─── Action Button Builder ──────────────────────────────────────────
// Fluent API for building htmx action buttons in table cells.
//
// Usage:
//   markup::btn("Cancel").danger().hx_post("/cancel/123").confirm("Cancel PID 123?")
//   markup::btn("Drop").danger().hx_post("/drop").vals("{\"col\":\"name\"}").confirm("Drop?")

class BtnBuilder {
    std::string label_;
    std::string cls_ = "btn btn-sm";
    std::string method_attr_;
    std::string url_;
    std::string confirm_;
    std::string target_;
    std::string swap_;
    std::string vals_;
    std::string include_;

public:
    explicit BtnBuilder(std::string_view label) : label_(label) {}

    auto primary()  -> BtnBuilder& { cls_ += " btn-primary"; return *this; }
    auto danger()   -> BtnBuilder& { cls_ += " btn-danger"; return *this; }
    auto warning_() -> BtnBuilder& { cls_ += " btn-warning"; return *this; }
    auto ghost()    -> BtnBuilder& { cls_ += " btn-ghost"; return *this; }

    auto hx_post(std::string_view url)   -> BtnBuilder& { method_attr_ = "hx-post"; url_ = url; return *this; }
    auto hx_get(std::string_view url)    -> BtnBuilder& { method_attr_ = "hx-get"; url_ = url; return *this; }
    auto hx_delete(std::string_view url) -> BtnBuilder& { method_attr_ = "hx-delete"; url_ = url; return *this; }
    auto confirm(std::string_view msg)   -> BtnBuilder& { confirm_ = msg; return *this; }
    auto target(std::string_view sel)    -> BtnBuilder& { target_ = sel; return *this; }
    auto swap(std::string_view s)        -> BtnBuilder& { swap_ = s; return *this; }
    auto vals(std::string_view json)     -> BtnBuilder& { vals_ = json; return *this; }
    auto include(std::string_view sel)   -> BtnBuilder& { include_ = sel; return *this; }

    // Implicit conversion to string (for use in Table::row cells)
    operator std::string() const { return build(); }

    auto build() const -> std::string {
        std::string out = "<button class=\"" + cls_ + "\"";
        if (!method_attr_.empty()) out += " " + method_attr_ + "=\"" + detail::esc(url_) + "\"";
        if (!confirm_.empty()) out += " hx-confirm=\"" + detail::esc(confirm_) + "\"";
        if (!target_.empty()) out += " hx-target=\"" + detail::esc(target_) + "\"";
        if (!swap_.empty()) out += " hx-swap=\"" + detail::esc(swap_) + "\"";
        if (!vals_.empty()) out += " hx-vals='" + std::string(vals_) + "'";
        if (!include_.empty()) out += " hx-include=\"" + detail::esc(include_) + "\"";
        out += ">" + detail::esc(label_) + "</button>";
        return out;
    }
};

inline auto btn(std::string_view label) -> BtnBuilder {
    return BtnBuilder(label);
}

} // namespace getgresql::ssr::markup
