qt_add_executable(sepa-xml-viewer
    main.cpp
)

# qt_add_qml_module rejects absolute QML paths unless QT_RESOURCE_ALIAS is set.
# The QML files live under src/ui/qml/, away from this CMakeLists.txt; the alias
# strips the on-disk prefix so the resource system sees the file at qrc:/qt/qml/sepa/viewer/Main.qml.
set_source_files_properties(
    "${CMAKE_SOURCE_DIR}/src/ui/qml/Main.qml"
    PROPERTIES QT_RESOURCE_ALIAS Main.qml
)

qt_add_qml_module(sepa-xml-viewer
    URI sepa.viewer
    VERSION 1.0
    QML_FILES
        "${CMAKE_SOURCE_DIR}/src/ui/qml/Main.qml"
)

target_include_directories(sepa-xml-viewer
    PRIVATE
        ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(sepa-xml-viewer
    PRIVATE
        sepa::warnings
        Qt6::Gui
        Qt6::Quick
)

set_target_properties(sepa-xml-viewer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    WIN32_EXECUTABLE OFF
    MACOSX_BUNDLE OFF
)
