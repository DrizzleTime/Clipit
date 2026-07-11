// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

Rectangle {
    id: toolbar

    required property string activeTool
    required property string selectedColor
    required property bool paletteVisible
    required property int annotationCount
    required property int selectionPixelWidth
    required property int selectionPixelHeight
    required property string activeToolLabel
    required property bool finishing

    signal toolSelected(string tool)
    signal paletteToggled()
    signal undoRequested()
    signal reselectRequested()
    signal cancelRequested()
    signal confirmRequested()

    width: 404
    height: 92
    radius: 14
    color: "#F21D2028"

    Column {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        Row {
            spacing: 6

            Repeater {
                model: [
                    { tool: "rect", icon: "rect" },
                    { tool: "arrow", icon: "arrow" },
                    { tool: "pen", icon: "pen" },
                    { tool: "text", icon: "text" },
                    { tool: "picker", icon: "picker" },
                    { tool: "blur", icon: "blur" },
                    { tool: "mosaic", icon: "mosaic" }
                ]
                delegate: Rectangle {
                    id: toolButton
                    required property var modelData
                    width: 34
                    height: 34
                    radius: 9
                    color: toolbar.activeTool === toolButton.modelData.tool
                           ? "#5B6EF5" : (toolMouse.containsMouse ? "#444955" : "transparent")
                    Glyph { anchors.centerIn: parent; width: 18; height: 18; name: toolButton.modelData.icon; color: "white" }
                    MouseArea {
                        id: toolMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: toolbar.toolSelected(toolButton.modelData.tool)
                    }
                }
            }

            Rectangle { width: 1; height: 24; anchors.verticalCenter: parent.verticalCenter; color: "#4A4F5B" }

            Rectangle {
                width: 92
                height: 34
                radius: 9
                color: colorMouse.containsMouse || toolbar.paletteVisible ? "#444955" : "transparent"
                Row {
                    anchors.centerIn: parent
                    spacing: 7
                    Rectangle { width: 18; height: 18; radius: 6; color: toolbar.selectedColor; border.color: "#80FFFFFF" }
                    Text { text: toolbar.selectedColor; color: "white"; font.pixelSize: 11; font.family: "monospace" }
                }
                MouseArea {
                    id: colorMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: toolbar.paletteToggled()
                }
            }
        }

        Row {
            spacing: 7

            Text {
                width: 90
                anchors.verticalCenter: parent.verticalCenter
                text: toolbar.selectionPixelWidth + " × " + toolbar.selectionPixelHeight
                color: "#D9DCE5"
                font.pixelSize: 11
                font.family: "monospace"
            }
            Text {
                width: 112
                anchors.verticalCenter: parent.verticalCenter
                text: toolbar.activeToolLabel
                color: "#AEB4C2"
                font.pixelSize: 11
                elide: Text.ElideRight
            }
            Rectangle {
                width: 34; height: 34; radius: 9
                color: undoMouse.containsMouse ? "#444955" : "transparent"
                opacity: toolbar.annotationCount > 0 ? 1 : 0.4
                Glyph { anchors.centerIn: parent; width: 18; height: 18; name: "undo"; color: "white" }
                MouseArea { id: undoMouse; anchors.fill: parent; hoverEnabled: true; enabled: toolbar.annotationCount > 0; cursorShape: Qt.PointingHandCursor; onClicked: toolbar.undoRequested() }
            }
            Rectangle {
                width: 34; height: 34; radius: 9
                color: reselectMouse.containsMouse ? "#444955" : "transparent"
                Glyph { anchors.centerIn: parent; width: 18; height: 18; name: "region"; color: "white" }
                MouseArea { id: reselectMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: toolbar.reselectRequested() }
            }
            Rectangle {
                width: 34; height: 34; radius: 9
                color: cancelMouse.containsMouse ? "#4A4F5B" : "transparent"
                Glyph { anchors.centerIn: parent; width: 18; height: 18; name: "close"; color: "white" }
                MouseArea { id: cancelMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: toolbar.cancelRequested() }
            }
            Rectangle {
                width: 44; height: 34; radius: 9
                color: confirmMouse.pressed ? "#4658E8" : "#5B6EF5"
                opacity: toolbar.finishing ? 0.55 : 1
                Glyph { anchors.centerIn: parent; width: 19; height: 19; name: "check"; color: "white"; lineWidth: 2.2 }
                MouseArea { id: confirmMouse; anchors.fill: parent; enabled: !toolbar.finishing; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: toolbar.confirmRequested() }
            }
        }
    }
}
