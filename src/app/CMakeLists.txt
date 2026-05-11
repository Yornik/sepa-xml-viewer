qt_add_executable(sepa-xml-viewer
    main.cpp
)

# vcpkg builds Qt6 statically on Linux (the platforms/ directory contains
# `.a` archives, not `.so` shared libraries). With static Qt, platform
# plugins MUST be linked into the binary at compile time via
# qt_import_plugins — the runtime plugin loader cannot dlopen() a `.a`
# file. See https://doc.qt.io/qt-6/qt-import-plugins.html.
#
# minimal is the lowest-common-denominator headless platform plugin (no
# rendering surface, just enough to satisfy QGuiApplication). It ships in
# every qtbase[gui] build. offscreen is more capable (supports off-screen
# rendering for tests that take screenshots) but vcpkg's qtbase port does
# not always build it on Linux, so we only import it if its target exists.
set(_sepa_static_platform_plugins Qt6::QMinimalIntegrationPlugin)
if(TARGET Qt6::QOffscreenIntegrationPlugin)
    list(APPEND _sepa_static_platform_plugins Qt6::QOffscreenIntegrationPlugin)
endif()
qt_import_plugins(sepa-xml-viewer INCLUDE ${_sepa_static_platform_plugins})

target_include_directories(sepa-xml-viewer
    PRIVATE
        ${CMAKE_BINARY_DIR}/include
)

# QML module is owned by the sepa_ui_qml backing library in src/ui/.
# Linking the auto-generated `sepa_ui_qmlplugin` static archive registers the
# QML types into this executable. See src/ui/CMakeLists.txt for the rationale.
target_link_libraries(sepa-xml-viewer
    PRIVATE
        sepa::warnings
        sepa_ui_qml
        sepa_ui_qmlplugin
        Qt6::Gui
        Qt6::Quick
)

set_target_properties(sepa-xml-viewer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    # On Windows in Release configurations, hide the console window — desktop
    # users expect a GUI app, not a flashing cmd.exe. Debug builds keep the
    # console for diagnostics.
    WIN32_EXECUTABLE $<CONFIG:Release>
    # macOS .dmg packaging requires a bundle. Bundle on by default also makes
    # the dev workflow `open ./build/.../bin/sepa-xml-viewer.app` work.
    MACOSX_BUNDLE ON
)

# Install rules used by CPack. Per platform:
#   Linux:   binary -> /usr/bin/, .desktop and icon installed by Packaging.cmake
#   macOS:   bundle -> /Applications/sepa-xml-viewer.app
#   Windows: binary -> <install_prefix>/bin/sepa-xml-viewer.exe
#            (windeployqt copies Qt's own DLLs alongside it; the matching
#            install(RUNTIME_DEPENDENCY_SET sepa_runtime_deps ...) call in
#            cmake/Packaging.cmake then walks the executable's full DLL graph
#            and copies non-Qt transitive deps such as z.dll that windeployqt
#            doesn't follow. Tagging the target here is the "register"; the
#            "deploy" half lives next to the rest of the Windows packaging.)
install(TARGETS sepa-xml-viewer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION bin
    RUNTIME_DEPENDENCY_SET sepa_runtime_deps
)
