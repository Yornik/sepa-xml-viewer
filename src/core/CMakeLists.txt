# sepa_core — pure C++ parser. No Qt Quick / QML / GUI dependencies; the
# UI links this and translates the parsed Node tree into Qt models. Keeping
# the parser separable from the UI is what Phase 6's CLI / WASM build will
# rely on later.

find_package(pugixml CONFIG REQUIRED)

# Drop the parser headers under <build>/include/sepa/ so callers write
# `#include "sepa/sepa.h"` rather than relative paths. Matches the existing
# `sepa/version.h` convention.
configure_file(sepa.h "${CMAKE_BINARY_DIR}/include/sepa/sepa.h" COPYONLY)

add_library(sepa_core STATIC
    sepa.cpp
)

target_include_directories(sepa_core
    PUBLIC
        "${CMAKE_BINARY_DIR}/include"
)

target_link_libraries(sepa_core
    PUBLIC
        Qt6::Core
    PRIVATE
        sepa::warnings
        pugixml::pugixml
)

add_library(sepa::core ALIAS sepa_core)
