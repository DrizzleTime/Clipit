// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "screencapturebackend.h"

namespace Clipit {

class QtScreenCaptureBackend final : public ScreenCaptureBackend
{
    Q_OBJECT

public:
    explicit QtScreenCaptureBackend(QObject *parent = nullptr);

    bool available() const override;
    void capture() override;
    void cancel() override;
};

} // namespace Clipit
