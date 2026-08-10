// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusmenuimporter.h"
#include "dbusmenutypes.h"
#include "menuregistry.h"

#include <QDBusConnection>
#include <QDBusVariant>
#include <QSignalSpy>
#include <QtTest>

class FakeDbusMenu final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.canonical.dbusmenu")

public:
    FakeDbusMenu()
    {
        root.id = 0;

        dgm::DbusMenuLayoutItem file;
        file.id = 1;
        file.properties.insert(QStringLiteral("label"), QStringLiteral("_File"));
        root.children.append(file);
    }

    uint revision = 1;
    dgm::DbusMenuLayoutItem root;
    int lastEventItem = -1;
    QString lastEventName;
    int lastAboutToShowItem = -1;

public slots:
    void GetLayout(int parentId,
                   int recursionDepth,
                   const QStringList &propertyNames,
                   uint &outRevision,
                   dgm::DbusMenuLayoutItem &outItem) const
    {
        Q_UNUSED(parentId);
        Q_UNUSED(recursionDepth);
        Q_UNUSED(propertyNames);
        outRevision = revision;
        outItem = root;
    }

    void Event(int id, const QString &eventId, const QDBusVariant &data, uint timestamp)
    {
        Q_UNUSED(data);
        Q_UNUSED(timestamp);
        lastEventItem = id;
        lastEventName = eventId;
        emit eventReceived();
    }

    bool AboutToShow(int id)
    {
        lastAboutToShowItem = id;
        emit aboutToShowReceived();
        return false;
    }

signals:
    void LayoutUpdated(uint revision, int parentId);
    void ItemsPropertiesUpdated(dgm::DbusMenuItemList updated,
                                dgm::DbusMenuItemKeysList removed);

    void eventReceived();
    void aboutToShowReceived();
};

class DbusMenuImporterTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void importsAndTracksMenu();

private:
    static constexpr auto kService = "org.deepin.GlobalMenu.TestExporter";
    static constexpr auto kPath = "/Menu";
    FakeDbusMenu m_fakeMenu;
};

void DbusMenuImporterTest::initTestCase()
{
    dgm::registerDbusMenuMetaTypes();

    auto bus = QDBusConnection::sessionBus();
    QVERIFY2(bus.isConnected(), "The test must run inside a D-Bus session");
    QVERIFY(bus.registerService(QString::fromLatin1(kService)));
    QVERIFY(bus.registerObject(QString::fromLatin1(kPath),
                               &m_fakeMenu,
                               QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals));
}

void DbusMenuImporterTest::cleanupTestCase()
{
    auto bus = QDBusConnection::sessionBus();
    bus.unregisterObject(QString::fromLatin1(kPath));
    bus.unregisterService(QString::fromLatin1(kService));
}

void DbusMenuImporterTest::importsAndTracksMenu()
{
    dgm::DbusMenuImporter importer;
    importer.setEndpoint({QString::fromLatin1(kService),
                          QDBusObjectPath(QString::fromLatin1(kPath))});

    QSignalSpy refreshSpy(&importer, &dgm::DbusMenuImporter::refreshFinished);
    importer.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(!refreshSpy.isEmpty(), 3000);
    QVERIFY(refreshSpy.takeFirst().constFirst().toBool());

    QVERIFY(importer.isReady());
    QCOMPARE(importer.revision(), 1U);
    QCOMPARE(importer.rootItem().children.size(), 1);
    QCOMPARE(importer.rootItem().children.constFirst().displayLabel(), QStringLiteral("File"));

    QSignalSpy layoutSpy(&importer, &dgm::DbusMenuImporter::layoutChanged);

    dgm::DbusMenuItem update;
    update.id = 1;
    update.properties.insert(QStringLiteral("enabled"), false);
    emit m_fakeMenu.ItemsPropertiesUpdated({update}, {});

    QTRY_VERIFY_WITH_TIMEOUT(!layoutSpy.isEmpty(), 3000);
    QVERIFY(!importer.rootItem().children.constFirst().isEnabled());
    layoutSpy.clear();

    m_fakeMenu.root.children[0].properties.insert(QStringLiteral("label"), QStringLiteral("_Edit"));
    ++m_fakeMenu.revision;
    emit m_fakeMenu.LayoutUpdated(m_fakeMenu.revision, 0);

    QTRY_VERIFY_WITH_TIMEOUT(importer.revision() == m_fakeMenu.revision, 3000);
    QCOMPARE(importer.rootItem().children.constFirst().displayLabel(), QStringLiteral("Edit"));

    QSignalSpy aboutSpy(&m_fakeMenu, &FakeDbusMenu::aboutToShowReceived);
    importer.prepareSubmenu(1);
    QTRY_COMPARE_WITH_TIMEOUT(aboutSpy.count(), 1, 3000);
    QCOMPARE(m_fakeMenu.lastAboutToShowItem, 1);

    QSignalSpy eventSpy(&m_fakeMenu, &FakeDbusMenu::eventReceived);
    importer.triggerAction(1, 123U);
    QTRY_COMPARE_WITH_TIMEOUT(eventSpy.count(), 1, 3000);
    QCOMPARE(m_fakeMenu.lastEventItem, 1);
    QCOMPARE(m_fakeMenu.lastEventName, QStringLiteral("clicked"));
}

QTEST_GUILESS_MAIN(DbusMenuImporterTest)
#include "tst_dbusmenuimporter.moc"
