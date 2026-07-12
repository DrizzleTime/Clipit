// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QVariantList>
#include <QVector>

namespace GrabInk {

enum class AnnotationType {
    Rectangle,
    Arrow,
    Pen,
    Text,
    Blur,
    Mosaic,
};

struct Annotation
{
    AnnotationType type = AnnotationType::Rectangle;
    QColor color = QColor(QStringLiteral("#FF5967"));
    qreal width = 3.0;
    QPointF start;
    QPointF end;
    QVector<QPointF> points;
    QString text;
    qreal fontSize = 18.0;
};

struct SelectionGeometry
{
    QRectF selection;
    QSizeF viewport;
};

bool parseAnnotations(const QVariantList &values, QVector<Annotation> *annotations,
                      QString *error);

} // namespace GrabInk
