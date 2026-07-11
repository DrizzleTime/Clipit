// SPDX-License-Identifier: GPL-3.0-or-later

#include "portalscreencapturebackend.h"

#include <QFile>
#include <QImage>
#include <QTimer>
#include <QUrl>
#include <QUuid>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>

namespace Clipit {

PortalScreenCaptureBackend::PortalScreenCaptureBackend(QObject *parent)
    : ScreenCaptureBackend(parent)
    , m_timeout(new QTimer(this))
{
    refreshPortal();

    auto *watcher = new QDBusServiceWatcher(
        QStringLiteral("org.freedesktop.portal.Desktop"), QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForRegistration
            | QDBusServiceWatcher::WatchForUnregistration,
        this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this,
            [this]() { refreshPortal(); });
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this]() {
        const bool changed = m_available;
        m_available = false;
        m_portalVersion = 0;
        if (changed)
            emit availableChanged();
        if (!m_requestPath.isEmpty())
            fail(tr("系统截图门户已退出"));
    });

    m_timeout->setSingleShot(true);
    m_timeout->setInterval(5 * 60 * 1000);
    connect(m_timeout, &QTimer::timeout, this, [this]() {
        if (!m_requestPath.isEmpty())
            fail(tr("系统截图请求超时"));
    });
}

PortalScreenCaptureBackend::~PortalScreenCaptureBackend()
{
    cleanupRequest(true);
}

void PortalScreenCaptureBackend::refreshPortal()
{
    QDBusInterface portal(QStringLiteral("org.freedesktop.portal.Desktop"),
                          QStringLiteral("/org/freedesktop/portal/desktop"),
                          QStringLiteral("org.freedesktop.portal.Screenshot"),
                          QDBusConnection::sessionBus());
    const bool available = portal.isValid();
    m_portalVersion = available ? portal.property("version").toUInt() : 0;
    if (m_available != available) {
        m_available = available;
        emit availableChanged();
    }
}

void PortalScreenCaptureBackend::capture()
{
    if (!m_available) {
        emit failed(tr("系统截图门户不可用，请安装 xdg-desktop-portal 及桌面后端"));
        return;
    }
    if (!m_requestPath.isEmpty()) {
        emit failed(tr("已有截图请求正在进行"));
        return;
    }

    QDBusConnection bus = QDBusConnection::sessionBus();
    const QString token = QStringLiteral("clipit_%1").arg(
        QUuid::createUuid().toString(QUuid::Id128));
    QString sender = bus.baseService();
    sender.remove(QLatin1Char(':'));
    sender.replace(QLatin1Char('.'), QLatin1Char('_'));
    m_requestPath = QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2")
                        .arg(sender, token);

    if (!bus.connect(QStringLiteral("org.freedesktop.portal.Desktop"), m_requestPath,
                     QStringLiteral("org.freedesktop.portal.Request"),
                     QStringLiteral("Response"), this,
                     SLOT(onPortalResponse(uint,QVariantMap)))) {
        m_requestPath.clear();
        emit failed(tr("无法监听系统截图响应"));
        return;
    }

    QVariantMap options;
    options.insert(QStringLiteral("handle_token"), token);
    if (m_portalVersion >= 2)
        options.insert(QStringLiteral("interactive"), false);
    if (m_portalVersion >= 3)
        options.insert(QStringLiteral("target"), QVariant::fromValue<quint32>(1u));
    options.insert(QStringLiteral("modal"), false);
    m_timeout->start();

    auto *portal = new QDBusInterface(QStringLiteral("org.freedesktop.portal.Desktop"),
                                      QStringLiteral("/org/freedesktop/portal/desktop"),
                                      QStringLiteral("org.freedesktop.portal.Screenshot"), bus,
                                      this);
    const QString predictedPath = m_requestPath;
    auto *watcher = new QDBusPendingCallWatcher(
        portal->asyncCall(QStringLiteral("Screenshot"), QString(), options), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, portal, predictedPath](QDBusPendingCallWatcher *call) {
                QDBusPendingReply<QDBusObjectPath> reply = *call;
                if (m_requestPath != predictedPath) {
                    call->deleteLater();
                    portal->deleteLater();
                    return;
                }
                if (reply.isError()) {
                    fail(tr("系统截图请求失败：%1").arg(reply.error().message()));
                } else {
                    const QString actualPath = reply.value().path();
                    if (actualPath.isEmpty()) {
                        fail(tr("系统返回了无效的截图请求路径"));
                    } else if (actualPath != m_requestPath) {
                        QDBusConnection bus = QDBusConnection::sessionBus();
                        if (!bus.connect(QStringLiteral("org.freedesktop.portal.Desktop"),
                                         actualPath,
                                         QStringLiteral("org.freedesktop.portal.Request"),
                                         QStringLiteral("Response"), this,
                                         SLOT(onPortalResponse(uint,QVariantMap)))) {
                            bus.disconnect(QStringLiteral("org.freedesktop.portal.Desktop"),
                                           predictedPath,
                                           QStringLiteral("org.freedesktop.portal.Request"),
                                           QStringLiteral("Response"), this,
                                           SLOT(onPortalResponse(uint,QVariantMap)));
                            m_requestPath = actualPath;
                            fail(tr("无法切换系统截图响应监听"));
                        } else {
                            bus.disconnect(QStringLiteral("org.freedesktop.portal.Desktop"),
                                           predictedPath,
                                           QStringLiteral("org.freedesktop.portal.Request"),
                                           QStringLiteral("Response"), this,
                                           SLOT(onPortalResponse(uint,QVariantMap)));
                            m_requestPath = actualPath;
                        }
                    }
                }
                call->deleteLater();
                portal->deleteLater();
            });
}

void PortalScreenCaptureBackend::cancel()
{
    if (m_requestPath.isEmpty())
        return;
    cleanupRequest(true);
    emit canceled(tr("已取消截图"));
}

void PortalScreenCaptureBackend::onPortalResponse(uint response,
                                                  const QVariantMap &results)
{
    cleanupRequest(false);
    if (response == 1) {
        emit canceled(tr("已取消截图"));
        return;
    }
    if (response != 0) {
        emit failed(tr("系统拒绝了截图请求"));
        return;
    }

    const QUrl uri(results.value(QStringLiteral("uri")).toString());
    const QString path = uri.toLocalFile();
    const QImage image(path);
    if (!path.isEmpty())
        QFile::remove(path);
    if (image.isNull()) {
        emit failed(tr("无法读取系统返回的截图"));
        return;
    }
    emit captured(image);
}

void PortalScreenCaptureBackend::cleanupRequest(bool closeRequest)
{
    m_timeout->stop();
    if (m_requestPath.isEmpty())
        return;
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.disconnect(QStringLiteral("org.freedesktop.portal.Desktop"), m_requestPath,
                   QStringLiteral("org.freedesktop.portal.Request"),
                   QStringLiteral("Response"), this,
                   SLOT(onPortalResponse(uint,QVariantMap)));
    if (closeRequest) {
        const QDBusMessage close = QDBusMessage::createMethodCall(
            QStringLiteral("org.freedesktop.portal.Desktop"), m_requestPath,
            QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Close"));
        bus.asyncCall(close);
    }
    m_requestPath.clear();
}

void PortalScreenCaptureBackend::fail(const QString &message)
{
    cleanupRequest(true);
    emit failed(message);
}

} // namespace Clipit
