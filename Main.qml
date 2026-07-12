// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import Clipit

ApplicationWindow {
    id: window
    required property ScreenshotService screenshotService
    width: 620
    height: screenshotService.hasPreview
            ? (screenshotService.wayland ? 640 : 570)
            : (screenshotService.wayland ? 500 : 430)
    minimumWidth: 520
    minimumHeight: 420
    visible: true
    title: qsTr("Clipit · 截图")
    color: darkMode ? "#17191F" : "#F5F6F8"

    readonly property bool darkMode: Application.styleHints.colorScheme === Qt.Dark
    readonly property color textPrimary: darkMode ? "#F5F7FA" : "#172033"
    readonly property color textSecondary: darkMode ? "#A9B0BF" : "#667085"
    readonly property color panel: darkMode ? "#22252D" : "#FFFFFF"
    readonly property color border: darkMode ? "#343844" : "#E6E8EC"
    readonly property color accent: "#5B6EF5"
    readonly property color accentDark: "#4658E8"

    function restoreWindow() {
        window.show()
        window.raise()
        window.requestActivate()
    }

    Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

    CaptureOverlay {
        id: captureOverlay
        screenshotService: window.screenshotService
    }

    FileDialog {
        id: saveDialog
        title: qsTr("保存截图副本")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("PNG 图片 (*.png)")]
        defaultSuffix: "png"
        currentFolder: window.screenshotService.saveDirectoryUrl
        onAccepted: window.screenshotService.saveCopy(selectedFile)
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("选择截图保存目录")
        currentFolder: window.screenshotService.saveDirectoryUrl
        onAccepted: window.screenshotService.setSaveDirectory(selectedFolder)
    }

    Popup {
        id: delayPopup
        width: 232
        height: 142
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: Overlay.overlay
        padding: 0

        background: Rectangle {
            color: window.panel
            radius: 14
            border.color: window.border
        }

        contentItem: Column {
            padding: 18
            spacing: 14

            Text {
                text: qsTr("延时区域截图")
                color: window.textPrimary
                font.pixelSize: 15
                font.weight: Font.DemiBold
            }

            Row {
                spacing: 8
                Repeater {
                    model: [3, 5, 10]
                    delegate: Rectangle {
                        id: delayOption
                        required property int modelData
                        width: 58
                        height: 40
                        radius: 9
                        color: delayMouse.containsMouse ? window.accent : (window.darkMode ? "#2D313C" : "#F2F4F7")

                        Text {
                            anchors.centerIn: parent
                            text: delayOption.modelData + qsTr(" 秒")
                            color: delayMouse.containsMouse ? "white" : window.textPrimary
                            font.pixelSize: 13
                            font.weight: Font.Medium
                        }

                        MouseArea {
                            id: delayMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                delayPopup.close()
                                window.screenshotService.captureRegion(delayOption.modelData)
                            }
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: settingsPopup
        width: Math.min(440, window.width - 40)
        height: 196
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: Overlay.overlay
        padding: 0

        background: Rectangle {
            color: window.panel
            radius: 16
            border.color: window.border
        }

        contentItem: Column {
            padding: 22
            spacing: 18

            Row {
                width: parent.width - 44

                Text {
                    width: parent.width - 36
                    text: qsTr("设置")
                    color: window.textPrimary
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                }

                Rectangle {
                    width: 32
                    height: 32
                    radius: 8
                    color: settingsClose.containsMouse ? (window.darkMode ? "#343844" : "#F0F2F5") : "transparent"
                    Glyph { anchors.centerIn: parent; width: 18; height: 18; name: "close"; color: window.textSecondary }
                    MouseArea {
                        id: settingsClose
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: settingsPopup.close()
                    }
                }
            }

            Column {
                width: parent.width - 44
                spacing: 8
                Text { text: qsTr("保存位置"); color: window.textPrimary; font.pixelSize: 14; font.weight: Font.Medium }

                Rectangle {
                    width: parent.width
                    height: 48
                    radius: 10
                    color: window.darkMode ? "#2A2E37" : "#F7F8FA"
                    border.color: window.border

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 13
                        anchors.rightMargin: 8
                        spacing: 10
                        Glyph { width: 19; height: 19; anchors.verticalCenter: parent.verticalCenter; name: "folder"; color: window.textSecondary }
                        Text {
                            width: parent.width - 93
                            anchors.verticalCenter: parent.verticalCenter
                            text: window.screenshotService.saveDirectory
                            color: window.textSecondary
                            font.pixelSize: 12
                            elide: Text.ElideMiddle
                        }
                        Rectangle {
                            width: 50
                            height: 32
                            anchors.verticalCenter: parent.verticalCenter
                            radius: 8
                            color: chooseMouse.containsMouse ? window.accent : (window.darkMode ? "#393E4A" : "#ECEFFE")
                            Text { anchors.centerIn: parent; text: qsTr("更改"); color: chooseMouse.containsMouse ? "white" : window.accent; font.pixelSize: 12; font.weight: Font.Medium }
                            MouseArea {
                                id: chooseMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: folderDialog.open()
                            }
                        }
                    }
                }
            }
        }
    }

    header: Rectangle {
        height: 68
        color: window.panel
        border.color: window.border

        Row {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 18
            spacing: 12

            Rectangle {
                width: 34
                height: 34
                anchors.verticalCenter: parent.verticalCenter
                radius: 10
                color: window.accent
                Glyph { anchors.centerIn: parent; width: 21; height: 21; name: "capture"; color: "white"; lineWidth: 1.9 }
            }

            Text {
                width: parent.width - 92
                anchors.verticalCenter: parent.verticalCenter
                text: "Clipit"
                color: window.textPrimary
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }

            Rectangle {
                width: 38
                height: 38
                anchors.verticalCenter: parent.verticalCenter
                radius: 10
                color: settingsMouse.containsMouse ? (window.darkMode ? "#30343E" : "#F1F3F6") : "transparent"

                Glyph { anchors.centerIn: parent; width: 20; height: 20; name: "settings"; color: window.textSecondary }
                MouseArea {
                    id: settingsMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: settingsPopup.open()
                }
            }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Column {
            width: parent.width
            spacing: 5
            Text {
                text: qsTr("捕捉屏幕上的内容")
                color: window.textPrimary
                font.pixelSize: 23
                font.weight: Font.DemiBold
            }
            Text {
                text: window.screenshotService.wayland
                      ? qsTr("安全读取画面后，由 Clipit 完成框选与标注")
                      : qsTr("拖动框选区域，松开后即可保存")
                color: window.textSecondary
                font.pixelSize: 13
            }
        }

        Rectangle {
            width: parent.width
            height: 94
            radius: 15
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: primaryMouse.pressed ? "#4658E8" : "#5B6EF5" }
                GradientStop { position: 1; color: primaryMouse.pressed ? "#704BCC" : "#8867E8" }
            }

            Row {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 16

                Rectangle {
                    width: 52
                    height: 52
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 14
                    color: "#28FFFFFF"
                    Glyph { anchors.centerIn: parent; width: 28; height: 28; name: "region"; color: "white"; lineWidth: 1.9 }
                }

                Column {
                    width: parent.width - 126
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
                    Text { text: qsTr("区域截图"); color: "white"; font.pixelSize: 18; font.weight: Font.DemiBold }
                    Text { text: window.screenshotService.wayland ? qsTr("通过 portal 获取画面并进入编辑器") : qsTr("自由框选并标注需要的画面"); color: "#DDE2FF"; font.pixelSize: 12 }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "›"
                    color: "white"
                    font.pixelSize: 30
                    font.weight: Font.Light
                }
            }

            MouseArea {
                id: primaryMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                enabled: !window.screenshotService.busy
                onClicked: window.screenshotService.captureRegion(0)
            }
        }

        Row {
            width: parent.width
            height: 82
            spacing: 12

            Repeater {
                model: [
                    { icon: "window", title: qsTr("窗口截图"), note: window.screenshotService.wayland ? qsTr("由系统选择窗口") : qsTr("当前活动窗口"), action: "window" },
                    { icon: "screen", title: qsTr("全屏截图"), note: qsTr("立即捕获"), action: "screen" },
                    { icon: "clock", title: qsTr("延时截图"), note: qsTr("3 / 5 / 10 秒"), action: "delay" }
                ]
                delegate: Rectangle {
                    id: actionCard
                    required property var modelData
                    width: (parent.width - 24) / 3
                    height: parent.height
                    radius: 13
                    color: secondaryMouse.containsMouse ? (window.darkMode ? "#292D36" : "#F9FAFF") : window.panel
                    border.color: secondaryMouse.containsMouse ? window.accent : window.border

                    Row {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 12
                        Rectangle {
                            width: 40
                            height: 40
                            anchors.verticalCenter: parent.verticalCenter
                            radius: 11
                            color: window.darkMode ? "#343947" : "#EEF0FF"
                            Glyph { anchors.centerIn: parent; width: 21; height: 21; name: actionCard.modelData.icon; color: window.accent }
                        }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 3
                            Text { text: actionCard.modelData.title; color: window.textPrimary; font.pixelSize: 14; font.weight: Font.DemiBold }
                            Text { text: actionCard.modelData.note; color: window.textSecondary; font.pixelSize: 11 }
                        }
                    }

                    MouseArea {
                        id: secondaryMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        enabled: !window.screenshotService.busy
                        onClicked: {
                            if (actionCard.modelData.action === "window")
                                window.screenshotService.captureWindow(0)
                            else if (actionCard.modelData.action === "screen")
                                window.screenshotService.captureFullscreen(0)
                            else
                                delayPopup.open()
                        }
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 52
            radius: 11
            visible: window.screenshotService.wayland
            color: window.darkMode ? "#252B35" : "#EFF7F4"
            border.color: window.screenshotService.portalAvailable ? (window.darkMode ? "#31483F" : "#CDE8DD") : "#E7B4B4"

            Row {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 11
                Glyph {
                    width: 20; height: 20; anchors.verticalCenter: parent.verticalCenter
                    name: "shield"; color: window.screenshotService.portalAvailable ? "#21966B" : "#D64545"
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Text { text: window.screenshotService.portalAvailable ? qsTr("Wayland 安全捕获已就绪") : qsTr("Wayland 截图门户不可用"); color: window.textPrimary; font.pixelSize: 12; font.weight: Font.Medium }
                    Text { text: qsTr("系统负责安全交付画面，框选和标注由 Clipit 完成"); color: window.textSecondary; font.pixelSize: 10 }
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 120
            radius: 13
            visible: window.screenshotService.hasPreview
            color: window.panel
            border.color: window.border

            Row {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 14

                Rectangle {
                    width: 142
                    height: 96
                    radius: 9
                    color: window.darkMode ? "#181A20" : "#EEF0F3"
                    clip: true
                    Image {
                        anchors.fill: parent
                        source: window.screenshotService.previewUrl
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: false
                    }
                }

                Column {
                    width: parent.width - 156
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 12
                    Column {
                        width: parent.width
                        spacing: 3
                        Text { text: qsTr("最近截图"); color: window.textSecondary; font.pixelSize: 11 }
                        Text { width: parent.width; text: window.screenshotService.lastFileName; color: window.textPrimary; font.pixelSize: 13; font.weight: Font.Medium; elide: Text.ElideMiddle }
                    }
                    Row {
                        spacing: 8
                        Rectangle {
                            width: 92; height: 34; radius: 8
                            color: copyMouse.containsMouse ? window.accent : (window.darkMode ? "#343947" : "#EEF0FF")
                            Row { anchors.centerIn: parent; spacing: 6; Glyph { width: 16; height: 16; name: "copy"; color: copyMouse.containsMouse ? "white" : window.accent } Text { text: qsTr("复制"); color: copyMouse.containsMouse ? "white" : window.accent; font.pixelSize: 12; font.weight: Font.Medium } }
                            MouseArea { id: copyMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: window.screenshotService.copyLatest() }
                        }
                        Rectangle {
                            width: 92; height: 34; radius: 8
                            color: folderMouse.containsMouse ? (window.darkMode ? "#3A3E48" : "#F0F2F5") : "transparent"
                            border.color: window.border
                            Row { anchors.centerIn: parent; spacing: 6; Glyph { width: 16; height: 16; name: "folder"; color: window.textSecondary } Text { text: qsTr("打开目录"); color: window.textPrimary; font.pixelSize: 12; font.weight: Font.Medium } }
                            MouseArea { id: folderMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: window.screenshotService.openSaveDirectory() }
                        }
                        Rectangle {
                            width: 72; height: 34; radius: 8
                            color: saveMouse.containsMouse ? (window.darkMode ? "#3A3E48" : "#F0F2F5") : "transparent"
                            border.color: window.border
                            Text { anchors.centerIn: parent; text: qsTr("另存为"); color: window.textPrimary; font.pixelSize: 12; font.weight: Font.Medium }
                            MouseArea { id: saveMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: saveDialog.open() }
                        }
                    }
                }
            }
        }

        Item { width: 1; height: 1 }
    }

    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: visible ? 18 : -50
        z: 100
        Behavior on anchors.bottomMargin { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
    }

    Connections {
        target: window.screenshotService
        function onCaptureStarted() { window.hide() }
        function onCaptureSaved(path) { window.restoreWindow() }
        function onCaptureCanceled(message) {
            window.restoreWindow()
            toast.showMessage(message, false)
        }
        function onCaptureCompleted(message) { toast.showMessage(message, false) }
        function onCaptureFailed(message) {
            window.restoreWindow()
            toast.showMessage(message, true)
        }
    }
}
