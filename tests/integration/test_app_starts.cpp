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
    // failing run is debuggable.
    std::string cmd = command + " 2>/dev/null";
    FILE* pipe = ::popen(cmd.c_str(), "r");
    if (pipe == nullptr) {
        throw std::runtime_error("failed to popen() the binary");
    }

    std::array<char, 4096> buf{};
    while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) {
        result.stdout_output.append(buf.data());
    }

    int rc = ::pclose(pipe);
    // pclose returns the wait(2) status; on POSIX the low byte is the signal,
    // bits 8..15 are the exit code. CTest only cares whether we report 0/1.
    if (rc == -1) {
        throw std::runtime_error("pclose() failed");
    }
    result.exit_code = (rc & 0xff00) >> 8;
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
