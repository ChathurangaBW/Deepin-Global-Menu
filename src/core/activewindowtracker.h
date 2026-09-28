// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>

namespace dgm {

struct ActiveWindowInfo
{
    quint32 nativeId = 0;
    quint32 x11Id = 0;
    qint64 pid = 0;
    QString appId;
    QString title;
    QString backend;

    QString gtkUniqueBusName;
    QString gtkApplicationObjectPath;
    QString gtkWindowObjectPath;
    QString gtkAppMenuObjectPath;
    QString gtkMenubarObjectPath;

    [[nodiscard]] bool isValid() const
    {
        return nativeId != 0 || x11Id != 0 || !appId.isEmpty() || !gtkUniqueBusName.isEmpty();
    }

    friend bool operator==(const ActiveWindowInfo &lhs, const ActiveWindowInfo &rhs)
    {
        return lhs.nativeId == rhs.nativeId
            && lhs.x11Id == rhs.x11Id
            && lhs.pid == rhs.pid
            && lhs.appId == rhs.appId
            && lhs.title == rhs.title
            && lhs.backend == rhs.backend
            && lhs.gtkUniqueBusName == rhs.gtkUniqueBusName
            && lhs.gtkApplicationObjectPath == rhs.gtkApplicationObjectPath
            && lhs.gtkWindowObjectPath == rhs.gtkWindowObjectPath
            && lhs.gtkAppMenuObjectPath == rhs.gtkAppMenuObjectPath
            && lhs.gtkMenubarObjectPath == rhs.gtkMenubarObjectPath;
    }

    friend bool operator!=(const ActiveWindowInfo &lhs, const ActiveWindowInfo &rhs)
    {
        return !(lhs == rhs);
    }
};

class ActiveWindowTracker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 activeWindowId READ activeWindowId NOTIFY activeWindowIdChanged)

public:
    explicit ActiveWindowTracker(QObject *parent = nullptr);
    ~ActiveWindowTracker() override = default;

    [[nodiscard]] quint32 activeWindowId() const;
    [[nodiscard]] const ActiveWindowInfo &activeWindowInfo() const;

    virtual bool start() = 0;
    virtual void stop() = 0;
    [[nodiscard]] virtual bool isRunning() const = 0;

signals:
    void activeWindowIdChanged(quint32 windowId);
    void activeWindowInfoChanged(const dgm::ActiveWindowInfo &info);

protected:
    void setActiveWindowId(quint32 windowId);
    void setActiveWindowInfo(const ActiveWindowInfo &info);

private:
    ActiveWindowInfo m_activeWindowInfo;
};

ActiveWindowTracker *createActiveWindowTracker(QObject *parent = nullptr);

} // namespace dgm

Q_DECLARE_METATYPE(dgm::ActiveWindowInfo)
