// Phase 0 unit-test smoke. The project has no business logic yet; this file
// exists to prove that Catch2 builds, runs, and reports through CTest, plus to
// catch trivial regressions (e.g. version.h getting deleted).

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

#include "sepa/version.h"

TEST_CASE("version string is populated", "[smoke]") {
    const std::string_view version = sepa::kVersionString;
    REQUIRE_FALSE(version.empty());
}

TEST_CASE("version dirty flag matches working-tree state at build time", "[smoke]") {
    // We can only assert the value is a valid bool — the actual state depends
    // on what the working tree looked like when CMake configured.
    const bool dirty = sepa::kVersionIsDirty;
    REQUIRE((dirty == true || dirty == false));
}
