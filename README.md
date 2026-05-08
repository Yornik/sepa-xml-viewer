# SEPA XML Viewer

A cross-platform desktop **viewer** for SEPA payment XML messages — the ISO 20022 files European banks, PSPs, and businesses exchange (`pain.001`, `pain.008`, `camt.05x`, the SRTP `pain.013` / `pain.014` family, and friends).

## Who this is for

HR professionals, payroll administrators, accountants, and other office users who occasionally need to open a SEPA file their bank or payroll system produced and understand what is in it — without learning ISO 20022 jargon or installing a developer toolchain. The viewer ships as a native installer on Linux, macOS, and Windows and runs out of the box.

## What it does, and what it does not do

- It opens a SEPA XML file from disk and renders it as a navigable, plain-language summary.
- It shows the raw XML alongside the human-friendly view.
- It validates against the published ISO 20022 / EPC schemas and translates validation errors into plain language.
- It is **read-only**. It does not initiate, sign, edit, or alter payments.
- It runs **fully offline**. It does not make network requests, send telemetry, fetch updates, or download schemas at runtime.

The "fully offline" guarantee is enforced at build time — Qt is configured without its networking module and no networking dependency is permitted in the dependency manifest.

## Status — init phase (Phase 0)

This repository is in the development-infrastructure setup phase. There is no buildable application yet. The full plan for what Phase 0 delivers is at **[`plan/00-init-phase.md`](plan/00-init-phase.md)** — read it before contributing.

Definition of done for init phase: `git tag v0.0.1 && git push --tags` produces installers for Linux (`.deb`, `.rpm`, AppImage), macOS (`.dmg`), and Windows (NSIS `.exe` + `.zip`) on a draft GitHub Release, and `main` is green across the CI matrix.

## Platforms

Linux, macOS, Windows — all from one C++20 / Qt 6 codebase.

## Tech stack

| Concern              | Choice                                          |
| -------------------- | ----------------------------------------------- |
| Language             | C++20                                           |
| Build system         | CMake 3.25+ with `CMakePresets.json`            |
| Dependency manager   | vcpkg (manifest mode)                           |
| GUI framework        | Qt 6 (Qt Quick / QML), dynamically linked, network feature disabled |
| XML library          | pugixml (parser); Xerces-C++ for XSD validation in Phase 2 |
| Test frameworks      | Catch2 v3 (unit + integration), Qt Test (GUI smoke) |
| Lint / format        | clang-format, clang-tidy, warnings-as-errors    |
| CI / packaging       | GitHub Actions matrix; CPack + AppImage + NSIS  |

Reasoning for each choice is in [`plan/00-init-phase.md`](plan/00-init-phase.md) §3.

## Building (after PR #2 lands)

```sh
cmake --preset dev-debug
cmake --build --preset dev-debug
ctest --preset dev-debug --output-on-failure
```

The build system itself arrives in PR #2 of the [§12 sequencing breakdown](plan/00-init-phase.md). Until then, this repository is documentation only.

## SEPA standards reference

The viewer is built against published standards rather than guesses:

- **ISO 20022** — message catalogue at https://www.iso20022.org (administered by SWIFT). Canonical XSDs and current message versions live there (e.g. `pain.001.001.13`, `pain.008.001.12`).
- **European Payments Council (EPC)** — implementation guidelines at https://www.europeanpaymentscouncil.eu, including the SRTP V4.0 trio (EPC258-22 Payee, EPC259-22 Inter-RTP, EPC260-22 Payer). All canonical URLs are listed in [`plan/00-init-phase.md`](plan/00-init-phase.md) §2.3.1.

## Example data

A placeholder-marked but schema-shaped SEPA Credit Transfer file (`pain.001.001.13`) lives in [`tests/example-xml/`](tests/example-xml/). See [`tests/example-xml/README.md`](tests/example-xml/README.md) for the placeholder convention. **No real bank data is committed to this repository.**

## License

[GPL-3.0-or-later](LICENSE).

## See also

- [`plan/00-init-phase.md`](plan/00-init-phase.md) — design, decisions, and definition of done.
- [`CLAUDE.md`](CLAUDE.md) — onboarding for contributors and AI agents.
- [`CHANGELOG.md`](CHANGELOG.md) — release notes.
