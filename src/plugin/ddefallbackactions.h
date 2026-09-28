// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <appletbridge.h>

#include <QObject>
#include <QModelIndex>

class QAbstractItemModel;

class DdeFallbackActions final : public QObject
{
    Q_OBJECT

public:
    explicit DdeFallbackActions(QObject *parent = nullptr);

public slots:
    void execute(const QString &action);

private:
    QAbstractItemModel *dataModel() const;
    QModelIndex activeIndex(QAbstractItemModel *model) const;
    static int roleForName(const QAbstractItemModel *model, const QByteArray &name);

    DS_NAMESPACE::DAppletBridge *m_bridge = nullptr;
};
