// SPDX-License-Identifier: GPL-3.0-or-later

#include "applicationoptions.h"

#include <QtTest/QTest>

class ApplicationOptionsTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultsToGui();
    void parsesCaptureModes();
    void parsesDelay();
    void rejectsConflictingModes();
    void rejectsDelayWithoutCaptureMode();
    void rejectsInvalidDelay_data();
    void rejectsInvalidDelay();
    void rejectsUnknownAndPositionalArguments();
    void parsesHelpAndVersion();
    void helpListsCaptureOptions();
};

void ApplicationOptionsTest::defaultsToGui()
{
    const auto result = parseApplicationOptions({QStringLiteral("grabink")});

    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode), static_cast<int>(LaunchMode::Gui));
    QCOMPARE(result.options.delaySeconds, 0);
}

void ApplicationOptionsTest::parsesCaptureModes()
{
    auto result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("--region")});
    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode),
             static_cast<int>(LaunchMode::RegionCapture));

    result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("-w")});
    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode),
             static_cast<int>(LaunchMode::WindowCapture));

    result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("-f")});
    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode),
             static_cast<int>(LaunchMode::FullscreenCapture));
}

void ApplicationOptionsTest::parsesDelay()
{
    auto result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("--fullscreen"),
         QStringLiteral("--delay"), QStringLiteral("5")});

    QVERIFY(result.isValid());
    QCOMPARE(result.options.delaySeconds, 5);

    result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("--window"),
         QStringLiteral("--delay"), QStringLiteral("2")});
    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode),
             static_cast<int>(LaunchMode::WindowCapture));
    QCOMPARE(result.options.delaySeconds, 2);
}

void ApplicationOptionsTest::rejectsConflictingModes()
{
    auto result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("--region"),
         QStringLiteral("--fullscreen")});
    QVERIFY(!result.isValid());

    result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("--region"),
         QStringLiteral("--window")});
    QVERIFY(!result.isValid());

    result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("--window"),
         QStringLiteral("--fullscreen")});
    QVERIFY(!result.isValid());
}

void ApplicationOptionsTest::rejectsDelayWithoutCaptureMode()
{
    const auto result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("--delay"), QStringLiteral("1")});

    QVERIFY(!result.isValid());
}

void ApplicationOptionsTest::rejectsInvalidDelay_data()
{
    QTest::addColumn<QString>("delay");

    QTest::newRow("not-a-number") << QStringLiteral("later");
    QTest::newRow("negative") << QStringLiteral("-1");
    QTest::newRow("too-large") << QStringLiteral("3601");
}

void ApplicationOptionsTest::rejectsInvalidDelay()
{
    QFETCH(QString, delay);
    const auto result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("--region"),
         QStringLiteral("--delay"), delay});

    QVERIFY(!result.isValid());
}

void ApplicationOptionsTest::rejectsUnknownAndPositionalArguments()
{
    auto result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("--unknown")});
    QVERIFY(!result.isValid());

    result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("region")});
    QVERIFY(!result.isValid());
}

void ApplicationOptionsTest::parsesHelpAndVersion()
{
    auto result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("--help")});
    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode), static_cast<int>(LaunchMode::Help));

    result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("--help-all")});
    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode), static_cast<int>(LaunchMode::Help));

    result = parseApplicationOptions(
        {QStringLiteral("grabink"), QStringLiteral("--version")});
    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode), static_cast<int>(LaunchMode::Version));
}

void ApplicationOptionsTest::helpListsCaptureOptions()
{
    const QString help = applicationHelpText(QStringLiteral("grabink"));

    QVERIFY(help.startsWith(QStringLiteral("Usage: grabink")));
    QVERIFY(help.contains(QStringLiteral("--region")));
    QVERIFY(help.contains(QStringLiteral("--window")));
    QVERIFY(help.contains(QStringLiteral("--fullscreen")));
    QVERIFY(help.contains(QStringLiteral("--delay")));
}

QTEST_APPLESS_MAIN(ApplicationOptionsTest)

#include "tst_applicationoptions.moc"
