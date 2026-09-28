// SPDX-License-Identifier: GPL-3.0-or-later

#include "appmenuregistrar.h"
#include "menuregistry.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QtTest>

class AppMenuRegistrarTest final : public QObject
{
    Q_OBJECT

private slots:
    void exportsGetMenusOverDbus();
};

void AppMenuRegistrarTest::exportsGetMenusOverDbus()
{
    auto bus = QDBusConnection::sessionBus();
    QVERIFY2(bus.isConnected(), "The test must run inside a D-Bus session");

    dgm::MenuRegistry registry;
    registry.registerWindow(
        20,
        QStringLiteral(":1.20"),
        QDBusObjectPath(QStringLiteral("/Menu20")));
    registry.registerWindow(
        10,
        QStringLiteral(":1.10"),
        QDBusObjectPath(QStringLiteral("/Menu10")));

    dgm::AppMenuRegistrar registrar(&registry);
    QVERIFY(registrar.start());

    QDBusInterface iface(
        QStringLiteral("com.canonical.AppMenu.Registrar"),
        QStringLiteral("/com/canonical/AppMenu/Registrar"),
        QStringLiteral("com.canonical.AppMenu.Registrar"),
        bus);
    QVERIFY(iface.isValid());

    const QDBusReply<dgm::RegistrarMenuList> reply =
        iface.call(QStringLiteral("GetMenus"));
    QVERIFY2(reply.isValid(), qPrintable(reply.error().message()));

    const auto menus = reply.value();
    QCOMPARE(menus.size(), 2);
    QCOMPARE(menus.at(0).windowId, 10U);
    QCOMPARE(menus.at(0).service, QStringLiteral(":1.10"));
    QCOMPARE(menus.at(0).objectPath.path(), QStringLiteral("/Menu10"));
    QCOMPARE(menus.at(1).windowId, 20U);
    QCOMPARE(menus.at(1).service, QStringLiteral(":1.20"));
    QCOMPARE(menus.at(1).objectPath.path(), QStringLiteral("/Menu20"));

    registrar.stop();
}

QTEST_GUILESS_MAIN(AppMenuRegistrarTest)

#include "tst_appmenuregistrar.moc"
