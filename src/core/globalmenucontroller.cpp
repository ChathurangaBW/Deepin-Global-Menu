// SPDX-License-Identifier: GPL-3.0-or-later

#include "globalmenucontroller.h"

#include "dbusmenuimporter.h"

namespace dgm {

GlobalMenuController::GlobalMenuController(MenuRegistry *registry, QObject *parent)
    : QObject(parent)
    , m_registry(registry)
    , m_importer(new DbusMenuImporter(this))
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

    connect(m_importer, &DbusMenuImporter::layoutChanged,
            this, &GlobalMenuController::menuItemsChanged);
    connect(m_importer, &DbusMenuImporter::errorStringChanged,
            this, &GlobalMenuController::menuStatusChanged);
    connect(m_importer, &DbusMenuImporter::refreshFinished,
            this, [this](bool) { emit menuStatusChanged(); });
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

bool GlobalMenuController::menuReady() const
{
    return m_importer->isReady();
}

uint GlobalMenuController::menuRevision() const
{
    return m_importer->revision();
}

QVariantList GlobalMenuController::menuItems() const
{
    return m_importer->topLevelItems();
}

QString GlobalMenuController::menuError() const
{
    return m_importer->errorString();
}

void GlobalMenuController::refreshMenu()
{
    m_importer->refresh();
}

void GlobalMenuController::triggerMenuAction(int itemId, uint timestamp)
{
    m_importer->triggerAction(itemId, timestamp);
}

void GlobalMenuController::prepareSubmenu(int itemId)
{
    m_importer->prepareSubmenu(itemId);
}

void GlobalMenuController::refreshEndpoint()
{
    const auto next = m_registry->menuForWindow(m_activeWindowId);
    if (m_endpoint.service == next.service && m_endpoint.objectPath.path() == next.objectPath.path()) {
        return;
    }

    m_endpoint = next;
    m_importer->setEndpoint(m_endpoint);

    if (m_endpoint.isValid()) {
        m_importer->refresh();
    }

    emit menuEndpointChanged();
    emit menuStatusChanged();
}

} // namespace dgm
