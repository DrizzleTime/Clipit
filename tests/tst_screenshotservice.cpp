// SPDX-License-Identifier: GPL-3.0-or-later

#include "screenshotservice.h"
#include "src/platform/screencapturebackend.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QTest>

class FakeCaptureBackend final : public GrabInk::ScreenCaptureBackend
{
    Q_OBJECT

public:
    explicit FakeCaptureBackend(bool available = true)
        : m_available(available)
    {
    }

    bool available() const override { return m_available; }
    void capture(GrabInk::CaptureTarget target) override
    {
        ++captureCount;
        lastTarget = target;
        if (m_image.isNull())
            emit failed(QStringLiteral("fake failure"));
        else
            emit captured(m_image);
    }
    void cancel() override { emit canceled(QStringLiteral("fake canceled")); }

    bool m_available = true;
    QImage m_image;
    int captureCount = 0;
    GrabInk::CaptureTarget lastTarget = GrabInk::CaptureTarget::Screen;
};

class ScreenshotServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void fullscreenCaptureTransitionsToIdle();
    void windowCaptureUsesActiveWindowTarget();
    void regionCaptureWaitsForSelection();
    void regionCaptureRendersAndSaves();
    void cancelsDelayedCapture();
    void unavailableBackendFails();
};

void ScreenshotServiceTest::fullscreenCaptureTransitionsToIdle()
{
    QTemporaryDir directory;
    auto *backend = new FakeCaptureBackend;
    backend->m_image = QImage(12, 10, QImage::Format_ARGB32);
    backend->m_image.fill(Qt::yellow);
    ScreenshotService service(backend, GrabInk::ScreenshotStorage(directory.path()), false);
    QSignalSpy savedSpy(&service, &ScreenshotService::captureSaved);

    service.captureFullscreen();

    QTRY_COMPARE_WITH_TIMEOUT(savedSpy.size(), 1, 1000);
    QCOMPARE(backend->lastTarget, GrabInk::CaptureTarget::Screen);
    QCOMPARE(service.state(), ScreenshotService::CaptureState::Idle);
    QVERIFY(service.hasPreview());
}

void ScreenshotServiceTest::windowCaptureUsesActiveWindowTarget()
{
    QTemporaryDir directory;
    auto *backend = new FakeCaptureBackend;
    backend->m_image = QImage(12, 10, QImage::Format_ARGB32);
    backend->m_image.fill(Qt::cyan);
    ScreenshotService service(backend, GrabInk::ScreenshotStorage(directory.path()), false);
    QSignalSpy savedSpy(&service, &ScreenshotService::captureSaved);

    service.captureWindow();

    QTRY_COMPARE_WITH_TIMEOUT(savedSpy.size(), 1, 1000);
    QCOMPARE(backend->lastTarget, GrabInk::CaptureTarget::ActiveWindow);
    QCOMPARE(service.state(), ScreenshotService::CaptureState::Idle);
}

void ScreenshotServiceTest::regionCaptureWaitsForSelection()
{
    QTemporaryDir directory;
    auto *backend = new FakeCaptureBackend;
    backend->m_image = QImage(12, 10, QImage::Format_ARGB32);
    backend->m_image.fill(Qt::white);
    ScreenshotService service(backend, GrabInk::ScreenshotStorage(directory.path()), false);
    QSignalSpy selectionSpy(&service, &ScreenshotService::selectionRequested);

    service.captureRegion();

    QTRY_COMPARE_WITH_TIMEOUT(selectionSpy.size(), 1, 1000);
    QCOMPARE(backend->lastTarget, GrabInk::CaptureTarget::Screen);
    QCOMPARE(service.state(), ScreenshotService::CaptureState::Selecting);
    QVERIFY(service.selectionSource().isValid());
    service.cancelSelection();
    QCOMPARE(service.state(), ScreenshotService::CaptureState::Idle);
}

void ScreenshotServiceTest::regionCaptureRendersAndSaves()
{
    QTemporaryDir directory;
    auto *backend = new FakeCaptureBackend;
    backend->m_image = QImage(20, 20, QImage::Format_ARGB32);
    backend->m_image.fill(Qt::white);
    ScreenshotService service(backend, GrabInk::ScreenshotStorage(directory.path()), false);
    QSignalSpy selectionSpy(&service, &ScreenshotService::selectionRequested);
    QSignalSpy savedSpy(&service, &ScreenshotService::captureSaved);

    service.captureRegion();
    QTRY_COMPARE_WITH_TIMEOUT(selectionSpy.size(), 1, 1000);
    const QVariantList annotations{
        QVariantMap{{QStringLiteral("type"), QStringLiteral("rect")},
                    {QStringLiteral("x1"), 1}, {QStringLiteral("y1"), 1},
                    {QStringLiteral("x2"), 8}, {QStringLiteral("y2"), 8}},
    };
    service.finishAnnotatedSelection(2, 2, 10, 10, 20, 20, annotations);

    QCOMPARE(savedSpy.size(), 1);
    QCOMPARE(service.state(), ScreenshotService::CaptureState::Idle);
    const QImage saved(savedSpy.first().first().toString());
    QCOMPARE(saved.size(), QSize(10, 10));
    QVERIFY(saved.pixelColor(1, 1) != QColor(Qt::white));
}

void ScreenshotServiceTest::cancelsDelayedCapture()
{
    QTemporaryDir directory;
    auto *backend = new FakeCaptureBackend;
    backend->m_image = QImage(10, 10, QImage::Format_ARGB32);
    ScreenshotService service(backend, GrabInk::ScreenshotStorage(directory.path()), false);
    QSignalSpy canceledSpy(&service, &ScreenshotService::captureCanceled);

    service.captureRegion(1);
    QCOMPARE(service.state(), ScreenshotService::CaptureState::Delaying);
    service.cancelSelection();

    QCOMPARE(canceledSpy.size(), 1);
    QCOMPARE(service.state(), ScreenshotService::CaptureState::Idle);
    QTest::qWait(250);
    QCOMPARE(backend->captureCount, 0);
}

void ScreenshotServiceTest::unavailableBackendFails()
{
    QTemporaryDir directory;
    auto *backend = new FakeCaptureBackend(false);
    ScreenshotService service(backend, GrabInk::ScreenshotStorage(directory.path()), true);
    QSignalSpy failedSpy(&service, &ScreenshotService::captureFailed);

    service.captureFullscreen();

    QCOMPARE(failedSpy.size(), 1);
    QCOMPARE(service.state(), ScreenshotService::CaptureState::Idle);
}

QTEST_MAIN(ScreenshotServiceTest)

#include "tst_screenshotservice.moc"
