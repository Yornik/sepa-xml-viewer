import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import Qt.labs.qmlmodels
import sepa.viewer

ApplicationWindow {
    id: root
    width: 1180
    height: 760
    minimumWidth: 720
    minimumHeight: 440
    visible: true
    title: SepaController.currentFilePath === ""
           ? qsTr("SEPA XML Viewer")
           : qsTr("SEPA XML Viewer — %1").arg(SepaController.currentFilePath)

    // Material theme + accent picked once at the window level — every
    // Controls element inherits these via Material attached properties.
    // System mirrors the OS light/dark setting; flip to Material.Dark or
    // Material.Light to override.
    Material.theme: Material.System
    Material.accent: Material.Indigo
    Material.primary: Material.Indigo

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Open…")
                shortcut: StandardKey.Open
                onTriggered: openDialog.open()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Quit")
                shortcut: StandardKey.Quit
                onTriggered: Qt.quit()
            }
        }
    }

    FileDialog {
        id: openDialog
        title: qsTr("Open a SEPA XML file")
        nameFilters: [qsTr("SEPA XML files (*.xml)"), qsTr("All files (*)")]
        onAccepted: SepaController.loadFile(selectedFile)
    }

    // Drop area covers the entire window so a file can be dropped anywhere.
    DropArea {
        anchors.fill: parent
        onDropped: function(drop) {
            if (drop.hasUrls && drop.urls.length > 0) {
                SepaController.loadFile(drop.urls[0]);
                drop.acceptProposedAction();
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Error / info banner above the content. Hidden when there's no
        // message or when the drop-here screen is taking care of it.
        Rectangle {
            Layout.fillWidth: true
            visible: SepaController.statusMessage !== "" && SepaController.hasContent
            color: SepaController.isError ? "#fde7e9" : "#e7f3ff"
            Layout.preferredHeight: bannerLabel.implicitHeight + 16
            Label {
                id: bannerLabel
                anchors.fill: parent
                anchors.margins: 8
                text: SepaController.statusMessage
                color: SepaController.isError ? "#a00" : "#036"
                wrapMode: Text.WordWrap
            }
        }

        // Drop-here screen — shown until a file is successfully loaded.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !SepaController.hasContent

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 18
                width: Math.min(parent.width - 60, 620)

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("SEPA XML Viewer")
                    font.pixelSize: 34
                    font.weight: Font.Medium
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: SepaController.statusMessage === ""
                          ? qsTr("Drop a SEPA file here, or use File → Open…")
                          : SepaController.statusMessage
                    color: SepaController.isError ? "#a00" : "#666"
                    wrapMode: Text.WordWrap
                    font.pixelSize: 14
                    Layout.fillWidth: true
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("This MVP supports pain.001.001.13 only. " +
                               "Other ISO 20022 SEPA versions arrive in a later release.")
                    opacity: 0.55
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Open SEPA file…")
                    highlighted: true
                    onClicked: openDialog.open()
                }
            }
        }

        // Main viewer — visible once a file is parsed.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: SepaController.hasContent

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                TabBar {
                    id: tabs
                    Layout.fillWidth: true
                    TabButton { text: qsTr("Tree + Detail") }
                    TabButton { text: qsTr("Raw XML") }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: tabs.currentIndex

                    // ── Tab 1: Tree + Detail ─────────────────────────────
                    SplitView {
                        orientation: Qt.Horizontal

                        ScrollView {
                            SplitView.preferredWidth: 420
                            SplitView.minimumWidth: 240
                            clip: true

                            TreeView {
                                id: tree
                                model: SepaController.treeModel
                                selectionModel: ItemSelectionModel { id: sel }
                                anchors.fill: parent
                                rowSpacing: 0
                                Connections {
                                    target: SepaController
                                    function onStateChanged() {
                                        if (SepaController.hasContent) {
                                            tree.expandRecursively(-1, 1);
                                            const rootIdx = tree.index(0, 0);
                                            sel.setCurrentIndex(rootIdx,
                                                                ItemSelectionModel.ClearAndSelect);
                                        }
                                    }
                                }

                                delegate: TreeViewDelegate {
                                    required property var model
                                    contentItem: Label {
                                        text: model.display
                                        elide: Label.ElideRight
                                        font.pixelSize: 13
                                    }
                                }
                            }
                        }

                        // Detail pane — fields for the current selection.
                        ScrollView {
                            SplitView.fillWidth: true
                            SplitView.minimumWidth: 280
                            clip: true

                            ListView {
                                id: detail
                                anchors.fill: parent
                                model: sel.currentIndex.valid
                                       ? SepaController.fieldsForIndex(sel.currentIndex)
                                       : []
                                spacing: 0
                                clip: true
                                header: Rectangle {
                                    width: detail.width
                                    height: 36
                                    color: root.Material.theme === Material.Dark
                                           ? "#2c2c2c" : "#f0f0f0"
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        Label {
                                            text: qsTr("Field")
                                            font.bold: true
                                            Layout.preferredWidth: 220
                                            opacity: 0.8
                                        }
                                        Label {
                                            text: qsTr("Value")
                                            font.bold: true
                                            opacity: 0.8
                                            Layout.fillWidth: true
                                        }
                                    }
                                }

                                delegate: Item {
                                    width: detail.width
                                    height: 30
                                    Rectangle {
                                        anchors.fill: parent
                                        color: index % 2 === 0
                                               ? "transparent"
                                               : (root.Material.theme === Material.Dark
                                                  ? "#222222" : "#fafafa")
                                    }
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        Label {
                                            Layout.preferredWidth: 220
                                            text: modelData.name
                                            opacity: 0.65
                                            elide: Label.ElideRight
                                            font.pixelSize: 12
                                        }
                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.value
                                            elide: Label.ElideRight
                                            wrapMode: Text.NoWrap
                                            font.pixelSize: 13
                                        }
                                    }
                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        width: parent.width
                                        height: 1
                                        color: root.Material.theme === Material.Dark
                                               ? "#2a2a2a" : "#eee"
                                    }
                                }
                            }
                        }
                    }

                    // ── Tab 2: Raw XML ───────────────────────────────────
                    ScrollView {
                        clip: true
                        TextArea {
                            readOnly: true
                            selectByMouse: true
                            wrapMode: TextEdit.NoWrap
                            font.family: "monospace"
                            font.pixelSize: 12
                            text: SepaController.rawXml
                        }
                    }
                }
            }
        }

        // Status bar at the bottom — file stats, always visible once loaded.
        Rectangle {
            Layout.fillWidth: true
            visible: SepaController.hasContent
            Layout.preferredHeight: 26
            color: root.Material.theme === Material.Dark ? "#1f1f1f" : "#f5f5f5"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 18

                Label {
                    text: SepaController.summary
                    font.pixelSize: 11
                    opacity: 0.7
                }
                Item { Layout.fillWidth: true }
                Label {
                    text: SepaController.currentFilePath
                    font.pixelSize: 11
                    opacity: 0.5
                    elide: Label.ElideLeft
                    Layout.maximumWidth: 460
                }
            }
        }
    }
}
