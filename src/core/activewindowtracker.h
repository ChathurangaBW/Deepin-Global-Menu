// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

namespace dgm {

class ActiveWindowTracker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 activeWindowId READ activeWindowId NOTIFY activeWindowIdChanged)

public:
    explicit ActiveWindowTracker(QObject *parent = nullptr);
    ~ActiveWindowTracker() override = default;

    [[nodiscard]] quint32 activeWindowId() const;

    virtual bool start() = 0;
    virtual void stop() = 0;
    [[nodiscard]] virtual bool isRunning() const = 0;

signals:
    void activeWindowIdChanged(quint32 windowId);

protected:
    void setActiveWindowId(quint32 windowId);

private:
    quint32 m_activeWindowId = 0;
};

ActiveWindowTracker *createActiveWindowTracker(QObject *parent = nullptr);

} // namespace dgm
