cmake_minimum_required(VERSION 3.25)

# Block in-source builds before project() so a misconfigured tree fails fast.
include(${CMAKE_SOURCE_DIR}/cmake/PreventInSourceBuilds.cmake)

project(sepa-xml-viewer
    VERSION 0.0.0
    DESCRIPTION "Cross-platform offline viewer for SEPA payment XML messages"
    HOMEPAGE_URL "https://github.com/Yornik/sepa-xml-viewer"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(cmake/CompilerWarnings.cmake)
include(cmake/ProjectVersion.cmake)

add_subdirectory(src)
