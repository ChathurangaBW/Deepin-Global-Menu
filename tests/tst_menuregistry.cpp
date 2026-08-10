// SPDX-License-Identifier: GPL-3.0-or-later

#include "menuregistry.h"

#include <QSignalSpy>
#include <QtTest>

class MenuRegistryTest final : public QObject
{
    Q_OBJECT

private slots:
    void registersAndLooksUpWindow();
    void unregistersAllWindowsForService();
};

void MenuRegistryTest::registersAndLooksUpWindow()
{
    dgm::MenuRegistry registry;
    QSignalSpy registeredSpy(&registry, &dgm::MenuRegistry::windowRegistered);

    registry.registerWindow(42,
                            QStringLiteral(":1.42"),
                            QDBusObjectPath(QStringLiteral("/com/example/Menu")));

    QCOMPARE(registeredSpy.count(), 1);
    QVERIFY(registry.contains(42));

    const auto endpoint = registry.menuForWindow(42);
    QCOMPARE(endpoint.service, QStringLiteral(":1.42"));
    QCOMPARE(endpoint.objectPath.path(), QStringLiteral("/com/example/Menu"));
    QVERIFY(endpoint.isValid());
}

void MenuRegistryTest::unregistersAllWindowsForService()
{
    dgm::MenuRegistry registry;
    QSignalSpy unregisteredSpy(&registry, &dgm::MenuRegistry::windowUnregistered);

    registry.registerWindow(1, QStringLiteral(":1.10"), QDBusObjectPath(QStringLiteral("/Menu1")));
    registry.registerWindow(2, QStringLiteral(":1.10"), QDBusObjectPath(QStringLiteral("/Menu2")));
    registry.registerWindow(3, QStringLiteral(":1.11"), QDBusObjectPath(QStringLiteral("/Menu3")));

    registry.unregisterService(QStringLiteral(":1.10"));

    QCOMPARE(unregisteredSpy.count(), 2);
    QVERIFY(!registry.contains(1));
    QVERIFY(!registry.contains(2));
    QVERIFY(registry.contains(3));
}

QTEST_MAIN(MenuRegistryTest)

#include "tst_menuregistry.moc"
