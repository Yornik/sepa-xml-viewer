#include <iostream>
#include <string>
#include <string_view>

#include "sepa/version.h"

namespace {

constexpr std::string_view kAppName = "SEPA XML Viewer";

void print_version() {
    std::cout << kAppName << ' ' << sepa::kVersionString;
    if (sepa::kVersionIsDirty) {
        std::cout << " (working tree dirty)";
    }
    std::cout << '\n';
}

void print_help() {
    std::cout
        << kAppName << " - read-only offline viewer for SEPA payment XML messages.\n"
        << "\n"
        << "Usage:\n"
        << "  sepa-xml-viewer [--version] [--help]\n"
        << "\n"
        << "Phase 0 build: this binary is the build-pipeline smoke test.\n"
        << "It does not yet open SEPA files; running it with --version\n"
        << "confirms that the build, linking, and version-injection wiring works.\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--version" || arg == "-v") {
            print_version();
            return 0;
        }
        if (arg == "--help" || arg == "-h") {
            print_help();
            return 0;
        }
    }

    print_version();
    print_help();
    return 0;
}
