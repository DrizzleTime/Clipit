// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: layer

    required property url source
    required property var annotations
    required property real viewportWidth
    required property real viewportHeight
    property var draftAnnotation: null

    function requestPaint() {
        canvas.requestPaint()
    }

    Image {
        x: -layer.x
        y: -layer.y
        width: layer.viewportWidth
        height: layer.viewportHeight
        source: layer.source
        fillMode: Image.PreserveAspectFit
        cache: false
        asynchronous: false
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        enabled: false
        antialiasing: true

        function drawEffectPreview(ctx, item, mosaic) {
            const x = Math.min(item.x1, item.x2)
            const y = Math.min(item.y1, item.y2)
            const w = Math.abs(item.x2 - item.x1)
            const h = Math.abs(item.y2 - item.y1)
            if (w < 1 || h < 1)
                return
            ctx.save()
            ctx.beginPath()
            ctx.rect(x, y, w, h)
            ctx.clip()
            if (mosaic) {
                const block = 9
                for (let py = y; py < y + h; py += block) {
                    for (let px = x; px < x + w; px += block) {
                        const odd = (Math.floor((px - x) / block)
                                     + Math.floor((py - y) / block)) % 2
                        ctx.fillStyle = odd ? "#8098A2B3" : "#80D0D5DE"
                        ctx.fillRect(px, py, block, block)
                    }
                }
            } else {
                ctx.fillStyle = "#70D9DDE5"
                ctx.fillRect(x, y, w, h)
                ctx.strokeStyle = "#80FFFFFF"
                ctx.lineWidth = 3
                for (let offset = -h; offset < w; offset += 12) {
                    ctx.beginPath()
                    ctx.moveTo(x + offset, y + h)
                    ctx.lineTo(x + offset + h, y)
                    ctx.stroke()
                }
            }
            ctx.restore()
        }

        function drawAnnotation(ctx, item) {
            if (!item)
                return
            if (item.type === "blur" || item.type === "mosaic") {
                drawEffectPreview(ctx, item, item.type === "mosaic")
                return
            }
            ctx.strokeStyle = item.color || "#FF5967"
            ctx.fillStyle = item.color || "#FF5967"
            ctx.lineWidth = item.width || 3
            ctx.lineCap = "round"
            ctx.lineJoin = "round"
            if (item.type === "rect") {
                ctx.strokeRect(Math.min(item.x1, item.x2), Math.min(item.y1, item.y2),
                               Math.abs(item.x2 - item.x1), Math.abs(item.y2 - item.y1))
            } else if (item.type === "arrow") {
                const angle = Math.atan2(item.y2 - item.y1, item.x2 - item.x1)
                const head = 13
                ctx.beginPath()
                ctx.moveTo(item.x1, item.y1)
                ctx.lineTo(item.x2, item.y2)
                ctx.moveTo(item.x2, item.y2)
                ctx.lineTo(item.x2 - head * Math.cos(angle - Math.PI / 6),
                           item.y2 - head * Math.sin(angle - Math.PI / 6))
                ctx.moveTo(item.x2, item.y2)
                ctx.lineTo(item.x2 - head * Math.cos(angle + Math.PI / 6),
                           item.y2 - head * Math.sin(angle + Math.PI / 6))
                ctx.stroke()
            } else if (item.type === "pen" && item.points && item.points.length > 0) {
                ctx.beginPath()
                ctx.moveTo(item.points[0].x, item.points[0].y)
                for (let i = 1; i < item.points.length; ++i)
                    ctx.lineTo(item.points[i].x, item.points[i].y)
                ctx.stroke()
            } else if (item.type === "text") {
                ctx.save()
                ctx.font = (item.fontSize || 18) + "px sans-serif"
                ctx.textBaseline = "top"
                ctx.fillText(item.text || "", item.x1, item.y1)
                ctx.restore()
            }
        }

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            for (let i = 0; i < layer.annotations.length; ++i) {
                const item = layer.annotations[i]
                if (item.type === "blur" || item.type === "mosaic")
                    drawAnnotation(ctx, item)
            }
            if (layer.draftAnnotation
                    && (layer.draftAnnotation.type === "blur"
                        || layer.draftAnnotation.type === "mosaic"))
                drawAnnotation(ctx, layer.draftAnnotation)
            for (let j = 0; j < layer.annotations.length; ++j) {
                const item = layer.annotations[j]
                if (item.type !== "blur" && item.type !== "mosaic")
                    drawAnnotation(ctx, item)
            }
            if (layer.draftAnnotation
                    && layer.draftAnnotation.type !== "blur"
                    && layer.draftAnnotation.type !== "mosaic")
                drawAnnotation(ctx, layer.draftAnnotation)
        }
    }
}
