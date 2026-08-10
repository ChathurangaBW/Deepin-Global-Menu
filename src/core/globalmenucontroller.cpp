// SPDX-License-Identifier: GPL-3.0-or-later

#include "globalmenucontroller.h"

namespace dgm {

GlobalMenuController::GlobalMenuController(MenuRegistry *registry, QObject *parent)
    : QObject(parent)
    , m_registry(registry)
{
    Q_ASSERT(m_registry);

    connect(m_registry, &MenuRegistry::windowRegistered, this,
            [this](quint32 windowId) {
                if (windowId == m_activeWindowId) {
                    refreshEndpoint();
                }
            });

    connect(m_registry, &MenuRegistry::windowUnregistered, this,
            [this](quint32 windowId) {
                if (windowId == m_activeWindowId) {
                    refreshEndpoint();
                }
            });
}

quint32 GlobalMenuController::activeWindowId() const
{
    return m_activeWindowId;
}

void GlobalMenuController::setActiveWindowId(quint32 windowId)
{
    if (m_activeWindowId == windowId) {
        return;
    }

    m_activeWindowId = windowId;
    emit activeWindowIdChanged();
    refreshEndpoint();
}

bool GlobalMenuController::hasMenu() const
{
    return m_endpoint.isValid();
}

QString GlobalMenuController::menuService() const
{
    return m_endpoint.service;
}

QString GlobalMenuController::menuObjectPath() const
{
    return m_endpoint.objectPath.path();
}

void GlobalMenuController::refreshEndpoint()
{
    const auto next = m_registry->menuForWindow(m_activeWindowId);
    if (m_endpoint.service == next.service && m_endpoint.objectPath.path() == next.objectPath.path()) {
        return;
    }

    m_endpoint = next;
    emit menuEndpointChanged();
}

} // namespace dgm
