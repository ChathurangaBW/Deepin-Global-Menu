// SPDX-License-Identifier: GPL-3.0-or-later

#include "menuregistry.h"

namespace dgm {

MenuRegistry::MenuRegistry(QObject *parent)
    : QObject(parent)
{
}

void MenuRegistry::registerWindow(quint32 windowId, const QString &service, const QDBusObjectPath &objectPath)
{
    const MenuEndpoint endpoint{service, objectPath};
    m_windows.insert(windowId, endpoint);
    emit windowRegistered(windowId, service, objectPath);
}

void MenuRegistry::unregisterWindow(quint32 windowId)
{
    if (m_windows.remove(windowId) > 0) {
        emit windowUnregistered(windowId);
    }
}

void MenuRegistry::unregisterService(const QString &service)
{
    QList<quint32> staleWindows;
    staleWindows.reserve(m_windows.size());

    for (auto it = m_windows.cbegin(); it != m_windows.cend(); ++it) {
        if (it.value().service == service) {
            staleWindows.push_back(it.key());
        }
    }

    for (const quint32 windowId : staleWindows) {
        unregisterWindow(windowId);
    }
}

MenuEndpoint MenuRegistry::menuForWindow(quint32 windowId) const
{
    return m_windows.value(windowId);
}

const QHash<quint32, MenuEndpoint> &MenuRegistry::entries() const
{
    return m_windows;
}

bool MenuRegistry::contains(quint32 windowId) const
{
    return m_windows.contains(windowId);
}

} // namespace dgm
