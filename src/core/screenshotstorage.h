// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>
#include <QString>

namespace GrabInk {

class ScreenshotStorage
{
public:
    ScreenshotStorage();
    explicit ScreenshotStorage(QString directory);

    QString directory() const { return m_directory; }
    void setDirectory(const QString &directory, bool persist = true);

    bool save(const QImage &image, QString *path, QString *error) const;
    bool saveCopy(const QString &sourcePath, const QString &destinationPath,
                  QString *error) const;
    bool createSelectionImage(const QImage &image, QString *path, QString *error) const;

private:
    static QString defaultDirectory();
    static bool writePngAtomically(const QImage &image, const QString &path, QString *error);

    QString m_directory;
};

} // namespace GrabInk
