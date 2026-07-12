// SPDX-License-Identifier: GPL-3.0-or-later

#include "applicationoptions.h"

#include <QCommandLineOption>
#include <QCommandLineParser>

namespace {

void configureParser(QCommandLineParser &parser)
{
    parser.setApplicationDescription(QStringLiteral("Clipit 截图工具"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption(
        {QStringLiteral("r"), QStringLiteral("region")},
        QStringLiteral("直接开始区域截图，只显示框选层。")));
    parser.addOption(QCommandLineOption(
        {QStringLiteral("w"), QStringLiteral("window")},
        QStringLiteral("开始窗口截图，不启动 Clipit 主窗口。")));
    parser.addOption(QCommandLineOption(
        {QStringLiteral("f"), QStringLiteral("fullscreen")},
        QStringLiteral("直接进行全屏截图，不启动 Clipit 主窗口。")));
    parser.addOption(QCommandLineOption(
        {QStringLiteral("d"), QStringLiteral("delay")},
        QStringLiteral("截图前等待的秒数，范围为 0 到 3600。"),
        QStringLiteral("seconds")));
}

} // namespace

ApplicationOptionsParseResult parseApplicationOptions(const QStringList &arguments)
{
    QCommandLineParser parser;
    configureParser(parser);

    ApplicationOptionsParseResult result;
    if (!parser.parse(arguments)) {
        result.error = parser.errorText();
        return result;
    }

    if (parser.isSet(QStringLiteral("help"))
        || parser.isSet(QStringLiteral("help-all"))) {
        result.options.mode = LaunchMode::Help;
        return result;
    }
    if (parser.isSet(QStringLiteral("version"))) {
        result.options.mode = LaunchMode::Version;
        return result;
    }

    if (!parser.positionalArguments().isEmpty()) {
        result.error = QStringLiteral("不支持位置参数：%1")
                           .arg(parser.positionalArguments().join(QLatin1Char(' ')));
        return result;
    }

    const bool region = parser.isSet(QStringLiteral("region"));
    const bool window = parser.isSet(QStringLiteral("window"));
    const bool fullscreen = parser.isSet(QStringLiteral("fullscreen"));
    const int captureModeCount = static_cast<int>(region) + static_cast<int>(window)
                                 + static_cast<int>(fullscreen);
    if (captureModeCount > 1) {
        result.error = QStringLiteral("--region、--window 与 --fullscreen 不能同时使用");
        return result;
    }

    if (region)
        result.options.mode = LaunchMode::RegionCapture;
    else if (window)
        result.options.mode = LaunchMode::WindowCapture;
    else if (fullscreen)
        result.options.mode = LaunchMode::FullscreenCapture;

    if (parser.isSet(QStringLiteral("delay"))) {
        if (captureModeCount == 0) {
            result.error = QStringLiteral(
                "--delay 只能与 --region、--window 或 --fullscreen 一起使用");
            return result;
        }

        bool validDelay = false;
        const int delaySeconds = parser.value(QStringLiteral("delay")).toInt(&validDelay);
        if (!validDelay || delaySeconds < 0 || delaySeconds > 3600) {
            result.error = QStringLiteral("--delay 必须是 0 到 3600 之间的整数");
            return result;
        }
        result.options.delaySeconds = delaySeconds;
    }

    return result;
}

QString applicationHelpText(const QString &executableName)
{
    QCommandLineParser parser;
    configureParser(parser);
    QString help = parser.helpText();
    help.replace(QStringLiteral("<executable_name>"), executableName);
    return help;
}
