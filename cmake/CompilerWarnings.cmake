# Warnings-as-errors per plan/00-init-phase.md §3.7. Curated set per compiler.
#
# Apply to a target by linking against the sepa::warnings interface library:
#     target_link_libraries(my_target PRIVATE sepa::warnings)

set(_sepa_msvc_warnings
    /W4
    /WX
    /permissive-
    /w14242  # conversion: possible loss of data
    /w14254  # operator: conversion from larger bitfield to smaller
    /w14263  # member function does not override any base class virtual
    /w14265  # class has virtual functions but destructor is not virtual
    /w14287  # unsigned/negative constant mismatch
    /we4289  # nonstandard extension: loop control variable used outside loop
    /w14296  # expression is always 'boolean_value'
    /w14311  # pointer truncation
    /w14545  # expression before comma evaluates to a function not a value
    /w14546  # function call before comma missing argument list
    /w14547  # operator before comma has no effect; expected operator with side-effect
    /w14549  # operator before comma has no effect; did you intend ...?
    /w14555  # expression has no effect; expected expression with side-effect
    /w14619  # #pragma warning: there is no warning number
    /w14640  # construction of local static object is not thread-safe
    /w14826  # conversion from sign-extended to unsigned
    /w14905  # wide string literal cast to LPSTR
    /w14906  # string literal cast to LPWSTR
    /w14928  # illegal copy-initialization; more than one user-defined conversion
    # C4702 ("unreachable code") fires inside Qt's own headers (qmetatype.h,
    # qjsengine.h, qvariant.h) when MSVC instantiates Qt templates from the
    # QML-cache .cpp files qt_add_qml_module generates. /external:W0 doesn't
    # silence it because the warning is emitted at template-instantiation
    # time in our TU, not while compiling Qt's headers. The warning itself
    # is low-signal in template-heavy C++ (many template branches are
    # legitimately dead after optimization), and we still have GCC/Clang
    # equivalents catching real cases on the other matrix legs.
    /wd4702
)

set(_sepa_clang_warnings
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough
    -Werror
)

set(_sepa_gcc_warnings
    ${_sepa_clang_warnings}
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
)

add_library(sepa_warnings INTERFACE)
add_library(sepa::warnings ALIAS sepa_warnings)

if(MSVC)
    target_compile_options(sepa_warnings INTERFACE ${_sepa_msvc_warnings})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(sepa_warnings INTERFACE ${_sepa_clang_warnings})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(sepa_warnings INTERFACE ${_sepa_gcc_warnings})
endif()
