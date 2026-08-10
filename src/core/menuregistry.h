// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDBusObjectPath>
#include <QHash>
#include <QObject>
#include <QString>

namespace dgm {

struct MenuEndpoint
{
    QString service;
    QDBusObjectPath objectPath{QStringLiteral("/")};

    [[nodiscard]] bool isValid() const
    {
        // `/` is a valid D-Bus object path. Treat the endpoint as invalid only
        // when the exporter service is missing or Qt rejected/cleared the path.
        return !service.isEmpty() && !objectPath.path().isEmpty();
    }
};

class MenuRegistry final : public QObject
{
    Q_OBJECT

public:
    explicit MenuRegistry(QObject *parent = nullptr);

    void registerWindow(quint32 windowId, const QString &service, const QDBusObjectPath &objectPath);
    void unregisterWindow(quint32 windowId);
    void unregisterService(const QString &service);

    [[nodiscard]] MenuEndpoint menuForWindow(quint32 windowId) const;
    [[nodiscard]] bool contains(quint32 windowId) const;

signals:
    void windowRegistered(quint32 windowId, const QString &service, const QDBusObjectPath &objectPath);
    void windowUnregistered(quint32 windowId);

private:
    QHash<quint32, MenuEndpoint> m_windows;
};

} // namespace dgm
