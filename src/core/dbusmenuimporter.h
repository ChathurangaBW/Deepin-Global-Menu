// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "dbusmenutypes.h"
#include "menuregistry.h"

#include <QObject>
#include <QString>
#include <QVariantList>

class QDBusInterface;

namespace dgm {

class DbusMenuImporter final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY layoutChanged)
    Q_PROPERTY(uint revision READ revision NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList topLevelItems READ topLevelItems NOTIFY layoutChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    explicit DbusMenuImporter(QObject *parent = nullptr);
    ~DbusMenuImporter() override;

    void setEndpoint(const MenuEndpoint &endpoint);
    [[nodiscard]] MenuEndpoint endpoint() const;
    [[nodiscard]] bool isReady() const;
    [[nodiscard]] uint revision() const;
    [[nodiscard]] const DbusMenuLayoutItem &rootItem() const;
    [[nodiscard]] QVariantList topLevelItems() const;
    [[nodiscard]] QString errorString() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void triggerAction(int itemId, uint timestamp = 0);
    Q_INVOKABLE void prepareSubmenu(int itemId);

signals:
    void layoutChanged();
    void errorStringChanged();
    void refreshFinished(bool success);

private slots:
    void onLayoutUpdated(uint revision, int parentId);
    void onItemsPropertiesUpdated(dgm::DbusMenuItemList updated,
                                  dgm::DbusMenuItemKeysList removed);

private:
    void connectRemoteSignals();
    void disconnectRemoteSignals();
    void clearLayout();
    void setErrorString(const QString &errorString);
    static DbusMenuLayoutItem *findItem(DbusMenuLayoutItem &root, int id);

    MenuEndpoint m_endpoint;
    QDBusInterface *m_interface = nullptr;
    DbusMenuLayoutItem m_root;
    uint m_revision = 0;
    bool m_ready = false;
    QString m_errorString;
    quint64 m_generation = 0;
    quint64 m_refreshSerial = 0;
};

} // namespace dgm
