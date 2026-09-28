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
            this, [this](bool) {
                if (m_source == Source::Gtk) {
                    emit menuStatusChanged();
                }
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

QString GlobalMenuController::menuSource() const
{
    switch (m_source) {
    case Source::DbusMenu:
        return QStringLiteral("dbusmenu");
    case Source::Gtk:
        return QStringLiteral("gtk");
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
        if (m_gtkImporter->context().isValid()) {
            selectSource(Source::Gtk);
            m_gtkImporter->refresh();
        } else {
            selectSource(Source::None);
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
        return;
    }

    m_gtkImporter->setContext(context);
    if (!m_gtkImporter->context().isValid()) {
        return;
    }

    selectSource(Source::Gtk);
    m_gtkImporter->refresh();
}

} // namespace dgm
