#pragma once

#include <cstddef>
#include <type_traits>

namespace getgresql::meta {

// ─── Type list: the backbone of compile-time route tables ───────────

template <typename... Ts>
struct TypeList {
    static constexpr std::size_t size = sizeof...(Ts);
};

// ─── Indexing ───────────────────────────────────────────────────────

template <std::size_t I, typename List>
struct AtImpl;

template <std::size_t I, typename Head, typename... Tail>
struct AtImpl<I, TypeList<Head, Tail...>> : AtImpl<I - 1, TypeList<Tail...>> {};

template <typename Head, typename... Tail>
struct AtImpl<0, TypeList<Head, Tail...>> {
    using type = Head;
};

template <std::size_t I, typename List>
using At = typename AtImpl<I, List>::type;

// ─── ForEach: invoke a callable on each type ────────────────────────

template <typename List, typename F>
struct ForEachImpl;

template <typename... Ts, typename F>
struct ForEachImpl<TypeList<Ts...>, F> {
    static constexpr void apply(F&& f) {
        (f.template operator()<Ts>(), ...);
    }
};

template <typename List, typename F>
constexpr void for_each_type(F&& f) {
    ForEachImpl<List, F>::apply(std::forward<F>(f));
}

// ─── Contains ───────────────────────────────────────────────────────

template <typename T, typename List>
struct ContainsImpl;

template <typename T, typename... Ts>
struct ContainsImpl<T, TypeList<Ts...>> {
    static constexpr bool value = (std::is_same_v<T, Ts> || ...);
};

template <typename T, typename List>
inline constexpr bool contains_v = ContainsImpl<T, List>::value;

// ─── Append ─────────────────────────────────────────────────────────

template <typename List, typename T>
struct AppendImpl;

template <typename... Ts, typename T>
struct AppendImpl<TypeList<Ts...>, T> {
    using type = TypeList<Ts..., T>;
};

template <typename List, typename T>
using Append = typename AppendImpl<List, T>::type;

// ─── Concat ─────────────────────────────────────────────────────────

template <typename A, typename B>
struct ConcatImpl;

template <typename... As, typename... Bs>
struct ConcatImpl<TypeList<As...>, TypeList<Bs...>> {
    using type = TypeList<As..., Bs...>;
};

template <typename A, typename B>
using Concat = typename ConcatImpl<A, B>::type;

} // namespace getgresql::meta
