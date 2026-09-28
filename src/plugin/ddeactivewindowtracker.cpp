// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddeactivewindowtracker.h"

#include <appletbridge.h>

#include <QAbstractItemModel>
#include <QGuiApplication>
#include <QModelIndex>
#include <QVariant>

DdeActiveWindowTracker::DdeActiveWindowTracker(QObject *parent)
    : dgm::ActiveWindowTracker(parent)
    , m_bridge(new DS_NAMESPACE::DAppletBridge(
          QStringLiteral("org.deepin.ds.dock.taskmanager"), this))
{
    m_retryTimer.setInterval(500);
    connect(&m_retryTimer, &QTimer::timeout,
            this, &DdeActiveWindowTracker::refreshBinding);
}

DdeActiveWindowTracker::~DdeActiveWindowTracker()
{
    stop();
}

bool DdeActiveWindowTracker::start()
{
    if (m_running) {
        return true;
    }

    if (QGuiApplication::platformName() != QStringLiteral("wayland")) {
        return false;
    }

    m_running = true;
    refreshBinding();
    if (!m_model) {
        m_retryTimer.start();
    }
    return true;
}

void DdeActiveWindowTracker::stop()
{
    m_retryTimer.stop();

    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }
    m_model.clear();
    m_running = false;
    setActiveWindowInfo({});
}

bool DdeActiveWindowTracker::isRunning() const
{
    return m_running;
}

void DdeActiveWindowTracker::refreshBinding()
{
    if (!m_running) {
        return;
    }

    if (m_model) {
        refreshActiveWindow();
        m_retryTimer.stop();
        return;
    }

    if (!m_bridge || !m_bridge->isValid()) {
        if (!m_retryTimer.isActive()) {
            m_retryTimer.start();
        }
        return;
    }

    auto *applet = m_bridge->applet();
    if (!applet) {
        if (!m_retryTimer.isActive()) {
            m_retryTimer.start();
        }
        return;
    }

    auto *model = applet->property("dataModel").value<QAbstractItemModel *>();
    if (!model) {
        if (!m_retryTimer.isActive()) {
            m_retryTimer.start();
        }
        return;
    }

    bindModel(model);
    m_retryTimer.stop();
}

void DdeActiveWindowTracker::bindModel(QAbstractItemModel *model)
{
    if (m_model == model) {
        refreshActiveWindow();
        return;
    }

    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }

    m_model = model;
    if (!m_model) {
        setActiveWindowInfo({});
        if (m_running) {
            m_retryTimer.start();
        }
        return;
    }

    connect(m_model, &QAbstractItemModel::dataChanged,
            this, [this] { refreshActiveWindow(); });
    connect(m_model, &QAbstractItemModel::rowsInserted,
            this, [this] { refreshActiveWindow(); });
    connect(m_model, &QAbstractItemModel::rowsRemoved,
            this, [this] { refreshActiveWindow(); });
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &DdeActiveWindowTracker::refreshActiveWindow);
    connect(m_model, &QAbstractItemModel::layoutChanged,
            this, [this] { refreshActiveWindow(); });
    connect(m_model, &QObject::destroyed,
            this, [this] {
                m_model.clear();
                setActiveWindowInfo({});
                if (m_running) {
                    m_retryTimer.start();
                }
            });

    refreshActiveWindow();
}

void DdeActiveWindowTracker::refreshActiveWindow()
{
    if (!m_model) {
        setActiveWindowInfo({});
        return;
    }

    const int activeRole = roleForName(m_model, QByteArrayLiteral("active"));
    if (activeRole < 0) {
        setActiveWindowInfo({});
        return;
    }

    const int windowIdRole = roleForName(m_model, QByteArrayLiteral("winId"));
    const int windowsRole = roleForName(m_model, QByteArrayLiteral("windows"));
    const int desktopIdRole = roleForName(m_model, QByteArrayLiteral("desktopId"));
    const int identityRole = roleForName(m_model, QByteArrayLiteral("identity"));
    const int titleRole = roleForName(m_model, QByteArrayLiteral("title"));
    const int nameRole = roleForName(m_model, QByteArrayLiteral("name"));

    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QModelIndex index = m_model->index(row, 0);
        if (!index.isValid() || !m_model->data(index, activeRole).toBool()) {
            continue;
        }

        dgm::ActiveWindowInfo info;
        info.backend = QStringLiteral("treeland");

        if (windowIdRole >= 0) {
            info.nativeId = m_model->data(index, windowIdRole).toUInt();
        }
        if (info.nativeId == 0 && windowsRole >= 0) {
            const QStringList windows = m_model->data(index, windowsRole).toStringList();
            if (!windows.isEmpty()) {
                info.nativeId = windows.constFirst().toUInt();
            }
        }

        if (desktopIdRole >= 0) {
            info.appId = m_model->data(index, desktopIdRole).toString();
        }
        if (nameRole >= 0) {
            info.appName = m_model->data(index, nameRole).toString();
        }
        if (titleRole >= 0) {
            info.title = m_model->data(index, titleRole).toString();
        }
        if (identityRole >= 0) {
            const QStringList identity = m_model->data(index, identityRole).toStringList();
            for (auto it = identity.crbegin(); it != identity.crend(); ++it) {
                bool ok = false;
                const qint64 pid = it->toLongLong(&ok);
                if (ok && pid > 0) {
                    info.pid = pid;
                    break;
                }
            }
            if (info.appId.isEmpty() && !identity.isEmpty()) {
                info.appId = identity.constFirst();
            }
        }

        setActiveWindowInfo(info);
        return;
    }

    setActiveWindowInfo({});
}

int DdeActiveWindowTracker::roleForName(const QAbstractItemModel *model,
                                        const QByteArray &name)
{
    if (!model) {
        return -1;
    }

    const auto roles = model->roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        if (it.value() == name) {
            return it.key();
        }
    }
    return -1;
}
