// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>
#include <QObject>

namespace GrabInk {

enum class CaptureTarget {
    Screen,
    ActiveWindow,
};

class ScreenCaptureBackend : public QObject
{
    Q_OBJECT

public:
    explicit ScreenCaptureBackend(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    virtual bool available() const = 0;
    virtual void capture(CaptureTarget target) = 0;
    virtual void cancel() = 0;

signals:
    void availableChanged();
    void captured(const QImage &image);
    void canceled(const QString &message);
    void failed(const QString &message);
};

} // namespace GrabInk
