add_executable(unit_tests
    test_smoke.cpp
    test_sepa_parser.cpp
)

target_include_directories(unit_tests
    PRIVATE
        ${CMAKE_BINARY_DIR}/include
)

# The parser test reads fixture files from tests/example-xml/. Hard-code the
# absolute path so the test binary isn't sensitive to the working directory.
target_compile_definitions(unit_tests
    PRIVATE
        SEPA_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/example-xml"
)

target_link_libraries(unit_tests
    PRIVATE
        sepa::warnings
        sepa::core
        Catch2::Catch2WithMain
)

catch_discover_tests(unit_tests
    TEST_PREFIX "unit."
)
