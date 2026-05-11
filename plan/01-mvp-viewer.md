# Phase 1 — MVP Viewer

Status: draft, 2026-05-11
Owner: @yornik
Scope: the smallest end-to-end vertical that lets a real user open a SEPA file and understand what is in it. Single current message version. Single basic UI. Ships as `v0.1.0`. Multi-version coverage, audience-facing UX polish, validation gating, exports, accessibility, and localization are explicitly later phases and do not block this one.

This plan is downstream of Phase 0 ([`plan/00-init-phase.md`](00-init-phase.md)) and upstream of Phase 2 ([`plan/02-multi-version-support.md`](02-multi-version-support.md)). Read both for context.

---

## 1. Goal

Open a `pain.001.001.13` (Customer Credit Transfer Initiation) XML file and render its contents — group header, payment information blocks, individual transactions — in a basic GUI. Read-only. Offline. No XSD validation gate (warn but render). One window, one file at a time.

That is the entire MVP. Ship it, then iterate.

## 2. Scope

### In scope
- Drag-and-drop file open (one file at a time).
- Detect the message family + version from the root namespace. If unrecognized, show a clear "this version isn't supported yet" message rather than crashing or rendering nothing.
- Parse `pain.001.001.13` into a canonical in-memory model.
- Render the parsed model in a two-pane UI: a tree on the left (group header → payment info → transactions), a detail pane on the right showing the fields of the selected node.
- Raw XML pane (third tab) with monospace text and basic indentation — no syntax highlighting yet.
- Window remembers size between launches. No "recent files" yet.
- Ship as `v0.1.0` through the existing release pipeline (no new packaging work — Phase 0 already shipped that).

### Out of scope (deferred to later phases)
- **Multi-version support.** Only `pain.001.001.13` here. `pain.001.001.03` / `.09` / `.10` and the other pain / pacs / camt families are Phase 2 ([`plan/02-multi-version-support.md`](02-multi-version-support.md)).
- **Plain-language UX.** The MVP renders raw ISO 20022 element names. The "Sender / Recipient / Amount / When" audience-facing translation is Phase 3 ([`plan/03-viewer-ui.md`](03-viewer-ui.md)).
- **XSD validation report.** MVP warns on parse failures but does not validate against the XSD. Validation gating + plain-language error messages are Phase 3.
- **Accessibility (screen reader, keyboard navigation, font scaling).** Phase 3 deliverable — the MVP is mouse-driven only.
- **Exports (.xlsx / CSV / JSON / PDF).** Phase 4.
- **Localization.** English UI only. Localized formatting (dates, numbers, currency) is Phase 4 ([`plan/05-localization.md`](05-localization.md)).
- **Signed / encrypted SEPA.** Phase 4 ([`plan/04-signing-and-encryption.md`](04-signing-and-encryption.md)).
- **Multi-document tabs / recent files / dark mode.** Phase 3.

## 3. Why this scope

The point of an MVP is to put a working tool in front of real users and learn what they actually need. Building multi-version support, accessibility, plain-language UX, and exports before any user has opened a single file is a guess about what matters. The MVP defers all of that and ships the minimum that proves the viewer works at all.

`pain.001.001.13` is the right starting format because it is the current version mandated by the EPC as of 15 November 2026 — the active version everyone is migrating to. Files in the wild are increasingly this version. Once the MVP works for `.13`, Phase 2's adapter pattern slots earlier versions into the same canonical model without rewriting the UI.

## 4. Architecture

Minimum that works:

```
+----------------------+
|  XML file on disk    |
+----------+-----------+
           |
           v
+----------------------+
|  src/core/sepa.cpp   |   One file. pugixml load + a
|  load(path) -> Doc   |   pain.001.001.13-shaped Doc with
|                      |   GroupHeader, [PaymentInfo, [Transaction]]
+----------+-----------+
           |
           v
+----------------------+
|  src/ui/qml/         |   QML TreeView binds to a model that
|  tree + detail tabs  |   walks Doc directly. No Q_GADGET layer.
+----------------------+
```

No namespace-detection module, no adapter pattern, no Qt-free core, no canonical-model layer. **One parser function for one version, bound directly to a Qt model the UI consumes.** All of the abstractions above are real in Phase 2 — they earn their keep once a second version exists to abstract over. Adding them now is architecture for hypothetical future code.

When Phase 2 lands, this PR's `sepa.cpp` becomes a candidate for refactoring into the adapter pattern from [`plan/02-multi-version-support.md`](02-multi-version-support.md) §4. That refactor is cheaper to do *once we know what shape multi-version actually needs* than to predict now.

**Library choice — XML parsing**: pugixml (already specified in [`plan/00-init-phase.md`](00-init-phase.md) §3.2). No XSD validation library yet; Xerces-C++ arrives in Phase 4 when validation becomes a UI feature.

## 5. The `pain.001.001.13` happy path

A `pain.001.001.13` file has three nesting levels we render:

1. **Group Header** (`<GrpHdr>`) — one per file. Message ID, creation date/time, total number of transactions, total control sum, initiating party.
2. **Payment Information** (`<PmtInf>`) — one or many per file. Payment method, batch booking, requested execution date, debtor party + account, charge bearer.
3. **Credit Transfer Transaction** (`<CdtTrfTxInf>`) — one or many per `PmtInf`. End-to-end ID, amount + currency, creditor party + account, remittance information.

The tree mirrors this nesting. The detail pane for each level shows that level's fields. No translation, no plain-language relabeling — raw element names. The MVP's audience is itself: a developer + a few willing early users who can tolerate raw ISO 20022 names.

## 6. UI

```
+--------------------------------------------------------------+
| File   View   Help                              SEPA XML Viewer |
+--------------------------------------------------------------+
|                                                              |
|  Drop a SEPA file here, or use File → Open...                |  ← initial state
|                                                              |
+--------------------------------------------------------------+
```

After loading a file:

```
+--------------------------------------------------------------+
| File   View   Help                              SEPA XML Viewer |
+----------------------------+---------------------------------+
| [Tree] [Raw XML]           |                                 |
+----------------------------+                                 |
| ▼ Group Header             | Field        Value              |
|   ▼ Payment Info #1        | -----        -----              |
|     • Transaction 1        | MsgId        ABC-2026-001       |
|     • Transaction 2        | CreDtTm      2026-05-11T10:30Z  |
|   ▼ Payment Info #2        | NbOfTxs      3                  |
|     • Transaction 3        | CtrlSum      4500.00            |
|                            | InitgPty/Nm  Acme GmbH          |
+----------------------------+---------------------------------+
```

Two tabs across the left: **Tree** (default) and **Raw XML** (monospace, no highlighting). Right pane shows fields of the selected tree node as a two-column table.

QML built on `Qt.labs.qmlmodels.TreeModel` (or a hand-rolled `QAbstractItemModel` if `qmlmodels` proves limiting). Detail pane is a plain `ListView` over a flat `(key, value)` model.

## 7. Validation behavior

The MVP does not validate against the XSD. It tries to parse. On structural failures (malformed XML, missing required nesting), it shows a single-line error in the detail pane and the raw XML tab still works so the user can see what they fed in.

On *recognition* failures (unrecognized namespace), the message is:

> This file is `<detected-family>` version `<detected-version>`. Currently only `pain.001.001.13` is supported. Multi-version coverage is on the roadmap — see [`plan/02-multi-version-support.md`](02-multi-version-support.md).

Explicit, not generic. The user should understand *why* it didn't work.

## 8. Definition of Done

1. ☐ Dropping a valid `pain.001.001.13` file on the window renders the tree and the detail pane. Interactive (subjectively snappy) on files with up to 100 transactions. Wire a benchmark only if a real fixture feels slow.
2. ☐ The Raw XML tab shows the file's content unmodified except for trivial indentation.
3. ☐ Switching tree nodes updates the detail pane synchronously.
4. ☐ Opening a non-SEPA XML file (or an unsupported SEPA version) shows the recognition-failure message in §7, not a crash.
5. ☐ Opening a malformed XML file shows a single-line error message, not a crash.
6. ☐ Catch2 unit test covers the parser against the existing `tests/example-xml/` fixture.
7. ☐ Integration test: command-line invocation of the binary with a fixture file path produces a non-zero diagnostic on stderr if parsing fails; zero exit on success. (Future-proofs the Phase 6 CLI.)
8. ☐ GUI smoke test on Windows: launches the binary with a fixture file argument, sleeps 3 s, asserts the process is alive (event loop healthy) and stderr does not contain `is not installed` or `failed to load`. This PR also adds the test — same shape as the Phase 0 v0.0.1 retag's smoke step.
9. ☐ `git tag v0.1.0 && git push --tags` produces the same six artefacts Phase 0 v0.0.1 produced, all carrying the new binary, all passing their existing CI smoke checks.
10. ☐ README has a one-paragraph "What v0.1.0 does and doesn't do" callout that sets correct expectations.

## 9. PR sequencing

MVP-first: the first PR puts something visible on screen. Everything else iterates from there.

| PR | Scope | Effort |
| -- | ----- | ------ |
| #1 | **First visible result.** `src/core/sepa.cpp` parses a `pain.001.001.13` happy-path fixture into an in-memory tree. Minimal QML `TreeView` shows the GroupHeader / PaymentInfo / Transaction nesting from a hard-coded fixture path. No file-open UI yet — the binary loads the fixture at startup, drops you straight into the tree. Catch2 covers the parser; the screen proves the binding works end-to-end. | 1.5d |
| #2 | Detail pane. Selecting a tree node shows the node's fields as a (key, value) table on the right. | 0.5d |
| #3 | File → Open and drag-and-drop replace the hard-coded fixture. Window remembers size between launches. | 0.5d |
| #4 | Raw XML tab. Recognition-failure UI for unsupported versions (read root `xmlns`, show the §7 message). Malformed-XML UI. | 1d |
| #5 | Integration test (CLI invocation with fixture path) + GUI smoke (event-loop-alive + no QML errors, reusing the Phase 0 §12 PR #9 Windows pattern). README "What v0.1.0 does" callout. Tag `v0.1.0`. | 0.5d |

Total ballpark: **~4 engineer-days**. Day 1 ends with a working tree on screen; everything after that improves an already-working thing rather than building toward one.

## 10. What success looks like for Phase 1

A user downloads the `v0.1.0` installer on Linux, macOS, or Windows. They open one of their own `pain.001.001.13` files (likely a recent payroll batch). They see the structure of the file in the tree, can drill into a single transaction, and can read the raw XML to verify nothing was hidden. They tell us what was missing — that informs Phase 2 vs Phase 3 ordering.

That is the entire bar. Polish, breadth, and depth all come after.
