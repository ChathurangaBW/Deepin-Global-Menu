// SPDX-License-Identifier: GPL-3.0-or-later

#include "activewindowtracker.h"
#include "globalmenucontroller.h"
#include "gtkmenutypes.h"
#include "menuregistry.h"

#include <QDBusObjectPath>
#include <QSignalSpy>
#include <QtTest>

class FakeActiveWindowTracker final : public dgm::ActiveWindowTracker
{
    Q_OBJECT

public:
    using dgm::ActiveWindowTracker::ActiveWindowTracker;

    bool start() override
    {
        m_running = true;
        return true;
    }

    void stop() override
    {
        m_running = false;
        setActiveWindowId(0);
    }

    [[nodiscard]] bool isRunning() const override
    {
        return m_running;
    }

    void activate(quint32 windowId)
    {
        setActiveWindowId(windowId);
    }

    void activateInfo(const dgm::ActiveWindowInfo &info)
    {
        setActiveWindowInfo(info);
    }

private:
    bool m_running = false;
};

class FakeGtkMenuService final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.gtk.Menus")

public slots:
    void Start(const QList<uint> &groups, dgm::GtkMenuSectionList &content) const
    {
        Q_UNUSED(groups);

        QVariantMap file;
        file.insert(QStringLiteral("label"), QStringLiteral("_File"));
        file.insert(QStringLiteral("action"), QStringLiteral("app.quit"));

        dgm::GtkMenuSection root;
        root.groupId = 0;
        root.menuId = 0;
        root.items = {file};
        content = {root};
    }

    void End(const QList<uint> &groups)
    {
        Q_UNUSED(groups);
    }
};

class GlobalMenuControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void followsTrackerAndRegistry();
    void providesSafeFallbackMenu();
    void exposesShortcutFallbackWhenAvailable();
    void fallsBackFromDeadDbusMenuToGtk();
};

void GlobalMenuControllerTest::followsTrackerAndRegistry()
{
    dgm::MenuRegistry registry;
    dgm::GlobalMenuController controller(&registry);
    FakeActiveWindowTracker tracker;

    registry.registerWindow(42,
                            QStringLiteral(":1.42"),
                            QDBusObjectPath(QStringLiteral("/Menu42")));

    controller.setActiveWindowTracker(&tracker);

    QSignalSpy activeSpy(&controller, &dgm::GlobalMenuController::activeWindowIdChanged);
    tracker.activate(42);

    QCOMPARE(activeSpy.count(), 1);
    QCOMPARE(controller.activeWindowId(), 42U);
    QVERIFY(controller.hasMenu());
    QCOMPARE(controller.menuService(), QStringLiteral(":1.42"));
    QCOMPARE(controller.menuObjectPath(), QStringLiteral("/Menu42"));

    tracker.activate(7);
    QCOMPARE(controller.activeWindowId(), 7U);
    QVERIFY(controller.hasMenu());
    QCOMPARE(controller.menuSource(), QStringLiteral("fallback"));
    QVERIFY(controller.menuReady());

    registry.registerWindow(7,
                            QStringLiteral(":1.7"),
                            QDBusObjectPath(QStringLiteral("/Menu7")));

    QVERIFY(controller.hasMenu());
    QCOMPARE(controller.menuService(), QStringLiteral(":1.7"));
    QCOMPARE(controller.menuObjectPath(), QStringLiteral("/Menu7"));

    tracker.stop();
    QCOMPARE(controller.activeWindowId(), 0U);
    QVERIFY(!controller.hasMenu());
}

void GlobalMenuControllerTest::providesSafeFallbackMenu()
{
    dgm::MenuRegistry registry;
    dgm::GlobalMenuController controller(&registry);
    FakeActiveWindowTracker tracker;
    controller.setActiveWindowTracker(&tracker);

    dgm::ActiveWindowInfo info;
    info.nativeId = 77;
    info.appId = QStringLiteral("org.example.Editor.desktop");
    info.appName = QStringLiteral("Example Editor");
    info.backend = QStringLiteral("treeland");
    tracker.activateInfo(info);

    QCOMPARE(controller.menuSource(), QStringLiteral("fallback"));
    QVERIFY(controller.hasMenu());
    QVERIFY(controller.menuReady());

    const auto top = controller.menuItems();
    QCOMPARE(top.size(), 2);

    const auto applicationMenu = top.constFirst().toMap();
    QCOMPARE(applicationMenu.value(QStringLiteral("label")).toString(),
             QStringLiteral("Example Editor"));

    const auto children = applicationMenu.value(QStringLiteral("children")).toList();
    QVERIFY(children.size() >= 4);

    QSignalSpy actionSpy(&controller,
                         &dgm::GlobalMenuController::fallbackActionRequested);
    controller.triggerMenuAction(
        children.constFirst().toMap().value(QStringLiteral("id")).toInt());

    QCOMPARE(actionSpy.count(), 1);
    QCOMPARE(actionSpy.takeFirst().constFirst().toString(),
             QStringLiteral("new-instance"));
}

void GlobalMenuControllerTest::exposesShortcutFallbackWhenAvailable()
{
    dgm::MenuRegistry registry;
    dgm::GlobalMenuController controller(&registry);
    FakeActiveWindowTracker tracker;
    controller.setActiveWindowTracker(&tracker);
    controller.setShortcutActionsAvailable(true);

    dgm::ActiveWindowInfo info;
    info.nativeId = 88;
    info.appId = QStringLiteral("org.example.Browser.desktop");
    info.appName = QStringLiteral("Example Browser");
    info.backend = QStringLiteral("treeland");
    tracker.activateInfo(info);

    QCOMPARE(controller.menuSource(), QStringLiteral("fallback"));

    const auto top = controller.menuItems();
    QCOMPARE(top.size(), 6);
    QCOMPARE(top.at(1).toMap().value(QStringLiteral("label")).toString(),
             QStringLiteral("File"));
    QCOMPARE(top.at(2).toMap().value(QStringLiteral("label")).toString(),
             QStringLiteral("Edit"));
    QCOMPARE(top.at(3).toMap().value(QStringLiteral("label")).toString(),
             QStringLiteral("View"));
    QCOMPARE(top.at(5).toMap().value(QStringLiteral("label")).toString(),
             QStringLiteral("Help"));

    const auto editChildren =
        top.at(2).toMap().value(QStringLiteral("children")).toList();

    QVariantMap copyItem;
    for (const auto &entry : editChildren) {
        const auto item = entry.toMap();
        if (item.value(QStringLiteral("label")).toString() == QStringLiteral("Copy")) {
            copyItem = item;
            break;
        }
    }
    QVERIFY(!copyItem.isEmpty());

    QSignalSpy shortcutSpy(&controller,
                           &dgm::GlobalMenuController::shortcutRequested);
    controller.triggerMenuAction(copyItem.value(QStringLiteral("id")).toInt());

    QCOMPARE(shortcutSpy.count(), 1);
    QCOMPARE(shortcutSpy.takeFirst().constFirst().toString(),
             QStringLiteral("Ctrl+C"));
}

void GlobalMenuControllerTest::fallsBackFromDeadDbusMenuToGtk()
{
    constexpr auto gtkService = "org.deepin.GlobalMenu.ControllerGtk";
    constexpr auto menuPath =
        "/org/deepin/GlobalMenu/ControllerGtk/menus/MenuBar";

    dgm::registerGtkMenuMetaTypes();
    auto bus = QDBusConnection::sessionBus();
    QVERIFY(bus.isConnected());

    FakeGtkMenuService gtkMenus;
    QVERIFY(bus.registerService(QString::fromLatin1(gtkService)));
    QVERIFY(bus.registerObject(QString::fromLatin1(menuPath),
                               &gtkMenus,
                               QDBusConnection::ExportAllSlots));

    dgm::MenuRegistry registry;
    registry.registerWindow(
        99,
        QStringLiteral("org.deepin.GlobalMenu.MissingDbusExporter"),
        QDBusObjectPath(QStringLiteral("/MissingMenu")));

    dgm::GlobalMenuController controller(&registry);
    FakeActiveWindowTracker tracker;
    controller.setActiveWindowTracker(&tracker);

    dgm::ActiveWindowInfo info;
    info.nativeId = 99;
    info.x11Id = 99;
    info.appId = QString::fromLatin1(gtkService);
    info.appName = QStringLiteral("Controller GTK");
    info.backend = QStringLiteral("x11");
    tracker.activateInfo(info);

    QTRY_COMPARE_WITH_TIMEOUT(controller.menuSource(),
                              QStringLiteral("gtk"),
                              5000);
    QVERIFY(controller.menuReady());
    QCOMPARE(controller.menuService(), QString::fromLatin1(gtkService));

    const auto top = controller.menuItems();
    QCOMPARE(top.size(), 1);
    QCOMPARE(top.constFirst().toMap().value(QStringLiteral("label")).toString(),
             QStringLiteral("File"));

    bus.unregisterObject(QString::fromLatin1(menuPath));
    bus.unregisterService(QString::fromLatin1(gtkService));
}

QTEST_GUILESS_MAIN(GlobalMenuControllerTest)

#include "tst_globalmenucontroller.moc"
