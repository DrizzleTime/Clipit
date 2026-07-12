// SPDX-License-Identifier: GPL-3.0-or-later

#include "screenshotstorage.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>

namespace GrabInk {

ScreenshotStorage::ScreenshotStorage()
{
    m_directory = QSettings().value(QStringLiteral("saveDirectory")).toString();
    if (m_directory.isEmpty())
        m_directory = defaultDirectory();
}

ScreenshotStorage::ScreenshotStorage(QString directory)
    : m_directory(std::move(directory))
{
}

void ScreenshotStorage::setDirectory(const QString &directory, bool persist)
{
    m_directory = directory;
    if (persist)
        QSettings().setValue(QStringLiteral("saveDirectory"), m_directory);
}

bool ScreenshotStorage::save(const QImage &image, QString *path, QString *error) const
{
    if (image.isNull()) {
        *error = QStringLiteral("截图结果为空");
        return false;
    }
    QDir directory(m_directory);
    if (!directory.mkpath(QStringLiteral("."))) {
        *error = QStringLiteral("无法创建保存目录：%1").arg(m_directory);
        return false;
    }
    const QString stamp = QDateTime::currentDateTime().toString(
        QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    *path = directory.filePath(QStringLiteral("GrabInk-%1.png").arg(stamp));
    return writePngAtomically(image, *path, error);
}

bool ScreenshotStorage::saveCopy(const QString &sourcePath, const QString &destinationPath,
                                 QString *error) const
{
    if (sourcePath.isEmpty() || destinationPath.isEmpty()) {
        *error = QStringLiteral("截图副本路径无效");
        return false;
    }
    if (QFileInfo(sourcePath).absoluteFilePath()
        == QFileInfo(destinationPath).absoluteFilePath()) {
        return true;
    }

    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("无法读取原截图：%1").arg(source.errorString());
        return false;
    }
    QSaveFile destination(destinationPath);
    if (!destination.open(QIODevice::WriteOnly)) {
        *error = QStringLiteral("无法创建截图副本：%1").arg(destination.errorString());
        return false;
    }
    while (!source.atEnd()) {
        const QByteArray block = source.read(64 * 1024);
        if (block.isEmpty() && source.error() != QFileDevice::NoError) {
            destination.cancelWriting();
            *error = QStringLiteral("读取原截图失败：%1").arg(source.errorString());
            return false;
        }
        if (destination.write(block) != block.size()) {
            destination.cancelWriting();
            *error = QStringLiteral("写入截图副本失败：%1").arg(destination.errorString());
            return false;
        }
    }
    if (!destination.commit()) {
        *error = QStringLiteral("提交截图副本失败：%1").arg(destination.errorString());
        return false;
    }
    return true;
}

bool ScreenshotStorage::createSelectionImage(const QImage &image, QString *path,
                                             QString *error) const
{
    *path = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                .filePath(QStringLiteral("grabink-selection-%1.png")
                              .arg(QUuid::createUuid().toString(QUuid::Id128)));
    if (!writePngAtomically(image, *path, error)) {
        *error = QStringLiteral("无法准备截图选区：%1").arg(*error);
        return false;
    }
    return true;
}

QString ScreenshotStorage::defaultDirectory()
{
    QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (pictures.isEmpty())
        pictures = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return QDir(pictures).filePath(QStringLiteral("GrabInk"));
}

bool ScreenshotStorage::writePngAtomically(const QImage &image, const QString &path,
                                           QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        *error = file.errorString();
        return false;
    }
    if (!image.save(&file, "PNG")) {
        file.cancelWriting();
        *error = QStringLiteral("PNG 编码失败");
        return false;
    }
    if (!file.commit()) {
        *error = file.errorString();
        return false;
    }
    return true;
}

} // namespace GrabInk
