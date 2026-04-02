#pragma once

// ─── Compile-Time Typed CSS DSL ──────────────────────────────────────
//
// ZERO raw CSS needed. Every CSS feature is expressible:
//
//   Properties:     background(var("bg-1")), font_size(rem(1.3))
//   Pseudo-elements: rule("&::before", { content("\\2139"), font_size(rem(1)) })
//   @keyframes:     keyframes("spin", { {"to", { transform("rotate(360deg)") }} })
//   @media:         media("max-width: 900px", { rule(".grid", { grid_template_columns("1fr") }) })
//   Vendor prefix:  webkit("backdrop-filter", "blur(4px)")
//   Scoping:        s.scoped("my-component")
//

#include <format>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr::css {

// ─── Value Types ─────────────────────────────────────────────────────

struct Length { std::string value; };
struct Var    { std::string value; };
struct Color  { std::string value; };

// Length constructors
inline auto px(int n) -> Length { return {std::to_string(n) + "px"}; }
inline auto px(double n) -> Length { return {std::format("{:.1f}px", n)}; }
inline auto rem(double n) -> Length { return {std::format("{}rem", n)}; }
inline auto em(double n) -> Length { return {std::format("{}em", n)}; }
inline auto pct(double n) -> Length { return {std::format("{}%", n)}; }
inline auto vh(double n) -> Length { return {std::format("{}vh", n)}; }
inline auto vw(double n) -> Length { return {std::format("{}vw", n)}; }

// CSS custom property reference
inline auto var(std::string_view name) -> Var { return {"var(--" + std::string(name) + ")"}; }

// Colors
inline auto hex(std::string_view h) -> Color { return {std::string(h)}; }
inline auto rgb(int r, int g, int b) -> Color { return {std::format("rgb({},{},{})", r, g, b)}; }
inline auto rgba(int r, int g, int b, double a) -> Color { return {std::format("rgba({},{},{},{})", r, g, b, a)}; }
inline auto current_color() -> Color { return {"currentColor"}; }

// ─── CSS Declaration ─────────────────────────────────────────────────

struct CssProp {
    std::string_view property;
    std::string value;
};

// ─── Property Functions (comprehensive) ──────────────────────────────
// Every function is overloaded for Var, Length, Color, string_view as appropriate.

// Escape hatch — for ANY property
inline auto prop(std::string_view name, std::string_view value) -> CssProp { return {name, std::string(value)}; }

// Vendor-prefixed property
inline auto webkit(std::string_view name, std::string_view value) -> CssProp { return {"-webkit-" + std::string(name), std::string(value)}; }
inline auto moz(std::string_view name, std::string_view value) -> CssProp { return {"-moz-" + std::string(name), std::string(value)}; }

// ── Display & Layout ─────────────────────────────────────────────────
inline auto display(std::string_view v) -> CssProp { return {"display", std::string(v)}; }
inline auto display_flex() -> CssProp { return {"display", "flex"}; }
inline auto display_grid() -> CssProp { return {"display", "grid"}; }
inline auto display_block() -> CssProp { return {"display", "block"}; }
inline auto display_inline() -> CssProp { return {"display", "inline"}; }
inline auto display_inline_flex() -> CssProp { return {"display", "inline-flex"}; }
inline auto display_inline_block() -> CssProp { return {"display", "inline-block"}; }
inline auto display_none() -> CssProp { return {"display", "none"}; }
inline auto display_table() -> CssProp { return {"display", "table"}; }
inline auto display_table_cell() -> CssProp { return {"display", "table-cell"}; }
inline auto display_table_row() -> CssProp { return {"display", "table-row"}; }

inline auto flex(std::string_view v) -> CssProp { return {"flex", std::string(v)}; }
inline auto flex(int n) -> CssProp { return {"flex", std::to_string(n)}; }
inline auto flex_direction(std::string_view v) -> CssProp { return {"flex-direction", std::string(v)}; }
inline auto flex_column() -> CssProp { return {"flex-direction", "column"}; }
inline auto flex_row() -> CssProp { return {"flex-direction", "row"}; }
inline auto flex_wrap(std::string_view v) -> CssProp { return {"flex-wrap", std::string(v)}; }
inline auto flex_shrink(int n) -> CssProp { return {"flex-shrink", std::to_string(n)}; }
inline auto flex_grow(int n) -> CssProp { return {"flex-grow", std::to_string(n)}; }

inline auto align_items(std::string_view v) -> CssProp { return {"align-items", std::string(v)}; }
inline auto align_center() -> CssProp { return {"align-items", "center"}; }
inline auto align_start() -> CssProp { return {"align-items", "flex-start"}; }
inline auto align_end() -> CssProp { return {"align-items", "flex-end"}; }
inline auto align_baseline() -> CssProp { return {"align-items", "baseline"}; }
inline auto align_self(std::string_view v) -> CssProp { return {"align-self", std::string(v)}; }

inline auto justify_content(std::string_view v) -> CssProp { return {"justify-content", std::string(v)}; }
inline auto justify_center() -> CssProp { return {"justify-content", "center"}; }
inline auto justify_between() -> CssProp { return {"justify-content", "space-between"}; }
inline auto justify_start() -> CssProp { return {"justify-content", "flex-start"}; }
inline auto justify_end() -> CssProp { return {"justify-content", "flex-end"}; }

inline auto gap(Var v) -> CssProp { return {"gap", v.value}; }
inline auto gap(Length l) -> CssProp { return {"gap", l.value}; }
inline auto gap(std::string_view v) -> CssProp { return {"gap", std::string(v)}; }

inline auto grid_template_columns(std::string_view v) -> CssProp { return {"grid-template-columns", std::string(v)}; }
inline auto grid_template_rows(std::string_view v) -> CssProp { return {"grid-template-rows", std::string(v)}; }
inline auto grid_column(std::string_view v) -> CssProp { return {"grid-column", std::string(v)}; }
inline auto grid_row(std::string_view v) -> CssProp { return {"grid-row", std::string(v)}; }

// ── Sizing ───────────────────────────────────────────────────────────
inline auto width(Length l) -> CssProp { return {"width", l.value}; }
inline auto width(Var v) -> CssProp { return {"width", v.value}; }
inline auto width(std::string_view v) -> CssProp { return {"width", std::string(v)}; }
inline auto height(Length l) -> CssProp { return {"height", l.value}; }
inline auto height(Var v) -> CssProp { return {"height", v.value}; }
inline auto height(std::string_view v) -> CssProp { return {"height", std::string(v)}; }
inline auto min_width(Length l) -> CssProp { return {"min-width", l.value}; }
inline auto min_width(std::string_view v) -> CssProp { return {"min-width", std::string(v)}; }
inline auto min_height(Length l) -> CssProp { return {"min-height", l.value}; }
inline auto min_height(std::string_view v) -> CssProp { return {"min-height", std::string(v)}; }
inline auto max_width(Length l) -> CssProp { return {"max-width", l.value}; }
inline auto max_width(std::string_view v) -> CssProp { return {"max-width", std::string(v)}; }
inline auto max_height(Length l) -> CssProp { return {"max-height", l.value}; }
inline auto max_height(std::string_view v) -> CssProp { return {"max-height", std::string(v)}; }
inline auto aspect_ratio(std::string_view v) -> CssProp { return {"aspect-ratio", std::string(v)}; }

// ── Spacing ──────────────────────────────────────────────────────────
inline auto padding(Var v) -> CssProp { return {"padding", v.value}; }
inline auto padding(Length l) -> CssProp { return {"padding", l.value}; }
inline auto padding(std::string_view v) -> CssProp { return {"padding", std::string(v)}; }
inline auto padding_top(Var v) -> CssProp { return {"padding-top", v.value}; }
inline auto padding_bottom(Var v) -> CssProp { return {"padding-bottom", v.value}; }
inline auto padding_left(Var v) -> CssProp { return {"padding-left", v.value}; }
inline auto padding_right(Var v) -> CssProp { return {"padding-right", v.value}; }
inline auto margin(Var v) -> CssProp { return {"margin", v.value}; }
inline auto margin(Length l) -> CssProp { return {"margin", l.value}; }
inline auto margin(std::string_view v) -> CssProp { return {"margin", std::string(v)}; }
inline auto margin_top(Var v) -> CssProp { return {"margin-top", v.value}; }
inline auto margin_top(Length l) -> CssProp { return {"margin-top", l.value}; }
inline auto margin_bottom(Var v) -> CssProp { return {"margin-bottom", v.value}; }
inline auto margin_bottom(Length l) -> CssProp { return {"margin-bottom", l.value}; }
inline auto margin_left(Var v) -> CssProp { return {"margin-left", v.value}; }
inline auto margin_right(Var v) -> CssProp { return {"margin-right", v.value}; }

// ── Typography ───────────────────────────────────────────────────────
inline auto font_size(Var v) -> CssProp { return {"font-size", v.value}; }
inline auto font_size(Length l) -> CssProp { return {"font-size", l.value}; }
inline auto font_weight(int w) -> CssProp { return {"font-weight", std::to_string(w)}; }
inline auto font_family(Var v) -> CssProp { return {"font-family", v.value}; }
inline auto font_family(std::string_view v) -> CssProp { return {"font-family", std::string(v)}; }
inline auto font_style(std::string_view v) -> CssProp { return {"font-style", std::string(v)}; }
inline auto font_variant_numeric(std::string_view v) -> CssProp { return {"font-variant-numeric", std::string(v)}; }
inline auto line_height(double v) -> CssProp { return {"line-height", std::format("{}", v)}; }
inline auto letter_spacing(Length l) -> CssProp { return {"letter-spacing", l.value}; }
inline auto letter_spacing(std::string_view v) -> CssProp { return {"letter-spacing", std::string(v)}; }
inline auto text_align(std::string_view v) -> CssProp { return {"text-align", std::string(v)}; }
inline auto text_align_center() -> CssProp { return {"text-align", "center"}; }
inline auto text_align_right() -> CssProp { return {"text-align", "right"}; }
inline auto text_transform(std::string_view v) -> CssProp { return {"text-transform", std::string(v)}; }
inline auto text_decoration(std::string_view v) -> CssProp { return {"text-decoration", std::string(v)}; }
inline auto text_shadow(std::string_view v) -> CssProp { return {"text-shadow", std::string(v)}; }
inline auto text_overflow(std::string_view v) -> CssProp { return {"text-overflow", std::string(v)}; }
inline auto white_space(std::string_view v) -> CssProp { return {"white-space", std::string(v)}; }
inline auto word_break(std::string_view v) -> CssProp { return {"word-break", std::string(v)}; }
inline auto overflow_wrap(std::string_view v) -> CssProp { return {"overflow-wrap", std::string(v)}; }
inline auto user_select(std::string_view v) -> CssProp { return {"user-select", std::string(v)}; }
inline auto tab_size(int n) -> CssProp { return {"tab-size", std::to_string(n)}; }

// ── Color & Background ───────────────────────────────────────────────
inline auto color(Var v) -> CssProp { return {"color", v.value}; }
inline auto color(Color c) -> CssProp { return {"color", c.value}; }
inline auto color(std::string_view v) -> CssProp { return {"color", std::string(v)}; }
inline auto background(Var v) -> CssProp { return {"background", v.value}; }
inline auto background(Color c) -> CssProp { return {"background", c.value}; }
inline auto background(std::string_view v) -> CssProp { return {"background", std::string(v)}; }
inline auto background_color(std::string_view v) -> CssProp { return {"background-color", std::string(v)}; }
inline auto background_image(std::string_view v) -> CssProp { return {"background-image", std::string(v)}; }
inline auto background_size(std::string_view v) -> CssProp { return {"background-size", std::string(v)}; }
inline auto background_position(std::string_view v) -> CssProp { return {"background-position", std::string(v)}; }
inline auto background_repeat(std::string_view v) -> CssProp { return {"background-repeat", std::string(v)}; }
inline auto background_clip(std::string_view v) -> CssProp { return {"background-clip", std::string(v)}; }
inline auto opacity(double v) -> CssProp { return {"opacity", std::format("{}", v)}; }

// ── Border ───────────────────────────────────────────────────────────
inline auto border(std::string_view v) -> CssProp { return {"border", std::string(v)}; }
inline auto border_top(std::string_view v) -> CssProp { return {"border-top", std::string(v)}; }
inline auto border_bottom(std::string_view v) -> CssProp { return {"border-bottom", std::string(v)}; }
inline auto border_left(std::string_view v) -> CssProp { return {"border-left", std::string(v)}; }
inline auto border_right(std::string_view v) -> CssProp { return {"border-right", std::string(v)}; }
inline auto border_radius(Var v) -> CssProp { return {"border-radius", v.value}; }
inline auto border_radius(Length l) -> CssProp { return {"border-radius", l.value}; }
inline auto border_radius(std::string_view v) -> CssProp { return {"border-radius", std::string(v)}; }
inline auto border_color(Var v) -> CssProp { return {"border-color", v.value}; }
inline auto border_color(std::string_view v) -> CssProp { return {"border-color", std::string(v)}; }
inline auto border_collapse(std::string_view v) -> CssProp { return {"border-collapse", std::string(v)}; }
inline auto border_spacing(std::string_view v) -> CssProp { return {"border-spacing", std::string(v)}; }
inline auto outline(std::string_view v) -> CssProp { return {"outline", std::string(v)}; }
inline auto outline_offset(std::string_view v) -> CssProp { return {"outline-offset", std::string(v)}; }

// ── Effects ──────────────────────────────────────────────────────────
inline auto box_shadow(Var v) -> CssProp { return {"box-shadow", v.value}; }
inline auto box_shadow(std::string_view v) -> CssProp { return {"box-shadow", std::string(v)}; }
inline auto transition(std::string_view v) -> CssProp { return {"transition", std::string(v)}; }
inline auto transform(std::string_view v) -> CssProp { return {"transform", std::string(v)}; }
inline auto animation(std::string_view v) -> CssProp { return {"animation", std::string(v)}; }
inline auto filter(std::string_view v) -> CssProp { return {"filter", std::string(v)}; }
inline auto backdrop_filter(std::string_view v) -> CssProp { return {"backdrop-filter", std::string(v)}; }
inline auto cursor(std::string_view v) -> CssProp { return {"cursor", std::string(v)}; }
inline auto pointer_events(std::string_view v) -> CssProp { return {"pointer-events", std::string(v)}; }
inline auto visibility(std::string_view v) -> CssProp { return {"visibility", std::string(v)}; }
inline auto resize(std::string_view v) -> CssProp { return {"resize", std::string(v)}; }
inline auto appearance(std::string_view v) -> CssProp { return {"appearance", std::string(v)}; }
inline auto scroll_behavior(std::string_view v) -> CssProp { return {"scroll-behavior", std::string(v)}; }

// ── Position ─────────────────────────────────────────────────────────
inline auto position(std::string_view v) -> CssProp { return {"position", std::string(v)}; }
inline auto position_relative() -> CssProp { return {"position", "relative"}; }
inline auto position_absolute() -> CssProp { return {"position", "absolute"}; }
inline auto position_fixed() -> CssProp { return {"position", "fixed"}; }
inline auto position_sticky() -> CssProp { return {"position", "sticky"}; }
inline auto top(Length l) -> CssProp { return {"top", l.value}; }
inline auto top(std::string_view v) -> CssProp { return {"top", std::string(v)}; }
inline auto bottom(Length l) -> CssProp { return {"bottom", l.value}; }
inline auto bottom(std::string_view v) -> CssProp { return {"bottom", std::string(v)}; }
inline auto left(Length l) -> CssProp { return {"left", l.value}; }
inline auto left(std::string_view v) -> CssProp { return {"left", std::string(v)}; }
inline auto right(Length l) -> CssProp { return {"right", l.value}; }
inline auto right(std::string_view v) -> CssProp { return {"right", std::string(v)}; }
inline auto inset(std::string_view v) -> CssProp { return {"inset", std::string(v)}; }
inline auto z_index(int n) -> CssProp { return {"z-index", std::to_string(n)}; }

// ── Overflow ─────────────────────────────────────────────────────────
inline auto overflow(std::string_view v) -> CssProp { return {"overflow", std::string(v)}; }
inline auto overflow_hidden() -> CssProp { return {"overflow", "hidden"}; }
inline auto overflow_auto() -> CssProp { return {"overflow", "auto"}; }
inline auto overflow_x(std::string_view v) -> CssProp { return {"overflow-x", std::string(v)}; }
inline auto overflow_y(std::string_view v) -> CssProp { return {"overflow-y", std::string(v)}; }

// ── Table ────────────────────────────────────────────────────────────
inline auto table_layout(std::string_view v) -> CssProp { return {"table-layout", std::string(v)}; }
inline auto vertical_align(std::string_view v) -> CssProp { return {"vertical-align", std::string(v)}; }

// ── Content & Pseudo-element ─────────────────────────────────────────
inline auto content(std::string_view v) -> CssProp { return {"content", "\"" + std::string(v) + "\""}; }
inline auto content_none() -> CssProp { return {"content", "''"}; }
inline auto content_raw(std::string_view v) -> CssProp { return {"content", std::string(v)}; }

// ── Scrollbar ────────────────────────────────────────────────────────
inline auto scrollbar_width(std::string_view v) -> CssProp { return {"scrollbar-width", std::string(v)}; }
inline auto scrollbar_color(std::string_view v) -> CssProp { return {"scrollbar-color", std::string(v)}; }

// ── CSS Custom Property Definition ───────────────────────────────────
inline auto custom_prop(std::string_view name, std::string_view value) -> CssProp {
    return {"--" + std::string(name), std::string(value)};
}

// ── Box Sizing ───────────────────────────────────────────────────────
inline auto box_sizing(std::string_view v) -> CssProp { return {"box-sizing", std::string(v)}; }

// ─── CSS Rule ────────────────────────────────────────────────────────
// Selector + declarations. Selectors support ALL CSS syntax:
//   "&"           → component root
//   "&:hover"     → pseudo-class on root
//   "&::before"   → pseudo-element on root
//   ".child"      → descendant
//   "&.variant"   → root with class
//   "&:nth-child(even)" → pseudo-class
//   "[data-x]"    → attribute selector

struct Rule {
    std::string selector;
    std::vector<CssProp> props;
};

inline auto rule(std::string_view selector, std::initializer_list<CssProp> props) -> Rule {
    return {std::string(selector), {props.begin(), props.end()}};
}

// ─── @keyframes ──────────────────────────────────────────────────────

struct KeyframeStep {
    std::string position;  // "0%", "50%", "100%", "from", "to"
    std::vector<CssProp> props;
};

struct Keyframes {
    std::string name;
    std::vector<KeyframeStep> steps;

    auto render() const -> std::string {
        std::string out = "@keyframes " + name + " {\n";
        for (auto& s : steps) {
            out += "  " + s.position + " { ";
            for (auto& p : s.props) {
                out += std::string(p.property) + ": " + p.value + "; ";
            }
            out += "}\n";
        }
        out += "}\n";
        return out;
    }
};

inline auto keyframe(std::string_view pos, std::initializer_list<CssProp> props) -> KeyframeStep {
    return {std::string(pos), {props.begin(), props.end()}};
}

inline auto keyframes(std::string_view name, std::initializer_list<KeyframeStep> steps) -> Keyframes {
    return {std::string(name), {steps.begin(), steps.end()}};
}

// ─── @media ──────────────────────────────────────────────────────────

struct MediaQuery {
    std::string condition;
    std::vector<Rule> rules;

    auto render() const -> std::string {
        std::string out = "@media (" + condition + ") {\n";
        for (auto& r : rules) {
            out += "  " + r.selector + " { ";
            for (auto& p : r.props) {
                out += std::string(p.property) + ": " + p.value + "; ";
            }
            out += "}\n";
        }
        out += "}\n";
        return out;
    }

    auto scoped(std::string_view scope) const -> std::string {
        std::string out = "@media (" + condition + ") {\n";
        for (auto& r : rules) {
            if (r.selector.starts_with("&")) {
                out += "  ." + std::string(scope) + r.selector.substr(1);
            } else {
                out += "  ." + std::string(scope) + " " + r.selector;
            }
            out += " { ";
            for (auto& p : r.props) {
                out += std::string(p.property) + ": " + p.value + "; ";
            }
            out += "}\n";
        }
        out += "}\n";
        return out;
    }
};

inline auto media(std::string_view condition, std::initializer_list<Rule> rules) -> MediaQuery {
    return {std::string(condition), {rules.begin(), rules.end()}};
}

// ─── Raw CSS Block ───────────────────────────────────────────────────
// For vendor-prefixed pseudo-elements, scrollbar styles, etc. that
// can't be expressed as standard rules. Explicit opt-in.

struct RawBlock {
    std::string css;
};

inline auto raw_css(std::string_view css) -> RawBlock {
    return {std::string(css)};
}

// ─── Stylesheet ──────────────────────────────────────────────────────
// Collects rules, keyframes, media queries, and raw blocks.

struct Stylesheet {
    std::vector<Rule> rules;
    std::vector<Keyframes> animations;
    std::vector<MediaQuery> queries;
    std::vector<RawBlock> raw_blocks;

    auto add(Rule r) -> Stylesheet& { rules.push_back(std::move(r)); return *this; }
    auto add(Keyframes k) -> Stylesheet& { animations.push_back(std::move(k)); return *this; }
    auto add(MediaQuery m) -> Stylesheet& { queries.push_back(std::move(m)); return *this; }
    auto add(RawBlock b) -> Stylesheet& { raw_blocks.push_back(std::move(b)); return *this; }

    auto render() const -> std::string {
        std::string out;
        out.reserve(rules.size() * 80 + animations.size() * 120);
        for (auto& k : animations) out += k.render();
        for (auto& r : rules) {
            out += r.selector + " { ";
            for (auto& p : r.props) out += std::string(p.property) + ": " + p.value + "; ";
            out += "}\n";
        }
        for (auto& m : queries) out += m.render();
        for (auto& b : raw_blocks) out += b.css + "\n";
        return out;
    }

    auto scoped(std::string_view scope) const -> std::string {
        std::string out;
        out.reserve(rules.size() * 80 + animations.size() * 120);
        for (auto& k : animations) out += k.render();  // keyframes are global
        for (auto& r : rules) {
            if (r.selector.starts_with("&")) {
                out += "." + std::string(scope) + r.selector.substr(1);
            } else {
                out += "." + std::string(scope) + " " + r.selector;
            }
            out += " { ";
            for (auto& p : r.props) out += std::string(p.property) + ": " + p.value + "; ";
            out += "}\n";
        }
        for (auto& m : queries) out += m.scoped(scope);
        for (auto& b : raw_blocks) out += b.css + "\n";
        return out;
    }
};

inline auto stylesheet(std::initializer_list<Rule> rules) -> Stylesheet {
    return {{rules.begin(), rules.end()}, {}, {}, {}};
}

// Extended stylesheet builder
struct StylesheetBuilder {
    Stylesheet sheet;

    auto rules(std::initializer_list<Rule> rs) -> StylesheetBuilder& {
        sheet.rules.insert(sheet.rules.end(), rs.begin(), rs.end());
        return *this;
    }
    auto anim(Keyframes k) -> StylesheetBuilder& {
        sheet.animations.push_back(std::move(k));
        return *this;
    }
    auto media_query(MediaQuery m) -> StylesheetBuilder& {
        sheet.queries.push_back(std::move(m));
        return *this;
    }
    auto raw(std::string_view css) -> StylesheetBuilder& {
        sheet.raw_blocks.push_back({std::string(css)});
        return *this;
    }
    auto build() -> Stylesheet { return std::move(sheet); }
};

inline auto build_stylesheet() -> StylesheetBuilder { return {}; }

// ─── Grid Helpers ────────────────────────────────────────────────────
// Common grid patterns as one-liners.

inline auto grid_auto_fill(Length min_size) -> CssProp {
    return {"grid-template-columns", "repeat(auto-fill, minmax(" + min_size.value + ", 1fr))"};
}
inline auto grid_auto_fit(Length min_size) -> CssProp {
    return {"grid-template-columns", "repeat(auto-fit, minmax(" + min_size.value + ", 1fr))"};
}
inline auto grid_cols(int n) -> CssProp {
    return {"grid-template-columns", "repeat(" + std::to_string(n) + ", 1fr)"};
}
inline auto place_items(std::string_view v) -> CssProp { return {"place-items", std::string(v)}; }
inline auto place_content(std::string_view v) -> CssProp { return {"place-content", std::string(v)}; }

// ─── Calc Helper ─────────────────────────────────────────────────────

inline auto calc(std::string_view expr) -> std::string { return "calc(" + std::string(expr) + ")"; }

// ─── Font Presets (reference design tokens) ──────────────────────────

inline auto text_xs()    -> CssProp { return {"font-size", "var(--font-size-xs)"}; }
inline auto text_sm()    -> CssProp { return {"font-size", "var(--font-size-sm)"}; }
inline auto text_base()  -> CssProp { return {"font-size", "var(--font-size-base)"}; }
inline auto text_md()    -> CssProp { return {"font-size", "var(--font-size-md)"}; }
inline auto text_lg()    -> CssProp { return {"font-size", "var(--font-size-lg)"}; }

inline auto weight_normal()   -> CssProp { return {"font-weight", "400"}; }
inline auto weight_medium()   -> CssProp { return {"font-weight", "500"}; }
inline auto weight_semibold() -> CssProp { return {"font-weight", "600"}; }
inline auto weight_bold()     -> CssProp { return {"font-weight", "700"}; }

inline auto font_mono() -> CssProp { return {"font-family", "var(--font-mono)"}; }
inline auto font_sans() -> CssProp { return {"font-family", "var(--font-sans)"}; }

// ─── Spacing Shorthands ─────────────────────────────────────────────

inline auto margin_x(std::string_view v) -> CssProp { return {"margin-inline", std::string(v)}; }
inline auto margin_y(std::string_view v) -> CssProp { return {"margin-block", std::string(v)}; }
inline auto padding_x(std::string_view v) -> CssProp { return {"padding-inline", std::string(v)}; }
inline auto padding_y(std::string_view v) -> CssProp { return {"padding-block", std::string(v)}; }

// ─── Truncation ─────────────────────────────────────────────────────

inline auto truncate() -> CssProp { return {"text-overflow", "ellipsis"}; }
// Common pattern: overflow:hidden + text-overflow:ellipsis + white-space:nowrap
// Applied as three separate props since CssProp is one prop at a time.
// Use with: overflow_hidden(), white_space("nowrap"), truncate()

// ─── ScopedCss Concept ───────────────────────────────────────────────

template<typename C>
concept ScopedCss = requires {
    { C::scope_name } -> std::convertible_to<std::string_view>;
    { C::styles() } -> std::same_as<Stylesheet>;
};

} // namespace getgresql::ssr::css
