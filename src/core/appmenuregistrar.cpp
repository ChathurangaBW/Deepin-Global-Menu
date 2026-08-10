// SPDX-License-Identifier: GPL-3.0-or-later

#include "appmenuregistrar.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>

namespace dgm {

namespace {
constexpr auto kRegistrarService = "com.canonical.AppMenu.Registrar";
constexpr auto kRegistrarPath = "/com/canonical/AppMenu/Registrar";
}

AppMenuRegistrar::AppMenuRegistrar(MenuRegistry *registry, QObject *parent)
    : QObject(parent)
    , m_registry(registry)
{
    Q_ASSERT(m_registry);
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
        connect(iface, &QDBusConnectionInterface::serviceOwnerChanged,
                this,
                [this](const QString &service, const QString &oldOwner, const QString &newOwner) {
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

} // namespace dgm
