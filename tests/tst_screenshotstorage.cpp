// SPDX-License-Identifier: GPL-3.0-or-later

#include "src/core/screenshotstorage.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QTest>

class ScreenshotStorageTest : public QObject
{
    Q_OBJECT

private slots:
    void savesPng();
    void replacesCopyAtomically();
    void preservesDestinationWhenSourceFails();
};

void ScreenshotStorageTest::savesPng()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    Clipit::ScreenshotStorage storage(directory.path());
    QImage image(8, 6, QImage::Format_ARGB32);
    image.fill(Qt::cyan);
    QString path;
    QString error;

    QVERIFY2(storage.save(image, &path, &error), qPrintable(error));
    QVERIFY(QFile::exists(path));
    QCOMPARE(QImage(path).size(), image.size());
}

void ScreenshotStorageTest::replacesCopyAtomically()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString sourcePath = directory.filePath(QStringLiteral("source.png"));
    const QString destinationPath = directory.filePath(QStringLiteral("destination.png"));
    QImage source(4, 4, QImage::Format_ARGB32);
    source.fill(Qt::red);
    QVERIFY(source.save(sourcePath));
    QImage oldDestination(4, 4, QImage::Format_ARGB32);
    oldDestination.fill(Qt::blue);
    QVERIFY(oldDestination.save(destinationPath));
    Clipit::ScreenshotStorage storage(directory.path());
    QString error;

    QVERIFY2(storage.saveCopy(sourcePath, destinationPath, &error), qPrintable(error));
    QCOMPARE(QImage(destinationPath).pixelColor(0, 0), QColor(Qt::red));
}

void ScreenshotStorageTest::preservesDestinationWhenSourceFails()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString destinationPath = directory.filePath(QStringLiteral("destination.png"));
    QImage original(4, 4, QImage::Format_ARGB32);
    original.fill(Qt::blue);
    QVERIFY(original.save(destinationPath));
    Clipit::ScreenshotStorage storage(directory.path());
    QString error;

    QVERIFY(!storage.saveCopy(directory.filePath(QStringLiteral("missing.png")),
                              destinationPath, &error));
    QCOMPARE(QImage(destinationPath).pixelColor(0, 0), QColor(Qt::blue));
}

QTEST_APPLESS_MAIN(ScreenshotStorageTest)

#include "tst_screenshotstorage.moc"
