include(FetchContent)
find_package(Threads REQUIRED)

# ── PostgreSQL (libpq) ───────────────────────────────────────────────
find_package(PostgreSQL REQUIRED)

# ── Boost (Beast, Asio, JSON) ────────────────────────────────────────
find_package(Boost 1.83 REQUIRED)

# ── Doctest (testing) ────────────────────────────────────────────────
if(BUILD_TESTS)
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.11
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(doctest)
endif()
