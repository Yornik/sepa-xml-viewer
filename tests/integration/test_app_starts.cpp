// Phase 0 integration smoke. Launches the actual application binary against its
// CLI surface and asserts the contract documented in `--help`. The binary path
// is provided by the CMake harness through the SEPA_VIEWER_BINARY environment
// variable (set in tests/integration/CMakeLists.txt via a generator expression).

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

// MSVC names POSIX popen/pclose with a leading underscore (_popen/_pclose) and
// they live in <stdio.h>. Using bare popen on MSVC fails with C2039
// ("'popen': is not a member of '`global namespace''"). Wrap both names so the
// rest of the test code can call portable_popen / portable_pclose uniformly.
namespace {

inline std::FILE* portable_popen(const char* command, const char* mode) {
#ifdef _MSC_VER
    return ::_popen(command, mode);
#else
    return ::popen(command, mode);
#endif
}

inline int portable_pclose(std::FILE* stream) {
#ifdef _MSC_VER
    return ::_pclose(stream);
#else
    return ::pclose(stream);
#endif
}

}  // namespace

namespace {

std::string binary_path() {
    const char* p = std::getenv("SEPA_VIEWER_BINARY");
    if (p == nullptr || p[0] == '\0') {
        throw std::runtime_error(
            "SEPA_VIEWER_BINARY is not set; integration tests must be run via "
            "ctest --preset <preset>, which injects the binary path.");
    }
    return p;
}

struct CommandResult {
    int exit_code{};
    std::string stdout_output;
};

CommandResult run(const std::string& command) {
    CommandResult result;

    // Capture stdout via popen. Stderr stays on the test runner's stream so a
    // failing run is debuggable. The redirect spelling differs between shells
    // (POSIX `2>/dev/null` works under Git Bash on Windows runners too, since
    // the Windows runners have `nul` symlinked into Git Bash's environment).
    std::string cmd = command + " 2>/dev/null";
    std::FILE* pipe = portable_popen(cmd.c_str(), "r");
    if (pipe == nullptr) {
        throw std::runtime_error("failed to popen() the binary");
    }

    std::array<char, 4096> buf{};
    while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) {
        result.stdout_output.append(buf.data());
    }

    int rc = portable_pclose(pipe);
    // pclose returns the wait(2) status on POSIX (low byte signal, bits 8..15
    // exit code) and the program's exit code directly on Windows. The
    // (rc & 0xff00) >> 8 extraction works on POSIX; on Windows the result is
    // already the exit code, so the >> 8 shift would corrupt it. Branch.
    if (rc == -1) {
        throw std::runtime_error("pclose() failed");
    }
#ifdef _MSC_VER
    result.exit_code = rc;
#else
    result.exit_code = (rc & 0xff00) >> 8;
#endif
    return result;
}

}  // namespace

// Test names intentionally do not lead with `--` or `-` because Catch2's CLI
// parser interprets such tokens as flags when catch_discover_tests passes the
// name as a positional argument.

TEST_CASE("version long flag exits 0 and prints app name", "[integration]") {
    const auto result = run(binary_path() + " --version");
    REQUIRE(result.exit_code == 0);
    REQUIRE(result.stdout_output.find("SEPA XML Viewer") != std::string::npos);
}

TEST_CASE("help long flag exits 0 and prints the usage block", "[integration]") {
    const auto result = run(binary_path() + " --help");
    REQUIRE(result.exit_code == 0);
    REQUIRE(result.stdout_output.find("Usage:") != std::string::npos);
}

TEST_CASE("version short flag (-v) exits 0 and prints app name", "[integration]") {
    const auto result = run(binary_path() + " -v");
    REQUIRE(result.exit_code == 0);
    REQUIRE(result.stdout_output.find("SEPA XML Viewer") != std::string::npos);
}

TEST_CASE("help short flag (-h) exits 0 and prints the usage block", "[integration]") {
    const auto result = run(binary_path() + " -h");
    REQUIRE(result.exit_code == 0);
    REQUIRE(result.stdout_output.find("Usage:") != std::string::npos);
}
