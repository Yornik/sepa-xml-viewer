add_executable(gui_tests
    test_window_opens.cpp
)

target_link_libraries(gui_tests
    PRIVATE
        sepa::warnings
        Catch2::Catch2WithMain
)

# The GUI smoke launches the actual binary with QT_QPA_PLATFORM=offscreen so it
# works on CI runners that have no display. CMake provides the path the same
# way the integration layer does.
catch_discover_tests(gui_tests
    TEST_PREFIX "gui."
    PROPERTIES
        ENVIRONMENT "SEPA_VIEWER_BINARY=$<TARGET_FILE:sepa-xml-viewer>"
)

add_dependencies(gui_tests sepa-xml-viewer)
