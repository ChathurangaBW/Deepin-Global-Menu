// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusmenutypes.h"

#include <QDBusArgument>
#include <QDBusMetaType>
#include <QDBusVariant>
#include <QMetaType>
#include <QVariant>
#include <QVariantList>

namespace dgm {

namespace {
QString removeMnemonicMarkers(const QString &label)
{
    QString result;
    result.reserve(label.size());

    for (qsizetype i = 0; i < label.size(); ++i) {
        if (label.at(i) != QLatin1Char('_')) {
            result.append(label.at(i));
            continue;
        }

        if (i + 1 < label.size() && label.at(i + 1) == QLatin1Char('_')) {
            result.append(QLatin1Char('_'));
            ++i;
        }
    }

    return result;
}
} // namespace

QString DbusMenuLayoutItem::displayLabel() const
{
    return removeMnemonicMarkers(properties.value(QStringLiteral("label")).toString());
}

bool DbusMenuLayoutItem::isEnabled() const
{
    return properties.value(QStringLiteral("enabled"), true).toBool();
}

bool DbusMenuLayoutItem::isVisible() const
{
    return properties.value(QStringLiteral("visible"), true).toBool();
}

bool DbusMenuLayoutItem::isSeparator() const
{
    return properties.value(QStringLiteral("type")).toString() == QStringLiteral("separator");
}

QVariantMap DbusMenuLayoutItem::toVariantMap() const
{
    QVariantMap result = properties;
    result.insert(QStringLiteral("id"), id);
    result.insert(QStringLiteral("label"), displayLabel());
    result.insert(QStringLiteral("enabled"), isEnabled());
    result.insert(QStringLiteral("visible"), isVisible());
    result.insert(QStringLiteral("separator"), isSeparator());

    QVariantList childList;
    childList.reserve(children.size());
    for (const auto &child : children) {
        childList.append(child.toVariantMap());
    }
    result.insert(QStringLiteral("children"), childList);

    return result;
}

QDBusArgument &operator<<(QDBusArgument &argument, const DbusMenuItem &item)
{
    argument.beginStructure();
    argument << item.id << item.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DbusMenuItem &item)
{
    argument.beginStructure();
    argument >> item.id >> item.properties;
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const DbusMenuItemKeys &item)
{
    argument.beginStructure();
    argument << item.id << item.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DbusMenuItemKeys &item)
{
    argument.beginStructure();
    argument >> item.id >> item.properties;
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const DbusMenuLayoutItem &item)
{
    argument.beginStructure();
    argument << item.id << item.properties;
    argument.beginArray(QMetaType::fromType<QDBusVariant>());

    for (const auto &child : item.children) {
        argument << QDBusVariant(QVariant::fromValue(child));
    }

    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DbusMenuLayoutItem &item)
{
    argument.beginStructure();
    argument >> item.id >> item.properties;

    item.children.clear();
    argument.beginArray();
    while (!argument.atEnd()) {
        QDBusVariant wrappedChild;
        argument >> wrappedChild;

        const QVariant childVariant = wrappedChild.variant();
        if (childVariant.metaType() == QMetaType::fromType<QDBusArgument>()) {
            QDBusArgument childArgument = childVariant.value<QDBusArgument>();
            DbusMenuLayoutItem child;
            childArgument >> child;
            item.children.append(child);
            continue;
        }

        if (childVariant.canConvert<DbusMenuLayoutItem>()) {
            item.children.append(childVariant.value<DbusMenuLayoutItem>());
        }
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

void registerDbusMenuMetaTypes()
{
    static const bool registered = [] {
        qDBusRegisterMetaType<DbusMenuItem>();
        qDBusRegisterMetaType<DbusMenuItemList>();
        qDBusRegisterMetaType<DbusMenuItemKeys>();
        qDBusRegisterMetaType<DbusMenuItemKeysList>();
        qDBusRegisterMetaType<DbusMenuLayoutItem>();
        return true;
    }();

    Q_UNUSED(registered);
}

} // namespace dgm
