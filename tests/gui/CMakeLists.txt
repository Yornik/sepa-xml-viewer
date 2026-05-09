add_executable(gui_tests
    test_window_opens.cpp
)

target_link_libraries(gui_tests
    PRIVATE
        sepa::warnings
        Catch2::Catch2WithMain
)

# MSVC's /WX promotes C4996 ("'getenv': This function or variable may be
# unsafe. Consider using _dupenv_s instead.") into a hard error. Test code
# legitimately calls std::getenv to discover the binary path; suppress the
# warning at the test-target level only — the production binary still gets
# the full warning set.
if(MSVC)
    target_compile_definitions(gui_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Note on Qt plugin discovery:
#
# vcpkg-installed Qt does NOT bake a runtime plugin path into the binary, so a
# bare run of the GUI smoke fails with `qt.qpa.plugin: Could not find the Qt
# platform plugin "offscreen" in ""`. We tried setting QT_PLUGIN_PATH via this
# catch_discover_tests PROPERTIES ENVIRONMENT call — the env var did not
# actually reach the spawned test process for reasons we couldn't trace
# cleanly. Falling back to setting QT_PLUGIN_PATH at the workflow level via
# $GITHUB_ENV before the test step (.github/workflows/ci.yml) is more reliable
# and is what currently does the job. Local builds find Qt's plugins via the
# brew Qt's compiled-in path, so they don't need QT_PLUGIN_PATH set.
#
# If you make this CMake-level setting work in the future, the env-var spec is:
#     SEPA_VIEWER_BINARY=$<TARGET_FILE:sepa-xml-viewer>;QT_PLUGIN_PATH=<plugins>
# but the workflow setup will still work as a belt-and-suspenders.

catch_discover_tests(gui_tests
    TEST_PREFIX "gui."
    PROPERTIES
        ENVIRONMENT "SEPA_VIEWER_BINARY=$<TARGET_FILE:sepa-xml-viewer>"
)

add_dependencies(gui_tests sepa-xml-viewer)
