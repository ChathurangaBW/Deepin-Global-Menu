// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDBusSignature>
#include <QDBusVariant>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QString>
#include <QVariantMap>

class QDBusArgument;

namespace dgm {

struct GtkMenuLink
{
    uint groupId = 0;
    uint menuId = 0;

    friend bool operator==(const GtkMenuLink &lhs, const GtkMenuLink &rhs)
    {
        return lhs.groupId == rhs.groupId && lhs.menuId == rhs.menuId;
    }
};

struct GtkMenuSection
{
    uint groupId = 0;
    uint menuId = 0;
    QList<QVariantMap> items;
};

using GtkMenuSectionList = QList<GtkMenuSection>;

struct GtkMenuChange
{
    uint groupId = 0;
    uint menuId = 0;
    uint position = 0;
    uint removed = 0;
    QList<QVariantMap> added;
};

using GtkMenuChangeList = QList<GtkMenuChange>;

struct GtkActionDescription
{
    bool enabled = true;
    QDBusSignature parameterType;
    QList<QDBusVariant> state;
};

using GtkActionDescriptionMap = QMap<QString, GtkActionDescription>;
using GtkActionEnabledMap = QMap<QString, bool>;
using DbusVariantList = QList<QDBusVariant>;

QDBusArgument &operator<<(QDBusArgument &argument, const GtkMenuLink &link);
const QDBusArgument &operator>>(const QDBusArgument &argument, GtkMenuLink &link);

QDBusArgument &operator<<(QDBusArgument &argument, const GtkMenuSection &section);
const QDBusArgument &operator>>(const QDBusArgument &argument, GtkMenuSection &section);

QDBusArgument &operator<<(QDBusArgument &argument, const GtkMenuChange &change);
const QDBusArgument &operator>>(const QDBusArgument &argument, GtkMenuChange &change);

QDBusArgument &operator<<(QDBusArgument &argument, const GtkActionDescription &description);
const QDBusArgument &operator>>(const QDBusArgument &argument, GtkActionDescription &description);

void registerGtkMenuMetaTypes();

} // namespace dgm

Q_DECLARE_METATYPE(dgm::GtkMenuLink)
Q_DECLARE_METATYPE(dgm::GtkMenuSection)
Q_DECLARE_METATYPE(dgm::GtkMenuSectionList)
Q_DECLARE_METATYPE(dgm::GtkMenuChange)
Q_DECLARE_METATYPE(dgm::GtkMenuChangeList)
Q_DECLARE_METATYPE(dgm::GtkActionDescription)
Q_DECLARE_METATYPE(dgm::GtkActionDescriptionMap)
Q_DECLARE_METATYPE(dgm::GtkActionEnabledMap)
Q_DECLARE_METATYPE(dgm::DbusVariantList)
