import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 800
    height: 600
    minimumWidth: 480
    minimumHeight: 320
    visible: true
    title: qsTr("SEPA XML Viewer")

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 16

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("SEPA XML Viewer")
            font.pixelSize: 28
            font.weight: Font.Medium
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Phase 0 build — drag-and-drop and the SEPA UX arrive later (see plan/02-viewer-ui.md).")
            opacity: 0.7
            wrapMode: Text.WordWrap
            Layout.maximumWidth: 480
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
