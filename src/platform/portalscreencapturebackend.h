// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "screencapturebackend.h"

#include <QVariantMap>

class QTimer;

namespace GrabInk {

class PortalScreenCaptureBackend final : public ScreenCaptureBackend
{
    Q_OBJECT

public:
    explicit PortalScreenCaptureBackend(QObject *parent = nullptr);
    ~PortalScreenCaptureBackend() override;

    bool available() const override { return m_available; }
    void capture(CaptureTarget target) override;
    void cancel() override;

private slots:
    void onPortalResponse(uint response, const QVariantMap &results);

private:
    void refreshPortal();
    void cleanupRequest(bool closeRequest);
    void fail(const QString &message);

    bool m_available = false;
    uint m_portalVersion = 0;
    uint m_availableTargets = 0;
    QString m_requestPath;
    QTimer *m_timeout = nullptr;
};

} // namespace GrabInk
