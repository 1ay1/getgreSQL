#include <doctest/doctest.h>
#include "core/static_string.hpp"
#include "core/type_list.hpp"

using namespace getgresql;
using namespace getgresql::meta;

TEST_CASE("TypeList basics") {
    using List = TypeList<int, double, char>;
    static_assert(List::size == 3);
    static_assert(std::is_same_v<At<0, List>, int>);
    static_assert(std::is_same_v<At<1, List>, double>);
    static_assert(std::is_same_v<At<2, List>, char>);
}

TEST_CASE("TypeList contains") {
    using List = TypeList<int, double, char>;
    static_assert(contains_v<int, List>);
    static_assert(contains_v<double, List>);
    static_assert(!contains_v<float, List>);
}

TEST_CASE("TypeList append") {
    using List = TypeList<int, double>;
    using Extended = Append<List, char>;
    static_assert(Extended::size == 3);
    static_assert(std::is_same_v<At<2, Extended>, char>);
}

TEST_CASE("TypeList concat") {
    using A = TypeList<int, double>;
    using B = TypeList<char, float>;
    using C = Concat<A, B>;
    static_assert(C::size == 4);
    static_assert(std::is_same_v<At<0, C>, int>);
    static_assert(std::is_same_v<At<3, C>, float>);
}

TEST_CASE("Compile-time route path properties") {
    // These are really compile-time checks - if they compile, they pass
    constexpr StaticString root{"/"};
    constexpr StaticString db{"/db/{name}"};
    constexpr StaticString assets{"/assets/{path...}"};

    static_assert(!root.has_params());
    static_assert(db.has_params());
    static_assert(db.param_count() == 1);
    static_assert(!db.is_catch_all());
    static_assert(assets.is_catch_all());

    CHECK(true);  // if we got here, all static_asserts passed
}
