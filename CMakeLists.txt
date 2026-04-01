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
    src/http/server.cpp
    src/api/handlers_db.cpp
    src/api/handlers_query.cpp
    src/api/handlers_monitor.cpp
    src/api/handlers_admin.cpp
    src/api/handlers_schema.cpp
    src/api/handlers_tree.cpp
    src/api/handlers_static.cpp
    src/config/config.cpp
)

target_include_directories(getgresql PRIVATE src)
target_link_libraries(getgresql PRIVATE
    PostgreSQL::PostgreSQL
    Boost::headers
    Threads::Threads
)

# ── Asset embedding ──────────────────────────────────────────────────
# Source files in css/src/ and js/src/ are bundled into single files.
# Other files (htmx.min.js) are embedded individually.
file(GLOB CSS_SRC_FILES ${CMAKE_SOURCE_DIR}/assets/css/src/*.css)
file(GLOB JS_SRC_FILES  ${CMAKE_SOURCE_DIR}/assets/js/src/*.js)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/embedded_assets.hpp
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/EmbedAssets.cmake
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/generated/embedded_assets.hpp
    DEPENDS
        ${CMAKE_SOURCE_DIR}/cmake/EmbedAssets.cmake
        ${CSS_SRC_FILES}
        ${JS_SRC_FILES}
    COMMENT "Bundling and embedding static assets"
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
