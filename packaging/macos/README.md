# macOS packaging assets

Phase 0 ships a `.dmg` via CPack's `DragNDrop` generator + `macdeployqt`.
No custom volume background, no `.icns`, no Apple-Developer-signed
notarization — see `plan/00-init-phase.md` §10 for the deferred-signing
rationale.

Phase 2/4 will add:
- A real `.icns` for the bundle
- A bespoke `Info.plist.in` (currently using CMake-generated defaults)
- `codesign` + `xcrun notarytool` steps gated on `MACOS_SIGNING_IDENTITY`
  + Apple Developer notarization secrets
