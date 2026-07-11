// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import Clipit

Window {
    id: overlay
    required property ScreenshotService screenshotService
    visible: false
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    title: qsTr("选择并标注截图")

    property real startX: 0
    property real startY: 0
    property real selectionX: 0
    property real selectionY: 0
    property real selectionWidth: 0
    property real selectionHeight: 0
    property real pointerX: 0
    property real pointerY: 0
    property real textLocalX: 0
    property real textLocalY: 0
    property bool editing: false
    property bool finishing: false
    property bool colorPaletteVisible: false
    property bool textEditing: false
    property string activeTool: ""
    property string previousDrawTool: "pen"
    property string selectedColor: "#FF5967"
    property string hoveredPixelColor: "#000000"
    property var annotations: []
    property var draftAnnotation: null
    readonly property color shadeColor: "#52000000"
    readonly property bool selectionValid: selectionWidth >= 8 && selectionHeight >= 8
    readonly property real imageLeft: (width - baseImage.paintedWidth) / 2
    readonly property real imageTop: (height - baseImage.paintedHeight) / 2
    readonly property real imageRight: imageLeft + baseImage.paintedWidth
    readonly property real imageBottom: imageTop + baseImage.paintedHeight
    readonly property real selectableLeft: imageLeft
    readonly property real selectableTop: imageTop
    readonly property real selectableRight: imageRight
    readonly property real selectableBottom: imageBottom
    readonly property real imageScaleX: baseImage.sourceSize.width > 0
                                        && baseImage.paintedWidth > 0
                                        ? baseImage.sourceSize.width / baseImage.paintedWidth : 1
    readonly property real imageScaleY: baseImage.sourceSize.height > 0
                                        && baseImage.paintedHeight > 0
                                        ? baseImage.sourceSize.height / baseImage.paintedHeight : 1

    onAnnotationsChanged: annotationLayer.requestPaint()
    onDraftAnnotationChanged: annotationLayer.requestPaint()
    onClosing: close => {
        close.accepted = false
        overlay.cancel()
    }

    function showOverlay() {
        selectionX = 0
        selectionY = 0
        selectionWidth = 0
        selectionHeight = 0
        pointerX = 0
        pointerY = 0
        editing = false
        finishing = false
        colorPaletteVisible = false
        textEditing = false
        activeTool = ""
        previousDrawTool = "pen"
        annotations = []
        draftAnnotation = null
        showFullScreen()
        requestActivate()
    }

    function clampImageX(value) {
        return Math.max(selectableLeft, Math.min(selectableRight, value))
    }

    function clampImageY(value) {
        return Math.max(selectableTop, Math.min(selectableBottom, value))
    }

    function cancel() {
        if (finishing)
            return
        hide()
        overlay.screenshotService.cancelSelection()
    }

    function handleEscape() {
        if (textEditing) {
            textEditing = false
            textInput.text = ""
        } else if (colorPaletteVisible) {
            colorPaletteVisible = false
        } else {
            cancel()
        }
    }

    function resetSelection() {
        editing = false
        activeTool = ""
        colorPaletteVisible = false
        textEditing = false
        textInput.text = ""
        annotations = []
        draftAnnotation = null
        selectionWidth = 0
        selectionHeight = 0
        annotationLayer.requestPaint()
    }

    function undo() {
        if (annotations.length === 0)
            return
        annotations = annotations.slice(0, annotations.length - 1)
    }

    function selectTool(tool) {
        if (textEditing)
            commitText()
        colorPaletteVisible = false
        if (tool !== "picker" && tool !== "blur" && tool !== "mosaic")
            previousDrawTool = tool
        activeTool = activeTool === tool ? "" : tool
    }

    function beginText(point) {
        textLocalX = point.x
        textLocalY = point.y
        textInput.text = ""
        textEditing = true
        Qt.callLater(function() { textInput.forceActiveFocus() })
    }

    function commitText() {
        if (!textEditing)
            return
        const value = textInput.text.trim()
        textEditing = false
        textInput.text = ""
        if (value.length === 0)
            return
        annotations = annotations.concat([{
            type: "text",
            x1: textLocalX,
            y1: textLocalY,
            text: value,
            color: selectedColor,
            width: 1,
            fontSize: 18
        }])
    }

    function confirm() {
        if (!selectionValid || finishing)
            return
        if (textEditing)
            commitText()
        finishing = true
        const resultAnnotations = annotations
        hide()
        overlay.screenshotService.finishAnnotatedSelection(
                    selectionX - imageLeft, selectionY - imageTop,
                    selectionWidth, selectionHeight,
                    baseImage.paintedWidth, baseImage.paintedHeight,
                    resultAnnotations)
        finishing = false
    }

    function pointInsideSelection(x, y) {
        return x >= selectionX && x <= selectionX + selectionWidth
                && y >= selectionY && y <= selectionY + selectionHeight
    }

    function localPoint(x, y) {
        return {
            x: Math.max(0, Math.min(selectionWidth, x - selectionX)),
            y: Math.max(0, Math.min(selectionHeight, y - selectionY))
        }
    }

    function toolLabel(tool) {
        if (tool === "rect") return qsTr("矩形")
        if (tool === "arrow") return qsTr("箭头")
        if (tool === "pen") return qsTr("画笔")
        if (tool === "text") return qsTr("文字")
        if (tool === "picker") return qsTr("像素取色")
        if (tool === "blur") return qsTr("模糊")
        if (tool === "mosaic") return qsTr("马赛克")
        return qsTr("未选择工具")
    }

    Shortcut { sequence: "Escape"; enabled: overlay.visible && !overlay.textEditing; onActivated: overlay.handleEscape() }
    Shortcut { sequence: "Return"; enabled: overlay.visible && overlay.selectionValid && !overlay.textEditing; onActivated: overlay.confirm() }
    Shortcut { sequence: "Ctrl+Z"; enabled: overlay.visible && overlay.editing && !overlay.textEditing; onActivated: overlay.undo() }

    Connections {
        target: overlay.screenshotService
        function onSelectionRequested() { overlay.showOverlay() }
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        z: -2
    }

    Image {
        id: baseImage
        anchors.fill: parent
        source: overlay.screenshotService.selectionSource
        fillMode: Image.PreserveAspectFit
        cache: false
        asynchronous: false
    }

    AnnotationLayer {
        id: annotationLayer
        x: overlay.selectionX
        y: overlay.selectionY
        width: overlay.selectionWidth
        height: overlay.selectionHeight
        clip: true
        visible: overlay.selectionValid
        z: 1
        source: overlay.screenshotService.selectionSource
        annotations: overlay.annotations
        draftAnnotation: overlay.draftAnnotation
        viewportWidth: overlay.width
        viewportHeight: overlay.height
    }

    Rectangle {
        anchors.fill: parent
        color: overlay.shadeColor
        visible: !overlay.selectionValid
        z: 2
    }
    Rectangle { x: 0; y: 0; width: parent.width; height: overlay.selectionY; color: overlay.shadeColor; visible: overlay.selectionValid; z: 2 }
    Rectangle { x: 0; y: overlay.selectionY; width: overlay.selectionX; height: overlay.selectionHeight; color: overlay.shadeColor; visible: overlay.selectionValid; z: 2 }
    Rectangle { x: overlay.selectionX + overlay.selectionWidth; y: overlay.selectionY; width: parent.width - x; height: overlay.selectionHeight; color: overlay.shadeColor; visible: overlay.selectionValid; z: 2 }
    Rectangle { x: 0; y: overlay.selectionY + overlay.selectionHeight; width: parent.width; height: parent.height - y; color: overlay.shadeColor; visible: overlay.selectionValid; z: 2 }

    Rectangle {
        x: overlay.selectionX
        y: overlay.selectionY
        width: overlay.selectionWidth
        height: overlay.selectionHeight
        color: "transparent"
        border.width: 2
        border.color: "#6F80FF"
        visible: overlay.selectionValid
        z: 3
    }

    MouseArea {
        id: selectMouse
        anchors.fill: parent
        enabled: !overlay.textEditing
        cursorShape: !overlay.editing ? Qt.CrossCursor
                     : overlay.activeTool === "text" ? Qt.IBeamCursor
                     : overlay.activeTool !== "" ? Qt.CrossCursor : Qt.ArrowCursor
        z: 5

        onPressed: mouse => {
            if (overlay.finishing)
                return
            overlay.pointerX = mouse.x
            overlay.pointerY = mouse.y
            if (!overlay.editing) {
                const imageX = overlay.clampImageX(mouse.x)
                const imageY = overlay.clampImageY(mouse.y)
                overlay.startX = imageX
                overlay.startY = imageY
                overlay.selectionX = imageX
                overlay.selectionY = imageY
                overlay.selectionWidth = 0
                overlay.selectionHeight = 0
                return
            }
            if (overlay.activeTool === "" || !overlay.pointInsideSelection(mouse.x, mouse.y))
                return

            if (overlay.activeTool === "picker") {
                const picked = overlay.screenshotService.pixelColor(
                            mouse.x - overlay.imageLeft, mouse.y - overlay.imageTop,
                            baseImage.paintedWidth, baseImage.paintedHeight)
                overlay.selectedColor = picked
                overlay.hoveredPixelColor = picked
                overlay.activeTool = overlay.previousDrawTool
                return
            }

            const p = overlay.localPoint(mouse.x, mouse.y)
            if (overlay.activeTool === "text") {
                overlay.beginText(p)
                return
            }

            overlay.draftAnnotation = {
                type: overlay.activeTool,
                color: overlay.selectedColor,
                width: overlay.activeTool === "pen" ? 4 : 3,
                x1: p.x, y1: p.y, x2: p.x, y2: p.y,
                points: [{ x: p.x, y: p.y }]
            }
        }

        onPositionChanged: mouse => {
            overlay.pointerX = mouse.x
            overlay.pointerY = mouse.y
            if (overlay.editing && overlay.activeTool === "picker"
                    && overlay.pointInsideSelection(mouse.x, mouse.y)) {
                overlay.hoveredPixelColor = overlay.screenshotService.pixelColor(
                            mouse.x - overlay.imageLeft, mouse.y - overlay.imageTop,
                            baseImage.paintedWidth, baseImage.paintedHeight)
            }
            if (!pressed || overlay.finishing)
                return
            if (!overlay.editing) {
                const imageX = overlay.clampImageX(mouse.x)
                const imageY = overlay.clampImageY(mouse.y)
                overlay.selectionX = Math.min(overlay.startX, imageX)
                overlay.selectionY = Math.min(overlay.startY, imageY)
                overlay.selectionWidth = Math.abs(imageX - overlay.startX)
                overlay.selectionHeight = Math.abs(imageY - overlay.startY)
                return
            }
            if (!overlay.draftAnnotation)
                return
            const p = overlay.localPoint(mouse.x, mouse.y)
            overlay.draftAnnotation.x2 = p.x
            overlay.draftAnnotation.y2 = p.y
            if (overlay.draftAnnotation.type === "pen")
                overlay.draftAnnotation.points.push({ x: p.x, y: p.y })
            annotationLayer.requestPaint()
        }

        onReleased: {
            if (overlay.finishing)
                return
            if (!overlay.editing) {
                if (overlay.selectionValid) {
                    overlay.editing = true
                }
                return
            }
            if (!overlay.draftAnnotation)
                return
            const dx = Math.abs(overlay.draftAnnotation.x2 - overlay.draftAnnotation.x1)
            const dy = Math.abs(overlay.draftAnnotation.y2 - overlay.draftAnnotation.y1)
            if (overlay.draftAnnotation.type === "pen" || dx >= 2 || dy >= 2)
                overlay.annotations = overlay.annotations.concat([overlay.draftAnnotation])
            overlay.draftAnnotation = null
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 26
        width: hintText.implicitWidth + 34
        height: 38
        radius: 10
        color: "#D91B1E26"
        z: 10

        Text {
            id: hintText
            anchors.centerIn: parent
            text: !overlay.editing
                  ? qsTr("拖动选择截图区域  ·  Esc 取消")
                  : overlay.activeTool === "picker"
                    ? qsTr("移动鼠标查看像素颜色，点击应用颜色")
                    : qsTr("选择工具进行标注  ·  Ctrl+Z 撤销  ·  Enter 完成")
            color: "white"
            font.pixelSize: 12
            font.weight: Font.Medium
        }
    }

    Rectangle {
        id: pickerBadge
        x: Math.max(10, Math.min(overlay.width - width - 10, overlay.pointerX + 18))
        y: Math.max(10, Math.min(overlay.height - height - 10, overlay.pointerY + 18))
        width: 116
        height: 38
        radius: 10
        color: "#F21D2028"
        visible: overlay.editing && overlay.activeTool === "picker"
                 && overlay.pointInsideSelection(overlay.pointerX, overlay.pointerY)
        z: 18

        Row {
            anchors.centerIn: parent
            spacing: 8
            Rectangle { width: 20; height: 20; radius: 5; color: overlay.hoveredPixelColor; border.color: "#80FFFFFF" }
            Text { text: overlay.hoveredPixelColor; color: "white"; font.pixelSize: 12; font.family: "monospace" }
        }
    }

    Rectangle {
        id: textEditor
        x: Math.max(8, Math.min(overlay.width - width - 8,
                              overlay.selectionX + overlay.textLocalX))
        y: Math.max(8, Math.min(overlay.height - height - 8,
                              overlay.selectionY + overlay.textLocalY))
        width: Math.max(140, Math.min(300,
                                     overlay.selectionWidth - overlay.textLocalX))
        height: 40
        radius: 7
        color: "#F21D2028"
        border.color: overlay.selectedColor
        visible: overlay.textEditing
        z: 30

        TextInput {
            id: textInput
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            color: overlay.selectedColor
            selectionColor: "#5B6EF5"
            selectedTextColor: "white"
            verticalAlignment: TextInput.AlignVCenter
            font.pixelSize: 18
            clip: true
            Keys.onReturnPressed: event => {
                event.accepted = true
                overlay.commitText()
            }
            Keys.onEscapePressed: event => {
                event.accepted = true
                overlay.textEditing = false
                textInput.text = ""
            }
        }
    }

    CaptureToolbar {
        id: toolbar
        x: Math.max(12, Math.min(parent.width - width - 12,
                                overlay.selectionX + overlay.selectionWidth - width))
        y: Math.max(12, overlay.selectionY + overlay.selectionHeight + 10 + height <= parent.height
                    ? overlay.selectionY + overlay.selectionHeight + 10
                    : overlay.selectionY - height - 10)
        visible: overlay.editing && overlay.selectionValid
        z: 20
        activeTool: overlay.activeTool
        selectedColor: overlay.selectedColor
        paletteVisible: overlay.colorPaletteVisible
        annotationCount: overlay.annotations.length
        selectionPixelWidth: Math.round(overlay.selectionWidth * overlay.imageScaleX)
        selectionPixelHeight: Math.round(overlay.selectionHeight * overlay.imageScaleY)
        activeToolLabel: overlay.toolLabel(overlay.activeTool)
        finishing: overlay.finishing
        onToolSelected: tool => overlay.selectTool(tool)
        onPaletteToggled: overlay.colorPaletteVisible = !overlay.colorPaletteVisible
        onUndoRequested: overlay.undo()
        onReselectRequested: overlay.resetSelection()
        onCancelRequested: overlay.cancel()
        onConfirmRequested: overlay.confirm()
    }

    ColorPalette {
        id: colorPalette
        x: Math.max(12, Math.min(overlay.width - width - 12, toolbar.x + toolbar.width - width))
        y: toolbar.y >= height + 20 ? toolbar.y - height - 8
                                    : toolbar.y + toolbar.height + 8
        visible: overlay.colorPaletteVisible && toolbar.visible
        z: 25
        selectedColor: overlay.selectedColor
        onColorSelected: color => {
            overlay.selectedColor = color
            overlay.colorPaletteVisible = false
        }
    }
}
