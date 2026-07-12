// SPDX-License-Identifier: GPL-3.0-or-later

#include "clipboardservice.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QProcess>
#include <QStandardPaths>

namespace GrabInk {

bool ClipboardService::copyImage(const QImage &image, QString *error)
{
    if (image.isNull()) {
        *error = QStringLiteral("剪贴板图片为空");
        return false;
    }
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard) {
        *error = QStringLiteral("系统剪贴板不可用");
        return false;
    }
    clipboard->setImage(image);
    return true;
}

bool ClipboardService::persistWaylandPng(const QString &path, QString *error)
{
#ifdef Q_OS_LINUX
    const QString wlCopy = QStandardPaths::findExecutable(QStringLiteral("wl-copy"));
    if (wlCopy.isEmpty()) {
        *error = QStringLiteral("未找到 wl-copy，请安装 wl-clipboard");
        return false;
    }
    QProcess process;
    process.setProgram(wlCopy);
    process.setArguments({QStringLiteral("--type"), QStringLiteral("image/png")});
    process.setStandardInputFile(path);
    process.start();
    if (!process.waitForStarted(1000)) {
        *error = QStringLiteral("无法启动 wl-copy：%1").arg(process.errorString());
        return false;
    }
    if (!process.waitForFinished(3000)) {
        process.kill();
        process.waitForFinished(1000);
        *error = QStringLiteral("wl-copy 写入剪贴板超时");
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        const QString details = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        *error = details.isEmpty()
                     ? QStringLiteral("wl-copy 写入剪贴板失败")
                     : QStringLiteral("wl-copy 写入剪贴板失败：%1").arg(details);
        return false;
    }
    return true;
#else
    Q_UNUSED(path)
    *error = QStringLiteral("当前平台不支持 wl-copy");
    return false;
#endif
}

} // namespace GrabInk
