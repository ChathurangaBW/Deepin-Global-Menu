// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "menuregistry.h"

#include <QObject>
#include <QString>

namespace dgm {

class GlobalMenuController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 activeWindowId READ activeWindowId WRITE setActiveWindowId NOTIFY activeWindowIdChanged)
    Q_PROPERTY(bool hasMenu READ hasMenu NOTIFY menuEndpointChanged)
    Q_PROPERTY(QString menuService READ menuService NOTIFY menuEndpointChanged)
    Q_PROPERTY(QString menuObjectPath READ menuObjectPath NOTIFY menuEndpointChanged)

public:
    explicit GlobalMenuController(MenuRegistry *registry, QObject *parent = nullptr);

    [[nodiscard]] quint32 activeWindowId() const;
    void setActiveWindowId(quint32 windowId);

    [[nodiscard]] bool hasMenu() const;
    [[nodiscard]] QString menuService() const;
    [[nodiscard]] QString menuObjectPath() const;

signals:
    void activeWindowIdChanged();
    void menuEndpointChanged();

private:
    void refreshEndpoint();

    MenuRegistry *m_registry = nullptr;
    quint32 m_activeWindowId = 0;
    MenuEndpoint m_endpoint;
};

} // namespace dgm
