#include "sepa/version.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QString>
#include <Qt>

#include <iostream>
#include <string_view>

namespace {

constexpr std::string_view kAppName = "SEPA XML Viewer";

void printVersion() {
    std::cout << kAppName << ' ' << sepa::kVersionString;
    if (sepa::kVersionIsDirty) {
        std::cout << " (working tree dirty)";
    }
    std::cout << '\n';
}

void printHelp() {
    std::cout << kAppName << " - read-only offline viewer for SEPA payment XML messages.\n"
              << "\n"
              << "Usage:\n"
              << "  sepa-xml-viewer            Open the (currently empty) main window.\n"
              << "  sepa-xml-viewer --version  Print version and exit.\n"
              << "  sepa-xml-viewer --help     Print this help text and exit.\n"
              << "\n"
              << "Phase 0 build: a window opens with a placeholder label. SEPA parsing,\n"
              << "drag-and-drop, and the rest of the audience-facing UX arrive in later\n"
              << "phases (see plan/02-viewer-ui.md).\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--version" || arg == "-v") {
            printVersion();
            return 0;
        }
        if (arg == "--help" || arg == "-h") {
            printHelp();
            return 0;
        }
    }

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("SEPA XML Viewer"));
    QGuiApplication::setOrganizationName(QStringLiteral("Yornik"));
    QGuiApplication::setApplicationVersion(QString::fromUtf8(sepa::kVersionString));

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(1); },
        Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("sepa.viewer"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return QGuiApplication::exec();
}
