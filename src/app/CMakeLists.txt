qt_add_executable(sepa-xml-viewer
    main.cpp
)

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
