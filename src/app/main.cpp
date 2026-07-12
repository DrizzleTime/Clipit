// SPDX-License-Identifier: GPL-3.0-or-later

#include "applicationoptions.h"
#include "screenshotservice.h"
#include "src/platform/clipboardservice.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QTextStream>
#include <QTimer>

#include <memory>

namespace {

constexpr int CaptureFailedExitCode = 1;
constexpr int InvalidArgumentsExitCode = 2;
constexpr int CaptureCanceledExitCode = 3;
constexpr int ClipboardFailedExitCode = 4;

int runGui(QGuiApplication &app)
{
    ScreenshotService screenshotService;
    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("screenshotService"), QVariant::fromValue(&screenshotService)},
    });
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(CaptureFailedExitCode); },
        Qt::QueuedConnection);
    engine.loadFromModule("Clipit", "Main");
    if (engine.rootObjects().isEmpty())
        return CaptureFailedExitCode;

    return QGuiApplication::exec();
}

int runCaptureCommand(QGuiApplication &app, const ApplicationOptions &options)
{
    app.setQuitOnLastWindowClosed(false);

    ScreenshotService screenshotService;
    QObject::connect(&screenshotService, &ScreenshotService::captureSaved, &app,
                     [&app, &screenshotService](const QString &path) {
                         QTextStream(stdout) << path << Qt::endl;
                         if (screenshotService.wayland()) {
                             QString clipboardError;
                             if (!Clipit::ClipboardService::persistWaylandPng(
                                     path, &clipboardError)) {
                                 QTextStream(stderr)
                                     << QStringLiteral("截图已保存，但复制到剪贴板失败：%1")
                                            .arg(clipboardError)
                                     << Qt::endl;
                                 app.exit(ClipboardFailedExitCode);
                                 return;
                             }
                         }
                         app.exit(0);
                     });
    QObject::connect(&screenshotService, &ScreenshotService::captureFailed, &app,
                     [&app](const QString &message) {
                         QTextStream(stderr) << message << Qt::endl;
                         app.exit(CaptureFailedExitCode);
                     });
    QObject::connect(&screenshotService, &ScreenshotService::captureCanceled, &app,
                     [&app](const QString &message) {
                         QTextStream(stderr) << message << Qt::endl;
                         app.exit(CaptureCanceledExitCode);
                     });

    std::unique_ptr<QQmlApplicationEngine> selectionEngine;
    if (options.mode == LaunchMode::RegionCapture) {
        selectionEngine = std::make_unique<QQmlApplicationEngine>();
        selectionEngine->setInitialProperties({
            {QStringLiteral("screenshotService"), QVariant::fromValue(&screenshotService)},
        });
        selectionEngine->loadFromModule("Clipit", "CaptureOverlay");
        if (selectionEngine->rootObjects().isEmpty()) {
            QTextStream(stderr) << QStringLiteral("无法加载区域截图框选层") << Qt::endl;
            return CaptureFailedExitCode;
        }
    }

    QTimer::singleShot(0, &screenshotService, [&screenshotService, options]() {
        if (options.mode == LaunchMode::RegionCapture)
            screenshotService.captureRegion(options.delaySeconds);
        else if (options.mode == LaunchMode::WindowCapture)
            screenshotService.captureWindow(options.delaySeconds);
        else
            screenshotService.captureFullscreen(options.delaySeconds);
    });
    return QGuiApplication::exec();
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral("Clipit"));
    QCoreApplication::setApplicationVersion(QStringLiteral(CLIPIT_VERSION));
    QCoreApplication::setOrganizationName(QStringLiteral("Clipit"));

    QStringList arguments;
    arguments.reserve(argc);
    for (int index = 0; index < argc; ++index)
        arguments.append(QString::fromLocal8Bit(argv[index]));

    const bool helpRequested = arguments.contains(QStringLiteral("-h"))
                               || arguments.contains(QStringLiteral("--help"))
                               || arguments.contains(QStringLiteral("--help-all"));
    if (helpRequested) {
        QTextStream(stdout) << applicationHelpText(arguments.constFirst());
        return 0;
    }
    const bool versionRequested = arguments.contains(QStringLiteral("-v"))
                                  || arguments.contains(QStringLiteral("--version"));
    if (versionRequested) {
        QTextStream(stdout) << QCoreApplication::applicationName() << QLatin1Char(' ')
                            << QCoreApplication::applicationVersion() << Qt::endl;
        return 0;
    }

    QSurfaceFormat surfaceFormat = QSurfaceFormat::defaultFormat();
    surfaceFormat.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(surfaceFormat);
    QQuickWindow::setDefaultAlphaBuffer(true);

    QGuiApplication app(argc, argv);
    const ApplicationOptionsParseResult parsed =
        parseApplicationOptions(QCoreApplication::arguments());
    if (!parsed.isValid()) {
        QTextStream(stderr) << parsed.error << Qt::endl
                            << QStringLiteral("使用 --help 查看可用参数。") << Qt::endl;
        return InvalidArgumentsExitCode;
    }

    switch (parsed.options.mode) {
    case LaunchMode::Help:
        QTextStream(stdout) << applicationHelpText(arguments.constFirst());
        return 0;
    case LaunchMode::Version:
        QTextStream(stdout) << QCoreApplication::applicationName() << QLatin1Char(' ')
                            << QCoreApplication::applicationVersion() << Qt::endl;
        return 0;
    case LaunchMode::RegionCapture:
    case LaunchMode::WindowCapture:
    case LaunchMode::FullscreenCapture:
    case LaunchMode::Gui:
        break;
    }

    switch (parsed.options.mode) {
    case LaunchMode::RegionCapture:
    case LaunchMode::WindowCapture:
    case LaunchMode::FullscreenCapture:
        return runCaptureCommand(app, parsed.options);
    case LaunchMode::Gui:
        return runGui(app);
    case LaunchMode::Help:
    case LaunchMode::Version:
        break;
    }

    return InvalidArgumentsExitCode;
}
