// SPDX-License-Identifier: GPL-3.0-or-later

#include "gtkmenutypes.h"

#include <QDBusArgument>
#include <QDBusMetaType>

namespace dgm {

QDBusArgument &operator<<(QDBusArgument &argument, const GtkMenuLink &link)
{
    argument.beginStructure();
    argument << link.groupId << link.menuId;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, GtkMenuLink &link)
{
    argument.beginStructure();
    argument >> link.groupId >> link.menuId;
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const GtkMenuSection &section)
{
    argument.beginStructure();
    argument << section.groupId << section.menuId;
    argument.beginArray(QMetaType::fromType<QVariantMap>());
    for (const auto &item : section.items) {
        argument << item;
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, GtkMenuSection &section)
{
    argument.beginStructure();
    argument >> section.groupId >> section.menuId;
    section.items.clear();
    argument.beginArray();
    while (!argument.atEnd()) {
        QVariantMap item;
        argument >> item;
        section.items.append(item);
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const GtkMenuChange &change)
{
    argument.beginStructure();
    argument << change.groupId << change.menuId << change.position << change.removed;
    argument.beginArray(QMetaType::fromType<QVariantMap>());
    for (const auto &item : change.added) {
        argument << item;
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, GtkMenuChange &change)
{
    argument.beginStructure();
    argument >> change.groupId >> change.menuId >> change.position >> change.removed;
    change.added.clear();
    argument.beginArray();
    while (!argument.atEnd()) {
        QVariantMap item;
        argument >> item;
        change.added.append(item);
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const GtkActionDescription &description)
{
    argument.beginStructure();
    argument << description.enabled << description.parameterType;
    argument.beginArray(QMetaType::fromType<QDBusVariant>());
    for (const auto &state : description.state) {
        argument << state;
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, GtkActionDescription &description)
{
    argument.beginStructure();
    argument >> description.enabled >> description.parameterType;
    description.state.clear();
    argument.beginArray();
    while (!argument.atEnd()) {
        QDBusVariant value;
        argument >> value;
        description.state.append(value);
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

void registerGtkMenuMetaTypes()
{
    static const bool registered = [] {
        qRegisterMetaType<GtkMenuSectionList>("dgm::GtkMenuSectionList");
        qRegisterMetaType<GtkMenuChangeList>("dgm::GtkMenuChangeList");
        qRegisterMetaType<GtkActionDescriptionMap>("dgm::GtkActionDescriptionMap");
        qRegisterMetaType<DbusVariantList>("dgm::DbusVariantList");

        qDBusRegisterMetaType<GtkMenuLink>();
        qDBusRegisterMetaType<GtkMenuSection>();
        qDBusRegisterMetaType<GtkMenuSectionList>();
        qDBusRegisterMetaType<GtkMenuChange>();
        qDBusRegisterMetaType<GtkMenuChangeList>();
        qDBusRegisterMetaType<GtkActionDescription>();
        qDBusRegisterMetaType<GtkActionDescriptionMap>();
        qDBusRegisterMetaType<DbusVariantList>();
        qDBusRegisterMetaType<QList<uint>>();
        return true;
    }();

    Q_UNUSED(registered);
}

} // namespace dgm
