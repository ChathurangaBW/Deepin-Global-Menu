// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusmenutypes.h"

#include <QtTest>

class DbusMenuTypesTest final : public QObject
{
    Q_OBJECT

private slots:
    void normalizesMnemonicMarkers();
    void usesProtocolDefaults();
    void exportsRecursiveVariantTree();
};

void DbusMenuTypesTest::normalizesMnemonicMarkers()
{
    dgm::DbusMenuLayoutItem item;
    item.properties.insert(QStringLiteral("label"), QStringLiteral("_File"));
    QCOMPARE(item.displayLabel(), QStringLiteral("File"));

    item.properties.insert(QStringLiteral("label"), QStringLiteral("Save __As"));
    QCOMPARE(item.displayLabel(), QStringLiteral("Save _As"));
}

void DbusMenuTypesTest::usesProtocolDefaults()
{
    dgm::DbusMenuLayoutItem item;
    QVERIFY(item.isEnabled());
    QVERIFY(item.isVisible());
    QVERIFY(!item.isSeparator());

    item.properties.insert(QStringLiteral("enabled"), false);
    item.properties.insert(QStringLiteral("visible"), false);
    item.properties.insert(QStringLiteral("type"), QStringLiteral("separator"));

    QVERIFY(!item.isEnabled());
    QVERIFY(!item.isVisible());
    QVERIFY(item.isSeparator());
}

void DbusMenuTypesTest::exportsRecursiveVariantTree()
{
    dgm::DbusMenuLayoutItem root;
    root.id = 1;
    root.properties.insert(QStringLiteral("label"), QStringLiteral("_File"));

    dgm::DbusMenuLayoutItem child;
    child.id = 2;
    child.properties.insert(QStringLiteral("label"), QStringLiteral("_Open"));
    root.children.append(child);

    const QVariantMap map = root.toVariantMap();
    QCOMPARE(map.value(QStringLiteral("id")).toInt(), 1);
    QCOMPARE(map.value(QStringLiteral("label")).toString(), QStringLiteral("File"));

    const QVariantList children = map.value(QStringLiteral("children")).toList();
    QCOMPARE(children.size(), 1);
    const QVariantMap childMap = children.constFirst().toMap();
    QCOMPARE(childMap.value(QStringLiteral("id")).toInt(), 2);
    QCOMPARE(childMap.value(QStringLiteral("label")).toString(), QStringLiteral("Open"));
}

QTEST_MAIN(DbusMenuTypesTest)
#include "tst_dbusmenutypes.moc"
