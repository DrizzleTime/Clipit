// SPDX-License-Identifier: GPL-3.0-or-later

#include "src/core/annotation.h"
#include "src/core/annotationrenderer.h"

#include <QtTest/QTest>

class AnnotationRendererTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesKnownAnnotations();
    void rejectsUnknownAnnotation();
    void cropsAndRendersRectangle();
    void scalesSelectionCoordinates();
    void rendersMosaicInsideEffectBounds();
    void samplesViewportColor();
};

void AnnotationRendererTest::parsesKnownAnnotations()
{
    const QVariantList values{
        QVariantMap{{QStringLiteral("type"), QStringLiteral("rect")},
                    {QStringLiteral("x1"), 1}, {QStringLiteral("y1"), 2},
                    {QStringLiteral("x2"), 8}, {QStringLiteral("y2"), 9},
                    {QStringLiteral("color"), QStringLiteral("#123456")}},
        QVariantMap{{QStringLiteral("type"), QStringLiteral("text")},
                    {QStringLiteral("x1"), 2}, {QStringLiteral("y1"), 3},
                    {QStringLiteral("text"), QStringLiteral("GrabInk")}},
    };
    QVector<GrabInk::Annotation> annotations;
    QString error;

    QVERIFY(GrabInk::parseAnnotations(values, &annotations, &error));
    QCOMPARE(annotations.size(), 2);
    QCOMPARE(annotations.first().type, GrabInk::AnnotationType::Rectangle);
    QCOMPARE(annotations.first().color, QColor(QStringLiteral("#123456")));
    QCOMPARE(annotations.last().text, QStringLiteral("GrabInk"));
}

void AnnotationRendererTest::rejectsUnknownAnnotation()
{
    const QVariantList values{QVariantMap{{QStringLiteral("type"),
                                           QStringLiteral("unknown")}}};
    QVector<GrabInk::Annotation> annotations;
    QString error;

    QVERIFY(!GrabInk::parseAnnotations(values, &annotations, &error));
    QVERIFY(!error.isEmpty());
}

void AnnotationRendererTest::cropsAndRendersRectangle()
{
    QImage source(100, 80, QImage::Format_ARGB32_Premultiplied);
    source.fill(Qt::white);
    GrabInk::Annotation rectangle;
    rectangle.type = GrabInk::AnnotationType::Rectangle;
    rectangle.color = Qt::red;
    rectangle.width = 2;
    rectangle.start = QPointF(2, 2);
    rectangle.end = QPointF(17, 12);
    QString error;

    const QImage result = GrabInk::AnnotationRenderer::render(
        source, {{10, 10, 20, 15}, {100, 80}}, {rectangle}, &error);

    QVERIFY2(!result.isNull(), qPrintable(error));
    QCOMPARE(result.size(), QSize(20, 15));
    QVERIFY(result.pixelColor(2, 2) != QColor(Qt::white));
    QCOMPARE(result.pixelColor(10, 7), QColor(Qt::white));
}

void AnnotationRendererTest::scalesSelectionCoordinates()
{
    QImage source(200, 160, QImage::Format_ARGB32);
    source.fill(Qt::white);
    QString error;

    const QImage result = GrabInk::AnnotationRenderer::render(
        source, {{10, 5, 20, 15}, {100, 80}}, {}, &error);

    QVERIFY2(!result.isNull(), qPrintable(error));
    QCOMPARE(result.size(), QSize(40, 30));
}

void AnnotationRendererTest::rendersMosaicInsideEffectBounds()
{
    QImage source(24, 16, QImage::Format_ARGB32);
    for (int y = 0; y < source.height(); ++y) {
        for (int x = 0; x < source.width(); ++x)
            source.setPixelColor(x, y, (x + y) % 2 == 0 ? Qt::black : Qt::white);
    }
    GrabInk::Annotation mosaic;
    mosaic.type = GrabInk::AnnotationType::Mosaic;
    mosaic.start = QPointF(0, 0);
    mosaic.end = QPointF(12, 12);
    QString error;

    const QImage result = GrabInk::AnnotationRenderer::render(
        source, {{0, 0, 24, 16}, {24, 16}}, {mosaic}, &error);

    QVERIFY2(!result.isNull(), qPrintable(error));
    QVERIFY(result.pixelColor(0, 0) != source.pixelColor(0, 0));
    QCOMPARE(result.pixelColor(23, 15), source.pixelColor(23, 15));
}

void AnnotationRendererTest::samplesViewportColor()
{
    QImage source(2, 2, QImage::Format_ARGB32);
    source.fill(Qt::black);
    source.setPixelColor(1, 1, Qt::green);

    QCOMPARE(GrabInk::AnnotationRenderer::sampleColor(source, {75, 75}, {100, 100}),
             QColor(Qt::green));
}

QTEST_APPLESS_MAIN(AnnotationRendererTest)

#include "tst_annotationrenderer.moc"
