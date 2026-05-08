# Multi-Version SEPA Support — Phase 1 Architectural Commitment

Status: draft, 2026-05-08
Owner: @yornik
Scope: a capability-level commitment for the parser. Open and render every published version of every in-scope ISO 20022 SEPA message family. No writing, no editing, no version conversion. Init-phase work in [`plan/00-init-phase.md`](00-init-phase.md) is unaffected by this document.

---

## 1. Goal

The viewer must open and render any ISO 20022 SEPA message at any published version of its message type. Concretely, the in-scope families and the current versions verified in [`plan/00-init-phase.md`](00-init-phase.md) §2.2:

| Message family | Current version verified 2026-05-08 | Notes                                                |
| -------------- | ------------------------------------ | ---------------------------------------------------- |
| `pain.001`     | `pain.001.001.13`                    | Customer Credit Transfer Initiation (SCT)            |
| `pain.002`     | `pain.002.001.15`                    | Customer Payment Status Report                       |
| `pain.007`     | `pain.007.001.13`                    | Customer Payment Reversal                            |
| `pain.008`     | `pain.008.001.12`                    | Customer Direct Debit Initiation (SDD)               |
| `pain.013`     | `pain.013.001.10`                    | Creditor Payment Activation Request (SRTP)           |
| `pain.014`     | `pain.014.001.07`                    | SRTP status report                                    |
| `pacs.028`     | `pacs.028.001.03`                    | FI to FI Payment Status Request (SRTP)                |
| `camt.029`     | `camt.029.001.09`                    | Resolution of Investigation (SRTP)                   |
| `camt.052`     | TBD at Phase 1 entry                 | Bank-to-Customer Account Report                      |
| `camt.053`     | TBD at Phase 1 entry                 | Bank-to-Customer Statement                           |
| `camt.054`     | TBD at Phase 1 entry                 | Bank-to-Customer Debit/Credit Notification           |
| `camt.055`     | `camt.055.001.08`                    | Customer Payment Cancellation Request (SRTP)         |

For each family the goal is to support **all published versions** that ever shipped in production volume, not just the current version. The concrete historical version list is stocked when each adapter is implemented (§7); this document does not pre-claim a definitive count.

### Out of scope

- **Writing or editing files.** The viewer is read-only at every version.
- **Auto-conversion between versions.** That is a write operation under another name and is explicitly excluded — silently rewriting a `pain.001.001.03` archive to `pain.001.001.13` would change the file's content from what the user opened.
- **Pre-SEPA national formats** (DTAUS, MT940, MT103, etc.). Different codecs entirely; revisit after SEPA coverage is solid.

---

## 2. Why this matters — the competitive framing

Many enterprise-priced SEPA viewers — including SaaS tools that run hundreds to thousands of euros per seat per year — only render the version they shipped. Open an archived `pain.001.001.03` from 2015 in those tools and the result ranges from "unsupported format" to silent misrender. The audience for this viewer (HR, payroll, accounting, compliance — see [`plan/00-init-phase.md`](00-init-phase.md) §1 *Target audience*) frequently needs exactly those legacy archives:

- Auditors revisiting payroll runs from prior fiscal years.
- Forensic accountants reconstructing historical payments.
- Compliance staff during inspections.
- HR staff opening archived salary files from a former payroll system.

The 15 November 2026 EPC `pain.001.001.13` mandate amplifies the gap: organizations are migrating their active tooling right now, and their existing `pain.001.001.03` / `.05` / `.08` / `.09` archives are about to become harder to access — not from any technical change, just from focus shifting elsewhere. A viewer that opens any version transparently solves a real problem at exactly the moment the market is actively re-tooling.

This is not future-proofing; it is solving a problem the market has not solved.

---

## 3. Compatibility — what "backwards compatible" actually means

**At the XML level: no.** Different ISO 20022 versions live in different namespaces (`urn:iso:std:iso:20022:tech:xsd:pain.001.001.NN`), have different XSDs, and in some places have renamed or restructured elements. A `pain.001.001.03` file is not a valid `pain.001.001.13` file. You cannot "just upgrade the namespace string."

**At the semantic level: yes.** Every version of `pain.001` represents the same business concept — a customer initiating a credit transfer. The set of business fields evolves additively: newer versions add fields without changing the meaning of existing ones. Fields removed in newer versions (e.g. unstructured `<AdrLine>` in `pain.001.001.03` vs. the structured `<StrtNm>` / `<BldgNb>` / `<TwnNm>` mandate in `pain.001.001.13`) are replaced by equivalent fields that carry the same business information.

This means we can map any version to a canonical internal representation and render it uniformly. The architecture below leverages exactly that property.

---

## 4. Architecture — adapter pattern with a canonical model

```
+----------------------+
|  XML file on disk    |
+----------+-----------+
           |
           v
+----------------------+
|  Namespace detection |   src/core/detection/{namespace.h,cpp}
|  (root xmlns parse)  |
+----------+-----------+
           |
           v
+--------------------------------+
|  Per-version adapter           |   src/core/messages/pain.001/v13/
|  (one C++ TU per supported     |   src/core/messages/pain.001/v09/
|   version; parses that         |   src/core/messages/pain.001/v03/
|   version's XML quirks)        |   src/core/messages/pain.008/v12/
+--------------+-----------------+   ... etc
               |
               v
+----------------------+
|  Canonical model     |   src/core/model/{message.h, party.h, account.h, transaction.h}
|  (superset of all    |   - business fields exposed as std::optional<>
|   versions' fields;  |     where they only exist in some versions
|   immutable          |   - version metadata kept on Message: family, major,
|   value-types)       |     minor, build, source-namespace
+----------+-----------+
           |
           v
+----------------------+
|  UI                  |   knows nothing about XML version
|                      |   renders canonical model uniformly
+----------------------+
```

### Why this pattern

- **Detection** reads only the document root, extracts the namespace URI, and yields a `(family, major, minor, build)` tuple plus the original namespace string. No full parse is needed for routing.
- **Per-version adapters** isolate version-specific quirks. A fix to a `v03` quirk never touches `v09` logic. Adapters never share files; they share the canonical model and a small set of generic helpers (IBAN parsing, BIC parsing, ISO 4217 currency, ISO 8601 date).
- **Canonical model** is the superset. Fields added in later versions are `std::optional<>`; old-version files simply do not populate them, and the UI hides `nullopt` fields. Fields renamed across versions are mapped to a single canonical name.
- **UI consumes only the canonical model.** No `if (version == ...)` branches in UI code, ever. This is the rule that keeps multi-version support sustainable as the version count grows.

This is how mature multi-format tooling handles ISO 20022 internally. It scales linearly with version count and isolates risk per version.

### Anti-patterns explicitly rejected

- **One mega-parser with version branches** — a single function full of `if (ns endsWith "13") { ... } else if ("09") { ... }`. Becomes unmaintainable past three versions and concentrates risk in one file.
- **Forward-only support with auto-upgrade** — silently rewriting a `v03` file to `v13` in memory. Loses information; gives the user a different file than they opened; legally risky if the displayed message diverges from the file's actual content. Out of scope per §1.
- **XSLT version pivots** — technically elegant but adds an XSLT engine to the dependency footprint and the offline guarantee. Not worth the complexity.

---

## 5. Schema bundling — the open question

XSD validation is a key feature for the audience (it tells them whether the file their bank produced is well-formed). To stay offline, all in-scope XSDs must ship inside the installer.

**Open question — license / redistribution rights:**

- **ISO 20022 catalogue XSDs** are publicly downloadable from iso20022.org. The site does not display obvious redistribution-permission text on the catalogue page; the SWIFT terms of use need to be read carefully and possibly clarified directly with SWIFT before bundling.
- **EPC TVS XSDs** are publicly downloadable from europeanpaymentscouncil.eu. Same situation — public download, redistribution terms not surfaced on the download links.

This must be answered before the validation feature ships. Resolution paths in priority order:

1. **Bundle if redistribution is permitted.** Confirm with SWIFT (the ISO 20022 Registration Authority) and the EPC; document the permission grant in `third_party/schemas/LICENSE.txt` alongside the schemas.
2. **Bundle minimal validation rules of our own derivation.** Re-implement the relevant patterns from public ISO 20022 specifications (IBAN regex, BIC regex, SEPA charge-bearer enum, mandatory-field rules per IG). High effort, brittle as new versions appear; only pursue if (1) is denied.
3. **One-time online fetch at install, then offline forever.** Violates Goal #9 of the init plan. Accept only as a written, named exception, gated on a UI prompt and clearly documented in the README.
4. **Ship without XSDs; structural parse only.** Lose the schema-validation feature; the rest of the viewer still works. Acceptable as a transitional state while (1) or (2) is resolved.

The architecture works under any of these — the schema source lives behind an interface, and the validator is one consumer of the canonical model. The specific resolution can move without re-architecting.

---

## 6. Testing strategy

`tests/example-xml/` grows as adapter coverage grows. Convention from [`tests/example-xml/README.md`](../tests/example-xml/README.md) continues: per-message-type fixtures with clearly-marked placeholder content and checksum-valid synthetic IBANs. No real bank data, ever.

### Layered coverage

- **Unit tests (per adapter)**: each `pain.001/vNN/` adapter has its own Catch2 test that loads a representative fixture and asserts the canonical-model fields are populated correctly.
- **Cross-version equivalence tests**: a single business scenario (e.g. "EUR 123.45 from PLACEHOLDER Debtor to PLACEHOLDER Creditor on 2026-05-15") expressed across multiple versions; the canonical-model output should be identical for the fields that exist in all versions tested. This is the test that catches divergence between adapters.
- **GUI smoke tests**: open one fixture from each major-version family, verify the summary view renders without errors and shows the version metadata correctly ("This file is `pain.001.001.03`").
- **Negative tests**: open an XML file in an unrecognized version namespace; assert the friendly fallback path triggers (§8 Definition of Done).

### Fixture growth target

For each adapter shipped, at least:
- One "happy path" fixture with realistic placeholder content.
- One "edge case" fixture exercising version-specific quirks (e.g. `pain.001.001.03`'s `<AdrLine>` unstructured address, which `pain.001.001.13` no longer permits).

---

## 7. Phasing

- **Phase 1** — parser architecture established. Namespace detection, canonical model, adapter interface, and adapters for **current** versions only:
  - `pain.001.001.13`, `pain.008.001.12`, `pain.002.001.15`, `pain.007.001.13`
  - The full SRTP V4.0 set: `pain.013.001.10`, `pain.014.001.07`, `pacs.028.001.03`, `camt.029.001.09`, `camt.055.001.08`
  - Latest `camt.052` / `camt.053` / `camt.054` (exact versions confirmed at Phase 1 entry)
  - Multi-version *capability* lives in the architecture but is not yet stocked with historical adapters.
- **Phase 1.5 — historical coverage**. Stock historical adapters in priority order based on real-world archive prevalence:
  - `pain.001` legacy versions — `.03`, `.05`, `.08`, `.09` are highest-impact targets; most legacy archives use one of these.
  - `pain.008` legacy versions in parallel.
  - `camt.053` legacy versions (bank-statement archives go back further than payment-initiation files).
  - Less-used messages last.
- **Phase 2** — UI built on the canonical model, version-agnostic from day one. Includes the "this file is version X" indicator and the "newer than this app knows" graceful-degradation path (see §8).
- **Phase 3** — signature verification and decryption (covered in a separate plan once committed; sketched in conversation as `xmlsec1`-based, offline, with user-supplied private keys for decryption).

---

## 8. Definition of Done

A user can drag any of the following into the viewer and see a useful, accurate, plain-language summary:

1. A `pain.001.001.03` archive from 2014.
2. A `pain.001.001.09` file from 2022.
3. A `pain.001.001.13` file from 2026.
4. The same legacy-to-current span across `pain.008` and `camt.053`.
5. An SRTP V4.0 message bundle.

Across those, the canonical-model fields populated for the common business concepts are identical or equivalent — the cross-version equivalence test in §6 passes.

When the file is in an unrecognized version namespace, the viewer **never silent-fails**. It says so in plain language ("This file is `pain.001.001.99` — that version is newer than this app knows about. Try updating the app, or open the raw XML view to inspect it manually.") and opens the raw XML pane as the fallback. The user is not left wondering whether the file is corrupt or the app is broken.

---

## 9. Open Questions

1. **Schema redistribution license** (see §5). Highest-priority unresolved question.
2. **Earliest version we commit to.** `pain.001.001.02` exists in the ISO 20022 catalogue but may not have been used in production at meaningful volume. Real archives almost always start at `.03` or later. Pragmatic floor: `.03` unless real-world evidence appears for older. Confirm with two or three audit / payroll users when accessible.
3. **How aggressively to validate** at parse time. Strict XSD validation may reject malformed legacy files that real banks accepted in their day. Probably want a per-file "best-effort" toggle that downgrades validation errors to warnings — but the default needs to match user expectation, and that needs a quick user check.
4. **UI for fields that exist in some versions only.** When a `v03` file lacks a structured-address field that `v13` surfaces, do we hide the field entirely or show a "not present in this version" placeholder? Recommended default: hide; preserves the clean UX the audience expects. Power-user technical mode (toggleable, see [`plan/00-init-phase.md`](00-init-phase.md) §1 *Target audience*) shows the raw element-by-element view where the absence is implicit.
5. **Well-formed XML in a version namespace not yet implemented.** Open the raw view + a friendly "version X is on the roadmap" message? Or refuse and open raw only? Lean toward the friendly message — preserves the trust signal the audience needs.
