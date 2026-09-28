// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "gtkmenutypes.h"

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>

namespace dgm {

struct GtkMenuContext
{
    QString busName;
    QString applicationId;
    QString applicationName;
    QString appActionPath;
    QString windowActionPath;
    QString appMenuPath;
    QString menubarPath;

    [[nodiscard]] bool isValid() const
    {
        return !busName.isEmpty();
    }

    friend bool operator==(const GtkMenuContext &lhs, const GtkMenuContext &rhs)
    {
        return lhs.busName == rhs.busName
            && lhs.applicationId == rhs.applicationId
            && lhs.applicationName == rhs.applicationName
            && lhs.appActionPath == rhs.appActionPath
            && lhs.windowActionPath == rhs.windowActionPath
            && lhs.appMenuPath == rhs.appMenuPath
            && lhs.menubarPath == rhs.menubarPath;
    }

    friend bool operator!=(const GtkMenuContext &lhs, const GtkMenuContext &rhs)
    {
        return !(lhs == rhs);
    }
};

class GtkMenuImporter final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY layoutChanged)
    Q_PROPERTY(uint revision READ revision NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList topLevelItems READ topLevelItems NOTIFY layoutChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    explicit GtkMenuImporter(QObject *parent = nullptr);
    ~GtkMenuImporter() override;

    void setContext(const GtkMenuContext &context);
    [[nodiscard]] const GtkMenuContext &context() const;
    [[nodiscard]] bool isReady() const;
    [[nodiscard]] uint revision() const;
    [[nodiscard]] QVariantList topLevelItems() const;
    [[nodiscard]] QString errorString() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void triggerAction(int itemId);
    Q_INVOKABLE void prepareSubmenu(int itemId);

signals:
    void layoutChanged();
    void errorStringChanged();
    void refreshFinished(bool success);

private slots:
    void onGtkMenusChanged(const dgm::GtkMenuChangeList &changes);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    void onGtkActionsChanged(const QStringList &removed,
                             const dgm::GtkActionEnabledMap &enabledChanged,
                             const QVariantMap &stateChanged,
                             const dgm::GtkActionDescriptionMap &added);
#endif

private:
    struct ActionTarget
    {
        QString fullName;
        QString actionName;
        QString objectPath;
        QVariant target;
    };

    static GtkMenuContext normalizedContext(GtkMenuContext context);
    static QString baseObjectPathForAppId(const QString &applicationId);
    static QVariant unwrappedVariant(const QVariant &value);
    static bool parseMenuLink(const QVariant &value, GtkMenuLink &link);
    static quint64 sectionKey(uint groupId, uint menuId);
    static QString displayLabel(const QVariantMap &item);
    static QString humanizedActionName(const QString &name);
    static QString actionCategory(const QString &name);

    QString actionObjectPath(const QString &fullAction) const;
    QString bareActionName(const QString &fullAction) const;
    const GtkActionDescription *actionDescription(const QString &fullAction) const;
    QVariant actionState(const GtkActionDescription *description) const;

    QVariantList buildMenuPath(const QString &path);
    QVariantList buildSection(const QString &path,
                              uint groupId,
                              uint menuId,
                              QSet<quint64> &stack);
    QVariantMap buildMenuItem(const QString &path,
                              const QVariantMap &item,
                              QSet<quint64> &stack);
    QVariantList buildActionFallback();
    QVariantMap buildActionItem(const QString &fullAction,
                                const QString &objectPath,
                                const GtkActionDescription &description);

    int registerAction(const QString &fullAction, const QVariant &target);
    void startMenuRequest(const QString &path, quint64 serial);
    void startActionRequest(const QString &path, quint64 serial);
    void requestFinished(quint64 serial);
    void finishRefresh(quint64 serial);
    void connectRemoteSignals();
    void disconnectRemoteSignals();
    void unsubscribeMenus();
    void clear();
    void setErrorString(const QString &errorString);

    GtkMenuContext m_context;
    QHash<QString, GtkMenuSectionList> m_menuSections;
    QHash<QString, GtkActionDescriptionMap> m_actionDescriptions;
    QHash<int, ActionTarget> m_actions;
    QSet<QString> m_startedMenuPaths;
    QVariantList m_items;
    QString m_errorString;
    uint m_revision = 0;
    int m_nextActionId = 1;
    int m_pendingRequests = 0;
    bool m_hadSuccessfulRequest = false;
    bool m_ready = false;
    quint64 m_generation = 0;
    quint64 m_refreshSerial = 0;
};

} // namespace dgm
