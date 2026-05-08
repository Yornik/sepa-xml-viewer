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

int run_with_timeout_offscreen(const std::string& binary, int timeout_seconds) {
    // GNU coreutils `timeout` returns 124 when it had to kill the process,
    // otherwise it forwards the exit code. We accept 124 (forced kill after
    // the app survived its window) as success and any non-124 non-zero as
    // failure.
    const std::string cmd =
        "QT_QPA_PLATFORM=offscreen "
        "timeout --signal=TERM " + std::to_string(timeout_seconds) + " " +
        binary + " >/dev/null 2>&1";
    int rc = std::system(cmd.c_str());
    if (rc == -1) {
        throw std::runtime_error("std::system() failed to spawn the binary");
    }
    return (rc & 0xff00) >> 8;
}

}  // namespace

TEST_CASE("binary launches under offscreen Qt platform and stays alive", "[gui]") {
    // Two seconds is plenty for QML to load and the event loop to settle.
    // If the binary exits early with a non-zero code (e.g. QML parse error,
    // missing plugin, missing platform), the run shows that exit code.
    const int rc = run_with_timeout_offscreen(binary_path(), 2);
    REQUIRE(rc == 124);  // timeout fired; app was healthy until killed
}
