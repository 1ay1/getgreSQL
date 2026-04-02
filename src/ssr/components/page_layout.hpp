#pragma once
#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/js_dsl.hpp"
#include <concepts>
#include <string>
#include <string_view>

namespace getgresql::ssr {

struct PageLayout {
    struct Props {
        std::string_view title;
        std::string_view active_nav;
    };

    template<std::invocable<Html&> F>
    static auto render(const Props& p, Html& h, F&& content_fn) -> void {
        using namespace html;
        render_head(p, h);
        {
            auto ide = open<Div>(h, {cls("ide")});
            render_toolbar(p, h);
            render_sidebar(h);
            {
                auto ws = open<Div>(h, {cls("workspace")});
                {
                    auto tab_bar = open<Div>(h, {cls("tab-bar")});
                    {
                        auto tab = open<Div>(h, {cls("tab active")});
                        el<Span>(h, {cls("tab-label")}, p.title);
                    }
                    el<Div>(h, {cls("tab-bar-end")});
                }
                {
                    auto content = open<Div>(h, {cls("content")});
                    content_fn(h);
                }
            }
            render_status_bar(h);
        }
        h.close("body");
        h.close("html");
    }

    template<std::invocable<Html&> F>
    static auto render_full(const Props& p, Html& h, F&& content_fn) -> void {
        using namespace html;
        render_head(p, h);
        {
            auto ide = open<Div>(h, {cls("ide")});
            render_toolbar(p, h);
            render_sidebar(h);
            {
                auto ws = open<Div>(h, {cls("workspace")});
                {
                    auto tab_bar = open<Div>(h, {cls("tab-bar")});
                    {
                        auto tab = open<Div>(h, {cls("tab active")});
                        el<Span>(h, {cls("tab-label")}, p.title);
                    }
                    el<Div>(h, {cls("tab-bar-end")});
                }
                content_fn(h);
            }
            render_status_bar(h);
        }
        h.close("body");
        h.close("html");
    }

private:
    // Document structure uses non-RAII open/close because <html>, <body>
    // must stay open across multiple render_* calls. Closed in render/render_full.
    static auto render_head(const Props& p, Html& h) -> void {
        using namespace html;
        h.raw("<!DOCTYPE html>\n");
        h.open("html", "lang=\"en\" data-theme=\"dark\"");
        {
            auto head = open<Head>(h);
            void_el<Meta>(h, {attr("charset", "utf-8")});
            void_el<Meta>(h, {name("viewport"), attr("content", "width=device-width, initial-scale=1")});
            {
                auto t = open<Title>(h);
                h.text(p.title);
                h.raw(" - getgreSQL");
            }
            void_el<Link>(h, {rel("stylesheet"), href("/assets/css/components.css")});
            el<Script>(h, {src("/assets/js/components.js"), attr("defer", "")});
        }
        h.open("body");
    }

    static auto render_toolbar(const Props& p, Html& h) -> void {
        using namespace html;
        {
            auto header = open<Header>(h, {cls("toolbar")});
            el_raw<Span>(h, {cls("toolbar-brand")}, "getgreSQL<span class=\"version\">v0.1</span>");
            el<Span>(h, {cls("toolbar-sep")});
            {
                auto nav = open<Nav>(h, {cls("toolbar-nav")});
                constexpr struct { const char* href_; const char* icon; const char* label; } navs[] = {
                    {"/", "&#9635;", "Dashboard"}, {"/query", "&#9654;", "Query"},
                    {"/monitor", "&#9673;", "Monitor"},
                };
                for (auto& n : navs) {
                    auto nav_cls = std::string("toolbar-btn");
                    if (p.active_nav == n.label) nav_cls += " active";
                    auto a = open<A>(h, {href(n.href_), cls(nav_cls), data("spa", "")});
                    el_raw<Span>(h, {cls("icon")}, n.icon);
                    h.raw(" ").raw(n.label);
                }
            }
            el<Span>(h, {cls("toolbar-spacer")});
            {
                auto conn = open<Span>(h, {cls("toolbar-conn")});
                el<Span>(h, {cls("conn-dot")});
                el<Span>(h, {cls("conn-label"), id("toolbar-db")}, "Connected");
            }
            {
                auto actions = open<Div>(h, {cls("toolbar-actions")});
                el_raw<Button>(h, {js::on_click(js::call("openCommandPalette")), cls("toolbar-icon-btn"), title("Command Palette (Ctrl+K)")}, "&#8984;");
                el_raw<Button>(h, {js::on_click(js::call("toggleSidebar")), cls("toolbar-icon-btn"), title("Toggle Sidebar")}, "&#9776;");
                el_raw<Button>(h, {js::on_click(js::call("toggleTheme")), cls("toolbar-icon-btn"), title("Theme")}, "&#9680;");
            }
        }
    }

    static auto render_sidebar(Html& h) -> void {
        using namespace html;
        {
            auto aside = open<Aside>(h, {cls("sidebar")});
            {
                auto header = open<Div>(h, {cls("sidebar-header")});
                el<Span>(h, {cls("sidebar-title")}, "Explorer");
                {
                    auto actions = open<Div>(h, {cls("sidebar-actions")});
                    el_raw<Button>(h, {cls("sidebar-icon"), js::on_click(js::call("sidebarRefresh")), title("Refresh")}, "&#8635;");
                    el_raw<Button>(h, {cls("sidebar-icon"), js::on_click(js::call("sidebarCollapseAll")), title("Collapse All")}, "&#8722;");
                    el_raw<Button>(h, {cls("sidebar-icon"), js::on_click(js::call("toggleSidebar")), title("Hide Sidebar (Ctrl+B)")}, "&#10005;");
                }
            }
            {
                auto search = open<Div>(h, {cls("sidebar-search")});
                void_el<Input>(h, {type("text"), cls("sidebar-search-input"), id("sidebar-filter"),
                                   placeholder("Filter... (type to search)"), autocomplete("off")});
            }
            {
                auto tree = open<Div>(h, {cls("sidebar-tree"), id("sidebar-tree"),
                                          hx_get("/tree"), hx_trigger("load"), hx_swap("innerHTML")});
                el<Div>(h, {cls("loading")}, "Loading...");
            }
            el<Div>(h, {cls("resize-handle")});
        }
    }

    static auto render_status_bar(Html& h) -> void {
        using namespace html;
        {
            auto footer = open<Footer>(h, {cls("status-bar")});
            {
                auto item = open<Span>(h, {cls("status-item")});
                el<Span>(h, {cls("conn-dot")});
                h.raw(" ");
                el<Span>(h, {id("status-db")}, "Connected");
            }
            el<Span>(h, {cls("status-sep")});
            el<Span>(h, {cls("status-item"), id("status-info")});
            el<Span>(h, {cls("status-spacer")});
            {
                auto item = open<Span>(h, {cls("status-item")});
                el<Kbd>(h, {}, "Ctrl+B");
                h.raw(" Sidebar");
            }
            {
                auto item = open<Span>(h, {cls("status-item")});
                el<Kbd>(h, {}, "Ctrl+K");
                h.raw(" Command Palette");
            }
        }
    }
};

} // namespace getgresql::ssr
