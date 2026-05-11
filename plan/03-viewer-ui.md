# Phase 3 — Viewer UI for Non-Technical Users

Status: draft, 2026-05-08
Owner: @yornik
Scope: design and behavior of the GUI as the target audience experiences it (HR / payroll / accounting / compliance — see [`plan/00-init-phase.md`](00-init-phase.md) §1 *Target audience*). Phases 1–2 produce the parsed message model; this phase makes it usable for someone who has never read the ISO 20022 spec.

This document covers UX commitments. Pixel-level layout, exact strings, and Material color tokens are implementation-time decisions made against the principles below.

---

## 1. Goal

A non-technical user opens a SEPA file and within seconds understands:

- Who paid whom how much
- When it happened (or will happen)
- Whether the file is structurally valid
- Optionally, behind a toggle: the technical ISO 20022 element names, raw XML, structural tree

They never need to know what `pain.001.001.13` is. The version label is visible (so they can quote it to support if needed) but is subtle and never required to operate the tool.

---

## 2. Layout — three panes, two visible

Default opening view:

```
+------------------------------------------------------------+
|  filename.xml      [Credit Transfer batch · v13]   [VALID] |   <- top bar
+--------------------------------+---------------------------+
|                                |                           |
|        Summary view            |     Raw XML view          |
|        (default focus)         |     (collapsible)         |
|                                |                           |
|        - From / To             |     <Document>            |
|        - Amount                |       <CstmrCdtTrfInitn>  |
|        - When                  |         <GrpHdr>...       |
|        - Reference             |         <PmtInf>...       |
|        - ...                   |       </>                 |
|                                |     </>                   |
|                                |                           |
|                                | [tab: Tree | Raw]         |
+--------------------------------+---------------------------+
| Search [____________________]                  [? Help]    |   <- footer
+------------------------------------------------------------+
```

- **Top bar**: filename, plain-language message-type label, version label (subtle, monospace), validity banner (color-tinted by state).
- **Left pane (default focus)**: Summary view — the audience's primary interface.
- **Right pane**: Raw XML view by default; a small tab-strip lets power users switch to the Tree view. Tree view is intentionally not the default — it is jargon-y by definition.
- **Footer**: search box and a help button. Search is global across panes (§7).

The right pane is collapsible. On a narrow window the Summary pane takes the full width; the right pane reappears at >960 px.

---

## 3. Summary view — the audience's primary interface

The summary view is the difference between "useful viewer" and "yet another XML pretty-printer." Get this right.

### 3.1 Plain-language by default

Default labels use audience vocabulary, not ISO 20022 element names:

| Plain-language label | ISO 20022 element                  |
| -------------------- | ---------------------------------- |
| Sender               | `<Dbtr>` (Debtor)                  |
| Recipient            | `<Cdtr>` (Creditor)                |
| Sender's bank        | `<DbtrAgt>` (Debtor Agent)         |
| Recipient's bank     | `<CdtrAgt>` (Creditor Agent)       |
| Amount               | `<InstdAmt>` / `<Amt>`             |
| When                 | `<ReqdExctnDt>` / `<BookgDt>`      |
| Reference            | `<RmtInf>/<Ustrd>` or `<Strd>`     |
| Message ID           | `<MsgId>` / `<EndToEndId>`         |
| Statement period     | `<FrToDt>` (camt.053)              |
| Opening / closing balance | `<Bal>` Opbd / Clbd entries   |

Translation rules:
- Money: locale-correct rendering (`€1.234,56` in DE, `$1,234.56` in US, `123,45 €` in FR).
- Dates: locale-correct (`15 May 2026` in EN-GB, `15.05.2026` in DE, `15/05/2026` in FR).
- IBANs: rendered with spaces every 4 characters for legibility (`DE92 1234 5678 9876 5432 10`).
- BICs: uppercased, monospaced.
- Names: as-is, no truncation (long company names stay long).

### 3.2 Technical-mode toggle

A toolbar toggle (`Aa` icon, "Show technical names") swaps every label to its ISO 20022 element name. The data does not change; only the labels. The state persists across sessions.

This toggle is the project's contract with the technical user: they are not second-class citizens, they just are not the default audience.

### 3.3 Per-message-type layout

Each message type gets a layout designed for its primary content. Generic "list every field" layouts hide the user's actual question.

- **`pain.001` / `pain.008`**: a transaction-list layout. Header summary at top (sender, total amount, total transactions, requested execution date), per-transaction rows below (recipient, amount, reference). One file = many transactions = scrollable list.
- **`camt.053`** (statement): account header (IBAN, period, opening / closing balance), transaction list (date, counterparty, amount, reference). The user's question is "what came in and what went out and what is the balance now" — answer in that order.
- **`camt.054`** (notification): single-event focus. "On [date] [amount] was [credited / debited] to / from [account]. Reference: [...]"
- **SRTP `pain.013` family**: request-focused. "Request from [Payee] for [amount], expected by [date]. Status: [pending / accepted / rejected / ...]"
- **`pain.002` (status report)**: status-grouped. "Of [N] transactions submitted, [accepted / rejected / pending] [counts]." Per-transaction rows below if user expands.

The per-type layout dispatches off the canonical model's `family` field (see [`plan/02-multi-version-support.md`](02-multi-version-support.md) §4). A new message family adds a new QML component and registers it; the rest of the UI does not change.

---

## 4. Raw XML view

Power users want to see what's actually in the file. Casual users sometimes want to copy a value to send to support.

- **Syntax highlighting**: XML-aware (elements, attributes, text, comments, namespaces). Implemented via `QSyntaxHighlighter` + a `QPlainTextEdit` host inside QML, or Qt Quick Controls `TextArea` with custom highlighter.
- **Line numbers**: gutter on the left.
- **Folding**: expand/collapse element subtrees with click. Folded elements show `<Element>...</Element>` with a count of folded lines.
- **Pretty-printed**: the raw view shows the file pretty-printed, not as it sits on disk. The original file is preserved unchanged.
- **Jump-to-from-summary**: clicking a value in the Summary view scrolls the Raw XML view to the corresponding element and highlights it briefly. Bidirectional — clicking an XML element highlights its summary-view representation if one exists.
- **Find within file**: Ctrl+F focuses the search box (which already searches across all panes — see §7).

---

## 5. Tree view (behind a tab)

For users who want to walk the XML structure as a hierarchy. Behind the Tree tab, not visible by default — it is power-user territory.

- QML `TreeView` (Qt 6.x has improved this significantly) backed by a model that bridges the canonical model and the raw XML positions.
- Element name on the left; abbreviated value on the right. Click expands or collapses.
- Click highlights the corresponding raw XML position and the corresponding summary field.
- Type-ahead: typing on the keyboard while the tree has focus jumps to the next element matching.

---

## 6. File operations

- **Drag-and-drop is the primary entry point.** On first launch the entire window is a drop target with a large "Drag a SEPA file here, or click to browse" graphic. After the first file is loaded, the drop overlay still works but is invisible until a drag enters the window.
- **File-Open dialog** via menu and Ctrl+O — secondary path.
- **System file association**: registering `.xml` is too aggressive (XML is not exclusively SEPA). Instead, install with a "Open with → SEPA XML Viewer" right-click handler so users opt in per file.
- **CLI argument**: `sepa-xml-viewer myfile.xml` opens the file. Used by the file-association handler and by users who start from a terminal.
- **Recent files**: last 10, persisted in user profile dir. Listed in the File menu and on the empty-state drop screen.
- **Reload**: F5 / Ctrl+R re-reads the file from disk. Useful when another tool is regenerating the file while the viewer is open.
- **Close**: Ctrl+W closes the current file and returns to the empty drop screen. Quitting the app (Cmd+Q / Alt+F4) is separate.

Phase 3 is **single-document** — one window, one file at a time. Multi-document tabs are Phase 4 (see [`plan/00-init-phase.md`](00-init-phase.md) §13).

---

## 7. Search

Single search box (footer, footer-right, focus via Ctrl+F or by typing when no other field has focus).

- Searches across **all three panes simultaneously**: summary fields, raw XML text, tree-node text.
- Matches highlighted in all three. Hit count shown next to the search box ("3 matches").
- Plain text by default; toggle for regex (small `.*` button next to the search field).
- Case-insensitive default; toggle for case-sensitive.
- Enter navigates to the next match; Shift+Enter to the previous.

Search across multiple files is out of scope for Phase 3 — it requires multi-document support. Revisit in Phase 4.

---

## 8. Validation feedback

The Phase 2 parser (per [`plan/02-multi-version-support.md`](02-multi-version-support.md)) emits validation results as part of the canonical model. Phase 3 surfaces them as:

### 8.1 Top-bar banner

Color-tinted, three states:

- **Valid (green-tint)**: `Valid` plus a faint "no issues found" tooltip on hover.
- **Warnings (yellow-tint)**: `Warnings — N` clickable.
- **Errors (red-tint)**: `Errors — N` clickable.

Click expands a side panel listing every issue.

### 8.2 Issue list panel

Each issue rendered as:

- **Plain-language summary** (from the canonical model's translated-error table — see §12). Example: "The sender's account number (IBAN `DE92 1234 …`) failed its checksum. The IBAN may have been mistyped, or the file may be corrupt."
- **"Show technical details" expander** revealing the raw XSD-violation text or the parser-level error code. For users who want it, but never inflicted on those who don't.
- **"Show in file" link** that scrolls the Raw XML view to the offending element and highlights it.

### 8.3 Inline highlighting

The Summary view marks an offending field with a small red / yellow indicator next to the value. The Raw XML view marks the offending element with a left-margin marker. The Tree view marks the offending node.

### 8.4 Plain-language error table

Validation errors live in a translated string table keyed by error code:

```
ErrorCode -> { default-language: string, ... per-locale strings ... }
```

Phase 3 ships English-only with the table populated. Phase 4 adds translations via Qt Linguist (see §12). Untranslated codes fall back to the technical message + a "we don't have a friendly translation for this yet — see Show details" line.

---

## 9. Theming

**Qt Quick Controls 2 with the Material style** as the default theme.

- Modern aesthetic the audience reads as "professional desktop software" rather than "developer tool."
- Built-in light and dark modes.
- Sensible defaults for typography, spacing, elevation / shadows.
- Cross-platform consistency — same look on Linux / macOS / Windows. Important for users who switch between work and home machines.

Settings let the user switch to **Universal style** (Windows-y) or **Fusion** (more native). Default Material.

**Dark mode follows OS preference by default.** Override available in Settings. Material's dark palette is the active palette in dark mode — no separate skin to maintain.

Custom Material color overrides limited to: primary (the accent color used on the validity banner and toggles) and the three banner-tint colors. Everything else uses Material defaults.

---

## 10. Accessibility

Per [`plan/00-init-phase.md`](00-init-phase.md) §1 *Target audience*, accessibility is a **Phase 3 deliverable, not a later one**. Audiences in HR / payroll / managed-device environments include users who depend on assistive technology, and they should not wait for a hypothetical v2.0.

Commitments:

- **Screen reader support**: every interactive element has `Accessible.name` and `Accessible.role` set in QML. Banner state is exposed as accessible name (not just an icon glyph). Error list rows are an accessible list.
- **Keyboard navigation**: full tab order through controls; arrow-key navigation in the tree view; Esc closes dialogs; type-ahead in dropdowns; Enter activates focused buttons.
- **Font scaling**: respects OS DPI; built-in zoom (Ctrl+ / Ctrl- / Ctrl-0). Scale persists per user.
- **High contrast**: a separate high-contrast theme. Auto-detected via `Qt.styleHints.colorScheme` / Windows High Contrast / macOS Increase Contrast hints.
- **Color-blindness**: never rely on color alone. The validity banner has icon + text + color; a color-blind user reading the text gets the full state.

**Tested with**: NVDA on Windows, VoiceOver on macOS, Orca on Linux. Tests live in `tests/gui/` and run a basic smoke (open file → assert AT-tree contains expected elements with expected names).

---

## 11. State persistence

Per-file state — which tab is active, scroll position, search query, expander state — does **not** persist across launches. Reopening the file is a clean state.

Cross-launch persisted state (in user profile dir):

- Recent files list (last 10).
- Theme choice (Material / Universal / Fusion + light / dark / OS).
- Font size / zoom level.
- Technical-mode toggle preference.
- Window size and position.
- Trust store contents (deferred to Phase 4 — see [`plan/04-signing-and-encryption.md`](04-signing-and-encryption.md)).

Stored as a single JSON file under the platform-appropriate profile dir:

- Linux: `~/.config/sepa-xml-viewer/state.json`
- macOS: `~/Library/Application Support/sepa-xml-viewer/state.json`
- Windows: `%APPDATA%\sepa-xml-viewer\state.json`

Schema versioned. Forward-compat: unknown keys ignored; missing keys use defaults.

---

## 12. Localization-readiness (i18n in Phase 4)

Phase 3 ships **English-only** in the user-visible interface, but **i18n-ready**.

What that means in practice:

- Every user-visible string in QML wrapped in `qsTr("...")` from day one. No hard-coded English in QML files.
- Number, date, currency formatting uses `Qt.locale()` from day one — never hand-rolled.
- The plain-language error table (see §8.4) is populated for English with a structure that maps cleanly to one Qt Linguist `.ts` file per locale.
- `lupdate` runs as part of the build; the resulting `.ts` files live under `src/i18n/`.

The cheap part is doing it correctly from the first commit. The expensive part is retrofitting it. Phase 4 then ships actual translations.

**Committed Phase 4 languages:** English, German, Dutch, French. The full plan — language reasoning, what is and isn't translated, per-locale formatting tables, banking glossary, the data-vs-display split that keeps SEPA data interoperable across the EU regardless of UI locale, translation workflow, testing — lives in [`plan/05-localization.md`](05-localization.md). Phase 3 implementation must satisfy that plan's Phase 3 readiness criteria (`plan/05` §8): every string in `qsTr()`, every formatting via `Qt.locale()`, source-language `.ts` populated, `lupdate` / `lrelease` integrated into the build.

---

## 13. Onboarding — first-run experience

On first launch:

- **Drop screen** — full-window background with "Drag a SEPA file here, or click to browse" graphic. Same screen the user sees when they close a file.
- **Tiny "About SEPA files" link** in the corner of the drop screen — opens a small, optional dialog explaining what SEPA is, where these files come from, and an "Open the included sample file" button that loads `tests/example-xml/pain.001.001.13-credit-transfer.xml` (bundled with the installer for this purpose).
- **One-time toast** after the first file loads: "Tip: drag any field to copy it. Press Ctrl+F to search." Dismissible; never re-shown.

No usage tracking. No "send anonymous statistics." No upsells. The onboarding is a one-time courtesy, not a funnel.

---

## 14. Explicitly out of scope for Phase 3

These are real features but they belong elsewhere:

**Move to Phase 4:**
- Multi-document tabs.
- Export to `.xlsx` / CSV / JSON / PDF (with the caveat from [`plan/00-init-phase.md`](00-init-phase.md) §13 that `.xlsx` may bump to Phase 3 if user demand precedes the rest of Phase 4 — recheck before Phase 3 ships).
- Validation report export (related to export — same phase).
- Internationalization with actual translations (i18n-ready in Phase 3 → translations land in Phase 4).
- Multi-document search.

**Move to Phase 5 onward:**
- Compare two files side-by-side.
- Plug-in / extension points.

**Never:**
- Telemetry of any kind.
- Auto-update — see [`plan/00-init-phase.md`](00-init-phase.md) §13 Phase 5. Updates flow via re-download or OS package manager.
- "Cloud sync" of recent files / settings — would require networking, violates Goal #9.
- Editing payment data. The viewer is read-only.

---

## 15. Definition of Done

The audience can:

1. Drag a `pain.001.001.13` Customer Credit Transfer Initiation onto the viewer; within 2 seconds see "From PLACEHOLDER Debtor Name AG (DE92 1234 …) to PLACEHOLDER Creditor Name GmbH (DE29 8765 …). Amount: €123.45. When: 15 May 2026. Reference: PLACEHOLDER invoice 2026-001."
2. Toggle "technical mode" (or its localized label) and see the same data with ISO 20022 element names exposed.
3. Open a malformed file; see the validity banner change from green to yellow / red; click through to a plain-language explanation of each issue with a "show technical details" expander available.
4. Operate the viewer **using only the keyboard** — no mouse — to open, navigate, search, and close a file.
5. Open the viewer in **dark mode at 150% font scaling** and have everything still legible and aligned, with the chosen theme applied to all three panes consistently.
6. Use a screen reader (NVDA / VoiceOver / Orca) and have the tool announce: filename, message-type label, validity status, summary field labels and values, and per-issue text in the error list.
7. Switch from Linux to macOS to Windows and find the UI looking and behaving the same — no platform-specific surprises.

---

## 16. Open Questions

1. **Material vs Universal vs Fusion default**: Material reads as "modern" but some Windows users may prefer Universal for OS consistency. Default to Material with a Settings switch; reassess after first user feedback.
2. **Drop-screen sample-file button**: should the sample file open in a "demo mode" with a banner noting it's example data? Probably yes — prevents users from being confused about the placeholder values being real.
3. **Right-click context menu**: copy field, copy IBAN-only (without spaces), copy as JSON-snippet, search-this-value? Probably the first three for Phase 3; design at implementation time.
4. **Tree view's place in the tab strip**: tab order Raw / Tree, or Tree / Raw? Raw first reads as "what's actually in the file"; Tree first reads as "nicer to navigate." Lean Raw-first; revisit after first user feedback.
5. **Window-restore behavior**: restore exact previous window size and position, or only size? Position-restore is a UX win on multi-monitor setups but trips up users whose monitor configuration changed. Lean: restore size always, restore position only if the saved screen geometry still exists.
