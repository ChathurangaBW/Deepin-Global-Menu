// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>

class WaylandVirtualKeyboardManager;

class ShortcutExecutor final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availabilityChanged)

public:
    explicit ShortcutExecutor(QObject *parent = nullptr);
    ~ShortcutExecutor() override;

    [[nodiscard]] bool available() const;

public slots:
    void send(const QString &shortcut);

signals:
    void availabilityChanged();

private:
    void sendNow(const QString &shortcut);
    bool sendX11(const QString &shortcut);
    bool sendWayland(const QString &shortcut);

    WaylandVirtualKeyboardManager *m_waylandManager = nullptr;
};
