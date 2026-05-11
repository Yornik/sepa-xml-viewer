add_executable(integration_tests
    test_app_starts.cpp
)

target_link_libraries(integration_tests
    PRIVATE
        sepa::warnings
        Catch2::Catch2WithMain
)

# Fixture path for the --check tests. Same convention as the unit tests.
target_compile_definitions(integration_tests
    PRIVATE
        SEPA_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/example-xml"
)

# Same C4996 / std::getenv exemption as the GUI test target — see
# tests/gui/CMakeLists.txt for the rationale.
if(MSVC)
    target_compile_definitions(integration_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# The integration tests spawn the application binary, so they need its on-disk
# path. CMake passes that via a generator expression baked into the test
# environment — works for both single-config (Ninja) and multi-config (MSBuild)
# generators.
catch_discover_tests(integration_tests
    TEST_PREFIX "integration."
    PROPERTIES
        ENVIRONMENT "SEPA_VIEWER_BINARY=$<TARGET_FILE:sepa-xml-viewer>"
)

add_dependencies(integration_tests sepa-xml-viewer)
