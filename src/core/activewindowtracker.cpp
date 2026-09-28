// SPDX-License-Identifier: GPL-3.0-or-later

#include "activewindowtracker.h"

#ifdef DGM_HAVE_X11_TRACKER
#include "x11activewindowtracker.h"
#endif

#include <QtGlobal>

namespace dgm {

namespace {

class UnsupportedActiveWindowTracker final : public ActiveWindowTracker
{
public:
    using ActiveWindowTracker::ActiveWindowTracker;

    bool start() override
    {
        return false;
    }

    void stop() override
    {
        setActiveWindowId(0);
    }

    [[nodiscard]] bool isRunning() const override
    {
        return false;
    }
};

} // namespace

ActiveWindowTracker::ActiveWindowTracker(QObject *parent)
    : QObject(parent)
{
}

quint32 ActiveWindowTracker::activeWindowId() const
{
    return m_activeWindowId;
}

void ActiveWindowTracker::setActiveWindowId(quint32 windowId)
{
    if (m_activeWindowId == windowId) {
        return;
    }

    m_activeWindowId = windowId;
    emit activeWindowIdChanged(windowId);
}

ActiveWindowTracker *createActiveWindowTracker(QObject *parent)
{
#ifdef DGM_HAVE_X11_TRACKER
    if (!qEnvironmentVariableIsEmpty("DISPLAY")) {
        return new X11ActiveWindowTracker(parent);
    }
#endif

    return new UnsupportedActiveWindowTracker(parent);
}

} // namespace dgm
