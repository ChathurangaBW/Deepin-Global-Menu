// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusmenuimporter.h"
#include "menuregistry.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QTextStream>

namespace {
constexpr auto kRegistrarService = "com.canonical.AppMenu.Registrar";
constexpr auto kRegistrarPath = "/com/canonical/AppMenu/Registrar";
constexpr auto kRegistrarInterface = "com.canonical.AppMenu.Registrar";

void printItem(QTextStream &out, const dgm::DbusMenuLayoutItem &item, int depth)
{
    const QString indent(depth * 2, QLatin1Char(' '));

    out << indent << '[' << item.id << "] ";
    if (item.isSeparator()) {
        out << "---";
    } else {
        const QString label = item.displayLabel();
        out << (label.isEmpty() ? QStringLiteral("<unnamed>") : label);
    }

    if (!item.isEnabled()) {
        out << " [disabled]";
    }
    if (!item.isVisible()) {
        out << " [hidden]";
    }
    out << '\n';

    for (const auto &child : item.children) {
        printItem(out, child, depth + 1);
    }
}

void printTree(QTextStream &out, const dgm::DbusMenuImporter &importer)
{
    out << "DBusMenu " << importer.endpoint().service
        << ' ' << importer.endpoint().objectPath.path()
        << " revision=" << importer.revision() << '\n';

    for (const auto &child : importer.rootItem().children) {
        printItem(out, child, 0);
    }
    out.flush();
}

bool resolveWindowEndpoint(quint32 windowId, dgm::MenuEndpoint &endpoint, QString &error)
{
    QDBusInterface registrar(QString::fromLatin1(kRegistrarService),
                             QString::fromLatin1(kRegistrarPath),
                             QString::fromLatin1(kRegistrarInterface),
                             QDBusConnection::sessionBus());
    if (!registrar.isValid()) {
        error = QStringLiteral("AppMenu registrar is not available on the session bus");
        return false;
    }

    const QDBusMessage reply = registrar.call(QStringLiteral("GetMenuForWindow"), windowId);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        error = reply.errorMessage();
        return false;
    }

    const auto args = reply.arguments();
    if (args.size() < 2) {
        error = QStringLiteral("Registrar returned an incomplete GetMenuForWindow reply");
        return false;
    }

    endpoint.service = args.at(0).toString();
    endpoint.objectPath = qvariant_cast<QDBusObjectPath>(args.at(1));
    if (!endpoint.isValid()) {
        error = QStringLiteral("No DBusMenu is registered for window %1").arg(windowId);
        return false;
    }

    return true;
}
} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("dgm-inspect"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.4.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Inspect a com.canonical.dbusmenu tree"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption windowOption({QStringLiteral("w"), QStringLiteral("window")},
                                    QStringLiteral("Resolve the menu registered for an X11 window id (decimal or 0xHEX)."),
                                    QStringLiteral("id"));
    QCommandLineOption serviceOption({QStringLiteral("s"), QStringLiteral("service")},
                                     QStringLiteral("DBus service exporting com.canonical.dbusmenu."),
                                     QStringLiteral("service"));
    QCommandLineOption pathOption({QStringLiteral("p"), QStringLiteral("path")},
                                  QStringLiteral("DBus object path exporting com.canonical.dbusmenu."),
                                  QStringLiteral("path"));
    QCommandLineOption watchOption({QStringLiteral("W"), QStringLiteral("watch")},
                                   QStringLiteral("Keep running and print the tree whenever it changes."));

    parser.addOption(windowOption);
    parser.addOption(serviceOption);
    parser.addOption(pathOption);
    parser.addOption(watchOption);
    parser.process(app);

    QTextStream err(stderr);
    dgm::MenuEndpoint endpoint;

    const bool directEndpoint = parser.isSet(serviceOption) || parser.isSet(pathOption);
    if (directEndpoint) {
        if (!parser.isSet(serviceOption) || !parser.isSet(pathOption)) {
            err << "Both --service and --path are required for direct inspection.\n";
            return 2;
        }

        endpoint.service = parser.value(serviceOption);
        endpoint.objectPath = QDBusObjectPath(parser.value(pathOption));
        if (!endpoint.isValid()) {
            err << "Invalid DBus service/object path.\n";
            return 2;
        }
    } else if (parser.isSet(windowOption)) {
        bool ok = false;
        const quint32 windowId = parser.value(windowOption).toUInt(&ok, 0);
        if (!ok || windowId == 0) {
            err << "Invalid window id. Use decimal or 0xHEX.\n";
            return 2;
        }

        QString error;
        if (!resolveWindowEndpoint(windowId, endpoint, error)) {
            err << error << '\n';
            return 1;
        }
    } else {
        err << "Specify --window ID or both --service SERVICE --path PATH.\n";
        return 2;
    }

    dgm::DbusMenuImporter importer;
    const bool watch = parser.isSet(watchOption);
    QTextStream out(stdout);
    bool printedOnce = false;

    QObject::connect(&importer, &dgm::DbusMenuImporter::layoutChanged,
                     &app,
                     [&] {
                         if (!watch || !importer.isReady()) {
                             return;
                         }
                         if (printedOnce) {
                             out << "---\n";
                         }
                         printTree(out, importer);
                         printedOnce = true;
                     });

    QObject::connect(&importer, &dgm::DbusMenuImporter::refreshFinished,
                     &app,
                     [&](bool success) {
                         if (!success) {
                             err << "Failed to import DBusMenu: " << importer.errorString() << '\n';
                             app.exit(1);
                             return;
                         }

                         if (!watch) {
                             printTree(out, importer);
                             app.quit();
                         }
                     });

    importer.setEndpoint(endpoint);
    importer.refresh();
    return app.exec();
}
