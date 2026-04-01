#pragma once

// ─── getgreSQL SSR Framework — Next.js-like Page Architecture ────────
//
// Core idea: Shell-first rendering + parallel data sections.
//
// When a browser requests a page:
//   1. The shell (DOCTYPE, head, CSS/JS, toolbar, sidebar, tab bar)
//      renders INSTANTLY — zero DB queries needed.
//   2. Content sections are placeholder divs with hx-trigger="load".
//   3. The browser fires parallel requests for each section.
//   4. Sections render independently as their DB queries complete.
//
// This gives:
//   - 0ms perceived TTFB (shell is static, no data dependencies)
//   - Parallel data loading (browser sends N requests simultaneously)
//   - Progressive rendering (sections appear as they're ready)
//   - Natural loading states (each section shows a spinner)
//
// Usage:
//   // Define a page with sections
//   auto page = Page("Dashboard", "Dashboard")
//       .section("health",  "/dashboard/health",  "Checking health...")
//       .section("stats",   "/dashboard/stats",   "Loading stats...")
//       .section("content", "/dashboard/content",  "");
//
//   // Render shell (instant, no DB)
//   return Response::html(page.render());
//
//   // Section endpoints render data (hit by htmx in parallel)
//   auto HealthSection::handle(...) -> Response {
//       auto data = pg::health_checks(conn);
//       return Response::html(render_partial([&](Html& h) { ... }));
//   }

#include "ssr/engine.hpp"
#include "ssr/components.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace getgresql::ssr {

// ─── Section: a deferred content block ───────────────────────────────

struct Section {
    std::string id;
    std::string url;
    std::string loading_text;
    std::string poll;  // e.g. "every 3s" for live-updating sections
};

// ─── Page: shell-first renderer ──────────────────────────────────────

class Page {
    std::string title_;
    std::string nav_;
    std::vector<Section> sections_;
    bool full_height_ = false;  // true for query/explain pages (no .content wrapper)

public:
    Page(std::string_view title, std::string_view nav)
        : title_(title), nav_(nav) {}

    auto section(std::string_view id, std::string_view url,
                 std::string_view loading = "Loading...") -> Page& {
        sections_.push_back({std::string(id), std::string(url), std::string(loading), ""});
        return *this;
    }

    auto live_section(std::string_view id, std::string_view url,
                      std::string_view poll_interval,
                      std::string_view loading = "Loading...") -> Page& {
        sections_.push_back({std::string(id), std::string(url), std::string(loading), std::string(poll_interval)});
        return *this;
    }

    auto full_height() -> Page& { full_height_ = true; return *this; }

    // Render the shell with deferred sections
    auto render() const -> std::string {
        auto h = Html::with_capacity(4096);

        if (full_height_) {
            PageLayout::render_full({title_, nav_}, h, [&](Html& h) {
                render_sections(h);
            });
        } else {
            PageLayout::render({title_, nav_}, h, [&](Html& h) {
                render_sections(h);
            });
        }
        return std::move(h).finish();
    }

    // Render just the sections (for htmx partial responses)
    auto render_partial() const -> std::string {
        auto h = Html::with_capacity(2048);
        render_sections(h);
        return std::move(h).finish();
    }

private:
    auto render_sections(Html& h) const -> void {
        for (auto& s : sections_) {
            h.raw("<div id=\"").raw(s.id).raw("\"");
            h.raw(" hx-get=\"").raw(s.url).raw("\"");
            h.raw(" hx-trigger=\"load");
            if (!s.poll.empty()) h.raw(", ").raw(s.poll);
            h.raw("\" hx-swap=\"innerHTML\">");
            if (!s.loading_text.empty()) {
                h.raw("<div class=\"loading\">").text(s.loading_text).raw("</div>");
            }
            h.raw("</div>\n");
        }
    }
};

// ─── Cache: LRU component cache ──────────────────────────────────────
// Caches rendered HTML fragments by key with TTL.
// Thread-safe (uses mutex for concurrent access from worker threads).

#include <mutex>
#include <unordered_map>
#include <chrono>

class RenderCache {
    struct Entry {
        std::string html;
        std::chrono::steady_clock::time_point expires;
    };

    std::unordered_map<std::string, Entry> cache_;
    mutable std::mutex mu_;
    std::size_t max_entries_;

public:
    explicit RenderCache(std::size_t max_entries = 256) : max_entries_(max_entries) {}

    // Get cached HTML, or nullopt if expired/missing
    auto get(const std::string& key) const -> std::optional<std::string> {
        std::lock_guard lock(mu_);
        auto it = cache_.find(key);
        if (it == cache_.end()) return std::nullopt;
        if (std::chrono::steady_clock::now() > it->second.expires) return std::nullopt;
        return it->second.html;
    }

    // Store rendered HTML with TTL
    auto put(std::string key, std::string html, std::chrono::seconds ttl) -> void {
        std::lock_guard lock(mu_);
        // Evict oldest if at capacity
        if (cache_.size() >= max_entries_) {
            auto oldest = cache_.begin();
            for (auto it = cache_.begin(); it != cache_.end(); ++it) {
                if (it->second.expires < oldest->second.expires) oldest = it;
            }
            cache_.erase(oldest);
        }
        cache_[std::move(key)] = {std::move(html), std::chrono::steady_clock::now() + ttl};
    }

    // Get-or-compute: returns cached value or calls fn to compute it
    template<std::invocable<> F>
    auto get_or_compute(const std::string& key, std::chrono::seconds ttl, F&& fn) -> std::string {
        if (auto cached = get(key)) return *cached;
        auto html = fn();
        put(key, html, ttl);
        return html;
    }

    auto clear() -> void {
        std::lock_guard lock(mu_);
        cache_.clear();
    }

    auto size() const -> std::size_t {
        std::lock_guard lock(mu_);
        return cache_.size();
    }
};

// Global render cache instance
inline RenderCache& cache() {
    static RenderCache instance(512);
    return instance;
}

} // namespace getgresql::ssr
