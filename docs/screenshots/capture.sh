#!/usr/bin/env bash
set -euo pipefail

BIN="$(pwd)/build/dev-debug/bin/sepa-xml-viewer"
OUT="$(pwd)/docs/screenshots"
mkdir -p "$OUT"

if [[ ! -x "$BIN" ]]; then
    echo "Binary not found: $BIN" >&2
    exit 1
fi

# Capture the main app window (not the root Xvfb canvas) so screenshots are
# always sized to the actual window, not the virtual desktop. Finds the
# window by title prefix via xwininfo; passes its id to `import -window`.
capture() {
    local out="$1"; shift
    local wait="$1"; shift
    echo "  → $out"
    xvfb-run -a --server-args="-screen 0 1280x820x24" bash -c "
        export QT_QPA_PLATFORM=xcb
        '$BIN' \"\$@\" 2>/dev/null &
        gui_pid=\$!
        sleep $wait
        # The main window's title always starts with 'SEPA XML Viewer'.
        # xwininfo's -tree prints lines like:
        #     0xNN \"SEPA XML Viewer — /path/to/file\": (...)  WxH+X+Y  ...
        # The 1x1+0+0 sibling is a hidden Qt utility window; skip it.
        win=\$(xwininfo -root -tree | awk '/\"SEPA XML Viewer/ && !/1x1\\+0\\+0/ {print \$1; exit}')
        if [[ -z \"\$win\" ]]; then
            echo 'ERROR: could not find SEPA XML Viewer window' >&2
            kill \$gui_pid 2>/dev/null || true
            exit 1
        fi
        import -window \"\$win\" '$out'
        kill -TERM \$gui_pid 2>/dev/null || true
        wait \$gui_pid 2>/dev/null || true
    " _ "$@"
}

echo "Capturing screenshots →"

# 1) Drop-here screen (no fixture; default tab; Material+Indigo).
capture "$OUT/01-drop-here.png" 3

# 2) Tree + Detail with the 5-tx narrative fixture (payroll + suppliers).
capture "$OUT/02-payroll-and-suppliers.png" 4 \
    "$(pwd)/tests/example-xml/pain.001.001.13-payroll-and-suppliers.xml"

# The 2500-tx stress fixture renders correctly when launched manually (see
# the user's confirmation) but the headless xvfb-run capture path produces a
# black image for it — likely because Xvfb's software GL is starved by the
# model construction in the same tick the screenshot grabs. Skipping; the
# narrative fixture is the better hero screenshot anyway.

echo
echo "Done. Files:"
ls -lh "$OUT"
