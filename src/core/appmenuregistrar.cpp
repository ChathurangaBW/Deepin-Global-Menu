// SPDX-License-Identifier: GPL-3.0-or-later

#include "appmenuregistrar.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDebug>

#include <algorithm>

namespace dgm {

QDBusArgument &operator<<(QDBusArgument &argument, const RegistrarMenu &menu)
{
    argument.beginStructure();
    argument << menu.windowId << menu.service << menu.objectPath;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, RegistrarMenu &menu)
{
    argument.beginStructure();
    argument >> menu.windowId >> menu.service >> menu.objectPath;
    argument.endStructure();
    return argument;
}

namespace {
constexpr auto kRegistrarService = "com.canonical.AppMenu.Registrar";
constexpr auto kRegistrarPath = "/com/canonical/AppMenu/Registrar";
}

AppMenuRegistrar::AppMenuRegistrar(MenuRegistry *registry, QObject *parent)
    : QObject(parent)
    , m_registry(registry)
{
    Q_ASSERT(m_registry);
    qRegisterMetaType<RegistrarMenuList>("dgm::RegistrarMenuList");
    qDBusRegisterMetaType<RegistrarMenu>();
    qDBusRegisterMetaType<RegistrarMenuList>();
}

AppMenuRegistrar::~AppMenuRegistrar()
{
    stop();
}

bool AppMenuRegistrar::start()
{
    if (m_running) {
        return true;
    }

    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qWarning() << "Deepin Global Menu: session D-Bus is unavailable";
        return false;
    }

    if (!bus.registerService(QString::fromLatin1(kRegistrarService))) {
        qWarning() << "Deepin Global Menu: could not own" << kRegistrarService
                   << "- another registrar may already be running";
        return false;
    }

    const auto flags = QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals;
    if (!bus.registerObject(QString::fromLatin1(kRegistrarPath), this, flags)) {
        bus.unregisterService(QString::fromLatin1(kRegistrarService));
        qWarning() << "Deepin Global Menu: failed to export registrar object";
        return false;
    }

    if (auto *iface = bus.interface()) {
        if (m_ownerChangedConnection) {
            disconnect(m_ownerChangedConnection);
        }
        m_ownerChangedConnection = connect(
            iface,
            &QDBusConnectionInterface::serviceOwnerChanged,
            this,
            [this](const QString &service,
                   const QString &oldOwner,
                   const QString &newOwner) {
                if (!oldOwner.isEmpty() && newOwner.isEmpty()) {
                    m_registry->unregisterService(service);
                }
            });
    }

    m_running = true;
    return true;
}

void AppMenuRegistrar::stop()
{
    if (!m_running) {
        return;
    }

    auto bus = QDBusConnection::sessionBus();
    if (m_ownerChangedConnection) {
        disconnect(m_ownerChangedConnection);
        m_ownerChangedConnection = {};
    }

    bus.unregisterObject(QString::fromLatin1(kRegistrarPath));
    bus.unregisterService(QString::fromLatin1(kRegistrarService));
    m_running = false;
}

bool AppMenuRegistrar::isRunning() const
{
    return m_running;
}

void AppMenuRegistrar::RegisterWindow(quint32 windowId, const QDBusObjectPath &menuObjectPath)
{
    const QString service = calledFromDBus() ? message().service() : QString();
    if (service.isEmpty()) {
        qWarning() << "Deepin Global Menu: rejecting menu registration without a D-Bus sender";
        return;
    }

    m_registry->registerWindow(windowId, service, menuObjectPath);
    emit WindowRegistered(windowId, service, menuObjectPath);
}

void AppMenuRegistrar::UnregisterWindow(quint32 windowId)
{
    if (!m_registry->contains(windowId)) {
        return;
    }

    m_registry->unregisterWindow(windowId);
    emit WindowUnregistered(windowId);
}

void AppMenuRegistrar::GetMenuForWindow(quint32 windowId,
                                        QString &service,
                                        QDBusObjectPath &menuObjectPath) const
{
    const auto endpoint = m_registry->menuForWindow(windowId);
    service = endpoint.service;
    menuObjectPath = endpoint.objectPath;
}

RegistrarMenuList AppMenuRegistrar::GetMenus() const
{
    RegistrarMenuList menus;
    menus.reserve(m_registry->entries().size());

    for (auto it = m_registry->entries().cbegin();
         it != m_registry->entries().cend();
         ++it) {
        menus.append(RegistrarMenu{it.key(),
                                   it.value().service,
                                   it.value().objectPath});
    }

    std::sort(menus.begin(), menus.end(),
              [](const RegistrarMenu &lhs, const RegistrarMenu &rhs) {
                  return lhs.windowId < rhs.windowId;
              });
    return menus;
}

} // namespace dgm
