// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

enum class LaunchMode {
    Gui,
    RegionCapture,
    WindowCapture,
    FullscreenCapture,
    Help,
    Version,
};

struct ApplicationOptions
{
    LaunchMode mode = LaunchMode::Gui;
    int delaySeconds = 0;
};

struct ApplicationOptionsParseResult
{
    ApplicationOptions options;
    QString error;

    bool isValid() const { return error.isEmpty(); }
};

[[nodiscard]] ApplicationOptionsParseResult parseApplicationOptions(
    const QStringList &arguments);
QString applicationHelpText(const QString &executableName);
