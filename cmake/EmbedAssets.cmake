# EmbedAssets.cmake — generates empty asset map
# All CSS/JS is now co-located in C++ components.
# This file exists only to satisfy the build system.

set(OUTPUT_FILE ${CMAKE_ARGV4})

set(HEADER "#pragma once\n")
string(APPEND HEADER "#include <string_view>\n")
string(APPEND HEADER "#include <unordered_map>\n\n")
string(APPEND HEADER "namespace getgresql::assets {\n\n")
string(APPEND HEADER "inline const std::unordered_map<std::string_view, std::string_view> all = {};\n\n")
string(APPEND HEADER "} // namespace getgresql::assets\n")

get_filename_component(OUTPUT_DIR ${OUTPUT_FILE} DIRECTORY)
file(MAKE_DIRECTORY ${OUTPUT_DIR})
file(WRITE ${OUTPUT_FILE} "${HEADER}")
message(STATUS "Generated empty asset map (all assets in components)")
