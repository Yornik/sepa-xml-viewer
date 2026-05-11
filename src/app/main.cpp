#include "sepa_controller.h"

#include "sepa/sepa.h"
#include "sepa/version.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QString>
#include <QUrl>

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
              << "  sepa-xml-viewer                  Open the viewer window.\n"
              << "  sepa-xml-viewer <file>           Open the viewer and load <file>.\n"
              << "  sepa-xml-viewer --check <file>   Parse <file>, print status, exit\n"
              << "                                   (no GUI; future Phase 6 CLI seed).\n"
              << "  sepa-xml-viewer --version, -v    Print version and exit.\n"
              << "  sepa-xml-viewer --help,    -h    Print this help text and exit.\n"
              << "\n"
              << "MVP build (v0.1.0): supports pain.001.001.13 only. Multi-version\n"
              << "coverage is planned for v0.2.0 (see plan/02-multi-version-support.md).\n";
}

// --check <file>: parse the file, print a status line, exit 0 on success
// or 1 on any failure. Doesn't construct QGuiApplication, so it works in a
// headless / CI environment without a display server.
int runCheck(const QString& path) {
    const auto result = sepa::parseFile(path);
    switch (result.status) {
    case sepa::ParseStatus::Success:
        std::cout << "OK: " << result.detectedFamily.toStdString() << ' '
                  << result.detectedVersion.toStdString() << " parsed cleanly.\n";
        return 0;
    case sepa::ParseStatus::FileNotFound:
    case sepa::ParseStatus::MalformedXml:
    case sepa::ParseStatus::UnrecognizedMessage:
        std::cerr << "ERROR: " << result.errorMessage.toStdString() << '\n';
        return 1;
    }
    return 1;
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

    QString preloadFile;
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
        if (arg == "--check") {
            if (i + 1 >= argc) {
                std::cerr << "ERROR: --check requires a file path argument.\n";
                return 1;
            }
            return runCheck(QString::fromLocal8Bit(argv[i + 1]));
        }
        // Positional arg: treat as a file to load once the GUI is up.
        if (arg.empty() || arg[0] != '-') {
            preloadFile = QString::fromLocal8Bit(argv[i]);
        }
    }

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("SEPA XML Viewer"));
    QGuiApplication::setOrganizationName(QStringLiteral("Yornik"));
    QGuiApplication::setApplicationVersion(QString::fromUtf8(sepa::kVersionString));

    // Material is a deliberately Google-looking style that ships with Qt Quick
    // Controls 2 — bigger touch targets, hover/press states, drop shadows on
    // popups. Picking it once at startup applies to every Controls element
    // without each QML file having to opt in. Fusion (the default) looks
    // dated; Basic looks unfinished. See Main.qml for the theme/accent setup.
    QQuickStyle::setStyle(QStringLiteral("Material"));

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

    // Preload the file passed on the command line (if any). The QML side
    // already imported sepa.viewer so the singleton exists by the time we
    // get here. Done after the engine load so the controller's stateChanged
    // signal hits a fully-constructed UI.
    if (!preloadFile.isEmpty()) {
        auto* controller = engine.singletonInstance<sepa::ui::SepaController*>(
            QStringLiteral("sepa.viewer"), QStringLiteral("SepaController"));
        if (controller != nullptr) {
            controller->loadFile(QUrl::fromLocalFile(preloadFile));
        }
    }

    return QGuiApplication::exec();
}
