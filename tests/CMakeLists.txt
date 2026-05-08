# Three-layer test harness per plan/00-init-phase.md §6:
#   - unit/         Catch2 unit tests against project headers (no I/O, no Qt event loop)
#   - integration/  Catch2 tests that spawn the binary and exercise its CLI contract
#   - gui/          Catch2 + offscreen Qt-platform tests verifying the GUI loads cleanly
#
# Each layer compiles to its own executable, registered with CTest via
# catch_discover_tests so per-test names show up in `ctest -V` and CI.

add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(gui)
