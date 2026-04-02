#pragma once

// ─── Dashboard Widget SSR Components ─────────────────────────────────
// All dashboard rendering extracted from handlers. Zero raw HTML.

#include "ssr/engine.hpp"
#include "ssr/html_dsl.hpp"
#include "ssr/js_dsl.hpp"

#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr {

// ─── Dashboard Shell (htmx loading divs) ────────────────────────────

struct DashboardShell {
    static auto render(Html& h) -> void {
        using namespace html;
        {
            auto d = open<Div>(h, {id("dash-health"), hx_get("/dashboard/health"),
                hx_trigger("load"), hx_swap("innerHTML")});
        }
        {
            auto d = open<Div>(h, {id("dash-stats"), hx_get("/dashboard/stats"),
                hx_trigger("load"), hx_swap("innerHTML")});
        }
        {
            auto row = open<Div>(h, {cls("dash-row")});
            {
                auto d = open<Div>(h, {id("dash-activity"), hx_get("/dashboard/activity"),
                    hx_trigger("load, every 5s"), hx_swap("innerHTML")});
            }
            {
                auto d = open<Div>(h, {id("dash-top-tables"), hx_get("/dashboard/top-tables"),
                    hx_trigger("load"), hx_swap("innerHTML")});
            }
        }
        {
            auto d = open<Div>(h, {id("dash-content"), hx_get("/dashboard/content"),
                hx_trigger("load"), hx_swap("innerHTML")});
        }
    }
};

// ─── Server Info Bar ─────────────────────────────────────────────────

struct DashboardServer {
    struct Props {
        std::string_view version;
        std::string_view uptime;
        int pid;
    };

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;
        {
            auto bar = open<Div>(h, {cls("dash-server")});
            {
                auto info = open<Div>(h, {cls("dash-server-info")});
                el<Span>(h, {cls("dash-server-version")}, p.version);
                {
                    auto up = open<Span>(h, {cls("dash-server-uptime")});
                    h.raw("&#9716; ");
                    el<Strong>(h, {}, p.uptime);
                }
                el<Span>(h, {cls("dash-server-pid")}, "PID " + std::to_string(p.pid));
            }
            {
                auto actions = open<Div>(h, {cls("dash-server-actions")});
                el_raw<A>(h, {href("/query"), cls("btn btn-sm btn-ghost"), data("spa", "")}, "&#9654; Query");
                el_raw<A>(h, {href("/monitor"), cls("btn btn-sm btn-ghost"), data("spa", "")}, "&#9673; Monitor");
                el_raw<A>(h, {href("/settings"), cls("btn btn-sm btn-ghost"), data("spa", "")}, "&#9881; Settings");
            }
        }
    }
};

// ─── SVG Ring Gauge ──────────────────────────────────────────────────

struct DashboardRing {
    struct Props {
        double percent;
        std::string_view color_class;  // "success", "warning", "danger"
        std::string_view value_text;
        std::string_view sub_text;     // e.g. "/ 100"
        std::string_view title;
        std::vector<std::pair<std::string, std::string>> breakdown;  // {label, value}
    };

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;
        {
            auto card = open<Div>(h, {cls("dash-metric-card")});
            {
                auto ring_wrap = open<Div>(h, {cls("dash-ring-container")});
                // SVG ring
                h.raw(std::format(
                    "<svg class=\"dash-ring\" viewBox=\"0 0 120 120\">"
                    "<circle cx=\"60\" cy=\"60\" r=\"52\" fill=\"none\" stroke=\"var(--bg-3)\" stroke-width=\"8\"/>"
                    "<circle cx=\"60\" cy=\"60\" r=\"52\" fill=\"none\" stroke=\"var(--{})\" stroke-width=\"8\" "
                    "stroke-dasharray=\"{:.1f} {:.1f}\" stroke-dashoffset=\"81.7\" stroke-linecap=\"round\" "
                    "class=\"dash-ring-fill\"/></svg>",
                    p.color_class, p.percent * 3.267, (100.0 - p.percent) * 3.267
                ));
                {
                    auto label = open<Div>(h, {cls("dash-ring-label")});
                    el<Span>(h, {cls("dash-ring-value")}, p.value_text);
                    if (!p.sub_text.empty()) el<Span>(h, {cls("dash-ring-sub")}, p.sub_text);
                }
            }
            el<Div>(h, {cls("dash-metric-title")}, p.title);
            {
                auto bd = open<Div>(h, {cls("dash-metric-breakdown")});
                for (auto& [label, value] : p.breakdown) {
                    el<Span>(h, {cls("dash-metric-item")}, label + " " + value);
                }
            }
        }
    }
};

// ─── Big Number Metric Card ──────────────────────────────────────────

struct DashboardBigNumber {
    struct Props {
        std::string_view value;
        std::string_view label;
        std::string_view title;
        std::vector<std::pair<std::string, std::string>> breakdown;
    };

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;
        {
            auto card = open<Div>(h, {cls("dash-metric-card")});
            {
                auto big = open<Div>(h, {cls("dash-metric-big")});
                el<Span>(h, {cls("dash-big-num")}, p.value);
                el<Span>(h, {cls("dash-big-label")}, p.label);
            }
            el<Div>(h, {cls("dash-metric-title")}, p.title);
            {
                auto bd = open<Div>(h, {cls("dash-metric-breakdown")});
                for (auto& [label, value] : p.breakdown) {
                    el<Span>(h, {cls("dash-metric-item")}, label + " " + value);
                }
            }
        }
    }
};

// ─── Dashboard Section Card ──────────────────────────────────────────

struct DashboardSection {
    static auto begin(Html& h, std::string_view title, std::string_view action_html = "") -> void {
        using namespace html;
        h.open("div", "class=\"dashboard-section\"");
        {
            auto hdr = open<Div>(h, {cls("dashboard-section-header")});
            h.raw(title);  // title may contain HTML entities
            if (!action_html.empty()) h.raw(action_html);
        }
        h.open("div", "class=\"dashboard-section-body\"");
    }

    static auto end(Html& h) -> void {
        h.close("div"); // body
        h.close("div"); // section
    }
};

// ─── Database Card (for dashboard) ───────────────────────────────────

struct DatabaseCard {
    struct Props {
        std::string_view name;
        std::string_view size;
        double size_pct;
        double cache_hit_pct;
        long long connections;
        long long xact_commit;
        long long deadlocks;
    };

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;
        auto cache_cls = p.cache_hit_pct < 90 ? "danger" : p.cache_hit_pct < 99 ? "warning" : "success";
        {
            auto card = open<A>(h, {href("/db/" + std::string(p.name) + "/schemas"),
                                    cls("dash-db-card"), data("spa", "")});
            {
                auto hdr = open<Div>(h, {cls("dash-db-header")});
                el<Span>(h, {cls("dash-db-name")}, p.name);
                el<Span>(h, {cls("dash-db-size")}, p.size);
            }
            {
                auto bar = open<Div>(h, {cls("dash-db-bar")});
                el<Div>(h, {cls("dash-db-bar-fill"),
                    style("width:" + std::format("{:.0f}", p.size_pct) + "%")});
            }
            {
                auto stats = open<Div>(h, {cls("dash-db-stats")});
                {
                    auto s = open<Span>(h, {title("Connections")});
                    el_raw<Span>(h, {cls("dash-db-stat-icon")}, "&#9679;");
                    h.raw(std::to_string(p.connections) + " conn");
                }
                el<Span>(h, {cls("dash-db-cache-" + std::string(cache_cls))},
                    std::format("{:.0f}%", p.cache_hit_pct) + " cache");
                if (p.xact_commit > 0) {
                    std::string txn;
                    if (p.xact_commit >= 1000000) txn = std::format("{:.1f}M", p.xact_commit / 1e6);
                    else if (p.xact_commit >= 1000) txn = std::format("{:.1f}K", p.xact_commit / 1e3);
                    else txn = std::to_string(p.xact_commit);
                    el<Span>(h, {title("Commits")}, txn + " txn");
                }
                if (p.deadlocks > 0) {
                    el<Span>(h, {cls("dash-db-deadlocks"), title("Deadlocks")},
                        std::to_string(p.deadlocks) + " deadlocks");
                }
            }
        }
    }
};

// ─── Activity Row ────────────────────────────────────────────────────

struct ActivityRow {
    struct Props {
        std::string_view state;
        std::string_view database;
        std::string_view user;
        std::string_view duration;
        std::string_view query;
    };

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;
        auto state_cls = std::string(p.state) == "active" ? "active"
            : std::string(p.state) == "idle in transaction" ? "warning" : "idle";
        {
            auto row = open<Div>(h, {cls("dash-activity-row")});
            el_raw<Span>(h, {cls(std::string("dash-activity-state dash-activity-") + state_cls)}, "&#9679;");
            el<Span>(h, {cls("dash-activity-db")}, p.database);
            el<Span>(h, {cls("dash-activity-user")}, p.user);
            if (!p.duration.empty()) {
                el<Span>(h, {cls("dash-activity-duration")}, p.duration);
            }
            el<Span>(h, {cls("dash-activity-query")}, p.query);
        }
    }
};

// ─── Top Table Bar Row ───────────────────────────────────────────────

struct BarChartRow {
    struct Props {
        std::string_view name;
        std::string_view href_url;
        double percent;
        bool is_warning;
        std::string_view size;
        std::string_view meta;
    };

    static auto render(const Props& p, Html& h) -> void {
        using namespace html;
        {
            auto row = open<Div>(h, {cls("dash-bar-row")});
            {
                auto name = open<Span>(h, {cls("dash-bar-name")});
                el<A>(h, {href(p.href_url), data("spa", "")}, p.name);
            }
            {
                auto track = open<Div>(h, {cls("dash-bar-track")});
                {
                    auto inner = open<Div>(h);
                    el<Div>(h, {
                        cls(p.is_warning ? "dash-bar-fill dash-bar-warn" : "dash-bar-fill"),
                        style("width:" + std::format("{:.1f}", p.percent) + "%"),
                    });
                }
            }
            el<Span>(h, {cls("dash-bar-value")}, p.size);
            el_raw<Span>(h, {cls("dash-bar-meta")}, std::string(p.meta));
        }
    }
};

// ─── Empty State ─────────────────────────────────────────────────────

struct DashEmpty {
    static auto render(Html& h, std::string_view message) -> void {
        using namespace html;
        el<Div>(h, {cls("dash-empty")}, message);
    }
};

} // namespace getgresql::ssr
