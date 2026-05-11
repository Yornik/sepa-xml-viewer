#include "sepa/version.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QString>

#include <iostream>
#include <Qt>
#include <string_view>

#ifdef _WIN32
#include <windows.h>

#include <cstdio>
#endif

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
#ifdef _WIN32
    // The Windows build is a GUI-subsystem exe (WIN32_EXECUTABLE) so desktop
    // launches don't flash a console window. The side effect is that stdout
    // and stderr aren't attached to anything when the user runs the binary
    // from cmd or PowerShell — so --version / --help would print nothing.
    // Re-attach to the parent console (if any) so the CLI fast-path works.
    //
    // Only attach when stdout isn't already pointing somewhere (a pipe, a
    // file from `> out.txt`, etc.) — overriding an explicit redirection
    // would silently break shell pipelines and CI output capture.
    const auto attachConsoleIfNeeded = []() {
        HANDLE stdoutHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD stdoutType = (stdoutHandle == nullptr || stdoutHandle == INVALID_HANDLE_VALUE)
                               ? FILE_TYPE_UNKNOWN
                               : ::GetFileType(stdoutHandle);
        if (stdoutType != FILE_TYPE_UNKNOWN) {
            return;  // already redirected — leave it alone
        }
        if (!::AttachConsole(ATTACH_PARENT_PROCESS)) {
            return;  // no parent console (e.g. launched from Explorer)
        }
        FILE* unused = nullptr;
        ::freopen_s(&unused, "CONOUT$", "w", stdout);
        ::freopen_s(&unused, "CONOUT$", "w", stderr);
    };
    attachConsoleIfNeeded();
#endif

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
    // windeployqt deploys Qt's own QML modules (QtQuick, QtQuick.Controls,
    // QtQuick.Layouts, ...) into <exe-dir>/qml/. Qt doesn't add that path to
    // the QML import path automatically on Windows unless a qt.conf next to
    // the exe says so — and windeployqt doesn't always drop a qt.conf. Add
    // the directory explicitly so the deployed module tree is always found
    // regardless of qt.conf, env vars, or installer layout. Harmless on
    // platforms where the path doesn't exist; Qt just finds nothing there.
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/qml"));
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
