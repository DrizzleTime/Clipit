// SPDX-License-Identifier: GPL-3.0-or-later

#include "screenshotservice.h"

#include "src/core/annotation.h"
#include "src/core/annotationrenderer.h"
#include "src/platform/clipboardservice.h"
#include "src/platform/qtscreencapturebackend.h"
#include "src/platform/screencapturebackend.h"

#ifdef CLIPIT_HAS_DBUS_PORTAL
#include "src/platform/portalscreencapturebackend.h"
#endif

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QTimer>

namespace {

bool isWaylandSession()
{
    const QString platform = QGuiApplication::platformName();
    return platform.startsWith(QStringLiteral("wayland"), Qt::CaseInsensitive)
           || qEnvironmentVariable("XDG_SESSION_TYPE")
                      .compare(QStringLiteral("wayland"), Qt::CaseInsensitive) == 0
           || qEnvironmentVariableIsSet("WAYLAND_DISPLAY");
}

Clipit::ScreenCaptureBackend *createBackend(bool wayland)
{
#ifdef CLIPIT_HAS_DBUS_PORTAL
    if (wayland)
        return new Clipit::PortalScreenCaptureBackend;
#else
    Q_UNUSED(wayland)
#endif
    return new Clipit::QtScreenCaptureBackend;
}

} // namespace

ScreenshotService::ScreenshotService(QObject *parent)
    : ScreenshotService(createBackend(isWaylandSession()), Clipit::ScreenshotStorage(),
                        isWaylandSession(), parent)
{
}

ScreenshotService::ScreenshotService(Clipit::ScreenCaptureBackend *backend,
                                     Clipit::ScreenshotStorage storage, bool wayland,
                                     QObject *parent)
    : QObject(parent)
    , m_backend(backend)
    , m_storage(std::move(storage))
    , m_delayTimer(new QTimer(this))
    , m_wayland(wayland)
{
    Q_ASSERT(m_backend);
    m_backend->setParent(this);
    connectBackend();

    m_delayTimer->setSingleShot(true);
    connect(m_delayTimer, &QTimer::timeout, this, &ScreenshotService::performCapture);
}

void ScreenshotService::connectBackend()
{
    connect(m_backend, &Clipit::ScreenCaptureBackend::availableChanged, this,
            &ScreenshotService::portalAvailableChanged);
    connect(m_backend, &Clipit::ScreenCaptureBackend::captured, this,
            &ScreenshotService::handleCapturedImage);
    connect(m_backend, &Clipit::ScreenCaptureBackend::canceled, this,
            [this](const QString &message) {
                clearSelectionSource();
                setState(CaptureState::Idle);
                emit captureCanceled(message);
            });
    connect(m_backend, &Clipit::ScreenCaptureBackend::failed, this,
            &ScreenshotService::fail);
}

ScreenshotService::~ScreenshotService()
{
    clearSelectionSource();
}

bool ScreenshotService::portalAvailable() const
{
    return m_backend && m_backend->available();
}

QUrl ScreenshotService::previewUrl() const
{
    if (m_lastFilePath.isEmpty())
        return {};
    QUrl url = QUrl::fromLocalFile(m_lastFilePath);
    url.setQuery(QStringLiteral("v=%1").arg(m_previewRevision));
    return url;
}

QString ScreenshotService::lastFileName() const
{
    return QFileInfo(m_lastFilePath).fileName();
}

void ScreenshotService::captureRegion(int delaySeconds)
{
    startCapture(Mode::Region, delaySeconds);
}

void ScreenshotService::captureWindow(int delaySeconds)
{
    startCapture(Mode::Window, delaySeconds);
}

void ScreenshotService::captureFullscreen(int delaySeconds)
{
    startCapture(Mode::Fullscreen, delaySeconds);
}

void ScreenshotService::startCapture(Mode mode, int delaySeconds)
{
    if (busy()) {
        emit captureFailed(tr("已有截图任务正在进行"));
        return;
    }
    if (!portalAvailable()) {
        fail(m_wayland ? tr("系统截图门户不可用，请安装 xdg-desktop-portal 及桌面后端")
                       : tr("没有可用的屏幕"));
        return;
    }

    m_mode = mode;
    emit captureStarted();
    const int waitMs = qMax(0, delaySeconds) * 1000 + 220;
    setState(delaySeconds > 0 ? CaptureState::Delaying : CaptureState::Capturing);
    if (delaySeconds > 0)
        m_delayTimer->start(waitMs);
    else
        QTimer::singleShot(waitMs, this, &ScreenshotService::performCapture);
}

void ScreenshotService::performCapture()
{
    if (m_state != CaptureState::Delaying && m_state != CaptureState::Capturing)
        return;
    setState(CaptureState::Capturing);
    const auto target = m_mode == Mode::Window ? Clipit::CaptureTarget::ActiveWindow
                                               : Clipit::CaptureTarget::Screen;
    m_backend->capture(target);
}

void ScreenshotService::handleCapturedImage(const QImage &image)
{
    if (m_state != CaptureState::Capturing)
        return;
    if (image.isNull()) {
        fail(tr("截图结果为空"));
        return;
    }
    if (m_mode == Mode::Region)
        beginLocalSelection(image);
    else
        acceptImage(image);
}

void ScreenshotService::beginLocalSelection(const QImage &image)
{
    m_pendingImage = image;
    QString error;
    if (!m_storage.createSelectionImage(image, &m_selectionTempPath, &error)) {
        fail(error);
        return;
    }
    m_selectionSource = QUrl::fromLocalFile(m_selectionTempPath);
    setState(CaptureState::Selecting);
    emit selectionSourceChanged();
    emit selectionRequested();
}

void ScreenshotService::finishSelection(qreal x, qreal y, qreal width, qreal height,
                                        qreal viewWidth, qreal viewHeight)
{
    finishAnnotatedSelection(x, y, width, height, viewWidth, viewHeight, {});
}

void ScreenshotService::finishAnnotatedSelection(qreal x, qreal y, qreal width, qreal height,
                                                 qreal viewWidth, qreal viewHeight,
                                                 const QVariantList &annotationValues)
{
    if (m_state != CaptureState::Selecting || m_pendingImage.isNull()) {
        fail(tr("截图选区状态无效"));
        return;
    }

    QVector<Clipit::Annotation> annotations;
    QString error;
    if (!Clipit::parseAnnotations(annotationValues, &annotations, &error)) {
        fail(error);
        return;
    }
    const Clipit::SelectionGeometry geometry{
        QRectF(x, y, width, height), QSizeF(viewWidth, viewHeight)};
    setState(CaptureState::Saving);
    const QImage result = Clipit::AnnotationRenderer::render(
        m_pendingImage, geometry, annotations, &error);
    if (result.isNull()) {
        fail(error);
        return;
    }
    clearSelectionSource();
    acceptImage(result);
}

QString ScreenshotService::pixelColor(qreal x, qreal y, qreal viewWidth,
                                      qreal viewHeight) const
{
    return Clipit::AnnotationRenderer::sampleColor(
               m_pendingImage, QPointF(x, y), QSizeF(viewWidth, viewHeight))
        .name(QColor::HexRgb)
        .toUpper();
}

void ScreenshotService::cancelSelection()
{
    if (!busy())
        return;
    m_delayTimer->stop();
    if (m_state == CaptureState::Capturing) {
        m_backend->cancel();
        if (m_state != CaptureState::Capturing)
            return;
    }
    clearSelectionSource();
    setState(CaptureState::Idle);
    emit captureCanceled(tr("已取消截图"));
}

void ScreenshotService::clearSelectionSource()
{
    m_pendingImage = {};
    m_selectionSource = QUrl();
    if (!m_selectionTempPath.isEmpty()) {
        QFile::remove(m_selectionTempPath);
        m_selectionTempPath.clear();
    }
    emit selectionSourceChanged();
}

void ScreenshotService::acceptImage(const QImage &image)
{
    setState(CaptureState::Saving);
    QString path;
    QString error;
    if (!m_storage.save(image, &path, &error)) {
        fail(error);
        return;
    }
    if (!Clipit::ClipboardService::copyImage(image, &error)) {
        fail(tr("截图已保存，但复制到剪贴板失败：%1").arg(error));
        return;
    }

    m_lastFilePath = path;
    ++m_previewRevision;
    setState(CaptureState::Idle);
    emit previewChanged();
    emit captureSaved(path);
    emit captureCompleted(tr("截图已保存并复制到剪贴板"));
}

void ScreenshotService::copyLatest()
{
    const QImage image(m_lastFilePath);
    QString error;
    if (!Clipit::ClipboardService::copyImage(image, &error)) {
        emit captureFailed(error.isEmpty() ? tr("没有可复制的截图") : error);
        return;
    }
    emit captureCompleted(tr("已复制到剪贴板"));
}

void ScreenshotService::saveCopy(const QUrl &destination)
{
    if (m_lastFilePath.isEmpty() || !destination.isValid())
        return;
    QString path = destination.toLocalFile();
    if (!path.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive))
        path += QStringLiteral(".png");
    QString error;
    if (m_storage.saveCopy(m_lastFilePath, path, &error))
        emit captureCompleted(tr("副本已保存"));
    else
        emit captureFailed(error);
}

void ScreenshotService::setSaveDirectory(const QUrl &directory)
{
    const QString path = directory.toLocalFile();
    if (path.isEmpty() || path == m_storage.directory())
        return;
    m_storage.setDirectory(path);
    emit saveDirectoryChanged();
}

void ScreenshotService::openSaveDirectory()
{
    QDir().mkpath(m_storage.directory());
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_storage.directory()));
}

void ScreenshotService::setState(CaptureState state)
{
    if (m_state == state)
        return;
    const bool wasBusy = busy();
    m_state = state;
    emit stateChanged();
    if (wasBusy != busy())
        emit busyChanged();
}

void ScreenshotService::fail(const QString &message)
{
    m_delayTimer->stop();
    clearSelectionSource();
    setState(CaptureState::Idle);
    emit captureFailed(message);
}
