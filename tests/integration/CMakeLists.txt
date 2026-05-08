add_executable(integration_tests
    test_app_starts.cpp
)

target_link_libraries(integration_tests
    PRIVATE
        sepa::warnings
        Catch2::Catch2WithMain
)

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
