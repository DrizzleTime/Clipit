// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>
#include <QString>

namespace GrabInk {

class ClipboardService
{
public:
    static bool copyImage(const QImage &image, QString *error);
    static bool persistWaylandPng(const QString &path, QString *error);
};

} // namespace GrabInk
