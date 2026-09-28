// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "activewindowtracker.h"

#include <appletbridge.h>

#include <QPointer>
#include <QTimer>

class QAbstractItemModel;

class DdeActiveWindowTracker final : public dgm::ActiveWindowTracker
{
    Q_OBJECT

public:
    explicit DdeActiveWindowTracker(QObject *parent = nullptr);
    ~DdeActiveWindowTracker() override;

    bool start() override;
    void stop() override;
    [[nodiscard]] bool isRunning() const override;

private:
    void refreshBinding();
    void bindModel(QAbstractItemModel *model);
    void refreshActiveWindow();
    static int roleForName(const QAbstractItemModel *model, const QByteArray &name);

    DS_NAMESPACE::DAppletBridge *m_bridge = nullptr;
    QPointer<QAbstractItemModel> m_model;
    QTimer m_retryTimer;
    bool m_running = false;
};
