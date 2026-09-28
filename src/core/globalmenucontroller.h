// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "activewindowtracker.h"
#include "gtkmenutypes.h"
#include "menuregistry.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariantList>

namespace dgm {

class DbusMenuImporter;
class GtkMenuImporter;
struct GtkMenuContext;

class GlobalMenuController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 activeWindowId READ activeWindowId WRITE setActiveWindowId NOTIFY activeWindowIdChanged)
    Q_PROPERTY(QString activeApplicationId READ activeApplicationId NOTIFY activeWindowIdChanged)
    Q_PROPERTY(QString menuSource READ menuSource NOTIFY menuSourceChanged)
    Q_PROPERTY(bool hasMenu READ hasMenu NOTIFY menuEndpointChanged)
    Q_PROPERTY(QString menuService READ menuService NOTIFY menuEndpointChanged)
    Q_PROPERTY(QString menuObjectPath READ menuObjectPath NOTIFY menuEndpointChanged)
    Q_PROPERTY(bool menuReady READ menuReady NOTIFY menuItemsChanged)
    Q_PROPERTY(uint menuRevision READ menuRevision NOTIFY menuItemsChanged)
    Q_PROPERTY(QVariantList menuItems READ menuItems NOTIFY menuItemsChanged)
    Q_PROPERTY(QString menuError READ menuError NOTIFY menuStatusChanged)

public:
    explicit GlobalMenuController(MenuRegistry *registry, QObject *parent = nullptr);

    [[nodiscard]] quint32 activeWindowId() const;
    [[nodiscard]] QString activeApplicationId() const;
    void setActiveWindowId(quint32 windowId);
    void setActiveWindowTracker(ActiveWindowTracker *tracker);

    [[nodiscard]] QString menuSource() const;
    [[nodiscard]] bool hasMenu() const;
    [[nodiscard]] QString menuService() const;
    [[nodiscard]] QString menuObjectPath() const;

    [[nodiscard]] bool menuReady() const;
    [[nodiscard]] uint menuRevision() const;
    [[nodiscard]] QVariantList menuItems() const;
    [[nodiscard]] QString menuError() const;

    Q_INVOKABLE void refreshMenu();
    Q_INVOKABLE void triggerMenuAction(int itemId, uint timestamp = 0);
    Q_INVOKABLE void prepareSubmenu(int itemId);

signals:
    void activeWindowIdChanged();
    void menuSourceChanged();
    void menuEndpointChanged();
    void menuItemsChanged();
    void menuStatusChanged();
    void fallbackActionRequested(const QString &action);

private:
    enum class Source {
        None,
        DbusMenu,
        Gtk,
        Fallback
    };

    void setActiveWindowInfo(const ActiveWindowInfo &info);
    void refreshSource();
    void selectSource(Source source);
    GtkMenuContext gtkContextForActiveWindow() const;
    void tryGtkFallback();
    void selectFallback();
    void rebuildFallbackMenu();

    MenuRegistry *m_registry = nullptr;
    DbusMenuImporter *m_importer = nullptr;
    GtkMenuImporter *m_gtkImporter = nullptr;
    ActiveWindowTracker *m_windowTracker = nullptr;
    ActiveWindowInfo m_activeWindowInfo;
    MenuEndpoint m_endpoint;
    QVariantList m_fallbackItems;
    QHash<int, QString> m_fallbackActions;
    uint m_fallbackRevision = 0;
    Source m_source = Source::None;
};

} // namespace dgm
