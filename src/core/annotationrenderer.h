// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "annotation.h"

#include <QImage>

namespace Clipit {

class AnnotationRenderer
{
public:
    static QImage render(const QImage &source, const SelectionGeometry &geometry,
                         const QVector<Annotation> &annotations, QString *error);
    static QColor sampleColor(const QImage &source, const QPointF &viewportPoint,
                              const QSizeF &viewportSize);
};

} // namespace Clipit
