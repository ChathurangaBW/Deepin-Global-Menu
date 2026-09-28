// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "menuregistry.h"

#include <QObject>
#include <QString>
#include <QVariantList>

namespace dgm {

class ActiveWindowTracker;
class DbusMenuImporter;

class GlobalMenuController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 activeWindowId READ activeWindowId WRITE setActiveWindowId NOTIFY activeWindowIdChanged)
    Q_PROPERTY(bool hasMenu READ hasMenu NOTIFY menuEndpointChanged)
    Q_PROPERTY(QString menuService READ menuService NOTIFY menuEndpointChanged)
    Q_PROPERTY(QString menuObjectPath READ menuObjectPath NOTIFY menuEndpointChanged)
    Q_PROPERTY(bool menuReady READ menuReady NOTIFY menuItemsChanged)
    Q_PROPERTY(uint menuRevision READ menuRevision NOTIFY menuItemsChanged)
    Q_PROPERTY(QVariantList menuItems READ menuItems NOTIFY menuItemsChanged)
    Q_PROPERTY(QString menuError READ menuError NOTIFY menuStatusChanged)

public:
    explicit GlobalMenuController(MenuRegistry *registry, QObject *parent = nullptr);

    [[nodiscard]] quint32 activeWindowId() const;
    void setActiveWindowId(quint32 windowId);
    void setActiveWindowTracker(ActiveWindowTracker *tracker);

    [[nodiscard]] bool hasMenu() const;
    [[nodiscard]] QString menuService() const;
    [[nodiscard]] QString menuObjectPath() const;

    [[nodiscard]] bool menuReady() const;
    [[nodiscard]] uint menuRevision() const;
    [[nodiscard]] QVariantList menuItems() const;
    [[nodiscard]] QString menuError() const;

    Q_INVOKABLE void refreshMenu();
    Q_INVOKABLE void triggerMenuAction(int itemId, uint timestamp = 0);
    Q_INVOKABLE void prepareSubmenu(int itemId);

signals:
    void activeWindowIdChanged();
    void menuEndpointChanged();
    void menuItemsChanged();
    void menuStatusChanged();

private:
    void refreshEndpoint();

    MenuRegistry *m_registry = nullptr;
    DbusMenuImporter *m_importer = nullptr;
    ActiveWindowTracker *m_windowTracker = nullptr;
    quint32 m_activeWindowId = 0;
    MenuEndpoint m_endpoint;
};

} // namespace dgm
