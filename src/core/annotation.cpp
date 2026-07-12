// SPDX-License-Identifier: GPL-3.0-or-later

#include "annotation.h"

#include <cmath>

#include <QVariantMap>

namespace GrabInk {
namespace {

bool finitePoint(const QPointF &point)
{
    return std::isfinite(point.x()) && std::isfinite(point.y());
}

bool parseType(const QString &value, AnnotationType *type)
{
    if (value == QLatin1String("rect"))
        *type = AnnotationType::Rectangle;
    else if (value == QLatin1String("arrow"))
        *type = AnnotationType::Arrow;
    else if (value == QLatin1String("pen"))
        *type = AnnotationType::Pen;
    else if (value == QLatin1String("text"))
        *type = AnnotationType::Text;
    else if (value == QLatin1String("blur"))
        *type = AnnotationType::Blur;
    else if (value == QLatin1String("mosaic"))
        *type = AnnotationType::Mosaic;
    else
        return false;
    return true;
}

QPointF pointFromMap(const QVariantMap &map, const char *xKey, const char *yKey)
{
    return {map.value(QLatin1String(xKey)).toDouble(),
            map.value(QLatin1String(yKey)).toDouble()};
}

} // namespace

bool parseAnnotations(const QVariantList &values, QVector<Annotation> *annotations,
                      QString *error)
{
    annotations->clear();
    annotations->reserve(values.size());

    for (qsizetype index = 0; index < values.size(); ++index) {
        const QVariantMap map = values.at(index).toMap();
        Annotation annotation;
        const QString typeName = map.value(QStringLiteral("type")).toString();
        if (!parseType(typeName, &annotation.type)) {
            *error = QStringLiteral("第 %1 个标注类型无效：%2")
                         .arg(index + 1)
                         .arg(typeName);
            return false;
        }

        annotation.start = pointFromMap(map, "x1", "y1");
        annotation.end = pointFromMap(map, "x2", "y2");
        if (!finitePoint(annotation.start) || !finitePoint(annotation.end)) {
            *error = QStringLiteral("第 %1 个标注坐标无效").arg(index + 1);
            return false;
        }

        const QColor color(map.value(QStringLiteral("color"), QStringLiteral("#FF5967"))
                               .toString());
        annotation.color = color.isValid() ? color : QColor(QStringLiteral("#FF5967"));
        annotation.width = map.value(QStringLiteral("width"), 3.0).toDouble();
        if (!std::isfinite(annotation.width) || annotation.width <= 0)
            annotation.width = 3.0;

        if (annotation.type == AnnotationType::Pen) {
            const QVariantList points = map.value(QStringLiteral("points")).toList();
            annotation.points.reserve(points.size());
            for (const QVariant &pointValue : points) {
                const QVariantMap pointMap = pointValue.toMap();
                const QPointF point(pointMap.value(QStringLiteral("x")).toDouble(),
                                    pointMap.value(QStringLiteral("y")).toDouble());
                if (!finitePoint(point)) {
                    *error = QStringLiteral("第 %1 个画笔标注包含无效坐标").arg(index + 1);
                    return false;
                }
                annotation.points.append(point);
            }
        } else if (annotation.type == AnnotationType::Text) {
            annotation.text = map.value(QStringLiteral("text")).toString();
            annotation.fontSize = map.value(QStringLiteral("fontSize"), 18.0).toDouble();
            if (!std::isfinite(annotation.fontSize) || annotation.fontSize <= 0)
                annotation.fontSize = 18.0;
        }

        annotations->append(std::move(annotation));
    }

    return true;
}

} // namespace GrabInk
