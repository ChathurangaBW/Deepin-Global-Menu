// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddefallbackactions.h"

#include <QAbstractItemModel>
#include <QDebug>
#include <QMetaObject>
#include <QModelIndexList>
#include <QVariant>

DdeFallbackActions::DdeFallbackActions(QObject *parent)
    : QObject(parent)
    , m_bridge(new DS_NAMESPACE::DAppletBridge(
          QStringLiteral("org.deepin.ds.dock.taskmanager"), this))
{
}

void DdeFallbackActions::execute(const QString &action)
{
    auto *model = dataModel();
    const QModelIndex index = activeIndex(model);
    auto *applet = m_bridge ? m_bridge->applet() : nullptr;

    if (!model || !index.isValid() || !applet) {
        qWarning() << "Deepin Global Menu: fallback action unavailable:" << action;
        return;
    }

    bool invoked = false;

    if (action == QStringLiteral("new-instance")) {
        invoked = QMetaObject::invokeMethod(applet,
                                            "requestNewInstance",
                                            Q_ARG(QModelIndex, index),
                                            Q_ARG(QString, QString()));
    } else if (action == QStringLiteral("show-windows")) {
        const QModelIndexList indexes{index};
        invoked = QMetaObject::invokeMethod(applet,
                                            "requestWindowsView",
                                            Q_ARG(QModelIndexList, indexes));
    } else if (action == QStringLiteral("activate-toggle")) {
        invoked = QMetaObject::invokeMethod(applet,
                                            "requestActivate",
                                            Q_ARG(QModelIndex, index));
    } else if (action == QStringLiteral("quit")) {
        invoked = QMetaObject::invokeMethod(applet,
                                            "requestClose",
                                            Q_ARG(QModelIndex, index),
                                            Q_ARG(bool, false));
    } else if (action == QStringLiteral("force-quit")) {
        invoked = QMetaObject::invokeMethod(applet,
                                            "requestClose",
                                            Q_ARG(QModelIndex, index),
                                            Q_ARG(bool, true));
    }

    if (!invoked) {
        qWarning() << "Deepin Global Menu: failed to invoke fallback action:" << action;
    }
}

QAbstractItemModel *DdeFallbackActions::dataModel() const
{
    if (!m_bridge || !m_bridge->isValid()) {
        return nullptr;
    }

    auto *applet = m_bridge->applet();
    if (!applet) {
        return nullptr;
    }

    return applet->property("dataModel").value<QAbstractItemModel *>();
}

QModelIndex DdeFallbackActions::activeIndex(QAbstractItemModel *model) const
{
    if (!model) {
        return {};
    }

    const int activeRole = roleForName(model, QByteArrayLiteral("active"));
    if (activeRole < 0) {
        return {};
    }

    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, 0);
        if (index.isValid() && model->data(index, activeRole).toBool()) {
            return index;
        }
    }

    return {};
}

int DdeFallbackActions::roleForName(const QAbstractItemModel *model,
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
