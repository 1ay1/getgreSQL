# EmbedAssets.cmake — generates a C++ header with embedded file contents

set(ASSETS_DIR ${CMAKE_ARGV3})
set(OUTPUT_FILE ${CMAKE_ARGV4})

file(GLOB_RECURSE ASSET_FILES RELATIVE ${ASSETS_DIR} "${ASSETS_DIR}/*")

set(HEADER "#pragma once\n")
string(APPEND HEADER "#include <string_view>\n")
string(APPEND HEADER "#include <unordered_map>\n\n")
string(APPEND HEADER "namespace getgresql::assets {\n\n")

set(MAP_ENTRIES "")

foreach(ASSET ${ASSET_FILES})
    file(READ "${ASSETS_DIR}/${ASSET}" FILE_CONTENT HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," HEX_ARRAY "${FILE_CONTENT}")
    string(REGEX REPLACE "[^a-zA-Z0-9]" "_" VAR_NAME "${ASSET}")

    string(LENGTH "${FILE_CONTENT}" HEX_LEN)
    math(EXPR BYTE_LEN "${HEX_LEN} / 2")
    string(APPEND HEADER "inline const unsigned char ${VAR_NAME}_data[] = {${HEX_ARRAY} 0};\n")
    string(APPEND HEADER "inline const std::string_view ${VAR_NAME}{reinterpret_cast<const char*>(${VAR_NAME}_data), ${BYTE_LEN}};\n\n")

    string(APPEND MAP_ENTRIES "    {\"/${ASSET}\", ${VAR_NAME}},\n")
endforeach()

string(APPEND HEADER "inline const std::unordered_map<std::string_view, std::string_view> all = {\n")
string(APPEND HEADER "${MAP_ENTRIES}")
string(APPEND HEADER "};\n\n")
string(APPEND HEADER "} // namespace getgresql::assets\n")

get_filename_component(OUTPUT_DIR ${OUTPUT_FILE} DIRECTORY)
file(MAKE_DIRECTORY ${OUTPUT_DIR})
file(WRITE ${OUTPUT_FILE} "${HEADER}")
message(STATUS "Embedded ${ASSET_FILES} into ${OUTPUT_FILE}")
