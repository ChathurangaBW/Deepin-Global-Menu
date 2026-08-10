// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "menuregistry.h"

#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>

namespace dgm {

class AppMenuRegistrar final : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.canonical.AppMenu.Registrar")

public:
    explicit AppMenuRegistrar(MenuRegistry *registry, QObject *parent = nullptr);
    ~AppMenuRegistrar() override;

    [[nodiscard]] bool start();
    void stop();
    [[nodiscard]] bool isRunning() const;

public slots:
    void RegisterWindow(quint32 windowId, const QDBusObjectPath &menuObjectPath);
    void UnregisterWindow(quint32 windowId);
    void GetMenuForWindow(quint32 windowId, QString &service, QDBusObjectPath &menuObjectPath) const;

signals:
    void WindowRegistered(quint32 windowId, const QString &service, const QDBusObjectPath &menuObjectPath);
    void WindowUnregistered(quint32 windowId);

private:
    MenuRegistry *m_registry = nullptr;
    bool m_running = false;
};

} // namespace dgm
