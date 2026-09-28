// SPDX-License-Identifier: GPL-3.0-or-later

#include "gtkmenuimporter.h"
#include "gtkmenutypes.h"

#include <QDBusConnection>
#include <QDBusSignature>
#include <QDBusVariant>
#include <QSignalSpy>
#include <QtTest>

class FakeGtkMenus final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.gtk.Menus")

public:
    FakeGtkMenus()
    {
        QVariantMap file;
        file.insert(QStringLiteral("label"), QStringLiteral("_File"));
        file.insert(QStringLiteral(":submenu"),
                    QVariant::fromValue(dgm::GtkMenuLink{0, 1}));

        dgm::GtkMenuSection root;
        root.groupId = 0;
        root.menuId = 0;
        root.items = {file};

        QVariantMap quit;
        quit.insert(QStringLiteral("label"), QStringLiteral("_Quit"));
        quit.insert(QStringLiteral("action"), QStringLiteral("app.quit"));

        QVariantMap toggle;
        toggle.insert(QStringLiteral("label"), QStringLiteral("_Read Only"));
        toggle.insert(QStringLiteral("action"), QStringLiteral("app.read-only"));

        dgm::GtkMenuSection fileMenu;
        fileMenu.groupId = 0;
        fileMenu.menuId = 1;
        fileMenu.items = {quit, toggle};

        sections = {root, fileMenu};
    }

    dgm::GtkMenuSectionList sections;

public slots:
    void Start(const QList<uint> &groups, dgm::GtkMenuSectionList &content) const
    {
        Q_UNUSED(groups);
        content = sections;
    }

    void End(const QList<uint> &groups)
    {
        Q_UNUSED(groups);
    }
};

class FakeGtkActions final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.gtk.Actions")

public:
    FakeGtkActions()
    {
        dgm::GtkActionDescription quit;
        quit.enabled = true;
        quit.parameterType = QDBusSignature(QStringLiteral(""));

        dgm::GtkActionDescription readOnly;
        readOnly.enabled = false;
        readOnly.parameterType = QDBusSignature(QStringLiteral(""));
        readOnly.state = {QDBusVariant(QVariant(true))};

        descriptions.insert(QStringLiteral("quit"), quit);
        descriptions.insert(QStringLiteral("read-only"), readOnly);
    }

    dgm::GtkActionDescriptionMap descriptions;
    QString lastAction;

public slots:
    void DescribeAll(dgm::GtkActionDescriptionMap &actions) const
    {
        actions = descriptions;
    }

    void Activate(const QString &actionName,
                  const dgm::DbusVariantList &parameters,
                  const QVariantMap &platformData)
    {
        Q_UNUSED(parameters);
        Q_UNUSED(platformData);
        lastAction = actionName;
        emit activated(actionName);
    }

signals:
    void activated(const QString &actionName);
};

class GtkMenuImporterTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void importsGtkMenuAndActivatesActions();
    void fallsBackToActionGroups();

private:
    static constexpr auto kService = "org.deepin.GlobalMenu.TestGtk";
    static constexpr auto kMenuPath = "/MenuBar";
    static constexpr auto kActionsPath = "/App";

    FakeGtkMenus m_menus;
    FakeGtkActions m_actions;
};

void GtkMenuImporterTest::initTestCase()
{
    dgm::registerGtkMenuMetaTypes();

    auto bus = QDBusConnection::sessionBus();
    QVERIFY2(bus.isConnected(), "The test must run inside a D-Bus session");
    QVERIFY(bus.registerService(QString::fromLatin1(kService)));
    QVERIFY(bus.registerObject(QString::fromLatin1(kMenuPath),
                               &m_menus,
                               QDBusConnection::ExportAllSlots));
    QVERIFY(bus.registerObject(QString::fromLatin1(kActionsPath),
                               &m_actions,
                               QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals));
}

void GtkMenuImporterTest::cleanupTestCase()
{
    auto bus = QDBusConnection::sessionBus();
    bus.unregisterObject(QString::fromLatin1(kMenuPath));
    bus.unregisterObject(QString::fromLatin1(kActionsPath));
    bus.unregisterService(QString::fromLatin1(kService));
}

void GtkMenuImporterTest::importsGtkMenuAndActivatesActions()
{
    dgm::GtkMenuImporter importer;

    dgm::GtkMenuContext context;
    context.busName = QString::fromLatin1(kService);
    context.applicationName = QStringLiteral("Test GTK");
    context.appActionPath = QString::fromLatin1(kActionsPath);
    context.menubarPath = QString::fromLatin1(kMenuPath);
    importer.setContext(context);

    QSignalSpy refreshSpy(&importer, &dgm::GtkMenuImporter::refreshFinished);
    importer.refresh();

    QTRY_VERIFY_WITH_TIMEOUT(!refreshSpy.isEmpty(), 3000);
    QVERIFY(refreshSpy.takeFirst().constFirst().toBool());
    QVERIFY(importer.isReady());

    const auto top = importer.topLevelItems();
    QCOMPARE(top.size(), 1);

    const auto file = top.constFirst().toMap();
    QCOMPARE(file.value(QStringLiteral("label")).toString(), QStringLiteral("File"));

    const auto children = file.value(QStringLiteral("children")).toList();
    QCOMPARE(children.size(), 2);

    const auto quit = children.at(0).toMap();
    QCOMPARE(quit.value(QStringLiteral("label")).toString(), QStringLiteral("Quit"));
    QVERIFY(quit.value(QStringLiteral("enabled")).toBool());

    const auto readOnly = children.at(1).toMap();
    QCOMPARE(readOnly.value(QStringLiteral("label")).toString(), QStringLiteral("Read Only"));
    QVERIFY(!readOnly.value(QStringLiteral("enabled")).toBool());
    QCOMPARE(readOnly.value(QStringLiteral("toggle-type")).toString(),
             QStringLiteral("checkmark"));
    QCOMPARE(readOnly.value(QStringLiteral("toggle-state")).toInt(), 1);

    QSignalSpy actionSpy(&m_actions, &FakeGtkActions::activated);
    importer.triggerAction(quit.value(QStringLiteral("id")).toInt());
    QTRY_COMPARE_WITH_TIMEOUT(actionSpy.count(), 1, 3000);
    QCOMPARE(m_actions.lastAction, QStringLiteral("quit"));
}

void GtkMenuImporterTest::fallsBackToActionGroups()
{
    dgm::GtkMenuImporter importer;

    dgm::GtkMenuContext context;
    context.busName = QString::fromLatin1(kService);
    context.applicationName = QStringLiteral("Test GTK");
    context.appActionPath = QString::fromLatin1(kActionsPath);
    context.appMenuPath = QStringLiteral("/NoAppMenu");
    context.menubarPath = QStringLiteral("/NoMenuBar");
    importer.setContext(context);

    QSignalSpy refreshSpy(&importer, &dgm::GtkMenuImporter::refreshFinished);
    importer.refresh();

    QTRY_VERIFY_WITH_TIMEOUT(!refreshSpy.isEmpty(), 3000);
    QVERIFY(refreshSpy.takeFirst().constFirst().toBool());
    QVERIFY(importer.isReady());

    const auto top = importer.topLevelItems();
    QVERIFY(!top.isEmpty());

    bool foundApplicationMenu = false;
    for (const auto &entry : top) {
        const auto menu = entry.toMap();
        if (menu.value(QStringLiteral("label")).toString() == QStringLiteral("Test GTK")) {
            foundApplicationMenu = true;
            QVERIFY(!menu.value(QStringLiteral("children")).toList().isEmpty());
            break;
        }
    }
    QVERIFY(foundApplicationMenu);
}

QTEST_GUILESS_MAIN(GtkMenuImporterTest)
#include "tst_gtkmenuimporter.moc"
