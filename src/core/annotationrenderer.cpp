// SPDX-License-Identifier: GPL-3.0-or-later

#include "annotationrenderer.h"

#include <cmath>

#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace Clipit {
namespace {

QPointF scaledPoint(const QPointF &point, qreal scaleX, qreal scaleY,
                    qreal offsetX, qreal offsetY)
{
    return {offsetX + point.x() * scaleX, offsetY + point.y() * scaleY};
}

QRect effectRect(const Annotation &annotation, qreal scaleX, qreal scaleY,
                 qreal offsetX, qreal offsetY, const QRect &bounds)
{
    const QPointF first = scaledPoint(annotation.start, scaleX, scaleY, offsetX, offsetY);
    const QPointF second = scaledPoint(annotation.end, scaleX, scaleY, offsetX, offsetY);
    return QRectF(first, second).normalized().toAlignedRect().intersected(bounds);
}

void applyMosaic(QImage &image, const QRect &rect, int blockSize)
{
    if (rect.width() < 2 || rect.height() < 2)
        return;
    for (int blockY = rect.top(); blockY <= rect.bottom(); blockY += blockSize) {
        for (int blockX = rect.left(); blockX <= rect.right(); blockX += blockSize) {
            const QRect block(blockX, blockY,
                              qMin(blockSize, rect.right() - blockX + 1),
                              qMin(blockSize, rect.bottom() - blockY + 1));
            quint64 red = 0;
            quint64 green = 0;
            quint64 blue = 0;
            quint64 alpha = 0;
            quint64 count = 0;
            for (int y = block.top(); y <= block.bottom(); ++y) {
                const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
                for (int x = block.left(); x <= block.right(); ++x) {
                    const QRgb pixel = line[x];
                    red += qRed(pixel);
                    green += qGreen(pixel);
                    blue += qBlue(pixel);
                    alpha += qAlpha(pixel);
                    ++count;
                }
            }
            const QRgb average = qRgba(red / count, green / count, blue / count, alpha / count);
            for (int y = block.top(); y <= block.bottom(); ++y) {
                QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
                for (int x = block.left(); x <= block.right(); ++x)
                    line[x] = average;
            }
        }
    }
}

void applyBlur(QImage &image, const QRect &rect, int radius)
{
    if (rect.width() < 2 || rect.height() < 2)
        return;
    const QRect sampleRect = rect.adjusted(-radius, -radius, radius, radius)
                                 .intersected(image.rect());
    const QImage region = image.copy(sampleRect);
    const QSize reducedSize(qMax(1, region.width() / radius),
                            qMax(1, region.height() / radius));
    const QImage reduced = region.scaled(reducedSize, Qt::IgnoreAspectRatio,
                                         Qt::SmoothTransformation);
    const QImage blurred = reduced.scaled(rect.size(), Qt::IgnoreAspectRatio,
                                          Qt::SmoothTransformation);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QRect sourceRect(rect.topLeft() - sampleRect.topLeft(), rect.size());
    painter.drawImage(rect.topLeft(), blurred, sourceRect);
}

} // namespace

QImage AnnotationRenderer::render(const QImage &source, const SelectionGeometry &geometry,
                                  const QVector<Annotation> &annotations, QString *error)
{
    if (source.isNull() || geometry.viewport.width() <= 0
        || geometry.viewport.height() <= 0) {
        *error = QStringLiteral("截图选区状态无效");
        return {};
    }

    const qreal scaleX = source.width() / geometry.viewport.width();
    const qreal scaleY = source.height() / geometry.viewport.height();
    const int cropLeft = qFloor(geometry.selection.left() * scaleX);
    const int cropTop = qFloor(geometry.selection.top() * scaleY);
    const int cropRight = qCeil(geometry.selection.right() * scaleX);
    const int cropBottom = qCeil(geometry.selection.bottom() * scaleY);
    QRect cropRect(cropLeft, cropTop, cropRight - cropLeft, cropBottom - cropTop);
    cropRect = cropRect.intersected(source.rect());
    if (cropRect.width() < 2 || cropRect.height() < 2) {
        *error = QStringLiteral("截图选区过小");
        return {};
    }

    QImage result = source.copy(cropRect).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    result.setDevicePixelRatio(1.0);
    const qreal offsetX = geometry.selection.x() * scaleX - cropRect.left();
    const qreal offsetY = geometry.selection.y() * scaleY - cropRect.top();
    const qreal lineScale = (scaleX + scaleY) / 2.0;

    for (const Annotation &annotation : annotations) {
        if (annotation.type != AnnotationType::Blur
            && annotation.type != AnnotationType::Mosaic) {
            continue;
        }
        const QRect rect = effectRect(annotation, scaleX, scaleY, offsetX, offsetY,
                                      result.rect());
        if (annotation.type == AnnotationType::Mosaic)
            applyMosaic(result, rect, qMax(6, qRound(12 * lineScale)));
        else
            applyBlur(result, rect, qMax(8, qRound(16 * lineScale)));
    }

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    for (const Annotation &annotation : annotations) {
        if (annotation.type == AnnotationType::Blur
            || annotation.type == AnnotationType::Mosaic) {
            continue;
        }

        QPen pen(annotation.color);
        pen.setWidthF(qMax(1.0, annotation.width * lineScale));
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        const QPointF start = scaledPoint(annotation.start, scaleX, scaleY, offsetX, offsetY);
        const QPointF end = scaledPoint(annotation.end, scaleX, scaleY, offsetX, offsetY);
        if (annotation.type == AnnotationType::Rectangle) {
            painter.drawRect(QRectF(start, end).normalized());
        } else if (annotation.type == AnnotationType::Arrow) {
            const QLineF line(start, end);
            if (line.length() < 1)
                continue;
            painter.drawLine(line);
            const qreal angle = std::atan2(end.y() - start.y(), end.x() - start.x());
            const qreal head = qMin(13 * lineScale, line.length() * 0.45);
            constexpr qreal pi = 3.14159265358979323846;
            painter.drawLine(end, {end.x() - head * std::cos(angle - pi / 6),
                                   end.y() - head * std::sin(angle - pi / 6)});
            painter.drawLine(end, {end.x() - head * std::cos(angle + pi / 6),
                                   end.y() - head * std::sin(angle + pi / 6)});
        } else if (annotation.type == AnnotationType::Pen) {
            if (annotation.points.isEmpty())
                continue;
            const QPointF first = scaledPoint(annotation.points.first(), scaleX, scaleY,
                                              offsetX, offsetY);
            if (annotation.points.size() == 1) {
                painter.setBrush(annotation.color);
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(first, pen.widthF() / 2.0, pen.widthF() / 2.0);
                continue;
            }
            QPainterPath path;
            path.moveTo(first);
            for (qsizetype index = 1; index < annotation.points.size(); ++index)
                path.lineTo(scaledPoint(annotation.points.at(index), scaleX, scaleY,
                                        offsetX, offsetY));
            painter.drawPath(path);
        } else if (annotation.type == AnnotationType::Text && !annotation.text.isEmpty()) {
            QFont font = painter.font();
            font.setPixelSize(qMax(8, qRound(annotation.fontSize * scaleY)));
            painter.setFont(font);
            painter.drawText(QRectF(start.x(), start.y(), result.width() - start.x(),
                                    result.height() - start.y()),
                             Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,
                             annotation.text);
        }
    }

    return result;
}

QColor AnnotationRenderer::sampleColor(const QImage &source, const QPointF &viewportPoint,
                                       const QSizeF &viewportSize)
{
    if (source.isNull() || viewportSize.width() <= 0 || viewportSize.height() <= 0)
        return QColor(Qt::black);
    const int x = qBound(0, qFloor(viewportPoint.x() * source.width() / viewportSize.width()),
                         source.width() - 1);
    const int y = qBound(0, qFloor(viewportPoint.y() * source.height() / viewportSize.height()),
                         source.height() - 1);
    return QColor::fromRgba(source.pixel(x, y));
}

} // namespace Clipit
