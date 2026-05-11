# CLAUDE.md — sepa-xml-viewer

This file orients Claude (and any other AI agent or new human contributor) the first time they work in this repository. Read it end-to-end before making changes; it is short on purpose.

## What this project is

A modern, cross-platform desktop GUI application for viewing SEPA (Single Euro Payments Area) payment messages — the ISO 20022 XML files that European banks, PSPs, and businesses exchange. Phase 2 onward, the viewer parses files like `pain.001` credit-transfer initiations, `pain.008` direct-debit initiations, `camt.05x` account reports, and the `pain.013`/`pain.014`/`pacs.028`/`camt.055`/`camt.029` family used by SEPA Request-to-Pay (SRTP).

The viewer is read-only. It does not initiate payments, edit messages, or talk to bank APIs. Its job is to make a SEPA XML file legible — tree view, detail pane, schema validation report, raw XML with syntax highlighting.

Target platforms: **Linux**, **macOS**, **Windows** — all from one codebase.

## Project status

**Init phase (Phase 0).** No application code yet beyond what the init PRs introduce. The full plan for what Phase 0 delivers is at [`plan/00-init-phase.md`](plan/00-init-phase.md). Read that before you start any infrastructure work — it defines the scope fence and the Definition of Done.

If you find yourself building SEPA parsing logic during the init phase, stop. That belongs to Phase 2, which has not been planned yet.

## Toolchain at a glance

| Concern              | Choice                                          |
| -------------------- | ----------------------------------------------- |
| Language             | C++20                                           |
| Build system         | CMake 3.25+ with `CMakePresets.json`            |
| Dependency manager   | vcpkg (manifest mode)                           |
| GUI framework        | Qt 6 (Qt Quick / QML), dynamically linked       |
| XML library          | pugixml (Phase 2+); Xerces-C++ for XSD validation later |
| Test frameworks      | Catch2 v3 (unit + integration), Qt Test (GUI smoke) |
| Lint / format        | clang-format, clang-tidy, warnings-as-errors    |
| CI / packaging       | GitHub Actions (matrix: Linux/macOS/Windows), CPack + AppImage + NSIS |

The reasoning behind each of these is in [`plan/00-init-phase.md`](plan/00-init-phase.md) §3. Do not change any of them without updating that section first.

## Where things live

- `src/app/` — the application entry point. Phase 0 has just enough code to open a window.
- `src/core/` — SEPA parser library (Phase 2+). Empty or skeleton in Phase 0.
- `src/ui/qml/` — QML files for the GUI.
- `tests/unit/`, `tests/integration/`, `tests/gui/` — three test layers, all run in CI.
- `cmake/` — CMake helper modules (warnings, packaging, version-from-git).
- `packaging/{linux,macos,windows}/` — platform-specific resources (icons, `.desktop`, `Info.plist`, NSIS template).
- `plan/` — design documents. Always check here before designing something new.
- `.github/workflows/` — `ci.yml` (every push/PR) and `release.yml` (tag-triggered).

## Conventions an agent must follow

1. **Edit existing files before creating new ones.** The repo is small; if you find yourself adding a third file with overlapping responsibility, you are doing it wrong.
2. **No new top-level documentation files** unless asked. README, CHANGELOG, CLAUDE.md, and the `plan/` series cover everything an agent needs.
3. **Cross-platform first.** Any code path that uses POSIX-only or Windows-only APIs has to be wrapped behind a platform abstraction (or, better, replaced with a Qt API that works on all three).
4. **Three test layers.** A new feature in `src/core/` lands with a unit test. A feature that touches the filesystem or the binary's CLI lands with an integration test. A feature visible in the UI lands with a Qt Test smoke check.
5. **Warnings are errors.** Do not silence warnings with pragmas or attribute hacks. Fix the root cause.
6. **No SEPA test fixtures with real account data.** Use ISO 20022 catalogue samples or synthesize values. IBANs go through a checksum-valid generator if one is needed.
7. **Do not vendor third-party schemas (XSDs) into the repo.** A `tools/fetch-schemas.sh` script downloads them on demand into `third_party/schemas/` (gitignored). License terms vary; we re-check them at fetch time.
8. **Do not bypass CI gates.** Branch protection requires green builds. If a check is wrong, fix the check; don't `--no-verify` or admin-merge.
9. **Versioning is `git describe`-driven.** Do not hand-edit a version constant. The build pipeline injects it.
10. **Comments explain *why*, not *what*.** If a comment restates the code, delete it.

## How to build (once Phase 0 PRs land)

```sh
# One-time: install vcpkg and Ninja, set VCPKG_ROOT.
cmake --preset dev-debug
cmake --build --preset dev-debug
ctest --preset dev-debug --output-on-failure
```

Replace `dev-debug` with `ci-linux`, `ci-macos`, or `ci-windows` to match what CI runs. The full preset list lives in `CMakePresets.json`.

## Standards references (do not guess at SEPA)

The viewer is built against published standards, not assumptions. The authoritative sources are:

- **ISO 20022** — message catalogue at https://www.iso20022.org (administered by SWIFT). This is where the canonical XSDs and message versions live (e.g. `pain.001.001.13`, `pain.008.001.12`).
- **European Payments Council (EPC)** — implementation guidelines at https://www.europeanpaymentscouncil.eu. The EPC publishes the SEPA-specific business rules layered on top of ISO 20022, including the SRTP V4.0 trio (EPC258-22 Payee, EPC259-22 Inter-RTP, EPC260-22 Payer) — six documents in total (one IG PDF + one XSD bundle per role). All six URLs are listed in `plan/00-init-phase.md` §2.3.1.

If you need to make a claim about what a SEPA message should contain, cite one of those sources. Do not paraphrase them from memory.

## What "done" looks like for the init phase

`git tag v0.0.1 && git push --tags` produces installers for Linux (.deb, .rpm, AppImage), macOS (.dmg), and Windows (NSIS .exe + .zip) on a draft GitHub Release, every artifact's `--version` smoke check passes, and `main` is green across the matrix. The full criteria checklist is in [`plan/00-init-phase.md`](plan/00-init-phase.md) §11.

## Asking before acting

- Treat anything that publishes outside the local working copy (push, tag, release, signed artifact) as user-confirmation territory. Plan locally first; ask before pushing.
- If you discover unfamiliar files, branches, or in-progress work, investigate before deleting or overwriting.
- If a build fails, fix the root cause. Do not bypass with `--no-verify`, `clean -f`, or `reset --hard` to make the symptom go away.
