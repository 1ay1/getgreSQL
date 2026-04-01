#pragma once

// ─── Component Registry — Compile-time CSS/JS Collection ────────────
//
// Every component in the app is listed in AllComponents. At startup,
// collect_css() and collect_js() iterate the list and concatenate
// each component's co-located CSS/JS into a single bundle string.
//
// Deduplication is inherent: each type appears once in the TypeList.
// Components without css()/js() methods are silently skipped.
//
// To add a new component: include its header and add it to the list.

#include "core/type_list.hpp"
#include "ssr/engine.hpp"

// Include all components
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
#include "ssr/components/dv_edit.hpp"

#include <string>

namespace getgresql::ssr {

// Master list of all components. Order determines CSS cascade order.
using AllComponents = meta::TypeList<
    Alert,
    Badge,
    Breadcrumbs,
    HealthCard,
    ProgressBar,
    SearchInput,
    SectionTabs,
    SizeBar,
    StatCard,
    Table,
    TreeNode,
    DvLineagePanel
>;

// Collect all component CSS into a single string (called once at startup).
inline auto collect_css() -> std::string {
    std::string out;
    out.reserve(16384);
    meta::for_each_type<AllComponents>([&]<typename C>() {
        if constexpr (HasCss<C>) {
            out += C::css();
            out += '\n';
        }
    });
    return out;
}

// Collect all component JS into a single string (called once at startup).
inline auto collect_js() -> std::string {
    std::string out;
    out.reserve(8192);
    meta::for_each_type<AllComponents>([&]<typename C>() {
        if constexpr (HasJs<C>) {
            out += C::js();
            out += '\n';
        }
    });
    return out;
}

} // namespace getgresql::ssr
