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
    WIN32_EXECUTABLE OFF
    MACOSX_BUNDLE OFF
)
