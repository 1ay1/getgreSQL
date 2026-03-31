#pragma once

#include <string>
#include <string_view>

namespace getgresql::html {

// ─── HTML escaping ──────────────────────────────────────────────────
// Prevents XSS when inserting user data into HTML.

inline auto escape(std::string_view input) -> std::string {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;
        }
    }
    return out;
}

// Escape for use inside HTML attribute values
inline auto attr_escape(std::string_view input) -> std::string {
    return escape(input);  // same rules for our purposes
}

} // namespace getgresql::html
