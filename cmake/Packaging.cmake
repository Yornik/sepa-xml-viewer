# Packaging via CPack. Per platform:
#
#   Linux:   .deb + .rpm via CPack. AppImage is a separate post-CPack step
#            driven by packaging/linux/build-appimage.sh (executed by the
#            release workflow, not by `cmake --install` or `cpack` alone).
#   macOS:   DragNDrop (.dmg) wrapping the .app bundle (MACOSX_BUNDLE=ON on
#            the executable target). macdeployqt copies the Qt frameworks
#            and QML modules into the bundle at install time.
#   Windows: NSIS installer (.exe) plus a portable ZIP. windeployqt copies
#            Qt DLLs and QML modules next to the executable at install time.
#
# All three packagers ship the binary plus the Qt runtime so the resulting
# artefact installs and runs on a vanilla machine without any system Qt.

# Common metadata used by every generator.
set(CPACK_PACKAGE_NAME "sepa-xml-viewer")
set(CPACK_PACKAGE_VENDOR "Yornik")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Cross-platform offline viewer for SEPA payment XML messages")
set(CPACK_PACKAGE_DESCRIPTION
    "A read-only desktop viewer for SEPA / ISO 20022 payment XML files. \
Runs fully offline; never makes network requests; never edits or signs \
payment data. Targets HR, payroll, and accounting users who occasionally \
need to inspect a SEPA file from their bank or payroll system.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Yornik/sepa-xml-viewer")
# Prefer the git-derived version (set by cmake/ProjectVersion.cmake from
# `git describe --tags --dirty --always`, with the leading 'v' stripped) so
# artefact filenames read 'sepa-xml-viewer-0.0.1-...' rather than
# 'sepa-xml-viewer-0.0.0-...' (which is the project() VERSION baseline).
# Falls back to PROJECT_VERSION when git describe wasn't usable.
if(SEPA_VIEWER_PACKAGE_VERSION)
    set(CPACK_PACKAGE_VERSION "${SEPA_VIEWER_PACKAGE_VERSION}")
else()
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
endif()
set(CPACK_PACKAGE_CONTACT "Yornik <yornik@example.invalid>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# Stable filenames per generator: sepa-xml-viewer-<version>-<system>-<arch>.<ext>.
set(CPACK_PACKAGE_FILE_NAME "sepa-xml-viewer-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_PACKAGE_NAME "SEPA XML Viewer")
    set(CPACK_NSIS_DISPLAY_NAME "SEPA XML Viewer")
    set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "SEPA XML Viewer")

    # Run windeployqt during install so the Qt DLLs and QML modules end up
    # next to the binary in the install prefix.
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS ${Qt6_DIR}/../../../bin)
    if(WINDEPLOYQT_EXECUTABLE)
        install(CODE "execute_process(COMMAND
            \"${WINDEPLOYQT_EXECUTABLE}\"
            --release --no-translations --no-system-d3d-compiler --no-opengl-sw
            --qmldir \"${CMAKE_SOURCE_DIR}/src/ui/qml\"
            \"\${CMAKE_INSTALL_PREFIX}/bin/sepa-xml-viewer.exe\"
        )")
    else()
        message(WARNING "windeployqt not found; the Windows package will be missing Qt runtime files")
    endif()
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "SEPA XML Viewer ${PROJECT_VERSION}")
    set(CPACK_BUNDLE_NAME "SEPA XML Viewer")

    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt
        HINTS ${Qt6_DIR}/../../../bin)
    if(MACDEPLOYQT_EXECUTABLE)
        install(CODE "execute_process(COMMAND
            \"${MACDEPLOYQT_EXECUTABLE}\"
            \"\${CMAKE_INSTALL_PREFIX}/sepa-xml-viewer.app\"
            -qmldir=\"${CMAKE_SOURCE_DIR}/src/ui/qml\"
        )")
    else()
        message(WARNING "macdeployqt not found; the macOS .dmg will be missing Qt frameworks")
    endif()
elseif(UNIX)
    set(CPACK_GENERATOR "DEB;RPM")

    # DEB metadata.
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "office")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
    # Auto-discover shared-library dependencies. With vcpkg's static Qt the
    # binary depends on system libs (libstdc++, libgcc, X11/xkb, etc.) which
    # this picks up. With a dynamic-Qt build, vcpkg's libQt6*.so files do
    # not have package homes and shlibdeps would fail; see plan/00 §9.2.
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

    # RPM metadata.
    set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Office")
    set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_RPM_PACKAGE_AUTOREQ ON)

    # Linux .desktop file lands at /usr/share/applications/. The icon at
    # /usr/share/icons/hicolor/scalable/apps/ is the standard XDG location
    # linuxdeploy and desktop environments scan for application icons.
    install(FILES "${CMAKE_SOURCE_DIR}/packaging/linux/sepa-xml-viewer.desktop"
        DESTINATION share/applications)
    install(FILES "${CMAKE_SOURCE_DIR}/packaging/linux/sepa-xml-viewer.svg"
        DESTINATION share/icons/hicolor/scalable/apps)
endif()

include(CPack)
