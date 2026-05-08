# Init Phase Plan — sepa-xml-viewer

Status: draft, 2026-05-08
Owner: @yornik
Scope of this document: Phase 0 only — establish the development infrastructure required to build, test, package, and ship a cross-platform C++ GUI application end-to-end. Actual SEPA parsing logic and GUI feature work are explicitly out of scope here and live in later plans.

---

## 1. Goals and Non-Goals

### Goals (Phase 0 — what this plan delivers)

1. A C++ project skeleton that builds out-of-source on Linux, macOS, and Windows with a single `cmake --preset …` invocation.
2. Dependency management via vcpkg in manifest mode, with binary caching configured for CI so Qt does not rebuild from source on every run.
3. A "hello viewer" GUI application that opens, displays a placeholder window, and exits cleanly. No SEPA logic — this is the smoke target the entire pipeline ships.
4. A working test harness with three layers wired in from day one:
   - Unit tests (Catch2)
   - Integration tests (Catch2 + on-disk fixtures, even if the only fixture is a trivial XML stub)
   - GUI smoke test (Qt Test, launches the app and verifies the main window appears)
5. Quality gates enforced in CI: `clang-format` check, `clang-tidy` (curated checks, warnings-as-errors on touched code), compiler warnings as errors across all three platforms.
6. A CI workflow on every push and pull request that runs the full matrix (Linux, macOS, Windows) and is required to be green before merge.
7. A release workflow triggered by `v*` tags that produces native installers on each platform and uploads them to a GitHub Release.
8. CLAUDE.md and a `plan/` directory committed so future contributors (human or agent) can pick up context.
9. **Offline-only by build-time guarantee.** The application binary links no networking libraries; Qt is configured without its `network` feature in the dependency manifest. No HTTP client, no telemetry, no auto-update, no runtime schema fetch. The constraint lives in `vcpkg.json` and is enforced at every CI build, not just in user-facing copy.

### Non-Goals (deferred to later phases — do NOT build now)

- Any SEPA / ISO 20022 parsing, schema validation, or business logic.
- The actual viewer UX (tree pane, detail pane, search, theming).
- Code signing and Apple notarization (see §10 — flagged with rationale).
- Localization, accessibility audits, or performance profiling.
- Auto-update / Sparkle / WinSparkle integrations.
- Linux distro repositories (PPA, Flatpak, Snap). The release pipeline produces installable artifacts; pushing them to third-party repos is a later decision.
- Codecov, Coveralls, CodeQL, fuzzing infrastructure. They are not high-value gates at this stage and add CI noise.
- **Networking of any kind.** No HTTP/HTTPS client, no WebSocket, no auto-update fetcher, no telemetry collector, no runtime schema downloader. The viewer's job is to read a file from disk and render it; the network is not part of that job. See Goal #9 — this is enforced at the dependency manifest, not just policy.

The single sentence of definition: **`git tag v0.0.1 && git push --tags` produces installers for Linux, macOS, and Windows on a GitHub Release, and `main` is green across the matrix.** Everything in this plan exists to make that one sentence true.

### Target audience

The primary user is **a non-technical office worker** — HR professionals, payroll administrators, accountants, finance staff — who occasionally needs to open a SEPA file their bank or payroll system produced and understand what is in it. They are not programmers, do not know XPath, and have not read the ISO 20022 specification. They want to open a file, see "who paid whom what, when," and trust the tool not to break their machine.

This audience drives several Phase 1+ decisions, captured here so they are not re-derived later:

- **Plain-language UI by default**, with an opt-in technical mode that exposes raw ISO 20022 element names. Examples: "Sender" instead of "Debtor", "Recipient" instead of "Creditor", "€123.45" instead of "EUR 123.45", "15 May 2026" instead of "2026-05-15".
- **Drag-and-drop file open** as the primary entry point. File-Open dialog as a fallback, command-line file argument as a tertiary path.
- **Validation errors translated into plain language**, e.g. "the sender's account number (IBAN) failed its checksum — it may have been mistyped" instead of "element `IBAN` violates xs:pattern restriction `[A-Z]{2}[0-9]{2}[a-zA-Z0-9]{1,30}`".
- **No CLI surface as user experience**. `--version` exists only for the release-pipeline smoke gate; the audience does not use the terminal.
- **Localized formatting** — currency, date, number formatting follow the OS locale. A US/UK user sees "£" or "$" in their own conventions; a German user sees "€1.234,56".
- **Qt Quick (QML) reinforced as the GUI choice** over Qt Widgets — the audience benefits from polished modern controls and theming rather than the developer-tool aesthetic Qt Widgets produces by default. See §3.4 for the full reasoning.
- **Internationalization** via Qt Linguist (English plus at least one major EU language at first) is part of the Phase 3 commitment, not Phase 2.
- **Accessibility is not optional** when the audience includes office users on managed devices; screen-reader support, keyboard navigation, and font scaling are tracked as Phase 2 deliverables, not "nice-to-haves."
- **Trust signals matter more than functionality at first impression.** A real icon (not a placeholder), a code-signed installer (eventually), a clear "this app does not access the internet" message, and an unambiguous "read-only" status visible in the UI all build the trust the audience needs to actually open a payroll file.

Init phase (this plan) does **not** deliver any of these features. It records them so Phase 1+ planning starts from the right premise.

---

## 2. The SEPA Standard — Verified References

We do not invent or guess at SEPA message formats. The viewer will be developed against the published, authoritative artifacts. These are listed here so the parsing phase has a known starting point and so that any vendored sample data is sourced from a real published document.

### 2.1 Standards bodies

- **ISO 20022** — the underlying XML messaging standard. The catalogue and message definitions are administered by SWIFT and published at https://www.iso20022.org. Production SEPA XML uses the default ISO 20022 namespaces.
- **European Payments Council (EPC)** — publishes the SEPA-specific Implementation Guidelines (IGs) layered on top of ISO 20022 at https://www.europeanpaymentscouncil.eu. The EPC also publishes "Technical Validation Subset" (TVS) XSDs that encode the IG business rules; these are intended for validation, not for production namespace use.

### 2.2 Current message versions (verified 2026-05-08 against the ISO 20022 catalogue)

| Message              | Current version    | Purpose                                         |
| -------------------- | ------------------ | ----------------------------------------------- |
| `pain.001.001.13`    | current            | Customer Credit Transfer Initiation (SCT)       |
| `pain.002.001.15`    | current            | Customer Payment Status Report                  |
| `pain.007.001.13`    | current            | Customer Payment Reversal                       |
| `pain.008.001.12`    | current            | Customer Direct Debit Initiation (SDD)          |
| `pain.013.001.10`    | current (SRTP V4)  | Creditor Payment Activation Request (Request-to-Pay) |
| `pain.014.001.07`    | current (SRTP V4)  | Creditor Payment Activation Request Status      |
| `pacs.028.001.03`    | current (SRTP V4)  | FI to FI Payment Status Request                 |
| `camt.029.001.09`    | current (SRTP V4)  | Resolution of Investigation                     |
| `camt.052.001.x`     | TBD in Phase 1     | Bank-to-Customer Account Report                 |
| `camt.053.001.x`     | TBD in Phase 1     | Bank-to-Customer Statement                      |
| `camt.054.001.x`     | TBD in Phase 1     | Bank-to-Customer Debit/Credit Notification      |
| `camt.055.001.08`    | current (SRTP V4)  | Customer Payment Cancellation Request           |

The exact `camt.05x` versions in current EPC IGs will be confirmed when the parsing phase starts; do not vendor any XSDs based on guesses.

### 2.3 Current EPC implementation guidelines (2025 V1.0 / SRTP V4.0)

- SEPA Credit Transfer (SCT) Customer-to-PSP IG — 2025 v1.0
- SEPA Credit Transfer Inter-PSP IG — 2025 v1.0
- SEPA Direct Debit Core / B2B Customer-to-PSP IG — 2025 v1.0
- SEPA Direct Debit Core Inter-PSP IG — 2025 v1.0
- SEPA Instant Credit Transfer Inter-PSP IG — 2025 v1.0
- SEPA Request-to-Pay (SRTP) IG — V4.0, three-role document set (Payee + Inter-RTP + Payer)

#### 2.3.1 SRTP V4.0 reference document set (all six artifacts retrieved 2026-05-08)

The SRTP scheme is documented across three EPC document numbers — Payee-side (EPC258-22), Inter-RTP / PSP-to-PSP (EPC259-22), and Payer-side (EPC260-22) — each published as both an Implementation Guidelines PDF and a TVS XSD bundle ZIP. All six are listed below; they constitute the complete authoritative artifact set for SRTP and are the working reference for Phase 1 parser implementation.

| # | Document                                              | Format | Source                                                                                                                                                                                                                                  |
| - | ----------------------------------------------------- | ------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1 | EPC258-22 Payee–Payee's RTP SP SRTP IG V4.0           | PDF    | https://www.europeanpaymentscouncil.eu/sites/default/files/kb/file/2025-06/EPC258-22%20Payee-Payee%27s%20RTP%20SP%20SRTP%20IG%20V4.0.pdf                                                                                                |
| 2 | EPC258-22 Payee–Payee's RTP SP SRTP IG V4.0 XSDs      | ZIP    | https://www.europeanpaymentscouncil.eu/sites/default/files/kb/file/2025-06/EPC258-22%20Payee-Payee%27s%20RTP%20SP%20SRTP%20IG%20V4.0%20XSD.zip                                                                                          |
| 3 | EPC259-22 Inter-RTP SP SRTP IG V4.0                   | PDF    | https://www.europeanpaymentscouncil.eu/sites/default/files/kb/file/2025-06/EPC259-22%20Inter-RTP%20SP%20SRTP%20IG%20V4.0.pdf                                                                                                            |
| 4 | EPC259-22 Inter-RTP SP SRTP IG V4.0 XSDs              | ZIP    | https://www.europeanpaymentscouncil.eu/sites/default/files/kb/file/2025-06/EPC259-22%20Inter-RTP%20SP%20SRTP%20IG%20V4.0%20XSD.zip                                                                                                      |
| 5 | EPC260-22 Payer–Payer's RTP SP SRTP IG V4.0           | PDF    | https://www.europeanpaymentscouncil.eu/sites/default/files/kb/file/2025-06/EPC260-22%20Payer-Payer%27s%20RTP%20SP%20SRTP%20IG%20V4.0.pdf                                                                                                |
| 6 | EPC260-22 Payer–Payer's RTP SP SRTP IG V4.0 XSDs      | ZIP    | https://www.europeanpaymentscouncil.eu/sites/default/files/kb/file/2025-06/EPC260-22%20Payer-Payer%27s%20RTP%20SP%20SRTP%20IG%20V4.0%20XSD.zip                                                                                          |

#### 2.3.2 Datasets covered by SRTP V4.0 (extracted from the XSD bundles)

The three roles do not duplicate the same XSDs — each role exposes the subset of datasets it originates or receives. Combined coverage across all six artifacts:

- **Payee role (EPC258, 15 XSDs)** — DS01 (`pain.013.001.10`), DS04a/DS04b+ (`pain.014.001.07`), DS06/DS06b (`pain.014.001.07`), DS09N/DS09P, DS10 (`camt.055.001.08`), DS10b (`camt.029.001.09`), DS13N/DS13P, DS14RFC (`camt.055.001.08`), DS14RTP (`pacs.028.001.03`), DS17RFC/DS17RTP.
- **Inter-RTP role (EPC259, 12 XSDs)** — DS02 (`pain.013.001.10`), DS04b/DS05 (`pain.014.001.07`), DS08N/DS08P, DS11 (`camt.055.001.08`), DS12N/DS12P (`camt.029.001.09`), DS15RFC, DS15RTP (`pacs.028.001.03`), DS16RFC, DS16RTP.
- **Payer role (EPC260, 4 XSDs)** — DS03 (`pain.013.001.10`), DS07N/DS07P (`pain.014.001.07`), DS15b (`pacs.028.001.03`).

Practical consequence for the viewer: SRTP support requires the parser to recognize all three roles' datasets, but the underlying ISO 20022 message types reduce to five (`pain.013`, `pain.014`, `camt.055`, `camt.029`, `pacs.028`). Phase 1 will introduce these five plus the three "primary" SEPA messages (`pain.001`, `pain.002`, `pain.008`) and the bank-to-customer reporting trio (`camt.052`, `camt.053`, `camt.054`) — eleven message types total in the initial parser scope.

### 2.4 Compliance dates we must surface in the viewer (eventually)

- **15 November 2026** — unstructured address format may no longer be provided in EPC payment messages. The viewer should clearly flag unstructured addresses in messages dated on or after that day. (Implementation: Phase 2.)

### 2.5 Schema acquisition strategy

- During Phase 1 we will fetch the EPC TVS XSD bundles from the EPC document library and the ISO 20022 catalogue XSDs from iso20022.org.
- We will **not** vendor the XSDs into this repository until license terms have been re-checked — the EPC bundles are publicly downloadable but redistribution rights need confirmation. A `tools/fetch-schemas.sh` script downloading them on demand into `third_party/schemas/` (gitignored) is the working assumption.
- Sample test fixtures: prefer ISO 20022 catalogue sample messages (publicly available) over EPC documents for any committed fixtures. Anything sensitive-looking gets synthesized from scratch.

---

## 3. Technology Decisions

Each decision below states the choice, reasoning, and what would make us reverse it.

### 3.1 Language and standard

- **C++20**.
- Reasoning: concepts, ranges, `<format>`, designated initializers, three-way comparison — all well-supported by GCC 12+, Clang 15+, MSVC 19.34+ which are easily installed on every CI runner image we need. C++23 modules support is still uneven across toolchains, especially in CMake; not worth the risk in init phase.
- Reverse if: a hard dependency (Qt, vcpkg) drops support for C++20.

### 3.2 Build system

- **CMake 3.25+** with **CMakePresets.json** (schema version 6).
- Reasoning: the only sane cross-IDE / cross-CI orchestration option. Presets give us reproducible configure/build/test invocations across local dev and CI without bash glue.
- Sub-decision: out-of-source builds enforced via `cmake/PreventInSourceBuilds.cmake`.

### 3.3 Dependency management

- **vcpkg in manifest mode** (`vcpkg.json` at repo root, `VCPKG_ROOT` toolchain file integrated via preset).
- Reasoning: gitignore already references vcpkg; deep Qt + pugixml + Catch2 dependency management on three platforms with one config file. Manifest mode pins versions per repo, avoiding "works on my machine."
- **Critical**: enable vcpkg **binary caching** in CI from day one — without it, Qt builds from source on every CI run (45+ minutes per platform). We will use the GitHub Actions cache backend (`vcpkg/vcpkg.cmake` reads `VCPKG_BINARY_SOURCES` env var). Configured in the workflow, not the manifest.

### 3.4 GUI framework — Qt 6 (Qt Quick / QML)

- **Qt 6.7+** with **Qt Quick (QML)** as the primary UI layer; C++ provides the parser and exposes models to QML via `Q_PROPERTY` / `QAbstractItemModel`.
- Reasoning: the user asked for a "modern GUI viewer." Qt Quick is GPU-accelerated, declarative, supports modern theming (Material, Universal, Fusion, Basic), animations, and high-DPI handling out of the box. The target audience (HR / non-technical office users — see §1 *Target audience*) reinforces this choice: Qt Widgets produces an aesthetic that reads as a developer tool, while Qt Quick's modern controls and theming are closer to what office staff expect from polished desktop software they use daily. Qt Widgets remains the more conservative power-user toolkit but produces an aesthetic that reads as legacy on Windows 11 and macOS Sequoia. Qt 6 ships an upgraded `TreeView` and table primitives in QML which were the historical reasons to fall back to Widgets.
- **Linking: dynamic only.** Reasoning: Qt LGPL v3 permits dynamic linking without source-availability obligations on the application code. Static linking would require either a commercial Qt license or shipping object files for relinking — neither acceptable for an open-source-friendly init phase. Packages will bundle the required Qt runtime libraries via CPack and `windeployqt` / `macdeployqt`.
- **`qtbase` configured without the `network` feature** in `vcpkg.json`. This enforces the offline-only guarantee from §1 (Goal #9) at the dependency-manifest level — `QNetworkAccessManager` and friends are simply not linked into the binary, so the offline property cannot regress through accidental imports during feature work. Any future CI lint rule or grep gate that reasserts this is welcome but not the load-bearing constraint; the manifest is.
- Reverse if: QML `TreeView` performance is unacceptable on >10MB SEPA files. Fallback is a `QQuickWidget` embedding `QTreeView` from Qt Widgets, or a full Widgets pivot. This decision is reversible at Phase 2 with no infrastructure churn — both stacks share the same build/CI/packaging plumbing.

### 3.5 XML library

- **pugixml** (vcpkg `pugixml`).
- Reasoning: header + small TU, very fast DOM traversal, simple API, no external dependencies. The viewer needs DOM-style random access (tree view, jump-to-line, XPath-ish queries) more than streaming. libxml2 is heavier and brings GLib-flavored ergonomics; rapidxml is lower-level and abandoned.
- For XSD validation specifically (Phase 2+), pugixml does **not** validate. We will introduce **Xerces-C++** (vcpkg `xerces-c`) at that point. Init phase does not depend on either.

### 3.6 Test frameworks

- **Catch2 v3** (vcpkg `catch2`) for unit and integration tests.
- **Qt Test** (ships with Qt) for the GUI smoke test.
- Reasoning: Catch2 v3 has a real library form (no longer header-only-explosion), great fixture support, plays nicely with CTest. Qt Test is the de facto choice for QML/GUI smoke tests because it understands the Qt event loop.

### 3.7 Quality tools

- **clang-format** (LLVM 17+) — configuration committed at `.clang-format`. Style: based on `LLVM` with column 100 and project-specific tweaks.
- **clang-tidy** (LLVM 17+) — configuration committed at `.clang-tidy`. Curated check set, not the firehose default.
- **Compiler warnings**: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror` (GCC/Clang) and `/W4 /WX` (MSVC). Selected via `cmake/CompilerWarnings.cmake`.

### 3.8 Versioning and changelog

- **Semantic versioning**, single-source from `git describe --tags --dirty` injected into CMake via configure-time discovery, then into `version.h` and the application about box.
- A **CHANGELOG.md** maintained in [Keep-a-Changelog](https://keepachangelog.com) format. Release notes generated by hand from CHANGELOG sections at tag time (auto-extraction is straightforward — just `awk` the tagged section).

---

## 4. Repository Layout

```
sepa-xml-viewer/
├── CMakeLists.txt                  # Top-level — version, options, subdirs
├── CMakePresets.json               # ci-linux, ci-macos, ci-windows, dev-*
├── vcpkg.json                      # Manifest: qtbase, qtdeclarative, pugixml, catch2
├── vcpkg-configuration.json        # Optional: registry overrides
├── .clang-format
├── .clang-tidy
├── .gitignore                      # already present
├── .gitattributes                  # line-ending normalization
├── .editorconfig
├── LICENSE                         # already present (GPLv3 from initial commit)
├── README.md                       # already present, expand in Phase 0
├── CLAUDE.md                       # agent / contributor onboarding
├── CHANGELOG.md
├── cmake/
│   ├── CompilerWarnings.cmake
│   ├── PreventInSourceBuilds.cmake
│   ├── ProjectVersion.cmake        # git-describe → version.h
│   └── Packaging.cmake             # CPack config: NSIS, DragNDrop, DEB, RPM
├── src/
│   ├── CMakeLists.txt
│   ├── app/
│   │   ├── main.cpp                # Phase 0: opens an empty QQmlApplicationEngine window
│   │   └── application.{h,cpp}
│   ├── core/                       # Phase 1+: SEPA parsing — empty in Phase 0
│   │   └── CMakeLists.txt          # placeholder library target
│   ├── ui/                         # QML + Qt resource files
│   │   ├── qml/
│   │   │   └── Main.qml            # Phase 0: empty ApplicationWindow with title
│   │   └── ui.qrc
│   └── version.h.in                # configured by ProjectVersion.cmake
├── tests/
│   ├── CMakeLists.txt
│   ├── unit/
│   │   └── test_smoke.cpp          # Catch2 sanity test (1 == 1)
│   ├── integration/
│   │   └── test_app_starts.cpp     # spawns the binary, asserts exit code
│   ├── gui/
│   │   └── tst_mainwindow.cpp      # Qt Test: launches, checks window visible
│   └── example-xml/                # hand-written placeholder SEPA XML
│       ├── README.md               # placeholder convention + file inventory
│       └── pain.001.001.13-credit-transfer.xml
├── docs/                           # placeholder; agents only write here when asked
├── plan/
│   └── 00-init-phase.md            # this document
├── packaging/
│   ├── linux/
│   │   ├── sepa-xml-viewer.desktop
│   │   └── icons/                  # PNG/SVG launcher icons
│   ├── macos/
│   │   ├── Info.plist.in
│   │   └── icon.icns
│   └── windows/
│       ├── installer.nsi.in        # NSIS template if we choose NSIS over WiX
│       └── icon.ico
├── third_party/
│   └── schemas/                    # gitignored; populated by tools/fetch-schemas.sh
└── .github/
    ├── workflows/
    │   ├── ci.yml                  # build + test matrix on push/PR
    │   └── release.yml             # tag-triggered packaging + GitHub Release
    ├── CODEOWNERS                  # @yornik for everything in init phase
    └── pull_request_template.md
```

Why this layout: `src/core` separated from `src/app`/`src/ui` so the parser can become a static library used by both the GUI and any future CLI / fuzzer / WASM target. `packaging/` separated from `cmake/` so platform-specific resources (icons, `.desktop` files, `Info.plist`) live with the platform they belong to, while `cmake/Packaging.cmake` reads them.

---

## 5. Build System Details

### 5.1 CMakePresets.json

Three configure presets — `ci-linux`, `ci-macos`, `ci-windows` — and their matching build/test presets. Plus `dev-debug` and `dev-release` for local work. All presets:

- Set `CMAKE_TOOLCHAIN_FILE` to the vcpkg toolchain file (via `VCPKG_ROOT` env).
- Set `VCPKG_TARGET_TRIPLET` per platform (`x64-linux`, `x64-osx` or `arm64-osx`, `x64-windows`).
- Enable `CMAKE_EXPORT_COMPILE_COMMANDS=ON` for clangd / clang-tidy.
- Pin generator: `Ninja` on Linux/macOS, `Ninja` on Windows too (we install Ninja in CI; we do not use the MSBuild generator because compile_commands and clang-tidy are second-class there).

### 5.2 vcpkg.json

```jsonc
{
  "name": "sepa-xml-viewer",
  "version": "0.0.0",
  "dependencies": [
    { "name": "qtbase", "default-features": false, "features": ["gui", "widgets"] },
    "qtdeclarative",
    "pugixml",
    { "name": "catch2", "version>=": "3.5.0" }
  ],
  "builtin-baseline": "<latest stable vcpkg commit pinned at init time>"
}
```

### 5.3 macOS architecture target

- **arm64 only** for the init phase. The GitHub Actions `macos-14` runner is Apple Silicon; producing an arm64-only `.dmg` keeps CI simple and within the binary cache.
- Universal binaries (`lipo` of x64 + arm64 builds) deferred to Phase 2 if user demand exists. Documented up front so this is a deliberate choice, not an oversight.
- Reverse if: a stakeholder confirms x64 Mac users.

---

## 6. Testing Strategy

Three layers, all wired up in CI from day one. Each gets its own subdirectory under `tests/` and its own CTest label so the matrix can run them independently if needed.

### 6.1 Unit tests (`tests/unit/`)

- Catch2 test executables.
- Each module under `src/core` ships its own `test_<module>.cpp` (Phase 1+).
- Phase 0 deliverable: a single `test_smoke.cpp` proving the test harness builds and runs on all three platforms. Acceptance is non-zero exit on failure plus CTest integration via `catch_discover_tests`.

### 6.2 Integration tests (`tests/integration/`)

- Drive the parser with on-disk fixture XML files in `tests/fixtures/`. Phase 0 ships exactly one trivial XML stub to exercise file I/O wiring.
- Phase 1+: one fixture per supported message type (pain.001, pain.008, camt.053, …). Sourced from ISO 20022 catalogue samples or synthesized; never copy-pasted from real production data.
- Test framework: Catch2 (same as unit). Distinction is: integration tests touch the filesystem, unit tests do not.
- Performance budget per test: 1 second. Larger end-to-end runs go under a separate `slow` label and are not gating.

### 6.3 GUI smoke tests (`tests/gui/`)

- Qt Test framework (`QTEST_MAIN`).
- Phase 0 deliverable: launch the application's `QQmlApplicationEngine`, wait for the main window's `Component.onCompleted`, take a screenshot, assert the window is visible and at least 400×300. Closes cleanly.
- Headless-friendly: on Linux CI, runs under `xvfb-run`. macOS / Windows runners have a real session and run unwrapped.
- Phase 1+: add a fixture-load smoke (open file dialog programmatically, load a sample XML, assert the tree pane has > 0 nodes).

### 6.4 What is NOT tested in Phase 0

- Performance / memory (no realistic workload yet).
- Fuzzing (Phase 2 candidate, with libFuzzer or AFL++).
- Snapshot / pixel-diff UI tests (overkill for a non-existent UI).

---

## 7. Quality Gates (run in CI, fail the build)

| Gate                       | Tool             | Where                           |
| -------------------------- | ---------------- | ------------------------------- |
| Code formatting            | `clang-format`   | `ci.yml` job: `lint` (Linux only) |
| Static analysis            | `clang-tidy`     | `ci.yml` job: `tidy` (Linux only, runs against `compile_commands.json` from the Linux build) |
| Compiler warnings as errors | GCC/Clang/MSVC  | every build job                 |
| Unit tests pass            | CTest            | every build job                 |
| Integration tests pass     | CTest            | every build job                 |
| GUI smoke test passes      | CTest + xvfb     | every build job                 |

Format and tidy run only on Linux to keep CI cost bounded; the toolchain produces consistent results across platforms. Gates are **required checks** in branch protection on `main` (manual configuration step at the end of init phase).

---

## 8. CI Workflow — `.github/workflows/ci.yml`

Triggered on `push` to `main` and on `pull_request`.

### 8.1 Jobs

1. **`lint`** — Ubuntu 22.04, runs `clang-format --dry-run --Werror` over `src/` and `tests/`. Independent of build; fast feedback.
2. **`build-test`** — matrix:
   ```yaml
   matrix:
     include:
       - { os: ubuntu-22.04, preset: ci-linux,   triplet: x64-linux }
       - { os: macos-14,     preset: ci-macos,   triplet: arm64-osx }
       - { os: windows-2022, preset: ci-windows, triplet: x64-windows }
   ```
   Each job:
   - Installs Ninja and the platform's Qt build prerequisites (e.g. `libgl1-mesa-dev` and `libxkbcommon-dev` on Linux).
   - Restores the vcpkg binary cache via `actions/cache` keyed on `vcpkg.json` + triplet hash.
   - Runs `cmake --preset <preset>`, `cmake --build --preset <preset>`, `ctest --preset <preset> --output-on-failure`.
3. **`tidy`** — Ubuntu 22.04, depends on Linux build artifacts (or re-runs the configure step), runs `clang-tidy` over changed files only on PR (full sweep on `main`).

### 8.2 Branch protection

Configured manually after the workflow lands:
- `lint`, all three `build-test` matrix legs, and `tidy` are required.
- Require PR reviews before merge: 0 in init phase (single contributor), bumped to 1 once a second contributor exists.
- Linear history enforced on `main`.

### 8.3 vcpkg binary caching — the make-or-break detail

Without caching, every CI run builds Qt from source: ~30–45 minutes per OS, every PR. With GitHub Actions cache as the binary source, a warm cache run takes 1–3 minutes for the dependency restore step and the rest is the application build.

Set in each matrix job:
```yaml
env:
  VCPKG_BINARY_SOURCES: "clear;x-gha,readwrite"
  ACTIONS_CACHE_URL: ${{ env.ACTIONS_CACHE_URL }}
  ACTIONS_RUNTIME_TOKEN: ${{ env.ACTIONS_RUNTIME_TOKEN }}
```
(`x-gha` is vcpkg's GitHub Actions binary cache provider; the two ACTIONS_* env vars are exposed by setting `permissions: actions: read` and using `actions/github-script` to forward them, or via the `lukka/run-vcpkg` action which handles this.)

The first push will populate the cache (~40 min). Every subsequent run is fast.

---

## 9. Release Workflow & Packaging — `.github/workflows/release.yml`

Triggered on push of a tag matching `v*.*.*`.

### 9.1 Jobs

Mirror of the CI build matrix, plus packaging and publish:

1. **`package`** matrix on Ubuntu / macOS / Windows:
   - Reuses the same vcpkg cache as CI.
   - Configures with `CMAKE_BUILD_TYPE=Release`, runs `cmake --build`, runs the test suite (release-mode regressions exist), then `cpack` with the platform-specific generator(s).
   - Uploads each generated artifact as a workflow artifact named `dist-<os>-<triplet>`.
2. **`publish`** (Ubuntu, runs after `package`):
   - Downloads all platform artifacts.
   - Reads the matching version section out of `CHANGELOG.md` to use as release notes.
   - Creates a GitHub Release for the tag, marks it as draft until manually promoted (init phase) then attaches every artifact.

### 9.2 Per-platform packaging

| Platform | CPack generator      | Artifact                               | Notes                              |
| -------- | -------------------- | -------------------------------------- | ---------------------------------- |
| Linux    | `DEB`                | `sepa-xml-viewer_X.Y.Z_amd64.deb`      | Depends on system Qt at install time? No — we ship Qt with the package. Sub-decision: **bundle Qt in `/opt/sepa-xml-viewer/` and patch RPATH**. Avoids distro-Qt-version incompatibility. |
| Linux    | `RPM`                | `sepa-xml-viewer-X.Y.Z-1.x86_64.rpm`   | Same bundling strategy.            |
| Linux    | (script-based)       | `sepa-xml-viewer-X.Y.Z-x86_64.AppImage` | Built via `linuxdeploy` + `linuxdeploy-plugin-qt` after CPack. |
| macOS    | `DragNDrop`          | `sepa-xml-viewer-X.Y.Z.dmg`            | `macdeployqt` runs in CMake post-install to bundle Qt frameworks. **Unsigned in init phase** — see §10. |
| Windows  | `NSIS`               | `sepa-xml-viewer-X.Y.Z-windows-x64.exe` | `windeployqt` runs in CMake post-install to gather Qt DLLs and QML modules. **Unsigned in init phase**. |
| Windows  | `ZIP`                | `sepa-xml-viewer-X.Y.Z-windows-x64.zip` | Portable fallback for users who don't want to run an unsigned installer. |

NSIS over WiX: NSIS scripts are easier to template and the resulting installers are smaller. WiX produces .msi which is preferred for enterprise deployment but is overkill for an init phase. Reversible without changing the rest of the pipeline.

### 9.3 Release artifacts smoke check

Each `package` job ends with a "does the artifact actually launch?" step:
- Linux: `dpkg -i` the .deb in a clean container, run `sepa-xml-viewer --version`, assert exit 0.
- macOS: mount the .dmg, copy the .app to `/Applications`, run the embedded binary with `--version`.
- Windows: silent-install the NSIS exe, run the binary with `--version`.

If `--version` doesn't return cleanly, the release job fails and the draft release is not published. Init phase implements `--version` — it's the cheapest possible CLI surface that proves the binary loads.

---

## 10. Code Signing and Notarization — Deferred (with rationale)

Real packages on macOS and Windows are expected to be signed. Without signing:

- **macOS**: Gatekeeper blocks the .dmg with "cannot be opened because the developer cannot be verified." Users must right-click → Open → confirm. Apple Silicon adds notarization-stapling on top — without it, even after the right-click bypass, the OS may quarantine the binary. Cost: $99/year Apple Developer Program + ~5–15 min per notarization run in CI.
- **Windows**: SmartScreen shows "Windows protected your PC" and requires the user to click "More info" → "Run anyway." A code-signing certificate from a public CA costs $200–600/year; an EV cert (which builds SmartScreen reputation faster) is more.

**Decision for init phase: ship unsigned, document the warning UX in the README.** The release workflow is structured so adding signing later is a per-platform plug:

- macOS: insert a `codesign` + `xcrun notarytool` step between `cpack` and the artifact upload, gated on a `MACOS_SIGNING_IDENTITY` repo secret being present.
- Windows: insert a `signtool sign` step; gated on `WINDOWS_PFX_BASE64` and `WINDOWS_PFX_PASSWORD` secrets.
- Linux: GPG-sign the .deb / .rpm with `dpkg-sig` / `rpmsign`. Far cheaper (free). Defer mostly because consumer Linux users rarely verify signatures and our distribution channel is GitHub Releases, not a repo.

**Action item:** before the first user-facing release (post-init), the maintainer decides whether to budget for signing or to live with the warnings. The plan does not commit to either.

---

## 11. Definition of Done — Init Phase

All of the following must be true. Each is independently verifiable; together they satisfy the one-sentence goal in §1.

1. ☐ Repository contains `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`, and the layout in §4.
2. ☐ `cmake --preset dev-debug && cmake --build --preset dev-debug && ctest --preset dev-debug` succeeds locally on Linux.
3. ☐ Equivalent commands succeed on macOS and Windows (verified at minimum via CI green run).
4. ☐ Running the built application opens a window titled "SEPA XML Viewer" with the version from `git describe`. It exits cleanly when closed.
5. ☐ `<binary> --version` prints the version and exits 0. Used by the release smoke checks.
6. ☐ `tests/unit/test_smoke.cpp`, `tests/integration/test_app_starts.cpp`, and `tests/gui/tst_mainwindow.cpp` all pass on every CI matrix leg.
7. ☐ `clang-format --dry-run --Werror` passes on the entire tree.
8. ☐ `clang-tidy` runs cleanly (no warnings) on the curated check set.
9. ☐ CI workflow is green on `main`.
10. ☐ Pushing a tag `v0.0.1` produces a draft GitHub Release with at least: a `.deb`, a `.rpm`, an `.AppImage`, a `.dmg`, an NSIS `.exe`, and a Windows `.zip`. Each artifact's `--version` smoke check passes.
11. ☐ Branch protection rules enabled on `main` requiring `lint`, all `build-test` matrix legs, and `tidy`.
12. ☐ `README.md` updated with: build instructions per platform, current package warning UX (unsigned), link to this plan.
13. ☐ `CLAUDE.md` exists at repo root with project overview, toolchain summary, conventions, and a "where to look first" section pointing into `plan/`.
14. ☐ `CHANGELOG.md` exists with an `Unreleased` section and a `0.0.1` entry describing the init phase deliverables.

---

## 12. Sequencing — Suggested PR Breakdown

Each item below is a separate PR. Order matters; each builds on the previous. Estimated effort assumes one engineer familiar with CMake/Qt/vcpkg.

| PR  | Scope                                                                                                       | Effort |
| --- | ----------------------------------------------------------------------------------------------------------- | ------ |
| #1  | Repository scaffolding: `.editorconfig`, `.clang-format`, `.clang-tidy`, `.gitattributes`, expanded `README.md`, `CLAUDE.md`, `CHANGELOG.md`. No CMake yet. | 0.5d   |
| #2  | `CMakeLists.txt` + `CMakePresets.json` + `vcpkg.json` + `cmake/*.cmake` modules. Hello-world `src/app/main.cpp` (no Qt yet) that prints version. Local build only. | 1d     |
| #3  | Add Qt 6 to vcpkg manifest, switch `main.cpp` to `QQmlApplicationEngine` + minimal `Main.qml`. Verify launches locally on Linux. | 1d     |
| #4  | Test harness: Catch2 unit + integration smoke tests, Qt Test GUI smoke. Wire up CTest. | 0.5d   |
| #5  | CI workflow `.github/workflows/ci.yml` with the matrix and vcpkg binary cache. First push will be slow as cache populates; subsequent runs fast. | 1d     |
| #6  | Quality gates: `clang-format` and `clang-tidy` jobs, warnings-as-errors. | 0.5d   |
| #7  | Packaging: `cmake/Packaging.cmake` driving CPack for DEB/RPM/DragNDrop/NSIS, AppImage script, `windeployqt`/`macdeployqt` integration. Local-only verification. | 1.5d   |
| #8  | Release workflow `.github/workflows/release.yml` with artifact smoke checks. Tag `v0.0.1` to validate the pipeline end-to-end. | 1d     |
| #9  | Documentation pass: README install instructions per platform with the unsigned-binary warning UX. Branch protection enabled. | 0.5d   |

Total ballpark: **~7.5 engineer-days** for init phase, dominated by CI/packaging work, not application code.

---

## 13. Future Phases (placeholders, not commitments)

- **Phase 1 — Core SEPA parser**: pugixml-backed model for pain.001/002/008, camt.053/054, SRTP message family. XSD validation deferred or layered with Xerces-C++. Read-only; no editing.
- **Phase 2 — Viewer UI for non-technical users** (per the audience defined in §1 *Target audience*): a "summary" view that renders messages in plain language ("Sender", "Recipient", "Amount", "When") with the underlying ISO 20022 element names exposed via a toggle; raw XML pane with syntax highlighting; tree pane for power users; localized currency / date / number formatting; drag-and-drop file open; recent files; dark / light theming; the "flag unstructured addresses past 2026-11-15" rule; plain-language validation error messages translating XSD violations. Accessibility (screen-reader, keyboard navigation, font scaling) lands here, not later.
- **Phase 3 — Power-user features and i18n**: multi-document tabs, export to CSV / JSON / PDF, validation report, schema-version detection, internationalization via Qt Linguist (English plus at least one major EU language).
- **Phase 4 — Distribution polish**: code signing, Apple notarization, Linux distro repos / package-manager presence (so updates flow through the OS-level package manager rather than an in-app updater). **Auto-update is explicitly NOT in scope** — it would violate the offline-only guarantee from §1 Goal #9. Users update by downloading a new installer or by their package manager.
- **Phase 5 — Optional**: CLI `sepa-xml validate`/`sepa-xml dump` companion binary (reuses `src/core` library), WASM build for an in-browser viewer.

These are not commitments; they exist so reviewers can see this init phase isn't designed in isolation.

---

## 14. Open Questions for the Maintainer

These do not block init phase but should be answered before Phase 1 starts. Capture answers in a follow-up `plan/01-…` document.

1. **License**: the LICENSE file in the initial commit needs confirmation — is it intentional, and should the application's license headers reference it?
2. **App identifier**: macOS bundle ID and Windows AppUserModelID need a real value. Suggested: `org.yornik.sepa-xml-viewer` unless a better domain is owned. Used in `Info.plist`, NSIS install dir, `.desktop` `StartupWMClass`.
3. **Icon**: a placeholder geometric icon ships in init phase; a real icon is a Phase 2 task.
4. **Telemetry**: never. Confirmed by absence — calling it out explicitly so it doesn't sneak in.
5. **Update channel**: GitHub Releases page only in init phase. Auto-update is Phase 4.
