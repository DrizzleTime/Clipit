// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

Item {
    id: root
    property string name: "capture"
    property color color: "#FFFFFF"
    property real lineWidth: 1.8
    implicitWidth: 24
    implicitHeight: 24

    onNameChanged: canvas.requestPaint()
    onColorChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            const c = getContext("2d")
            const sx = width / 24
            const sy = height / 24
            c.reset()
            c.scale(sx, sy)
            c.strokeStyle = root.color
            c.fillStyle = root.color
            c.lineWidth = root.lineWidth
            c.lineCap = "round"
            c.lineJoin = "round"

            if (root.name === "capture") {
                c.strokeRect(4, 6, 16, 12)
                c.beginPath(); c.arc(12, 12, 3.2, 0, Math.PI * 2); c.stroke()
                c.beginPath(); c.moveTo(8, 6); c.lineTo(9.5, 4); c.lineTo(14.5, 4); c.lineTo(16, 6); c.stroke()
            } else if (root.name === "region") {
                c.beginPath(); c.moveTo(8, 3); c.lineTo(3, 3); c.lineTo(3, 8); c.stroke()
                c.beginPath(); c.moveTo(16, 3); c.lineTo(21, 3); c.lineTo(21, 8); c.stroke()
                c.beginPath(); c.moveTo(3, 16); c.lineTo(3, 21); c.lineTo(8, 21); c.stroke()
                c.beginPath(); c.moveTo(21, 16); c.lineTo(21, 21); c.lineTo(16, 21); c.stroke()
            } else if (root.name === "screen") {
                c.strokeRect(3, 4, 18, 13)
                c.beginPath(); c.moveTo(9, 21); c.lineTo(15, 21); c.moveTo(12, 17); c.lineTo(12, 21); c.stroke()
            } else if (root.name === "clock") {
                c.beginPath(); c.arc(12, 12, 8.5, 0, Math.PI * 2); c.stroke()
                c.beginPath(); c.moveTo(12, 7); c.lineTo(12, 12); c.lineTo(15.5, 14); c.stroke()
            } else if (root.name === "settings") {
                c.beginPath(); c.arc(12, 12, 3, 0, Math.PI * 2); c.stroke()
                c.beginPath()
                for (let i = 0; i <= 16; ++i) {
                    const a = i * Math.PI / 8
                    const radius = i % 2 === 0 ? 8.8 : 7
                    const x = 12 + Math.cos(a) * radius
                    const y = 12 + Math.sin(a) * radius
                    if (i === 0) c.moveTo(x, y); else c.lineTo(x, y)
                }
                c.closePath(); c.stroke()
            } else if (root.name === "copy") {
                c.strokeRect(8, 8, 11, 12)
                c.beginPath(); c.moveTo(16, 8); c.lineTo(16, 4); c.lineTo(5, 4); c.lineTo(5, 16); c.lineTo(8, 16); c.stroke()
            } else if (root.name === "folder") {
                c.beginPath(); c.moveTo(3, 7); c.lineTo(9, 7); c.lineTo(11, 9); c.lineTo(21, 9); c.lineTo(19, 19); c.lineTo(3, 19); c.closePath(); c.stroke()
            } else if (root.name === "shield") {
                c.beginPath(); c.moveTo(12, 3); c.lineTo(20, 6); c.lineTo(19, 13); c.quadraticCurveTo(18, 18, 12, 21); c.quadraticCurveTo(6, 18, 5, 13); c.lineTo(4, 6); c.closePath(); c.stroke()
                c.beginPath(); c.moveTo(8.5, 12); c.lineTo(11, 14.5); c.lineTo(15.5, 9.5); c.stroke()
            } else if (root.name === "close") {
                c.beginPath(); c.moveTo(6, 6); c.lineTo(18, 18); c.moveTo(18, 6); c.lineTo(6, 18); c.stroke()
            } else if (root.name === "check") {
                c.beginPath(); c.moveTo(5, 12); c.lineTo(10, 17); c.lineTo(19, 7); c.stroke()
            } else if (root.name === "rect") {
                c.strokeRect(4, 5, 16, 14)
            } else if (root.name === "arrow") {
                c.beginPath(); c.moveTo(4, 19); c.lineTo(19, 5); c.moveTo(12, 5); c.lineTo(19, 5); c.lineTo(19, 12); c.stroke()
            } else if (root.name === "pen") {
                c.beginPath(); c.moveTo(5, 19); c.lineTo(8, 13); c.lineTo(16, 5); c.lineTo(19, 8); c.lineTo(11, 16); c.closePath(); c.stroke()
                c.beginPath(); c.moveTo(5, 19); c.lineTo(11, 16); c.stroke()
            } else if (root.name === "undo") {
                c.beginPath(); c.moveTo(9, 7); c.lineTo(4, 11); c.lineTo(9, 15); c.moveTo(5, 11); c.lineTo(14, 11); c.quadraticCurveTo(20, 11, 20, 17); c.stroke()
            } else if (root.name === "text") {
                c.beginPath(); c.moveTo(5, 5); c.lineTo(19, 5); c.moveTo(12, 5); c.lineTo(12, 19); c.moveTo(8, 19); c.lineTo(16, 19); c.stroke()
            } else if (root.name === "picker") {
                c.beginPath(); c.moveTo(14, 4); c.lineTo(20, 10); c.lineTo(17, 13); c.lineTo(11, 7); c.closePath(); c.stroke()
                c.beginPath(); c.moveTo(12, 8); c.lineTo(5, 15); c.lineTo(4, 20); c.lineTo(9, 19); c.lineTo(16, 12); c.stroke()
            } else if (root.name === "blur") {
                c.beginPath(); c.arc(8, 9, 3, 0, Math.PI * 2); c.arc(15, 9, 3, 0, Math.PI * 2); c.arc(11.5, 15, 3, 0, Math.PI * 2); c.stroke()
            } else if (root.name === "mosaic") {
                c.strokeRect(4, 4, 7, 7); c.strokeRect(13, 4, 7, 7); c.strokeRect(4, 13, 7, 7); c.strokeRect(13, 13, 7, 7)
            }
        }
    }
}
