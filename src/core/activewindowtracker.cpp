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
        setActiveWindowInfo({});
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
    qRegisterMetaType<ActiveWindowInfo>();
}

quint32 ActiveWindowTracker::activeWindowId() const
{
    return m_activeWindowInfo.x11Id != 0
        ? m_activeWindowInfo.x11Id
        : m_activeWindowInfo.nativeId;
}

const ActiveWindowInfo &ActiveWindowTracker::activeWindowInfo() const
{
    return m_activeWindowInfo;
}

void ActiveWindowTracker::setActiveWindowId(quint32 windowId)
{
    ActiveWindowInfo info = m_activeWindowInfo;
    info.nativeId = windowId;
    info.x11Id = windowId;
    setActiveWindowInfo(info);
}

void ActiveWindowTracker::setActiveWindowInfo(const ActiveWindowInfo &info)
{
    if (m_activeWindowInfo == info) {
        return;
    }

    const quint32 previousId = activeWindowId();
    m_activeWindowInfo = info;
    const quint32 nextId = activeWindowId();

    if (previousId != nextId) {
        emit activeWindowIdChanged(nextId);
    }
    emit activeWindowInfoChanged(m_activeWindowInfo);
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
