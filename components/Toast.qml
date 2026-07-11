// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

Rectangle {
    id: toast

    property string message: ""
    property bool error: false

    function showMessage(value, isError) {
        message = value
        error = isError
        opacity = 1
        hideTimer.restart()
    }

    width: Math.min(toastText.implicitWidth + 56, parent.width - 40)
    height: 44
    radius: 12
    color: error ? "#D64545" : "#19202E"
    visible: opacity > 0
    opacity: 0

    Behavior on opacity { NumberAnimation { duration: 150 } }

    Row {
        anchors.centerIn: parent
        spacing: 9
        Glyph { width: 18; height: 18; name: toast.error ? "close" : "check"; color: "white"; lineWidth: 2 }
        Text { id: toastText; text: toast.message; color: "white"; font.pixelSize: 12; font.weight: Font.Medium }
    }
    Timer { id: hideTimer; interval: 2800; onTriggered: toast.opacity = 0 }
}
