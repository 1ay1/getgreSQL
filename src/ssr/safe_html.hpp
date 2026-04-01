#pragma once

// ─── SafeHtml — Type-level HTML injection prevention ─────────────────
//
// Two phantom types track whether content is safe to emit:
//
//   HtmlString<Escaped>  — HTML-escaped text (safe, produced by escape())
//   HtmlString<Trusted>  — Trusted raw HTML (component output, trusted literals)
//
// Escaped implicitly converts to Trusted (escaped content is always safe).
// User content MUST go through escape(). Component output uses trust().
//
// This makes XSS a compile error:
//   h.raw(user_input);           // still compiles (raw is for trusted HTML)
//   h.emit(escape(user_input));  // type-safe: escape returns Escaped
//   h.emit(user_input);          // compile error: string_view is neither type
//
// Auditing: grep for "trust(" to find all trust boundaries.

#include <string>
#include <string_view>
#include <concepts>

namespace getgresql::ssr {

// ── Phantom safety tags ──────────────────────────────────────────────

struct Escaped {};   // Content has been HTML-escaped
struct Trusted {};   // Content is trusted (component output, known-safe literals)

// ── HtmlString<Safety> — a string with compile-time safety tracking ──

template<typename Safety>
class HtmlString {
    std::string data_;

    struct PrivateTag {};
    explicit HtmlString(PrivateTag, std::string s) : data_(std::move(s)) {}

    template<typename> friend class HtmlString;
    friend auto escape(std::string_view s) -> HtmlString<Escaped>;
    friend auto trust(std::string_view s) -> HtmlString<Trusted>;
    friend auto trust(std::string s) -> HtmlString<Trusted>;

public:
    HtmlString() = default;

    // Escaped implicitly widens to Trusted (safe content is always safe to emit)
    operator HtmlString<Trusted>() const requires std::same_as<Safety, Escaped> {
        return HtmlString<Trusted>(PrivateTag{}, data_);
    }

    [[nodiscard]] auto view() const noexcept -> std::string_view { return data_; }
    [[nodiscard]] auto str() const& -> const std::string& { return data_; }
    [[nodiscard]] auto str() && -> std::string { return std::move(data_); }
    [[nodiscard]] auto size() const noexcept -> std::size_t { return data_.size(); }
    [[nodiscard]] auto empty() const noexcept -> bool { return data_.empty(); }

    // Concatenation preserves safety level
    auto operator+(const HtmlString& other) const -> HtmlString {
        return HtmlString(PrivateTag{}, data_ + other.data_);
    }
    auto operator+=(const HtmlString& other) -> HtmlString& {
        data_ += other.data_;
        return *this;
    }
};

// ── Factory functions — the ONLY way to create HtmlStrings ───────────

// Escape user content — the primary safe entry point
inline auto escape(std::string_view s) -> HtmlString<Escaped> {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;
        }
    }
    return HtmlString<Escaped>(HtmlString<Escaped>::PrivateTag{}, std::move(out));
}

// Trust a literal or component-generated string — auditable via grep
inline auto trust(std::string_view s) -> HtmlString<Trusted> {
    return HtmlString<Trusted>(HtmlString<Trusted>::PrivateTag{}, std::string(s));
}

inline auto trust(std::string s) -> HtmlString<Trusted> {
    return HtmlString<Trusted>(HtmlString<Trusted>::PrivateTag{}, std::move(s));
}

// ── Concepts for generic code ────────────────────────────────────────

template<typename T>
concept SafeHtml = requires(T t) {
    { t.view() } -> std::convertible_to<std::string_view>;
    requires std::same_as<T, HtmlString<Escaped>> || std::same_as<T, HtmlString<Trusted>>;
};

} // namespace getgresql::ssr
