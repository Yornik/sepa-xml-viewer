#!/usr/bin/env bash
# Build an AppImage on top of an already-installed sepa-xml-viewer.
#
# Usage:
#   cd build/<preset>
#   ../../packaging/linux/build-appimage.sh
#
# Expects:
#   - cmake --install . --prefix AppDir   (run before this script)
#   - linuxdeploy + linuxdeploy-plugin-qt on PATH (the release workflow
#     downloads them as binaries from their respective GitHub releases)
#
# Output:
#   sepa-xml-viewer-${PROJECT_VERSION}-x86_64.AppImage  in cwd

set -euo pipefail

if [[ ! -d AppDir ]]; then
    echo "AppDir/ not found. Run 'cmake --install . --prefix AppDir' first." >&2
    exit 1
fi

if ! command -v linuxdeploy >/dev/null; then
    echo "linuxdeploy not on PATH. Install it from:" >&2
    echo "  https://github.com/linuxdeploy/linuxdeploy/releases" >&2
    exit 1
fi

if ! command -v linuxdeploy-plugin-qt >/dev/null; then
    echo "linuxdeploy-plugin-qt not on PATH. Install it from:" >&2
    echo "  https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases" >&2
    exit 1
fi

# linuxdeploy expects the .desktop file and an icon next to the binary, plus
# Qt-specific paths supplied by the qt plugin.
linuxdeploy \
    --appdir AppDir \
    --executable AppDir/bin/sepa-xml-viewer \
    --desktop-file AppDir/share/applications/sepa-xml-viewer.desktop \
    --plugin qt \
    --output appimage
