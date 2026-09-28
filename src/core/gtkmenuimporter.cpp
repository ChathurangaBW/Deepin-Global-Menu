// SPDX-License-Identifier: GPL-3.0-or-later

#include "gtkmenuimporter.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QSet>
#include <QStringList>

#include <initializer_list>
#include <utility>

namespace dgm {

namespace {

constexpr auto kGtkMenusInterface = "org.gtk.Menus";
constexpr auto kGtkActionsInterface = "org.gtk.Actions";

QString strippedDesktopSuffix(QString appId)
{
    if (appId.endsWith(QStringLiteral(".desktop"), Qt::CaseInsensitive)) {
        appId.chop(8);
    }
    return appId;
}

bool startsWithAny(const QString &value, std::initializer_list<const char *> prefixes)
{
    for (const auto *prefix : prefixes) {
        if (value.startsWith(QString::fromLatin1(prefix))) {
            return true;
        }
    }
    return false;
}

} // namespace

GtkMenuImporter::GtkMenuImporter(QObject *parent)
    : QObject(parent)
{
    registerGtkMenuMetaTypes();
}

GtkMenuImporter::~GtkMenuImporter()
{
    disconnectRemoteSignals();
    unsubscribeMenus();
}

void GtkMenuImporter::setContext(const GtkMenuContext &context)
{
    const auto normalized = normalizedContext(context);
    if (m_context == normalized) {
        return;
    }

    disconnectRemoteSignals();
    unsubscribeMenus();
    ++m_generation;
    ++m_refreshSerial;
    m_context = normalized;
    clear();
    setErrorString({});
    connectRemoteSignals();
}

const GtkMenuContext &GtkMenuImporter::context() const
{
    return m_context;
}

bool GtkMenuImporter::isReady() const
{
    return m_ready;
}

uint GtkMenuImporter::revision() const
{
    return m_revision;
}

QVariantList GtkMenuImporter::topLevelItems() const
{
    return m_items;
}

QString GtkMenuImporter::errorString() const
{
    return m_errorString;
}

void GtkMenuImporter::refresh()
{
    if (!m_context.isValid()) {
        setErrorString(QStringLiteral("No GTK application D-Bus identity is available"));
        emit refreshFinished(false);
        return;
    }

    const quint64 serial = ++m_refreshSerial;
    unsubscribeMenus();
    m_menuSections.clear();
    m_actionDescriptions.clear();
    m_pendingRequests = 0;
    m_hadSuccessfulRequest = false;
    setErrorString({});

    QSet<QString> menuPaths;
    if (!m_context.appMenuPath.isEmpty()) {
        menuPaths.insert(m_context.appMenuPath);
    }
    if (!m_context.menubarPath.isEmpty()) {
        menuPaths.insert(m_context.menubarPath);
    }

    QSet<QString> actionPaths;
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // Qt before 6.8 incorrectly rejects the empty D-Bus signature used by
    // zero-parameter GAction descriptions. Menu hierarchy and activation
    // still work there; only DescribeAll-backed state/action fallback is
    // disabled.
    if (!m_context.appActionPath.isEmpty()) {
        actionPaths.insert(m_context.appActionPath);
    }
    if (!m_context.windowActionPath.isEmpty()) {
        actionPaths.insert(m_context.windowActionPath);
    }
    if (!m_context.menubarPath.isEmpty()) {
        actionPaths.insert(m_context.menubarPath);
    }
#endif

    for (const auto &path : menuPaths) {
        // GDBusMenuModel roots begin at group 0. Linked menu models may point
        // at any uint group ID; those are discovered recursively from the
        // returned :section/:submenu links rather than guessed.
        startMenuGroupRequest(path, 0, serial);
    }
    for (const auto &path : actionPaths) {
        startActionRequest(path, serial);
    }

    if (m_pendingRequests == 0) {
        setErrorString(QStringLiteral("GTK application exposes no candidate menu or action object paths"));
        emit refreshFinished(false);
    }
}

void GtkMenuImporter::triggerAction(int itemId)
{
    const auto it = m_actions.constFind(itemId);
    if (it == m_actions.cend() || it->objectPath.isEmpty() || it->actionName.isEmpty()) {
        return;
    }

    QDBusInterface actions(m_context.busName,
                           it->objectPath,
                           QString::fromLatin1(kGtkActionsInterface),
                           QDBusConnection::sessionBus());
    if (!actions.isValid()) {
        return;
    }

    DbusVariantList parameters;
    const QVariant target = unwrappedVariant(it->target);
    if (target.isValid()) {
        parameters.append(QDBusVariant(target));
    }

    actions.asyncCall(QStringLiteral("Activate"),
                      it->actionName,
                      QVariant::fromValue(parameters),
                      QVariantMap{});
}

void GtkMenuImporter::prepareSubmenu(int itemId)
{
    Q_UNUSED(itemId);
    // org.gtk.Menus has no AboutToShow equivalent. Refresh on open so
    // dynamic enabled/toggle state remains current without continuous polling.
    refresh();
}

GtkMenuContext GtkMenuImporter::normalizedContext(GtkMenuContext context)
{
    context.applicationId = strippedDesktopSuffix(context.applicationId.trimmed());

    if (context.busName.isEmpty() && context.applicationId.contains(QLatin1Char('.'))) {
        context.busName = context.applicationId;
    }

    QString basePath;
    if (!context.applicationId.isEmpty()) {
        basePath = baseObjectPathForAppId(context.applicationId);
    } else if (!context.busName.isEmpty() && !context.busName.startsWith(QLatin1Char(':'))) {
        basePath = baseObjectPathForAppId(context.busName);
    }

    if (context.appActionPath.isEmpty()) {
        context.appActionPath = basePath;
    }
    if (context.windowActionPath.isEmpty() && !basePath.isEmpty()) {
        context.windowActionPath = basePath + QStringLiteral("/window/1");
    }

    // GtkApplication commonly uses these paths. Exact X11 metadata wins.
    if (context.appMenuPath.isEmpty() && !basePath.isEmpty()) {
        context.appMenuPath = basePath + QStringLiteral("/menus/AppMenu");
    }
    if (context.menubarPath.isEmpty() && !basePath.isEmpty()) {
        context.menubarPath = basePath + QStringLiteral("/menus/MenuBar");
    }

    return context;
}

QString GtkMenuImporter::baseObjectPathForAppId(const QString &applicationId)
{
    QString path = strippedDesktopSuffix(applicationId.trimmed());
    path.replace(QLatin1Char('.'), QLatin1Char('/'));
    if (!path.startsWith(QLatin1Char('/'))) {
        path.prepend(QLatin1Char('/'));
    }
    return path;
}

QVariant GtkMenuImporter::unwrappedVariant(const QVariant &value)
{
    if (value.metaType() == QMetaType::fromType<QDBusVariant>()) {
        return value.value<QDBusVariant>().variant();
    }
    return value;
}

bool GtkMenuImporter::parseMenuLink(const QVariant &value, GtkMenuLink &link)
{
    const QVariant unwrapped = unwrappedVariant(value);

    if (unwrapped.canConvert<GtkMenuLink>()) {
        link = unwrapped.value<GtkMenuLink>();
        return true;
    }

    if (unwrapped.metaType() != QMetaType::fromType<QDBusArgument>()) {
        return false;
    }

    QDBusArgument argument = unwrapped.value<QDBusArgument>();
    argument >> link;
    return true;
}

quint64 GtkMenuImporter::sectionKey(uint groupId, uint menuId)
{
    return (static_cast<quint64>(groupId) << 32U) | static_cast<quint64>(menuId);
}

QString GtkMenuImporter::displayLabel(const QVariantMap &item)
{
    QString label = unwrappedVariant(item.value(QStringLiteral("label"))).toString();
    label.remove(QLatin1Char('_'));
    return label.trimmed();
}

QString GtkMenuImporter::humanizedActionName(const QString &name)
{
    QString cleaned = name;
    const auto dot = cleaned.lastIndexOf(QLatin1Char('.'));
    if (dot >= 0) {
        cleaned = cleaned.mid(dot + 1);
    }
    cleaned.replace(QLatin1Char('_'), QLatin1Char(' '));
    cleaned.replace(QLatin1Char('-'), QLatin1Char(' '));

    const auto words = cleaned.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QStringList titleWords;
    titleWords.reserve(words.size());
    for (QString word : words) {
        if (!word.isEmpty()) {
            word[0] = word.at(0).toUpper();
        }
        titleWords.append(word);
    }
    return titleWords.join(QLatin1Char(' '));
}

QString GtkMenuImporter::actionCategory(const QString &name)
{
    QString n = name.toLower();
    const auto dot = n.lastIndexOf(QLatin1Char('.'));
    if (dot >= 0) {
        n = n.mid(dot + 1);
    }

    if (startsWithAny(n, {"new-", "open", "save", "print", "export", "import",
                          "revert", "share", "close-", "page-", "properties"})) {
        return QStringLiteral("File");
    }
    if (startsWithAny(n, {"undo", "redo", "cut", "copy", "paste", "delete",
                          "select-", "find", "replace", "insert-"})) {
        return QStringLiteral("Edit");
    }
    if (startsWithAny(n, {"zoom-", "fullscreen", "reload", "refresh", "show-",
                          "sort-", "filter", "style-"})) {
        return QStringLiteral("View");
    }
    if (startsWithAny(n, {"bold", "italic", "underline", "font", "align-",
                          "indent", "unindent", "bullet-", "numbered-"})) {
        return QStringLiteral("Format");
    }
    if (startsWithAny(n, {"spell", "word-count", "document-", "comment"})) {
        return QStringLiteral("Tools");
    }
    if (startsWithAny(n, {"help", "about", "shortcut", "keyboard-shortcut"})) {
        return QStringLiteral("Help");
    }
    return QStringLiteral("Application");
}

QString GtkMenuImporter::actionObjectPath(const QString &fullAction) const
{
    if (fullAction.startsWith(QStringLiteral("win."))) {
        return !m_context.windowActionPath.isEmpty()
            ? m_context.windowActionPath
            : m_context.appActionPath;
    }
    if (fullAction.startsWith(QStringLiteral("unity."))) {
        return !m_context.menubarPath.isEmpty()
            ? m_context.menubarPath
            : m_context.appActionPath;
    }
    return m_context.appActionPath;
}

QString GtkMenuImporter::bareActionName(const QString &fullAction) const
{
    const auto dot = fullAction.indexOf(QLatin1Char('.'));
    return dot >= 0 ? fullAction.mid(dot + 1) : fullAction;
}

const GtkActionDescription *GtkMenuImporter::actionDescription(const QString &fullAction) const
{
    const QString path = actionObjectPath(fullAction);
    const QString actionName = bareActionName(fullAction);

    const auto pathIt = m_actionDescriptions.constFind(path);
    if (pathIt == m_actionDescriptions.cend()) {
        return nullptr;
    }

    const auto actionIt = pathIt->constFind(actionName);
    return actionIt != pathIt->cend() ? &actionIt.value() : nullptr;
}

QVariant GtkMenuImporter::actionState(const GtkActionDescription *description) const
{
    if (!description || description->state.isEmpty()) {
        return {};
    }
    return unwrappedVariant(description->state.constFirst().variant());
}

QVariantList GtkMenuImporter::buildMenuPath(const QString &path)
{
    const auto sections = m_menuSections.value(path);
    if (sections.isEmpty()) {
        return {};
    }

    uint groupId = 0;
    uint menuId = 0;
    bool hasRoot = false;
    for (const auto &section : sections) {
        if (section.groupId == 0 && section.menuId == 0) {
            hasRoot = true;
            break;
        }
    }
    if (!hasRoot) {
        groupId = sections.constFirst().groupId;
        menuId = sections.constFirst().menuId;
    }

    QSet<quint64> stack;
    return buildSection(path, groupId, menuId, stack);
}

QVariantList GtkMenuImporter::buildSection(const QString &path,
                                           uint groupId,
                                           uint menuId,
                                           QSet<quint64> &stack)
{
    const quint64 key = sectionKey(groupId, menuId);
    if (stack.contains(key)) {
        return {};
    }

    const auto sections = m_menuSections.value(path);
    const GtkMenuSection *section = nullptr;
    for (const auto &candidate : sections) {
        if (candidate.groupId == groupId && candidate.menuId == menuId) {
            section = &candidate;
            break;
        }
    }
    if (!section) {
        return {};
    }

    stack.insert(key);
    QVariantList result;

    for (const auto &item : section->items) {
        GtkMenuLink sectionLink;
        if (parseMenuLink(item.value(QStringLiteral(":section")), sectionLink)) {
            const auto nested = buildSection(path, sectionLink.groupId, sectionLink.menuId, stack);
            if (!nested.isEmpty()) {
                if (!result.isEmpty()) {
                    QVariantMap separator;
                    separator.insert(QStringLiteral("id"), m_nextActionId++);
                    separator.insert(QStringLiteral("separator"), true);
                    separator.insert(QStringLiteral("visible"), true);
                    separator.insert(QStringLiteral("enabled"), false);
                    separator.insert(QStringLiteral("children"), QVariantList{});
                    result.append(separator);
                }
                result.append(nested);
            }
            continue;
        }

        const auto built = buildMenuItem(path, item, stack);
        if (!built.isEmpty()) {
            result.append(built);
        }
    }

    stack.remove(key);
    return result;
}

QVariantMap GtkMenuImporter::buildMenuItem(const QString &path,
                                           const QVariantMap &item,
                                           QSet<quint64> &stack)
{
    const QString label = displayLabel(item);

    GtkMenuLink submenuLink;
    const bool hasSubmenu = parseMenuLink(item.value(QStringLiteral(":submenu")), submenuLink);
    QVariantList children;
    if (hasSubmenu) {
        children = buildSection(path, submenuLink.groupId, submenuLink.menuId, stack);
    }

    const QString fullAction = unwrappedVariant(item.value(QStringLiteral("action"))).toString();
    if (label.isEmpty() && !hasSubmenu && fullAction.isEmpty()) {
        return {};
    }

    const int id = !fullAction.isEmpty()
        ? registerAction(fullAction, item.value(QStringLiteral("target")))
        : m_nextActionId++;

    QVariantMap result = item;
    result.insert(QStringLiteral("id"), id);
    result.insert(QStringLiteral("label"), label);
    result.insert(QStringLiteral("visible"), true);
    result.insert(QStringLiteral("separator"), false);
    result.insert(QStringLiteral("children"), children);

    const auto *description = actionDescription(fullAction);
    bool enabled = description ? description->enabled : true;
    const QVariant target = unwrappedVariant(item.value(QStringLiteral("target")));
    if (description && !description->parameterType.signature().isEmpty() && !target.isValid()) {
        enabled = false;
    }
    result.insert(QStringLiteral("enabled"), enabled);

    const QVariant state = actionState(description);
    if (state.isValid()) {
        if (target.isValid()) {
            result.insert(QStringLiteral("toggle-type"), QStringLiteral("radio"));
            result.insert(QStringLiteral("toggle-state"), state == target ? 1 : 0);
        } else if (state.metaType().id() == QMetaType::Bool) {
            result.insert(QStringLiteral("toggle-type"), QStringLiteral("checkmark"));
            result.insert(QStringLiteral("toggle-state"), state.toBool() ? 1 : 0);
        }
    }

    if (hasSubmenu) {
        result.insert(QStringLiteral("children-display"), QStringLiteral("submenu"));
    }
    return result;
}

QVariantList GtkMenuImporter::buildActionFallback()
{
    const QStringList categoryOrder = {
        QStringLiteral("Application"),
        QStringLiteral("File"),
        QStringLiteral("Edit"),
        QStringLiteral("View"),
        QStringLiteral("Format"),
        QStringLiteral("Tools"),
        QStringLiteral("Help"),
    };

    QHash<QString, QVariantList> categories;
    QSet<QString> seen;

    for (auto pathIt = m_actionDescriptions.cbegin(); pathIt != m_actionDescriptions.cend(); ++pathIt) {
        QString prefix = QStringLiteral("app.");
        if (pathIt.key() == m_context.windowActionPath) {
            prefix = QStringLiteral("win.");
        } else if (pathIt.key() == m_context.menubarPath) {
            prefix = QStringLiteral("unity.");
        }

        for (auto actionIt = pathIt->cbegin(); actionIt != pathIt->cend(); ++actionIt) {
            if (!actionIt->parameterType.signature().isEmpty()) {
                continue;
            }

            const QString fullAction = prefix + actionIt.key();
            if (seen.contains(fullAction)) {
                continue;
            }
            seen.insert(fullAction);

            categories[actionCategory(fullAction)].append(
                buildActionItem(fullAction, pathIt.key(), actionIt.value()));
        }
    }

    QVariantList result;
    for (const auto &category : categoryOrder) {
        const auto children = categories.value(category);
        if (children.isEmpty()) {
            continue;
        }

        QVariantMap top;
        top.insert(QStringLiteral("id"), m_nextActionId++);
        top.insert(QStringLiteral("label"),
                   category == QStringLiteral("Application") && !m_context.applicationName.isEmpty()
                       ? m_context.applicationName
                       : category);
        top.insert(QStringLiteral("enabled"), true);
        top.insert(QStringLiteral("visible"), true);
        top.insert(QStringLiteral("separator"), false);
        top.insert(QStringLiteral("children-display"), QStringLiteral("submenu"));
        top.insert(QStringLiteral("children"), children);
        result.append(top);
    }

    return result;
}

QVariantMap GtkMenuImporter::buildActionItem(const QString &fullAction,
                                             const QString &objectPath,
                                             const GtkActionDescription &description)
{
    const int id = registerAction(fullAction, {});

    QVariantMap item;
    item.insert(QStringLiteral("id"), id);
    item.insert(QStringLiteral("label"), humanizedActionName(fullAction));
    item.insert(QStringLiteral("enabled"), description.enabled);
    item.insert(QStringLiteral("visible"), true);
    item.insert(QStringLiteral("separator"), false);
    item.insert(QStringLiteral("children"), QVariantList{});

    const QVariant state = actionState(&description);
    if (state.metaType().id() == QMetaType::Bool) {
        item.insert(QStringLiteral("toggle-type"), QStringLiteral("checkmark"));
        item.insert(QStringLiteral("toggle-state"), state.toBool() ? 1 : 0);
    }

    auto actionIt = m_actions.find(id);
    if (actionIt != m_actions.end()) {
        actionIt->objectPath = objectPath;
    }
    return item;
}

int GtkMenuImporter::registerAction(const QString &fullAction, const QVariant &target)
{
    const int id = m_nextActionId++;
    ActionTarget action;
    action.fullName = fullAction;
    action.actionName = bareActionName(fullAction);
    action.objectPath = actionObjectPath(fullAction);
    action.target = unwrappedVariant(target);
    m_actions.insert(id, action);
    return id;
}

void GtkMenuImporter::startMenuGroupRequest(const QString &path,
                                                  uint groupId,
                                                  quint64 serial)
{
    auto &groupsForPath = m_startedMenuGroups[path];
    if (groupsForPath.contains(groupId)) {
        return;
    }
    groupsForPath.insert(groupId);
    ++m_pendingRequests;

    const QString busName = m_context.busName;
    QDBusInterface menus(busName,
                         path,
                         QString::fromLatin1(kGtkMenusInterface),
                         QDBusConnection::sessionBus());

    const QList<uint> groups{groupId};
    const auto call = menus.asyncCall(QStringLiteral("Start"),
                                      QVariant::fromValue(groups));
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this,
            [this, watcher, serial, path, busName, groupId](QDBusPendingCallWatcher *) {
                QDBusPendingReply<GtkMenuSectionList> reply = *watcher;
                watcher->deleteLater();

                if (serial != m_refreshSerial) {
                    // A context switch or a newer refresh may have sent End
                    // before this asynchronous Start reached the exporter.
                    // End the exact late subscription once Start completes.
                    QDBusInterface staleMenus(
                        busName,
                        path,
                        QString::fromLatin1(kGtkMenusInterface),
                        QDBusConnection::sessionBus());
                    staleMenus.asyncCall(
                        QStringLiteral("End"),
                        QVariant::fromValue(QList<uint>{groupId}));
                    return;
                }

                if (!reply.isError()) {
                    const auto sections = reply.value();
                    mergeMenuSections(path, sections);
                    m_hadSuccessfulRequest = true;

                    const auto linkedGroups = linkedMenuGroups(sections);
                    for (const uint linkedGroup : linkedGroups) {
                        startMenuGroupRequest(path, linkedGroup, serial);
                    }
                }
                requestFinished(serial);
            });
}

void GtkMenuImporter::startActionRequest(const QString &path, quint64 serial)
{
    ++m_pendingRequests;

    QDBusInterface actions(m_context.busName,
                           path,
                           QString::fromLatin1(kGtkActionsInterface),
                           QDBusConnection::sessionBus());

    const auto call = actions.asyncCall(QStringLiteral("DescribeAll"));
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this,
            [this, watcher, serial, path](QDBusPendingCallWatcher *) {
                QDBusPendingReply<GtkActionDescriptionMap> reply = *watcher;
                watcher->deleteLater();

                if (serial != m_refreshSerial) {
                    return;
                }

                if (!reply.isError()) {
                    m_actionDescriptions.insert(path, reply.value());
                    m_hadSuccessfulRequest = true;
                }
                requestFinished(serial);
            });
}

void GtkMenuImporter::mergeMenuSections(const QString &path,
                                            const GtkMenuSectionList &sections)
{
    auto &stored = m_menuSections[path];

    for (const auto &section : sections) {
        bool replaced = false;
        for (auto &existing : stored) {
            if (existing.groupId == section.groupId
                && existing.menuId == section.menuId) {
                existing = section;
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            stored.append(section);
        }
    }
}

QSet<uint> GtkMenuImporter::linkedMenuGroups(
    const GtkMenuSectionList &sections) const
{
    QSet<uint> groups;

    for (const auto &section : sections) {
        for (const auto &item : section.items) {
            GtkMenuLink link;
            if (parseMenuLink(item.value(QStringLiteral(":section")), link)) {
                groups.insert(link.groupId);
            }
            if (parseMenuLink(item.value(QStringLiteral(":submenu")), link)) {
                groups.insert(link.groupId);
            }
        }
    }

    return groups;
}

void GtkMenuImporter::requestFinished(quint64 serial)
{
    if (serial != m_refreshSerial) {
        return;
    }

    --m_pendingRequests;
    if (m_pendingRequests <= 0) {
        finishRefresh(serial);
    }
}

void GtkMenuImporter::onGtkMenusChanged(const dgm::GtkMenuChangeList &changes)
{
    Q_UNUSED(changes);
    refresh();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
void GtkMenuImporter::onGtkActionsChanged(const QStringList &removed,
                                          const dgm::GtkActionEnabledMap &enabledChanged,
                                          const QVariantMap &stateChanged,
                                          const dgm::GtkActionDescriptionMap &added)
{
    Q_UNUSED(removed);
    Q_UNUSED(enabledChanged);
    Q_UNUSED(stateChanged);
    Q_UNUSED(added);
    refresh();
}
#endif

void GtkMenuImporter::finishRefresh(quint64 serial)
{
    if (serial != m_refreshSerial) {
        return;
    }

    m_actions.clear();
    m_nextActionId = 1;

    QVariantList nextItems;

    if (!m_context.appMenuPath.isEmpty()) {
        const auto appItems = buildMenuPath(m_context.appMenuPath);
        if (!appItems.isEmpty()) {
            QVariantMap applicationMenu;
            applicationMenu.insert(QStringLiteral("id"), m_nextActionId++);
            applicationMenu.insert(
                QStringLiteral("label"),
                !m_context.applicationName.isEmpty()
                    ? m_context.applicationName
                    : (!m_context.applicationId.isEmpty()
                           ? m_context.applicationId
                           : QStringLiteral("Application")));
            applicationMenu.insert(QStringLiteral("enabled"), true);
            applicationMenu.insert(QStringLiteral("visible"), true);
            applicationMenu.insert(QStringLiteral("separator"), false);
            applicationMenu.insert(QStringLiteral("children-display"), QStringLiteral("submenu"));
            applicationMenu.insert(QStringLiteral("children"), appItems);
            nextItems.append(applicationMenu);
        }
    }

    if (!m_context.menubarPath.isEmpty()
        && m_context.menubarPath != m_context.appMenuPath) {
        nextItems.append(buildMenuPath(m_context.menubarPath));
    }

    if (nextItems.isEmpty()) {
        nextItems = buildActionFallback();
    }

    const bool nextReady = !nextItems.isEmpty();
    const bool changed = m_items != nextItems || m_ready != nextReady;
    m_items = nextItems;
    m_ready = nextReady;

    if (m_ready) {
        ++m_revision;
        setErrorString({});
    } else if (m_hadSuccessfulRequest) {
        setErrorString(QStringLiteral("GTK application exports no usable menu items"));
    } else {
        setErrorString(QStringLiteral("GTK menu/action D-Bus endpoints are unavailable"));
    }

    if (changed) {
        emit layoutChanged();
    }
    emit refreshFinished(m_ready);
}

void GtkMenuImporter::connectRemoteSignals()
{
    if (!m_context.isValid()) {
        return;
    }

    auto bus = QDBusConnection::sessionBus();
    bus.connect(m_context.busName,
                QString(),
                QString::fromLatin1(kGtkMenusInterface),
                QStringLiteral("Changed"),
                this,
                SLOT(onGtkMenusChanged(dgm::GtkMenuChangeList)));

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    bus.connect(m_context.busName,
                QString(),
                QString::fromLatin1(kGtkActionsInterface),
                QStringLiteral("Changed"),
                this,
                SLOT(onGtkActionsChanged(QStringList,dgm::GtkActionEnabledMap,QVariantMap,dgm::GtkActionDescriptionMap)));
#endif
}

void GtkMenuImporter::disconnectRemoteSignals()
{
    if (!m_context.isValid()) {
        return;
    }

    auto bus = QDBusConnection::sessionBus();
    bus.disconnect(m_context.busName,
                   QString(),
                   QString::fromLatin1(kGtkMenusInterface),
                   QStringLiteral("Changed"),
                   this,
                   SLOT(onGtkMenusChanged(dgm::GtkMenuChangeList)));

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    bus.disconnect(m_context.busName,
                   QString(),
                   QString::fromLatin1(kGtkActionsInterface),
                   QStringLiteral("Changed"),
                   this,
                   SLOT(onGtkActionsChanged(QStringList,dgm::GtkActionEnabledMap,QVariantMap,dgm::GtkActionDescriptionMap)));
#endif
}

void GtkMenuImporter::unsubscribeMenus()
{
    if (!m_context.isValid() || m_startedMenuGroups.isEmpty()) {
        m_startedMenuGroups.clear();
        return;
    }

    for (auto it = m_startedMenuGroups.cbegin();
         it != m_startedMenuGroups.cend();
         ++it) {
        QList<uint> groups;
        groups.reserve(it.value().size());
        for (const uint group : it.value()) {
            groups.append(group);
        }
        if (groups.isEmpty()) {
            continue;
        }

        QDBusInterface menus(m_context.busName,
                             it.key(),
                             QString::fromLatin1(kGtkMenusInterface),
                             QDBusConnection::sessionBus());
        menus.asyncCall(QStringLiteral("End"), QVariant::fromValue(groups));
    }
    m_startedMenuGroups.clear();
}

void GtkMenuImporter::clear()
{
    const bool hadData = m_ready || !m_items.isEmpty() || m_revision != 0;
    m_menuSections.clear();
    m_actionDescriptions.clear();
    m_actions.clear();
    m_startedMenuGroups.clear();
    m_items.clear();
    m_revision = 0;
    m_nextActionId = 1;
    m_pendingRequests = 0;
    m_hadSuccessfulRequest = false;
    m_ready = false;

    if (hadData) {
        emit layoutChanged();
    }
}

void GtkMenuImporter::setErrorString(const QString &errorString)
{
    if (m_errorString == errorString) {
        return;
    }
    m_errorString = errorString;
    emit errorStringChanged();
}

} // namespace dgm
