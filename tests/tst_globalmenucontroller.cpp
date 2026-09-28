// SPDX-License-Identifier: GPL-3.0-or-later

#include "activewindowtracker.h"
#include "globalmenucontroller.h"
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

private:
    bool m_running = false;
};

class GlobalMenuControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void followsTrackerAndRegistry();
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
    QVERIFY(!controller.hasMenu());

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

QTEST_GUILESS_MAIN(GlobalMenuControllerTest)

#include "tst_globalmenucontroller.moc"
