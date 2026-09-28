// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "menuregistry.h"

#include <QDBusContext>
#include <QDBusObjectPath>
#include <QList>
#include <QMetaType>
#include <QObject>

class QDBusArgument;

namespace dgm {

struct RegistrarMenu
{
    quint32 windowId = 0;
    QString service;
    QDBusObjectPath objectPath{QStringLiteral("/")};
};

using RegistrarMenuList = QList<RegistrarMenu>;

QDBusArgument &operator<<(QDBusArgument &argument, const RegistrarMenu &menu);
const QDBusArgument &operator>>(const QDBusArgument &argument, RegistrarMenu &menu);

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
    RegistrarMenuList GetMenus() const;

signals:
    void WindowRegistered(quint32 windowId, const QString &service, const QDBusObjectPath &menuObjectPath);
    void WindowUnregistered(quint32 windowId);

private:
    MenuRegistry *m_registry = nullptr;
    QMetaObject::Connection m_ownerChangedConnection;
    bool m_running = false;
};

} // namespace dgm

Q_DECLARE_METATYPE(dgm::RegistrarMenu)
Q_DECLARE_METATYPE(dgm::RegistrarMenuList)
