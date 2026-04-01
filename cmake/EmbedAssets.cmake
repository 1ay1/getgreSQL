# EmbedAssets.cmake — generates a C++ header with embedded file contents
#
# Files in src/ subdirectories are concatenated into bundles:
#   assets/css/src/*.css  → bundled as /css/style.css
#   assets/js/src/*.js    → bundled as /js/app.js
# Other files are embedded individually as before.

set(ASSETS_DIR ${CMAKE_ARGV3})
set(OUTPUT_FILE ${CMAKE_ARGV4})

set(HEADER "#pragma once\n")
string(APPEND HEADER "#include <string_view>\n")
string(APPEND HEADER "#include <unordered_map>\n\n")
string(APPEND HEADER "namespace getgresql::assets {\n\n")

set(MAP_ENTRIES "")
set(BUNDLED_SOURCES "")

# ── Helper: embed a single blob as a named asset ─────────────────────
macro(embed_blob VAR_NAME SERVE_PATH CONTENT_HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _HEX_ARRAY "${CONTENT_HEX}")
    string(LENGTH "${CONTENT_HEX}" _HEX_LEN)
    math(EXPR _BYTE_LEN "${_HEX_LEN} / 2")
    string(APPEND HEADER "inline const unsigned char ${VAR_NAME}_data[] = {${_HEX_ARRAY} 0};\n")
    string(APPEND HEADER "inline const std::string_view ${VAR_NAME}{reinterpret_cast<const char*>(${VAR_NAME}_data), ${_BYTE_LEN}};\n\n")
    string(APPEND MAP_ENTRIES "    {\"${SERVE_PATH}\", ${VAR_NAME}},\n")
endmacro()

# ── Bundle: concatenate sorted src/ files into one blob ──────────────
macro(bundle_src_dir SRC_SUBDIR GLOB_PATTERN SERVE_PATH VAR_NAME)
    file(GLOB _SRC_FILES "${ASSETS_DIR}/${SRC_SUBDIR}/${GLOB_PATTERN}")
    list(SORT _SRC_FILES)
    set(_CONCAT "")
    foreach(_F ${_SRC_FILES})
        file(READ "${_F}" _FCONTENT)
        string(APPEND _CONCAT "${_FCONTENT}\n")
        get_filename_component(_FNAME "${_F}" NAME)
        list(APPEND BUNDLED_SOURCES "${SRC_SUBDIR}/${_FNAME}")
    endforeach()
    # Convert concatenated text to hex
    string(HEX "${_CONCAT}" _CONCAT_HEX)
    embed_blob("${VAR_NAME}" "${SERVE_PATH}" "${_CONCAT_HEX}")
    list(LENGTH _SRC_FILES _N)
    message(STATUS "  Bundled ${_N} files from ${SRC_SUBDIR}/ → ${SERVE_PATH}")
endmacro()

# ── Step 1: Create bundles from src/ directories ─────────────────────

bundle_src_dir("css/src" "*.css" "/css/style.css" "css_style_css")
bundle_src_dir("js/src"  "*.js"  "/js/app.js"     "js_app_js")

# ── Step 2: Embed remaining individual files (skip src/ dirs) ────────

file(GLOB_RECURSE ASSET_FILES RELATIVE ${ASSETS_DIR} "${ASSETS_DIR}/*")
foreach(ASSET ${ASSET_FILES})
    # Skip files inside src/ directories (already bundled)
    if("${ASSET}" MATCHES "/src/")
        continue()
    endif()

    file(READ "${ASSETS_DIR}/${ASSET}" FILE_CONTENT HEX)
    string(REGEX REPLACE "[^a-zA-Z0-9]" "_" VAR_NAME "${ASSET}")
    embed_blob("${VAR_NAME}" "/${ASSET}" "${FILE_CONTENT}")
endforeach()

# ── Step 3: Write the map and output ─────────────────────────────────

string(APPEND HEADER "inline const std::unordered_map<std::string_view, std::string_view> all = {\n")
string(APPEND HEADER "${MAP_ENTRIES}")
string(APPEND HEADER "};\n\n")
string(APPEND HEADER "} // namespace getgresql::assets\n")

get_filename_component(OUTPUT_DIR ${OUTPUT_FILE} DIRECTORY)
file(MAKE_DIRECTORY ${OUTPUT_DIR})
file(WRITE ${OUTPUT_FILE} "${HEADER}")
message(STATUS "Embedded assets into ${OUTPUT_FILE}")
