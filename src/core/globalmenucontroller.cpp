// SPDX-License-Identifier: GPL-3.0-or-later

#include "globalmenucontroller.h"

#include "dbusmenuimporter.h"
#include "gtkmenuimporter.h"

namespace dgm {

GlobalMenuController::GlobalMenuController(MenuRegistry *registry, QObject *parent)
    : QObject(parent)
    , m_registry(registry)
    , m_importer(new DbusMenuImporter(this))
    , m_gtkImporter(new GtkMenuImporter(this))
{
    Q_ASSERT(m_registry);

    connect(m_registry, &MenuRegistry::windowRegistered, this,
            [this](quint32 windowId) {
                if (windowId == m_activeWindowInfo.x11Id) {
                    refreshSource();
                }
            });

    connect(m_registry, &MenuRegistry::windowUnregistered, this,
            [this](quint32 windowId) {
                if (windowId == m_activeWindowInfo.x11Id) {
                    refreshSource();
                }
            });

    connect(m_importer, &DbusMenuImporter::layoutChanged,
            this, [this] {
                if (m_source == Source::DbusMenu) {
                    emit menuItemsChanged();
                }
            });
    connect(m_importer, &DbusMenuImporter::errorStringChanged,
            this, [this] {
                if (m_source == Source::DbusMenu) {
                    emit menuStatusChanged();
                }
            });
    connect(m_importer, &DbusMenuImporter::refreshFinished,
            this, [this](bool success) {
                if (m_source != Source::DbusMenu) {
                    return;
                }
                if (!success) {
                    tryGtkFallback();
                }
                emit menuStatusChanged();
            });

    connect(m_gtkImporter, &GtkMenuImporter::layoutChanged,
            this, [this] {
                if (m_source == Source::Gtk) {
                    emit menuItemsChanged();
                }
            });
    connect(m_gtkImporter, &GtkMenuImporter::errorStringChanged,
            this, [this] {
                if (m_source == Source::Gtk) {
                    emit menuStatusChanged();
                }
            });
    connect(m_gtkImporter, &GtkMenuImporter::refreshFinished,
            this, [this](bool success) {
                // A failed DBusMenu endpoint may still remain registered. Once
                // the controller has left the DBusMenu source, a verified GTK
                // export is allowed to replace the safe fallback even if that
                // stale registrar endpoint still exists.
                if (success && m_source != Source::DbusMenu) {
                    selectSource(Source::Gtk);
                } else if (!success && m_source == Source::Gtk) {
                    selectFallback();
                }
                emit menuStatusChanged();
            });
}

quint32 GlobalMenuController::activeWindowId() const
{
    return m_activeWindowInfo.x11Id != 0
        ? m_activeWindowInfo.x11Id
        : m_activeWindowInfo.nativeId;
}

QString GlobalMenuController::activeApplicationId() const
{
    return m_activeWindowInfo.appId;
}

void GlobalMenuController::setActiveWindowId(quint32 windowId)
{
    ActiveWindowInfo info = m_activeWindowInfo;
    info.nativeId = windowId;
    info.x11Id = windowId;
    if (windowId == 0) {
        info = {};
    }
    setActiveWindowInfo(info);
}

void GlobalMenuController::setActiveWindowTracker(ActiveWindowTracker *tracker)
{
    if (m_windowTracker == tracker) {
        return;
    }

    if (m_windowTracker) {
        disconnect(m_windowTracker, nullptr, this, nullptr);
    }

    m_windowTracker = tracker;
    if (!m_windowTracker) {
        setActiveWindowInfo({});
        return;
    }

    connect(m_windowTracker, &ActiveWindowTracker::activeWindowInfoChanged,
            this, &GlobalMenuController::setActiveWindowInfo);
    connect(m_windowTracker, &QObject::destroyed,
            this, [this, tracker] {
                if (m_windowTracker == tracker) {
                    m_windowTracker = nullptr;
                    setActiveWindowInfo({});
                }
            });

    setActiveWindowInfo(m_windowTracker->activeWindowInfo());
}

void GlobalMenuController::setShortcutActionsAvailable(bool available)
{
    if (m_shortcutActionsAvailable == available) {
        return;
    }

    m_shortcutActionsAvailable = available;
    if (m_source == Source::Fallback) {
        rebuildFallbackMenu();
    }
}

QString GlobalMenuController::menuSource() const
{
    switch (m_source) {
    case Source::DbusMenu:
        return QStringLiteral("dbusmenu");
    case Source::Gtk:
        return QStringLiteral("gtk");
    case Source::Fallback:
        return QStringLiteral("fallback");
    case Source::None:
        break;
    }
    return QStringLiteral("none");
}

bool GlobalMenuController::hasMenu() const
{
    switch (m_source) {
    case Source::DbusMenu:
        return m_endpoint.isValid();
    case Source::Gtk:
        return m_gtkImporter->context().isValid();
    case Source::Fallback:
        return m_activeWindowInfo.isValid();
    case Source::None:
        break;
    }
    return false;
}

QString GlobalMenuController::menuService() const
{
    if (m_source == Source::DbusMenu) {
        return m_endpoint.service;
    }
    if (m_source == Source::Gtk) {
        return m_gtkImporter->context().busName;
    }
    return {};
}

QString GlobalMenuController::menuObjectPath() const
{
    if (m_source == Source::DbusMenu) {
        return m_endpoint.objectPath.path();
    }
    if (m_source == Source::Gtk) {
        const auto &context = m_gtkImporter->context();
        return !context.menubarPath.isEmpty()
            ? context.menubarPath
            : context.appActionPath;
    }
    return {};
}

bool GlobalMenuController::menuReady() const
{
    switch (m_source) {
    case Source::DbusMenu:
        return m_importer->isReady();
    case Source::Gtk:
        return m_gtkImporter->isReady();
    case Source::Fallback:
        return !m_fallbackItems.isEmpty();
    case Source::None:
        break;
    }
    return false;
}

uint GlobalMenuController::menuRevision() const
{
    switch (m_source) {
    case Source::DbusMenu:
        return m_importer->revision();
    case Source::Gtk:
        return m_gtkImporter->revision();
    case Source::Fallback:
        return m_fallbackRevision;
    case Source::None:
        break;
    }
    return 0;
}

QVariantList GlobalMenuController::menuItems() const
{
    switch (m_source) {
    case Source::DbusMenu:
        return m_importer->topLevelItems();
    case Source::Gtk:
        return m_gtkImporter->topLevelItems();
    case Source::Fallback:
        return m_fallbackItems;
    case Source::None:
        break;
    }
    return {};
}

QString GlobalMenuController::menuError() const
{
    switch (m_source) {
    case Source::DbusMenu:
        return m_importer->errorString();
    case Source::Gtk:
        return m_gtkImporter->errorString();
    case Source::Fallback:
        return {};
    case Source::None:
        break;
    }
    return {};
}

void GlobalMenuController::refreshMenu()
{
    switch (m_source) {
    case Source::DbusMenu:
        m_importer->refresh();
        break;
    case Source::Gtk:
        m_gtkImporter->refresh();
        break;
    case Source::Fallback:
        rebuildFallbackMenu();
        break;
    case Source::None:
        break;
    }
}

void GlobalMenuController::triggerMenuAction(int itemId, uint timestamp)
{
    switch (m_source) {
    case Source::DbusMenu:
        m_importer->triggerAction(itemId, timestamp);
        break;
    case Source::Gtk:
        m_gtkImporter->triggerAction(itemId);
        break;
    case Source::Fallback: {
        const auto action = m_fallbackActions.value(itemId);
        if (action.startsWith(QStringLiteral("shortcut:"))) {
            emit shortcutRequested(action.mid(9));
        } else if (!action.isEmpty()) {
            emit fallbackActionRequested(action);
        }
        break;
    }
    case Source::None:
        break;
    }
}

void GlobalMenuController::prepareSubmenu(int itemId)
{
    switch (m_source) {
    case Source::DbusMenu:
        m_importer->prepareSubmenu(itemId);
        break;
    case Source::Gtk:
        m_gtkImporter->prepareSubmenu(itemId);
        break;
    case Source::Fallback:
        Q_UNUSED(itemId);
        break;
    case Source::None:
        break;
    }
}

void GlobalMenuController::setActiveWindowInfo(const ActiveWindowInfo &info)
{
    if (m_activeWindowInfo == info) {
        return;
    }

    const quint32 previousId = activeWindowId();
    const QString previousAppId = m_activeWindowInfo.appId;
    m_activeWindowInfo = info;

    if (previousId != activeWindowId() || previousAppId != m_activeWindowInfo.appId) {
        emit activeWindowIdChanged();
    }

    refreshSource();
}

void GlobalMenuController::refreshSource()
{
    const auto nextEndpoint = m_activeWindowInfo.x11Id != 0
        ? m_registry->menuForWindow(m_activeWindowInfo.x11Id)
        : MenuEndpoint{};

    const bool endpointChanged =
        m_endpoint.service != nextEndpoint.service
        || m_endpoint.objectPath.path() != nextEndpoint.objectPath.path();
    m_endpoint = nextEndpoint;

    if (m_endpoint.isValid()) {
        m_gtkImporter->setContext({});
        m_importer->setEndpoint(m_endpoint);
        selectSource(Source::DbusMenu);
        m_importer->refresh();
    } else {
        m_importer->setEndpoint({});
        const auto gtkContext = gtkContextForActiveWindow();
        m_gtkImporter->setContext(gtkContext);
        selectFallback();
        if (m_gtkImporter->context().isValid()) {
            m_gtkImporter->refresh();
        }
    }

    if (endpointChanged) {
        emit menuEndpointChanged();
    }
    emit menuStatusChanged();
}

void GlobalMenuController::selectSource(Source source)
{
    if (m_source == source) {
        return;
    }

    m_source = source;
    emit menuSourceChanged();
    emit menuEndpointChanged();
    emit menuItemsChanged();
    emit menuStatusChanged();
}

GtkMenuContext GlobalMenuController::gtkContextForActiveWindow() const
{
    GtkMenuContext context;
    context.busName = m_activeWindowInfo.gtkUniqueBusName;
    context.applicationId = m_activeWindowInfo.appId;
    context.applicationName = !m_activeWindowInfo.appName.isEmpty()
        ? m_activeWindowInfo.appName
        : m_activeWindowInfo.appId;
    context.appActionPath = m_activeWindowInfo.gtkApplicationObjectPath;
    context.windowActionPath = m_activeWindowInfo.gtkWindowObjectPath;
    context.appMenuPath = m_activeWindowInfo.gtkAppMenuObjectPath;
    context.menubarPath = m_activeWindowInfo.gtkMenubarObjectPath;
    return context;
}

void GlobalMenuController::tryGtkFallback()
{
    const auto context = gtkContextForActiveWindow();
    if (!context.isValid() && context.applicationId.isEmpty()) {
        selectFallback();
        return;
    }

    m_gtkImporter->setContext(context);
    if (!m_gtkImporter->context().isValid()) {
        selectFallback();
        return;
    }

    selectFallback();
    m_gtkImporter->refresh();
}

void GlobalMenuController::selectFallback()
{
    if (!m_activeWindowInfo.isValid()) {
        m_fallbackItems.clear();
        m_fallbackActions.clear();
        selectSource(Source::None);
        return;
    }

    rebuildFallbackMenu();
    selectSource(Source::Fallback);
}

void GlobalMenuController::rebuildFallbackMenu()
{
    m_fallbackActions.clear();

    int nextId = 900000;
    auto actionItem = [this, &nextId](const QString &label, const QString &action) {
        QVariantMap item;
        const int id = ++nextId;
        item.insert(QStringLiteral("id"), id);
        item.insert(QStringLiteral("label"), label);
        item.insert(QStringLiteral("enabled"), true);
        item.insert(QStringLiteral("visible"), true);
        item.insert(QStringLiteral("separator"), false);
        item.insert(QStringLiteral("children"), QVariantList{});
        m_fallbackActions.insert(id, action);
        return item;
    };

    auto separator = [&nextId] {
        QVariantMap item;
        item.insert(QStringLiteral("id"), ++nextId);
        item.insert(QStringLiteral("separator"), true);
        item.insert(QStringLiteral("enabled"), false);
        item.insert(QStringLiteral("visible"), true);
        item.insert(QStringLiteral("children"), QVariantList{});
        return item;
    };

    QString applicationLabel = m_activeWindowInfo.appName;
    if (applicationLabel.isEmpty()) {
        applicationLabel = m_activeWindowInfo.appId;
    }
    if (applicationLabel.endsWith(QStringLiteral(".desktop"), Qt::CaseInsensitive)) {
        applicationLabel.chop(8);
    }
    if (applicationLabel.isEmpty()) {
        applicationLabel = QStringLiteral("Application");
    }

    QVariantList applicationChildren;
    applicationChildren.append(actionItem(QStringLiteral("New Window"),
                                          QStringLiteral("new-instance")));
    applicationChildren.append(actionItem(QStringLiteral("Show All Windows"),
                                          QStringLiteral("show-windows")));
    applicationChildren.append(separator());
    applicationChildren.append(actionItem(QStringLiteral("Quit"),
                                          QStringLiteral("quit")));
    applicationChildren.append(actionItem(QStringLiteral("Force Quit"),
                                          QStringLiteral("force-quit")));

    QVariantMap applicationMenu;
    applicationMenu.insert(QStringLiteral("id"), ++nextId);
    applicationMenu.insert(QStringLiteral("label"), applicationLabel);
    applicationMenu.insert(QStringLiteral("enabled"), true);
    applicationMenu.insert(QStringLiteral("visible"), true);
    applicationMenu.insert(QStringLiteral("separator"), false);
    applicationMenu.insert(QStringLiteral("children-display"), QStringLiteral("submenu"));
    applicationMenu.insert(QStringLiteral("children"), applicationChildren);

    QVariantList windowChildren;
    windowChildren.append(actionItem(QStringLiteral("Minimize / Next Window"),
                                     QStringLiteral("activate-toggle")));
    windowChildren.append(actionItem(QStringLiteral("Show All Windows"),
                                     QStringLiteral("show-windows")));
    windowChildren.append(separator());
    windowChildren.append(actionItem(QStringLiteral("Close All Windows"),
                                     QStringLiteral("quit")));

    QVariantMap windowMenu;
    windowMenu.insert(QStringLiteral("id"), ++nextId);
    windowMenu.insert(QStringLiteral("label"), QStringLiteral("Window"));
    windowMenu.insert(QStringLiteral("enabled"), true);
    windowMenu.insert(QStringLiteral("visible"), true);
    windowMenu.insert(QStringLiteral("separator"), false);
    windowMenu.insert(QStringLiteral("children-display"), QStringLiteral("submenu"));
    windowMenu.insert(QStringLiteral("children"), windowChildren);

    QVariantList nextItems;
    nextItems.append(applicationMenu);

    auto shortcutItem = [&actionItem](const QString &label, const QString &shortcut) {
        QVariantMap item = actionItem(label, QStringLiteral("shortcut:") + shortcut);
        item.insert(QStringLiteral("shortcut"), shortcut);
        return item;
    };

    auto makeMenu = [&nextId](const QString &label, const QVariantList &children) {
        QVariantMap menu;
        menu.insert(QStringLiteral("id"), ++nextId);
        menu.insert(QStringLiteral("label"), label);
        menu.insert(QStringLiteral("enabled"), true);
        menu.insert(QStringLiteral("visible"), true);
        menu.insert(QStringLiteral("separator"), false);
        menu.insert(QStringLiteral("children-display"), QStringLiteral("submenu"));
        menu.insert(QStringLiteral("children"), children);
        return menu;
    };

    if (m_shortcutActionsAvailable) {
        QVariantList fileChildren;
        fileChildren.append(shortcutItem(QStringLiteral("New"), QStringLiteral("Ctrl+N")));
        fileChildren.append(shortcutItem(QStringLiteral("Open…"), QStringLiteral("Ctrl+O")));
        fileChildren.append(shortcutItem(QStringLiteral("Save"), QStringLiteral("Ctrl+S")));
        fileChildren.append(shortcutItem(QStringLiteral("Print…"), QStringLiteral("Ctrl+P")));
        nextItems.append(makeMenu(QStringLiteral("File"), fileChildren));

        QVariantList editChildren;
        editChildren.append(shortcutItem(QStringLiteral("Undo"), QStringLiteral("Ctrl+Z")));
        editChildren.append(shortcutItem(QStringLiteral("Redo"), QStringLiteral("Ctrl+Y")));
        editChildren.append(separator());
        editChildren.append(shortcutItem(QStringLiteral("Cut"), QStringLiteral("Ctrl+X")));
        editChildren.append(shortcutItem(QStringLiteral("Copy"), QStringLiteral("Ctrl+C")));
        editChildren.append(shortcutItem(QStringLiteral("Paste"), QStringLiteral("Ctrl+V")));
        editChildren.append(shortcutItem(QStringLiteral("Select All"), QStringLiteral("Ctrl+A")));
        editChildren.append(separator());
        editChildren.append(shortcutItem(QStringLiteral("Find…"), QStringLiteral("Ctrl+F")));
        nextItems.append(makeMenu(QStringLiteral("Edit"), editChildren));

        QVariantList viewChildren;
        viewChildren.append(shortcutItem(QStringLiteral("Full Screen"), QStringLiteral("F11")));
        viewChildren.append(separator());
        viewChildren.append(shortcutItem(QStringLiteral("Zoom In"), QStringLiteral("Ctrl+=")));
        viewChildren.append(shortcutItem(QStringLiteral("Zoom Out"), QStringLiteral("Ctrl+-")));
        viewChildren.append(shortcutItem(QStringLiteral("Reset Zoom"), QStringLiteral("Ctrl+0")));
        nextItems.append(makeMenu(QStringLiteral("View"), viewChildren));
    }

    nextItems.append(windowMenu);

    if (m_shortcutActionsAvailable) {
        QVariantList helpChildren;
        helpChildren.append(shortcutItem(QStringLiteral("Help"), QStringLiteral("F1")));
        nextItems.append(makeMenu(QStringLiteral("Help"), helpChildren));
    }

    if (m_fallbackItems == nextItems) {
        return;
    }

    m_fallbackItems = nextItems;
    ++m_fallbackRevision;
    if (m_source == Source::Fallback) {
        emit menuItemsChanged();
    }
}

} // namespace dgm
