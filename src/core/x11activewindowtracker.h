// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "activewindowtracker.h"

#include <xcb/xcb.h>

class QSocketNotifier;

namespace dgm {

class X11ActiveWindowTracker final : public ActiveWindowTracker
{
    Q_OBJECT

public:
    explicit X11ActiveWindowTracker(QObject *parent = nullptr);
    ~X11ActiveWindowTracker() override;

    bool start() override;
    void stop() override;
    [[nodiscard]] bool isRunning() const override;

private:
    void processEvents();
    void refreshActiveWindow();

    xcb_connection_t *m_connection = nullptr;
    xcb_window_t m_rootWindow = XCB_WINDOW_NONE;
    xcb_atom_t m_activeWindowAtom = XCB_ATOM_NONE;
    QSocketNotifier *m_notifier = nullptr;
    bool m_running = false;
};

} // namespace dgm
