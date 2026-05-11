# Windows packaging assets

Phase 0 ships an NSIS installer + portable ZIP via CPack's defaults plus
`windeployqt`. No custom installer banner, no `.ico`, no Authenticode
signing — see `plan/00-init-phase.md` §10 for the deferred-signing
rationale.

Phase 2/4 will add:
- A real `.ico` for the installer and the binary
- `installer.nsi.in` template if we want custom install pages
  (uninstaller hook, file associations, Start Menu entry)
- `signtool sign` step gated on `WINDOWS_PFX_BASE64` + `WINDOWS_PFX_PASSWORD` secrets
