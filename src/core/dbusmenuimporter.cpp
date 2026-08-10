// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusmenuimporter.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QStringList>
#include <QVariant>

namespace dgm {

namespace {
constexpr auto kDbusMenuInterface = "com.canonical.dbusmenu";
}

DbusMenuImporter::DbusMenuImporter(QObject *parent)
    : QObject(parent)
{
    registerDbusMenuMetaTypes();
}

DbusMenuImporter::~DbusMenuImporter()
{
    disconnectRemoteSignals();
}

void DbusMenuImporter::setEndpoint(const MenuEndpoint &endpoint)
{
    if (m_endpoint.service == endpoint.service
        && m_endpoint.objectPath.path() == endpoint.objectPath.path()) {
        return;
    }

    disconnectRemoteSignals();
    delete m_interface;
    m_interface = nullptr;

    ++m_generation;
    ++m_refreshSerial;
    m_endpoint = endpoint;
    clearLayout();
    setErrorString({});

    if (!m_endpoint.isValid()) {
        return;
    }

    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        setErrorString(QStringLiteral("Session D-Bus is unavailable"));
        return;
    }

    m_interface = new QDBusInterface(m_endpoint.service,
                                     m_endpoint.objectPath.path(),
                                     QString::fromLatin1(kDbusMenuInterface),
                                     bus,
                                     this);

    if (!m_interface->isValid()) {
        setErrorString(QStringLiteral("DBusMenu endpoint is not available"));
        return;
    }

    connectRemoteSignals();
}

MenuEndpoint DbusMenuImporter::endpoint() const
{
    return m_endpoint;
}

bool DbusMenuImporter::isReady() const
{
    return m_ready;
}

uint DbusMenuImporter::revision() const
{
    return m_revision;
}

const DbusMenuLayoutItem &DbusMenuImporter::rootItem() const
{
    return m_root;
}

QVariantList DbusMenuImporter::topLevelItems() const
{
    QVariantList items;
    items.reserve(m_root.children.size());
    for (const auto &child : m_root.children) {
        items.append(child.toVariantMap());
    }
    return items;
}

QString DbusMenuImporter::errorString() const
{
    return m_errorString;
}

void DbusMenuImporter::refresh()
{
    if (!m_endpoint.isValid() || !m_interface || !m_interface->isValid()) {
        setErrorString(QStringLiteral("No valid DBusMenu endpoint is selected"));
        emit refreshFinished(false);
        return;
    }

    const quint64 generation = m_generation;
    const quint64 serial = ++m_refreshSerial;

    const auto call = m_interface->asyncCall(QStringLiteral("GetLayout"),
                                             0,
                                             -1,
                                             QStringList{});
    auto *watcher = new QDBusPendingCallWatcher(call, this);

    connect(watcher, &QDBusPendingCallWatcher::finished,
            this,
            [this, watcher, generation, serial](QDBusPendingCallWatcher *) {
                QDBusPendingReply<uint, DbusMenuLayoutItem> reply = *watcher;
                watcher->deleteLater();

                if (generation != m_generation || serial != m_refreshSerial) {
                    return;
                }

                if (reply.isError()) {
                    setErrorString(reply.error().message());
                    emit refreshFinished(false);
                    return;
                }

                m_revision = reply.argumentAt<0>();
                m_root = reply.argumentAt<1>();
                m_ready = true;
                setErrorString({});
                emit layoutChanged();
                emit refreshFinished(true);
            });
}

void DbusMenuImporter::triggerAction(int itemId, uint timestamp)
{
    if (!m_interface || !m_interface->isValid()) {
        return;
    }

    const QVariant eventData = QVariant::fromValue(QDBusVariant(QVariant(0)));
    m_interface->asyncCall(QStringLiteral("Event"),
                           itemId,
                           QStringLiteral("clicked"),
                           eventData,
                           timestamp);
}

void DbusMenuImporter::prepareSubmenu(int itemId)
{
    if (!m_interface || !m_interface->isValid()) {
        return;
    }

    const quint64 generation = m_generation;
    const auto call = m_interface->asyncCall(QStringLiteral("AboutToShow"), itemId);
    auto *watcher = new QDBusPendingCallWatcher(call, this);

    connect(watcher, &QDBusPendingCallWatcher::finished,
            this,
            [this, watcher, generation](QDBusPendingCallWatcher *) {
                QDBusPendingReply<bool> reply = *watcher;
                watcher->deleteLater();

                if (generation != m_generation || reply.isError()) {
                    return;
                }

                if (reply.value()) {
                    refresh();
                }
            });
}

void DbusMenuImporter::onLayoutUpdated(uint revision, int parentId)
{
    Q_UNUSED(parentId);

    if (!m_ready || revision != m_revision) {
        refresh();
    }
}

void DbusMenuImporter::onItemsPropertiesUpdated(dgm::DbusMenuItemList updated,
                                                dgm::DbusMenuItemKeysList removed)
{
    if (!m_ready) {
        refresh();
        return;
    }

    bool changed = false;
    bool requiresRefresh = false;

    for (const auto &update : updated) {
        auto *item = findItem(m_root, update.id);
        if (!item) {
            requiresRefresh = true;
            continue;
        }

        for (auto it = update.properties.cbegin(); it != update.properties.cend(); ++it) {
            if (item->properties.value(it.key()) == it.value()) {
                continue;
            }
            item->properties.insert(it.key(), it.value());
            changed = true;
        }
    }

    for (const auto &removal : removed) {
        auto *item = findItem(m_root, removal.id);
        if (!item) {
            requiresRefresh = true;
            continue;
        }

        for (const auto &property : removal.properties) {
            changed = item->properties.remove(property) > 0 || changed;
        }
    }

    if (changed) {
        emit layoutChanged();
    }

    if (requiresRefresh) {
        refresh();
    }
}

void DbusMenuImporter::connectRemoteSignals()
{
    if (!m_endpoint.isValid()) {
        return;
    }

    auto bus = QDBusConnection::sessionBus();
    bus.connect(m_endpoint.service,
                m_endpoint.objectPath.path(),
                QString::fromLatin1(kDbusMenuInterface),
                QStringLiteral("LayoutUpdated"),
                this,
                SLOT(onLayoutUpdated(uint,int)));

    bus.connect(m_endpoint.service,
                m_endpoint.objectPath.path(),
                QString::fromLatin1(kDbusMenuInterface),
                QStringLiteral("ItemsPropertiesUpdated"),
                this,
                SLOT(onItemsPropertiesUpdated(dgm::DbusMenuItemList,dgm::DbusMenuItemKeysList)));
}

void DbusMenuImporter::disconnectRemoteSignals()
{
    if (!m_endpoint.isValid()) {
        return;
    }

    auto bus = QDBusConnection::sessionBus();
    bus.disconnect(m_endpoint.service,
                   m_endpoint.objectPath.path(),
                   QString::fromLatin1(kDbusMenuInterface),
                   QStringLiteral("LayoutUpdated"),
                   this,
                   SLOT(onLayoutUpdated(uint,int)));

    bus.disconnect(m_endpoint.service,
                   m_endpoint.objectPath.path(),
                   QString::fromLatin1(kDbusMenuInterface),
                   QStringLiteral("ItemsPropertiesUpdated"),
                   this,
                   SLOT(onItemsPropertiesUpdated(dgm::DbusMenuItemList,dgm::DbusMenuItemKeysList)));
}

void DbusMenuImporter::clearLayout()
{
    const bool hadLayout = m_ready || m_revision != 0 || m_root.id != -1 || !m_root.children.isEmpty();
    m_root = {};
    m_revision = 0;
    m_ready = false;

    if (hadLayout) {
        emit layoutChanged();
    }
}

void DbusMenuImporter::setErrorString(const QString &errorString)
{
    if (m_errorString == errorString) {
        return;
    }

    m_errorString = errorString;
    emit errorStringChanged();
}

DbusMenuLayoutItem *DbusMenuImporter::findItem(DbusMenuLayoutItem &root, int id)
{
    if (root.id == id) {
        return &root;
    }

    for (auto &child : root.children) {
        if (auto *item = findItem(child, id)) {
            return item;
        }
    }

    return nullptr;
}

} // namespace dgm
