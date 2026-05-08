add_executable(sepa-xml-viewer
    main.cpp
)

target_include_directories(sepa-xml-viewer
    PRIVATE
        ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(sepa-xml-viewer
    PRIVATE
        sepa::warnings
)

set_target_properties(sepa-xml-viewer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
