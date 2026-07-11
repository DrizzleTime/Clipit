// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

Rectangle {
    id: palette

    required property string selectedColor
    signal colorSelected(string color)

    width: 244
    height: 46
    radius: 12
    color: "#F21D2028"

    Row {
        anchors.centerIn: parent
        spacing: 5
        Repeater {
            model: ["#FF5967", "#FFB020", "#36B37E", "#3B82F6",
                    "#8B5CF6", "#00C2FF", "#FFFFFF", "#111827"]
            delegate: Rectangle {
                id: swatch
                required property string modelData
                width: 24
                height: 24
                radius: 7
                color: swatch.modelData
                border.width: palette.selectedColor.toUpperCase() === swatch.modelData ? 2 : 1
                border.color: palette.selectedColor.toUpperCase() === swatch.modelData ? "white" : "#60FFFFFF"
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: palette.colorSelected(swatch.modelData)
                }
            }
        }
    }
}
