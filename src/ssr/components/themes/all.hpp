#pragma once

// ─── Theme Registry ──────────────────────────────────────────────────
// Every theme is a [data-theme="name"] CSS block that overrides
// the design tokens defined in theme.hpp (:root).
//
// To add a new theme:
//   1. Create themes/my_theme.hpp with a struct + css() method
//   2. Include it here
//   3. Add the struct to AllThemes below
//   4. Add a metadata entry to THEME_LIST
//
// That's it. The component system collects the CSS automatically.

#include "ssr/components/themes/light.hpp"
#include "ssr/components/themes/monokai.hpp"
#include "ssr/components/themes/dracula.hpp"
#include "ssr/components/themes/nord.hpp"
#include "ssr/components/themes/solarized_dark.hpp"
#include "ssr/components/themes/solarized_light.hpp"
#include "ssr/components/themes/catppuccin_mocha.hpp"
#include "ssr/components/themes/catppuccin_latte.hpp"
#include "ssr/components/themes/gruvbox_dark.hpp"
#include "ssr/components/themes/gruvbox_light.hpp"
#include "ssr/components/themes/tokyo_night.hpp"
#include "ssr/components/themes/one_dark.hpp"
#include "ssr/components/themes/rose_pine.hpp"
#include "ssr/components/themes/everforest.hpp"
#include "ssr/components/themes/kanagawa.hpp"
#include "ssr/components/themes/ayu_dark.hpp"
#include "ssr/components/themes/ayu_mirage.hpp"
#include "ssr/components/themes/ayu_light.hpp"
#include "ssr/components/themes/synthwave.hpp"
#include "ssr/components/themes/cyberpunk.hpp"
#include "ssr/components/themes/matrix.hpp"
#include "ssr/components/themes/amber_terminal.hpp"
#include "ssr/components/themes/night_owl.hpp"
#include "ssr/components/themes/palenight.hpp"
#include "ssr/components/themes/horizon.hpp"
#include "ssr/components/themes/poimandres.hpp"
#include "ssr/components/themes/moonlight.hpp"
#include "ssr/components/themes/cobalt2.hpp"
#include "ssr/components/themes/andromeda.hpp"
#include "ssr/components/themes/shades_of_purple.hpp"
#include "ssr/components/themes/github_dimmed.hpp"
#include "ssr/components/themes/high_contrast.hpp"
#include "ssr/components/themes/paper.hpp"
#include "ssr/components/themes/tron.hpp"
#include "ssr/components/themes/vitesse_dark.hpp"
#include "ssr/components/themes/slack_dark.hpp"
#include "ssr/components/themes/midnight.hpp"

#include "core/type_list.hpp"

#include <array>
#include <string_view>

namespace getgresql::ssr {

// ─── All theme types (for CSS collection) ────────────────────────────

using AllThemes = meta::TypeList<
    ThemeLight,
    ThemeMonokai, ThemeDracula, ThemeNord,
    ThemeSolarizedDark, ThemeSolarizedLight,
    ThemeCatppuccinMocha, ThemeCatppuccinLatte,
    ThemeGruvboxDark, ThemeGruvboxLight,
    ThemeTokyoNight, ThemeOneDark,
    ThemeRosePine, ThemeEverforest, ThemeKanagawa,
    ThemeAyuDark, ThemeAyuMirage, ThemeAyuLight,
    ThemeSynthwave, ThemeCyberpunk, ThemeMatrix, ThemeAmberTerminal,
    ThemeNightOwl, ThemePalenight, ThemeHorizon,
    ThemePoimandres, ThemeMoonlight, ThemeCobalt2,
    ThemeAndromeda, ThemeShadesOfPurple,
    ThemeGitHubDimmed, ThemeHighContrast,
    ThemePaper, ThemeTron, ThemeVitesseDark,
    ThemeSlackDark, ThemeMidnight
>;

// ─── Theme metadata (for the picker UI) ──────────────────────────────

struct ThemeMeta {
    std::string_view id;    // matches data-theme="..."
    std::string_view name;  // display name
    std::string_view bg;    // preview background color
    std::string_view fg;    // preview accent color
};

// Default dark theme has id "" (uses :root tokens directly)
inline constexpr std::array THEME_LIST = {
    // ── Defaults ─────────────────────────────────────────────────
    ThemeMeta{"",                  "Dark",              "#0d1117", "#58a6ff"},
    ThemeMeta{"light",             "Light",             "#ffffff", "#0969da"},
    // ── Classics ─────────────────────────────────────────────────
    ThemeMeta{"monokai",           "Monokai",           "#272822", "#f92672"},
    ThemeMeta{"dracula",           "Dracula",           "#282a36", "#bd93f9"},
    ThemeMeta{"nord",              "Nord",              "#2e3440", "#88c0d0"},
    ThemeMeta{"one-dark",          "One Dark",          "#282c34", "#61afef"},
    ThemeMeta{"solarized-dark",    "Solarized Dark",    "#002b36", "#268bd2"},
    ThemeMeta{"solarized-light",   "Solarized Light",   "#fdf6e3", "#268bd2"},
    // ── Catppuccin ───────────────────────────────────────────────
    ThemeMeta{"catppuccin-mocha",  "Catppuccin Mocha",  "#1e1e2e", "#89b4fa"},
    ThemeMeta{"catppuccin-latte",  "Catppuccin Latte",  "#eff1f5", "#1e66f5"},
    // ── Gruvbox ──────────────────────────────────────────────────
    ThemeMeta{"gruvbox-dark",      "Gruvbox Dark",      "#282828", "#83a598"},
    ThemeMeta{"gruvbox-light",     "Gruvbox Light",     "#fbf1c7", "#076678"},
    // ── Japanese ─────────────────────────────────────────────────
    ThemeMeta{"tokyo-night",       "Tokyo Night",       "#1a1b26", "#7aa2f7"},
    ThemeMeta{"kanagawa",          "Kanagawa",          "#1f1f28", "#7e9cd8"},
    // ── Nature ───────────────────────────────────────────────────
    ThemeMeta{"rose-pine",         "Ros\xc3\xa9 Pine",  "#191724", "#c4a7e7"},
    ThemeMeta{"everforest",        "Everforest",        "#2d353b", "#7fbbb3"},
    // ── Ayu ──────────────────────────────────────────────────────
    ThemeMeta{"ayu-dark",          "Ayu Dark",          "#0b0e14", "#e6b450"},
    ThemeMeta{"ayu-mirage",        "Ayu Mirage",        "#1f2430", "#ffcc66"},
    ThemeMeta{"ayu-light",         "Ayu Light",         "#fafafa", "#f2ae49"},
    // ── Material ─────────────────────────────────────────────────
    ThemeMeta{"palenight",         "Palenight",         "#292d3e", "#82aaff"},
    ThemeMeta{"moonlight",         "Moonlight",         "#1e2030", "#82aaff"},
    // ── Moody ────────────────────────────────────────────────────
    ThemeMeta{"night-owl",         "Night Owl",         "#011627", "#82aaff"},
    ThemeMeta{"horizon",           "Horizon",           "#1c1e26", "#e95678"},
    ThemeMeta{"poimandres",        "Poimandres",        "#1b1e28", "#add7ff"},
    ThemeMeta{"cobalt2",           "Cobalt2",           "#122738", "#ffc600"},
    ThemeMeta{"andromeda",         "Andromeda",         "#23262e", "#00e8c6"},
    ThemeMeta{"shades-of-purple",  "Shades of Purple",  "#1e1e3f", "#fad000"},
    ThemeMeta{"vitesse-dark",      "Vitesse Dark",      "#121212", "#4d9375"},
    ThemeMeta{"midnight",          "Midnight",          "#0f0f1e", "#7b68ee"},
    // ── Platforms ────────────────────────────────────────────────
    ThemeMeta{"github-dimmed",     "GitHub Dimmed",     "#22272e", "#539bf5"},
    ThemeMeta{"slack-dark",        "Slack Dark",        "#1a1d21", "#1d9bd1"},
    // ── Retro / Fun ──────────────────────────────────────────────
    ThemeMeta{"synthwave",         "Synthwave \xe2\x80\x9984", "#1a1028", "#ff7edb"},
    ThemeMeta{"cyberpunk",         "Cyberpunk",         "#0a0a12", "#ff003c"},
    ThemeMeta{"matrix",            "Matrix",            "#000000", "#00ff41"},
    ThemeMeta{"amber",             "Amber Terminal",    "#0c0800", "#ffb000"},
    ThemeMeta{"tron",              "Tron",              "#0a0a0f", "#6ee2ff"},
    // ── Accessibility ────────────────────────────────────────────
    ThemeMeta{"high-contrast",     "High Contrast",     "#000000", "#71b7ff"},
    // ── Paper ────────────────────────────────────────────────────
    ThemeMeta{"paper",             "Paper",             "#f5f0eb", "#8b5c34"},
};

} // namespace getgresql::ssr
