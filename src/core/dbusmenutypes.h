// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class QDBusArgument;

namespace dgm {

struct DbusMenuItem
{
    int id = -1;
    QVariantMap properties;
};

using DbusMenuItemList = QList<DbusMenuItem>;

struct DbusMenuItemKeys
{
    int id = -1;
    QStringList properties;
};

using DbusMenuItemKeysList = QList<DbusMenuItemKeys>;

struct DbusMenuLayoutItem
{
    int id = -1;
    QVariantMap properties;
    QList<DbusMenuLayoutItem> children;

    [[nodiscard]] QString displayLabel() const;
    [[nodiscard]] bool isEnabled() const;
    [[nodiscard]] bool isVisible() const;
    [[nodiscard]] bool isSeparator() const;
    [[nodiscard]] QVariantMap toVariantMap() const;
};

QDBusArgument &operator<<(QDBusArgument &argument, const DbusMenuItem &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, DbusMenuItem &item);

QDBusArgument &operator<<(QDBusArgument &argument, const DbusMenuItemKeys &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, DbusMenuItemKeys &item);

QDBusArgument &operator<<(QDBusArgument &argument, const DbusMenuLayoutItem &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, DbusMenuLayoutItem &item);

void registerDbusMenuMetaTypes();

} // namespace dgm

Q_DECLARE_METATYPE(dgm::DbusMenuItem)
Q_DECLARE_METATYPE(dgm::DbusMenuItemList)
Q_DECLARE_METATYPE(dgm::DbusMenuItemKeys)
Q_DECLARE_METATYPE(dgm::DbusMenuItemKeysList)
Q_DECLARE_METATYPE(dgm::DbusMenuLayoutItem)
