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

# Locate Qt's plugin directory so the binary can find its platform plugins
# (offscreen, etc.) at runtime. vcpkg-installed Qt does NOT bake an RPATH that
# points at the plugins directory, so without QT_PLUGIN_PATH the runtime fails
# with `qt.qpa.plugin: Could not find the Qt platform plugin "offscreen" in ""`.
get_target_property(_qt6_core_loc Qt6::Core LOCATION)
get_filename_component(_qt6_lib_dir "${_qt6_core_loc}" DIRECTORY)
set(_qt6_plugins_dir "${_qt6_lib_dir}/../plugins")
cmake_path(NORMAL_PATH _qt6_plugins_dir)

# The GUI smoke launches the actual binary with QT_QPA_PLATFORM=offscreen so it
# works on CI runners that have no display. CMake provides the path the same
# way the integration layer does, plus QT_PLUGIN_PATH as discussed above.
catch_discover_tests(gui_tests
    TEST_PREFIX "gui."
    PROPERTIES
        ENVIRONMENT "SEPA_VIEWER_BINARY=$<TARGET_FILE:sepa-xml-viewer>;QT_PLUGIN_PATH=${_qt6_plugins_dir}"
)

add_dependencies(gui_tests sepa-xml-viewer)
