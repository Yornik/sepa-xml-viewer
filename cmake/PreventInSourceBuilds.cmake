# Reject in-source builds. CMake artefacts in the source tree pollute the
# repository, defeat reproducible-build assumptions, and quietly break later
# `cmake --preset` invocations. Configure into a separate build directory.

get_filename_component(_sepa_srcdir "${CMAKE_SOURCE_DIR}" REALPATH)
get_filename_component(_sepa_bindir "${CMAKE_BINARY_DIR}" REALPATH)

if("${_sepa_srcdir}" STREQUAL "${_sepa_bindir}")
    message(FATAL_ERROR
        "In-source builds are not allowed.\n"
        "Configure into a separate build directory, for example:\n"
        "    cmake --preset dev-debug\n"
        "or:\n"
        "    cmake -S . -B build -G Ninja\n"
    )
endif()
