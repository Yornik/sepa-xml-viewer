add_executable(unit_tests
    test_smoke.cpp
)

target_include_directories(unit_tests
    PRIVATE
        ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(unit_tests
    PRIVATE
        sepa::warnings
        Catch2::Catch2WithMain
)

catch_discover_tests(unit_tests
    TEST_PREFIX "unit."
)
