# Backing library for the QML module. Calling qt_add_qml_module here — in the
# same directory as the QML files under qml/ — keeps the QML resource paths
# fully relative. When qt_add_qml_module is invoked from a different directory
# (e.g. src/app/), Qt encodes the upward `../ui/` path into the qmlcache
# filenames and breaks Windows builds with `mkdir(... sepa-xml-viewer_../ui)`,
# since `..` is not a valid path component on Windows.

qt_add_library(sepa_ui_qml STATIC)

qt_add_qml_module(sepa_ui_qml
    URI sepa.viewer
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(sepa_ui_qml
    PRIVATE
        Qt6::Quick
)
