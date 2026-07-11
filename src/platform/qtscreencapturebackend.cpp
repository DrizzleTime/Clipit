// SPDX-License-Identifier: GPL-3.0-or-later

#include "qtscreencapturebackend.h"

#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>

namespace Clipit {

QtScreenCaptureBackend::QtScreenCaptureBackend(QObject *parent)
    : ScreenCaptureBackend(parent)
{
}

bool QtScreenCaptureBackend::available() const
{
    return QGuiApplication::primaryScreen() != nullptr;
}

void QtScreenCaptureBackend::capture()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        emit failed(tr("没有可用的屏幕"));
        return;
    }
    const QImage image = screen->grabWindow(0).toImage();
    if (image.isNull()) {
        emit failed(tr("无法读取屏幕画面"));
        return;
    }
    emit captured(image);
}

void QtScreenCaptureBackend::cancel()
{
}

} // namespace Clipit
