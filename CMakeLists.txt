cmake_minimum_required(VERSION 3.28)
project(getgreSQL VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)

# ── Main executable ──────────────────────────────────────────────────
add_executable(getgresql
    src/main.cpp
    src/pg/connection.cpp
    src/pg/pool.cpp
    src/pg/result.cpp
    src/pg/catalog.cpp
    src/pg/monitor.cpp
    src/api/handlers_db.cpp
    src/api/handlers_dv.cpp
    src/api/handlers_query.cpp
    src/api/handlers_monitor.cpp
    src/api/handlers_admin.cpp
    src/api/handlers_schema.cpp
    src/api/handlers_tree.cpp
    src/api/handlers_static.cpp
    src/api/handlers_connections.cpp
    src/config/config.cpp
    src/config/connections.cpp
)

target_include_directories(getgresql PRIVATE src)
target_link_libraries(getgresql PRIVATE
    PostgreSQL::PostgreSQL
    Boost::headers
    Threads::Threads
)

# ── Asset embedding ──────────────────────────────────────────────────
# CSS/JS are co-located in C++ components (src/ssr/components/*.hpp)
# and collected at startup by the component registry.
# Only vendor libraries (htmx) are still embedded as static files.
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/embedded_assets.hpp
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/EmbedAssets.cmake
        ${CMAKE_BINARY_DIR}/generated/embedded_assets.hpp
    DEPENDS
        ${CMAKE_SOURCE_DIR}/cmake/EmbedAssets.cmake
    COMMENT "Generating asset map (all assets in components)"
)
add_custom_target(embed_assets DEPENDS ${CMAKE_BINARY_DIR}/generated/embedded_assets.hpp)
add_dependencies(getgresql embed_assets)
target_include_directories(getgresql PRIVATE ${CMAKE_BINARY_DIR}/generated)

# ── Tests ────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
