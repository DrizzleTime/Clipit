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
    const auto result = parseApplicationOptions({QStringLiteral("appClipit")});

    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode), static_cast<int>(LaunchMode::Gui));
    QCOMPARE(result.options.delaySeconds, 0);
}

void ApplicationOptionsTest::parsesCaptureModes()
{
    auto result = parseApplicationOptions(
        {QStringLiteral("appClipit"), QStringLiteral("--region")});
    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode),
             static_cast<int>(LaunchMode::RegionCapture));

    result = parseApplicationOptions(
        {QStringLiteral("appClipit"), QStringLiteral("-f")});
    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode),
             static_cast<int>(LaunchMode::FullscreenCapture));
}

void ApplicationOptionsTest::parsesDelay()
{
    const auto result = parseApplicationOptions(
        {QStringLiteral("appClipit"), QStringLiteral("--fullscreen"),
         QStringLiteral("--delay"), QStringLiteral("5")});

    QVERIFY(result.isValid());
    QCOMPARE(result.options.delaySeconds, 5);
}

void ApplicationOptionsTest::rejectsConflictingModes()
{
    const auto result = parseApplicationOptions(
        {QStringLiteral("appClipit"), QStringLiteral("--region"),
         QStringLiteral("--fullscreen")});

    QVERIFY(!result.isValid());
}

void ApplicationOptionsTest::rejectsDelayWithoutCaptureMode()
{
    const auto result = parseApplicationOptions(
        {QStringLiteral("appClipit"), QStringLiteral("--delay"), QStringLiteral("1")});

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
        {QStringLiteral("appClipit"), QStringLiteral("--region"),
         QStringLiteral("--delay"), delay});

    QVERIFY(!result.isValid());
}

void ApplicationOptionsTest::rejectsUnknownAndPositionalArguments()
{
    auto result = parseApplicationOptions(
        {QStringLiteral("appClipit"), QStringLiteral("--unknown")});
    QVERIFY(!result.isValid());

    result = parseApplicationOptions(
        {QStringLiteral("appClipit"), QStringLiteral("region")});
    QVERIFY(!result.isValid());
}

void ApplicationOptionsTest::parsesHelpAndVersion()
{
    auto result = parseApplicationOptions(
        {QStringLiteral("appClipit"), QStringLiteral("--help")});
    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode), static_cast<int>(LaunchMode::Help));

    result = parseApplicationOptions(
        {QStringLiteral("appClipit"), QStringLiteral("--help-all")});
    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode), static_cast<int>(LaunchMode::Help));

    result = parseApplicationOptions(
        {QStringLiteral("appClipit"), QStringLiteral("--version")});
    QVERIFY(result.isValid());
    QCOMPARE(static_cast<int>(result.options.mode), static_cast<int>(LaunchMode::Version));
}

void ApplicationOptionsTest::helpListsCaptureOptions()
{
    const QString help = applicationHelpText(QStringLiteral("appClipit"));

    QVERIFY(help.startsWith(QStringLiteral("Usage: appClipit")));
    QVERIFY(help.contains(QStringLiteral("--region")));
    QVERIFY(help.contains(QStringLiteral("--fullscreen")));
    QVERIFY(help.contains(QStringLiteral("--delay")));
}

QTEST_APPLESS_MAIN(ApplicationOptionsTest)

#include "tst_applicationoptions.moc"
