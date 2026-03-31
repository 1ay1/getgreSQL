add_executable(tests
    test_static_string.cpp
    test_router.cpp
)

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tests PRIVATE doctest::doctest)

add_test(NAME all_tests COMMAND tests)
