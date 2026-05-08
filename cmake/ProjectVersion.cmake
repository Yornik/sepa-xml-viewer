# Derive the build's user-visible version from `git describe --tags --dirty --always`
# so the binary's --version output matches what was checked out.
#
# Falls back to the CMake project() VERSION when git or .git/ is not available
# (e.g. building from a sdist tarball).

find_package(Git QUIET)

set(SEPA_VIEWER_VERSION_STRING "${PROJECT_VERSION}")
set(SEPA_VIEWER_VERSION_GIT_DESCRIBE "")
set(SEPA_VIEWER_VERSION_DIRTY "0")

if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE _sepa_git_describe
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _sepa_git_describe_rc
    )
    if(_sepa_git_describe_rc EQUAL 0 AND _sepa_git_describe)
        set(SEPA_VIEWER_VERSION_GIT_DESCRIBE "${_sepa_git_describe}")
        set(SEPA_VIEWER_VERSION_STRING "${_sepa_git_describe}")
        if(_sepa_git_describe MATCHES ".*-dirty$")
            set(SEPA_VIEWER_VERSION_DIRTY "1")
        endif()
    endif()
endif()

message(STATUS "sepa-xml-viewer version: ${SEPA_VIEWER_VERSION_STRING}")

configure_file(
    "${CMAKE_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_BINARY_DIR}/include/sepa/version.h"
    @ONLY
)
