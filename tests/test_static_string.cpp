#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "core/static_string.hpp"

using namespace getgresql;

TEST_CASE("StaticString basics") {
    constexpr StaticString s{"hello"};
    static_assert(s.size() == 5);
    static_assert(s[0] == 'h');
    static_assert(s.sv() == "hello");

    CHECK(s.sv() == "hello");
    CHECK(s.size() == 5);
}

TEST_CASE("StaticString concatenation") {
    constexpr StaticString a{"hello"};
    constexpr StaticString b{" world"};
    constexpr auto c = a + b;
    static_assert(c.sv() == "hello world");
    static_assert(c.size() == 11);
}

TEST_CASE("StaticString path analysis") {
    constexpr StaticString simple{"/databases"};
    static_assert(!simple.has_params());
    static_assert(simple.param_count() == 0);

    constexpr StaticString param{"/db/{name}/tables"};
    static_assert(param.has_params());
    static_assert(param.param_count() == 1);

    constexpr StaticString multi{"/db/{db}/schema/{schema}/table/{table}"};
    static_assert(multi.param_count() == 3);

    constexpr StaticString catchall{"/assets/{path...}"};
    static_assert(catchall.is_catch_all());
    static_assert(catchall.param_count() == 1);
}

TEST_CASE("Path matching - exact") {
    auto m = match_path("/databases", "/databases");
    CHECK(m.matched);
    CHECK(m.param_count == 0);

    auto m2 = match_path("/databases", "/schemas");
    CHECK_FALSE(m2.matched);
}

TEST_CASE("Path matching - single param") {
    auto m = match_path("/db/{name}/tables", "/db/mydb/tables");
    CHECK(m.matched);
    CHECK(m.param_count == 1);
    CHECK(m.get("name") == "mydb");
}

TEST_CASE("Path matching - multiple params") {
    auto m = match_path("/db/{db}/schema/{schema}/table/{table}",
                        "/db/mydb/schema/public/table/users");
    CHECK(m.matched);
    CHECK(m.param_count == 3);
    CHECK(m.get("db") == "mydb");
    CHECK(m.get("schema") == "public");
    CHECK(m.get("table") == "users");
}

TEST_CASE("Path matching - catch-all") {
    auto m = match_path("/assets/{path...}", "/assets/css/style.css");
    CHECK(m.matched);
    CHECK(m.get("path") == "css/style.css");
}

TEST_CASE("Path matching - no match") {
    auto m = match_path("/db/{name}/tables", "/db/mydb/schemas");
    CHECK_FALSE(m.matched);
}
