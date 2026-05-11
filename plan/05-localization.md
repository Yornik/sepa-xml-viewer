# Localization — English, German, Dutch, French

Status: draft, 2026-05-08
Owner: @yornik
Scope: the four user-interface languages the viewer commits to support, the data-vs-display split that keeps the SEPA standard interoperable across the EU, the per-locale formatting rules, and the translation workflow. Phase 3 ships i18n-ready (English-only). Phase 4 ships actual translations.

---

## 1. Goal

The user interface is available in four languages by 1.0:

| Language    | Code | Primary regions                                                         |
| ----------- | ---- | ----------------------------------------------------------------------- |
| English     | `en` | UK / Ireland / international default; EU-wide professional lingua franca |
| German      | `de` | Germany / Austria / German-speaking Switzerland                          |
| Dutch       | `nl` | Netherlands / Flanders (Belgium)                                         |
| French      | `fr` | France / Wallonia (Belgium) / Luxembourg / Monaco                        |

The viewer auto-selects the UI language from the OS locale on first run and lets the user switch in Settings. Switching takes effect immediately — no restart needed.

---

## 2. Why these four

These four cover the bulk of SEPA-region office-worker exposure:

- **English** — the international default and the language a non-localized fallback should use. Many EU professionals work in English regardless of native language.
- **German** — Germany is the largest single SEPA market by volume; combined with Austria and Swiss-German offices it is a substantial audience. Also the dominant EBICS user-base ([`plan/04-signing-and-encryption.md`](04-signing-and-encryption.md) §9 covers EBICS framing).
- **French** — France itself, plus the Walloon region of Belgium, Luxembourg, and Monaco. France has its own banking conventions (separator, currency placement) distinct from the rest of the EU; getting the formatting right is a trust signal.
- **Dutch** — the Netherlands has high SEPA adoption and a population that often interacts with payment files directly via accounting and HR systems. Flanders (Belgium) is the second large Dutch-speaking SEPA region.

Languages **not** initially in scope but acknowledged candidates for later phases:
- Italian, Spanish, Polish, Portuguese — large SEPA volumes, real audiences. Adding them is straightforward (translation work, no code changes — see §8). Not committed by 1.0; revisit when there is real user demand or a translator volunteers.

---

## 3. Data vs display — the EU-wide-interoperability rule

This is the most important section of the localization plan.

**The underlying SEPA / ISO 20022 data is always in standard format. Localization only affects display.** This is non-negotiable because the SEPA standard exists precisely so that payment data is interoperable across the entire EU regardless of the parties' locales.

### 3.1 Standard format — never localized

These data elements stay in ISO standard form everywhere in the codebase, every export, every internal API, and every clipboard "copy as standard" operation:

| Element              | Standard                        | Example                                |
| -------------------- | ------------------------------- | -------------------------------------- |
| Decimal separator    | `.` (ISO 20022 + ISO 31-0)      | `123.45` — never `123,45` in raw data  |
| Thousand separator   | none in raw data                | `1234567.89` — no separator            |
| Date                 | ISO 8601                        | `2026-05-15` / `2026-05-15T10:30:00`   |
| Currency code        | ISO 4217                        | `EUR`, `USD`, `GBP`, `CHF`             |
| Country code         | ISO 3166-1 alpha-2              | `DE`, `NL`, `FR`                       |
| IBAN                 | ISO 13616                       | `DE92123456789876543210` (raw, no spaces) |
| BIC                  | ISO 9362                        | `BANKDEFFXXX`                          |

A user in Munich reading a payment file from Paris must see semantically the same data as the Parisian sender encoded in their file. The XML doesn't change, the canonical model doesn't change, the JSON export doesn't change. Only the *visible UI labels and number-formatting layer* change per the user's chosen UI language.

### 3.2 Display layer — locale-formatted

The visible UI applies locale formatting on top of the standard data:

| Locale  | Number `1234.56` | Date `2026-05-15` | Currency `EUR 123.45`   |
| ------- | ---------------- | ----------------- | ----------------------- |
| `en-GB` | `1,234.56`       | `15 May 2026`     | `€123.45`               |
| `en-US` | `1,234.56`       | `May 15, 2026`    | `€123.45`               |
| `de-DE` | `1.234,56`       | `15.05.2026`      | `123,45 €`              |
| `de-AT` | `1.234,56`       | `15.05.2026`      | `€ 123,45`              |
| `nl-NL` | `1.234,56`       | `15-05-2026`      | `€ 123,45`              |
| `nl-BE` | `1.234,56`       | `15/05/2026`      | `€ 123,45`              |
| `fr-FR` | `1 234,56`       | `15/05/2026`      | `123,45 €`              |
| `fr-BE` | `1.234,56`       | `15/05/2026`      | `123,45 €`              |
| `fr-LU` | `1.234,56`       | `15/05/2026`      | `123,45 €`              |

Notable specifics:
- `fr-FR` uses **non-breaking space (U+00A0)** as the thousand separator per French national standard. Qt's `Qt.locale()` handles this; never hand-roll.
- `nl-NL` uses `-` as date separator, `nl-BE` uses `/` — same language, different country, different format. Honor the `xx-YY` granularity, not just `xx`.
- IBAN visual grouping (every 4 chars: `DE92 1234 5678 9876 5432 10`) is universal across all locales — it is a readability convention from ISO 13616, not a localization. The underlying value never has spaces.

All formatting goes through `Qt.locale()`. No hand-rolled separator logic anywhere.

### 3.3 Exports — ISO standard by default, locale-formatted as opt-in

The viewer exports to CSV / JSON / Excel / PDF (per [`plan/00-init-phase.md`](00-init-phase.md) §13). The default for these exports is **ISO standard format** so a German user's exported CSV opens correctly in a French user's tool:

- CSV: numbers as `123.45`, dates as ISO 8601, currency code in its own column. UTF-8 with BOM. Separator: `,` by default with a UI option for `;` (regional preference).
- JSON: ISO 8601 dates, decimal numbers as JSON numbers (always `.`), currency code as a sibling field.
- Excel `.xlsx`: numbers stored as Excel numeric cells (no string formatting), dates as Excel date cells. Excel applies the *opener's* locale on open — a German user opens a German-locale view, a French user opens a French-locale view, both reading the same file.
- PDF: locale-formatted to match the UI language (PDF is a presentation artifact, not a data-interchange format).

Each export dialog has a "Use ISO standard format" toggle that the user can flip. Default is on for CSV / JSON, intentionally locked on for `.xlsx` (Excel handles localization at open time so we always store standard), intentionally off for PDF.

### 3.4 Clipboard — both options on right-click

Right-clicking any value in the Summary view offers:

- **Copy** — displayed text in the current UI locale (`123,45 €` for German user).
- **Copy as ISO standard** — `EUR 123.45` regardless of UI locale, suitable for pasting into another tool that expects standard format.

Default keyboard `Ctrl+C` follows the displayed format. Power users use the right-click for standard format.

---

## 4. What gets translated

Translation table covers everything the user reads:

- **All labels in the Summary view** ("Sender" / "Absender" / "Afzender" / "Émetteur" — see §6 glossary).
- **Validity-state banner text** ("Valid" / "Gültig" / "Geldig" / "Valide").
- **Error messages** in the validation panel (the plain-language part — the technical "show details" expander stays in standard XSD/parser language because those are jargon that translates poorly and the audience for those is technical anyway).
- **Menu items, dialog titles, button labels.**
- **Onboarding text** (the "drag a SEPA file here" graphic, the "About SEPA files" dialog, the one-time tip toast).
- **Settings panel** (theme names where they have a translation, descriptive labels otherwise).
- **The "About SEPA files" dialog** — country-specific framing where helpful (a German reader might benefit from "this is what your bank or DATEV produces"; a French reader from "ce que produit votre banque ou logiciel comptable").

---

## 5. What does NOT get translated

Things that stay in their canonical form regardless of UI language:

- **IBANs, BICs, currency codes, country codes** — these are international identifiers, not language.
- **ISO 20022 element names** in the technical-mode Summary view (`<Dbtr>`, `<CdtrAcct>`, etc.) — exposing them in another language would defeat the purpose of technical mode.
- **Raw XML pane content** — it is the raw document, not interpretation. Always shown as-is.
- **Sample placeholder content** in `tests/example-xml/` — `PLACEHOLDER Debtor Name AG` stays English-prefixed so it remains recognizable as test data across all locales.
- **Filenames, message-type identifiers** (`pain.001.001.13`) — these are technical identifiers, locale-independent.
- **Log files, debug output** — English. The audience does not read these; translators don't need to either.

---

## 6. Banking glossary — the four languages

The translation table needs banking-specific vocabulary, not just dictionary words. Initial mapping (review by native-speaker translators required before shipping):

| English          | German                          | Dutch                              | French                          |
| ---------------- | ------------------------------- | ---------------------------------- | ------------------------------- |
| Sender           | Auftraggeber / Absender         | Afzender                           | Émetteur / Donneur d'ordre      |
| Recipient        | Empfänger                       | Begunstigde / Ontvanger            | Bénéficiaire / Destinataire     |
| Amount           | Betrag                          | Bedrag                             | Montant                         |
| Reference        | Verwendungszweck / Mitteilung   | Mededeling / Omschrijving          | Communication / Référence       |
| Account          | Konto                           | Rekening                           | Compte                          |
| Account number   | Kontonummer (IBAN)              | Rekeningnummer (IBAN)              | Numéro de compte (IBAN)         |
| Bank             | Bank                            | Bank                               | Banque                          |
| Statement        | Kontoauszug                     | Rekeningafschrift                  | Relevé (de compte)              |
| Credit transfer  | Überweisung                     | Overschrijving                     | Virement                        |
| Direct debit     | Lastschrift                     | Incasso / Domiciliëring            | Prélèvement                     |
| Mandate          | Mandat                          | Mandaat                            | Mandat                          |
| Execution date   | Ausführungsdatum                | Uitvoeringsdatum                   | Date d'exécution                |
| Booking date     | Buchungsdatum                   | Boekingsdatum                      | Date de comptabilisation        |
| Value date       | Valuta / Valutadatum            | Valutadatum                        | Date de valeur                  |
| Opening balance  | Anfangssaldo / Startsaldo       | Beginsaldo                         | Solde initial / Solde d'ouverture |
| Closing balance  | Endsaldo / Schlusssaldo         | Eindsaldo                          | Solde final / Solde de clôture  |
| Valid (signed)   | Gültig                          | Geldig                             | Valide                          |
| Invalid          | Ungültig                        | Ongeldig                           | Non valide                      |
| Encrypted        | Verschlüsselt                   | Versleuteld                        | Chiffré                         |
| Decrypted        | Entschlüsselt                   | Ontsleuteld                        | Déchiffré                       |

Multiple options shown above (`Auftraggeber / Absender`) reflect cases where banking software in that region varies; translators pick the dominant one for the SEPA-tooling context. Maintain a single chosen term per concept once review lands; document choices in `src/i18n/glossary.md` so future translation work stays consistent.

---

## 7. Translation workflow

**Tooling**: Qt Linguist. The standard Qt i18n pipeline:

1. Source code wraps every visible string in `qsTr("...")` (QML) or `tr("...")` (C++) from day one.
2. `lupdate` extracts strings into `src/i18n/sepa-xml-viewer_<locale>.ts` files (one per locale).
3. Translators open the `.ts` files in Qt Linguist and provide translations.
4. `lrelease` compiles `.ts` → `.qm` (binary) — bundled with the installer.
5. The application loads `.qm` for the active locale at startup and reloads on locale switch.

**Repository layout for translations:**

```
src/
├── i18n/
│   ├── sepa-xml-viewer_en.ts        # source-language ts (used as fallback)
│   ├── sepa-xml-viewer_de.ts
│   ├── sepa-xml-viewer_nl.ts
│   ├── sepa-xml-viewer_fr.ts
│   ├── glossary.md                  # term decisions + translator notes
│   └── README.md                    # how to contribute translations
```

**Build integration** (Phase 3): CMake target `update-translations` runs `lupdate`; the regular build runs `lrelease` and bundles `.qm` files. The user-visible English strings drive `lupdate` extraction so that adding a new English string surfaces in every `.ts` file as untranslated, ready for translators.

**Translator sourcing for 1.0:**
- German: paid translator with banking domain experience (DATEV / SEPA familiarity preferred).
- Dutch: paid translator (NL banking ecosystem familiar).
- French: paid translator (FR/BE banking ecosystem familiar).
- Native-speaker review pass before each language ships. Plain-language banking copy is hard — a generic translator produces stilted text the audience reads as "automatic translation."

**Maintenance after 1.0:**
- Each new English string adds an untranslated entry in every `.ts` file. CI fails the release build if any `.ts` has untranslated entries — translation must be filled or marked "intentionally untranslated" before a release tags.
- Glossary changes require translator re-pass for every locale.
- Volunteers welcome for additional locales (IT, ES, PT, PL) once the workflow is proven.

---

## 8. Phasing

- **Phase 3** — i18n-ready. Every visible string in `qsTr()`. `lupdate` and `lrelease` integrated into the build. `src/i18n/sepa-xml-viewer_en.ts` populated. UI ships English-only but the infrastructure is there for translations to drop in without code changes.
- **Phase 4** — translations land:
  - **3a**: German first (largest SEPA market, biggest immediate-value win).
  - **3b**: Dutch and French in the same release window after German lands.
  - **3c**: native-speaker review pass for all three before tagging a 1.0 candidate.
- **Phase 5 onward** — additional locales by demand or volunteer. No code changes; only `.ts` files added and a one-line `lrelease` registration.

The cheap part is doing it right in Phase 3. The expensive part is retrofitting it after a year of hard-coded English strings have accumulated. Phase 3 commits to the cheap part.

---

## 9. Testing

Three layers, mirroring [`plan/00-init-phase.md`](00-init-phase.md) §6 and [`plan/03-viewer-ui.md`](03-viewer-ui.md):

**Unit tests (`tests/unit/i18n/`):**
- Locale formatting tests — given an amount and a locale, assert the rendered string. Covers all four languages and their regional variants (de-DE / de-AT / de-CH? — see Open Questions).
- ISO-standard-export tests — given an amount in a locale, assert the CSV / JSON serialization is in standard format regardless.
- Glossary key coverage — every key in `glossary.md` must appear in every `.ts` file.

**Integration tests (`tests/integration/i18n/`):**
- Load a `pain.001` fixture, switch to each of the four locales in turn, assert the Summary view renders the labels and values correctly per the locale formatting tables in §3.2.

**GUI smoke tests (`tests/gui/i18n/`):**
- For each language: launch the app with `LANG=<locale>`, assert the main window displays in the expected language (check the validity banner text and one menu label).
- Switch language at runtime, assert the UI updates without a restart.

**No automatic translation quality checks.** Translation correctness is a human-review concern, not a CI concern.

---

## 10. Open Questions

1. **Regional variant granularity.** Do we ship `de-DE` only, or `de-DE` + `de-AT` + `de-CH`? The differences are real but small (currency placement in `de-AT`, possibly some vocabulary in `de-CH`). Pragmatic default: ship `de` (using `de-DE` formatting as the canonical form), let `Qt.locale()` handle the regional formatting variations natively, ship a single translation per language. Revisit if user feedback warrants per-region overrides.
2. **Translator sourcing.** The plan above assumes paid translators for 1.0; budget needs to be confirmed. Volunteers may cover specific languages — that is fine for post-1.0 additions but the four committed languages should be paid+reviewed for the trust signal of a polished initial release.
3. **Untranslated-string fallback.** When a string is missing from a `.ts` file, Qt falls back to the `qsTr()` source string (English). Acceptable default. Alternative: ship "(untranslated: ...)" markers in development builds to make the gap obvious. Decide at Phase 4 entry.
4. **Plural forms.** Some strings need plural variants (`"1 transaction"` / `"5 transactions"` is `"1 Buchung"` / `"5 Buchungen"` in German but the rules vary across languages — Polish has three plural forms, French has two). Use Qt's `tr("%n transaction(s)", "", count)` plural-aware overload from the start; do not hard-code plural strings.
5. **Right-to-left readiness.** Out of scope for the four committed languages (all LTR). Revisit when adding Arabic / Hebrew is on the table; Qt Quick supports RTL natively but the layouts may need testing.
