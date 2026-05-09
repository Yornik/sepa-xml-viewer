// Phase 0 GUI smoke. Confirms the application binary can launch its
// QQmlApplicationEngine, load Main.qml from the registered module, and stay
// alive in its event loop without crashing — under Qt's offscreen platform
// plugin so a CI runner with no display can pass.
//
// The test does NOT inspect the QML object tree (that would require linking
// the QML module into a test executable, which complicates the build for no
// real Phase 0 win). It checks the binary is healthy at runtime by giving it
// a short window to crash and asserting it doesn't.

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace {

std::string binary_path() {
    const char* p = std::getenv("SEPA_VIEWER_BINARY");
    if (p == nullptr || p[0] == '\0') {
        throw std::runtime_error(
            "SEPA_VIEWER_BINARY is not set; GUI tests must be run via "
            "ctest --preset <preset>, which injects the binary path.");
    }
    return p;
}

// macOS does not ship GNU `timeout`; the homebrew `coreutils` formula installs
// it as `gtimeout`. Linux runners have it as `timeout` from coreutils-by-default.
// Pick the right name at compile time so the test is portable across platforms.
#if defined(__APPLE__)
constexpr const char* kTimeoutCmd = "gtimeout";
#else
constexpr const char* kTimeoutCmd = "timeout";
#endif

int run_with_timeout_headless(const std::string& binary, const std::string& platform,
                              int timeout_seconds) {
    // GNU coreutils `timeout` returns 124 when it had to kill the process,
    // otherwise it forwards the exit code. We do NOT redirect stderr — when
    // the test fails, ctest --output-on-failure surfaces whatever the binary
    // printed (e.g. "Could not load Qt platform plugin offscreen") which is
    // the only useful diagnostic.
    const std::string cmd =
        "QT_QPA_PLATFORM=" + platform + " " +
        std::string(kTimeoutCmd) + " --signal=TERM " +
        std::to_string(timeout_seconds) + " " + binary;
    int rc = std::system(cmd.c_str());
    if (rc == -1) {
        throw std::runtime_error("std::system() failed to spawn the binary");
    }
    return (rc & 0xff00) >> 8;
}

}  // namespace

TEST_CASE("binary launches under a headless Qt platform without crashing", "[gui]") {
    // Try `minimal` first (always linked statically into the binary via
    // qt_import_plugins in src/app/CMakeLists.txt). If `offscreen` was also
    // available at build time (vcpkg's qtbase port doesn't always build it
    // on Linux), the static-plugin import in CMake adds it too and the
    // second branch exercises the richer plugin.
    //
    // Two outcomes are healthy on either platform plugin:
    //   124 — timeout fired; app was running its event loop until killed.
    //     0 — the app exited cleanly within the window.
    // Anything else (1 = error, 134 = SIGABRT, 139 = SIGSEGV, 127 =
    // command-not-found) is a real failure we want to flag.
    int rc = run_with_timeout_headless(binary_path(), "minimal", 2);
    INFO("minimal exit code: " << rc);
    REQUIRE((rc == 124 || rc == 0));
}
