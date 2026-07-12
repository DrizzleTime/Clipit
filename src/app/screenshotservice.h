// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "src/core/screenshotstorage.h"

#include <QImage>
#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QTimer;

namespace Clipit {
class ScreenCaptureBackend;
}

class ScreenshotService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ScreenshotService is provided by the application")

public:
    enum class CaptureState {
        Idle,
        Delaying,
        Capturing,
        Selecting,
        Saving,
    };
    Q_ENUM(CaptureState)

    Q_PROPERTY(CaptureState state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool wayland READ wayland CONSTANT)
    Q_PROPERTY(bool portalAvailable READ portalAvailable NOTIFY portalAvailableChanged)
    Q_PROPERTY(bool hasPreview READ hasPreview NOTIFY previewChanged)
    Q_PROPERTY(QUrl previewUrl READ previewUrl NOTIFY previewChanged)
    Q_PROPERTY(QUrl selectionSource READ selectionSource NOTIFY selectionSourceChanged)
    Q_PROPERTY(QString lastFileName READ lastFileName NOTIFY previewChanged)
    Q_PROPERTY(QString lastFilePath READ lastFilePath NOTIFY previewChanged)
    Q_PROPERTY(QString saveDirectory READ saveDirectory NOTIFY saveDirectoryChanged)
    Q_PROPERTY(QUrl saveDirectoryUrl READ saveDirectoryUrl NOTIFY saveDirectoryChanged)

    explicit ScreenshotService(QObject *parent = nullptr);
    // The service takes ownership of the injected backend.
    ScreenshotService(Clipit::ScreenCaptureBackend *backend,
                      Clipit::ScreenshotStorage storage, bool wayland,
                      QObject *parent = nullptr);
    ~ScreenshotService() override;

    CaptureState state() const { return m_state; }
    bool busy() const { return m_state != CaptureState::Idle; }
    bool wayland() const { return m_wayland; }
    bool portalAvailable() const;
    bool hasPreview() const { return !m_lastFilePath.isEmpty(); }
    QUrl previewUrl() const;
    QUrl selectionSource() const { return m_selectionSource; }
    QString lastFileName() const;
    QString lastFilePath() const { return m_lastFilePath; }
    QString saveDirectory() const { return m_storage.directory(); }
    QUrl saveDirectoryUrl() const { return QUrl::fromLocalFile(m_storage.directory()); }

    Q_INVOKABLE void captureRegion(int delaySeconds = 0);
    Q_INVOKABLE void captureWindow(int delaySeconds = 0);
    Q_INVOKABLE void captureFullscreen(int delaySeconds = 0);
    Q_INVOKABLE void finishSelection(qreal x, qreal y, qreal width, qreal height,
                                     qreal viewWidth, qreal viewHeight);
    Q_INVOKABLE void finishAnnotatedSelection(qreal x, qreal y, qreal width, qreal height,
                                              qreal viewWidth, qreal viewHeight,
                                              const QVariantList &annotations);
    Q_INVOKABLE QString pixelColor(qreal x, qreal y, qreal viewWidth,
                                   qreal viewHeight) const;
    Q_INVOKABLE void cancelSelection();
    Q_INVOKABLE void copyLatest();
    Q_INVOKABLE void saveCopy(const QUrl &destination);
    Q_INVOKABLE void setSaveDirectory(const QUrl &directory);
    Q_INVOKABLE void openSaveDirectory();

signals:
    void stateChanged();
    void busyChanged();
    void portalAvailableChanged();
    void previewChanged();
    void selectionSourceChanged();
    void saveDirectoryChanged();
    void captureStarted();
    void selectionRequested();
    void captureSaved(const QString &path);
    void captureCanceled(const QString &message);
    void captureCompleted(const QString &message);
    void captureFailed(const QString &message);

private:
    enum class Mode { Region, Window, Fullscreen };

    void startCapture(Mode mode, int delaySeconds);
    void performCapture();
    void handleCapturedImage(const QImage &image);
    void beginLocalSelection(const QImage &image);
    void acceptImage(const QImage &image);
    void fail(const QString &message);
    void setState(CaptureState state);
    void clearSelectionSource();
    void connectBackend();

    Clipit::ScreenCaptureBackend *m_backend = nullptr;
    Clipit::ScreenshotStorage m_storage;
    QTimer *m_delayTimer = nullptr;
    CaptureState m_state = CaptureState::Idle;
    Mode m_mode = Mode::Region;
    bool m_wayland = false;
    QString m_lastFilePath;
    QString m_selectionTempPath;
    QUrl m_selectionSource;
    QImage m_pendingImage;
    quint64 m_previewRevision = 0;
};
