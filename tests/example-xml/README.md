# Example SEPA XML files

This directory contains hand-written example SEPA / ISO 20022 XML files. They exist for two purposes:

1. **Documentation** — a human reader can open one and see what a SEPA message of a given type looks like.
2. **Future test fixtures** — once the parser lands in Phase 1, these files become integration-test inputs.

## Placeholder convention

Every file in this directory is **synthetic test data**. None of the IBANs, BICs, names, addresses, or references correspond to real accounts, real people, or real businesses. The convention used:

- **IBANs** are checksum-valid (mod-97 = 1) but use obviously fake bank codes and account numbers, e.g. `DE92123456789876543210` — the BLZ `12345678` is not a real German bank code, the account `9876543210` is sequential digits.
- **BICs** use the placeholder pattern `BANKDEFFXXX`, `OTHRDEFFXXX`. The 4-letter institution codes `BANK` and `OTHR` are not real institutions.
- **Names** are prefixed with `PLACEHOLDER`, e.g. `PLACEHOLDER Debtor Name AG`.
- **Identifiers** (`MsgId`, `EndToEndId`, `InstrId`, `PmtInfId`, `OrgId`, …) include the literal token `PLACEHOLDER`.
- **Addresses** use the literal "PLACEHOLDER" in the street name; town, postcode, and country are realistic so the structured-address constraint (mandatory from 15 November 2026 per EPC) can be exercised.
- **Amounts and dates** are realistic SEPA values for visual fidelity; they do not encode any meaning.

## Validation status

These files are written to match the published ISO 20022 schema for the named message version. They have **not yet been validated against the official XSD** because the parser and Xerces-C++ integration arrive in Phase 1. Before relying on a file as a passing test fixture, run it through the Phase 1 validator and update this README's inventory.

## Adding a new example

When adding a new message type (e.g. `pain.008.001.12`, `camt.053.001.x`, `pain.013.001.10`):

- Name the file `<message-type>-<short-purpose>.xml`, e.g. `pain.008.001.12-direct-debit.xml`.
- Open with an XML comment block stating: message type, ISO 20022 version, intent, and a "TEST DATA — DO NOT USE" warning.
- Apply the placeholder convention above. Use a checksum-valid synthetic IBAN; do not copy IBANs from documentation, blog posts, or production data.
- Update the inventory below.

## File inventory

| File                                            | Message              | Description                                      |
| ----------------------------------------------- | -------------------- | ------------------------------------------------ |
| `pain.001.001.13-credit-transfer.xml`           | `pain.001.001.13`    | Single SEPA Credit Transfer Initiation. One debtor sending one EUR transfer to one creditor; both parties in Germany; structured postal addresses (compliant with the 15 November 2026 EPC unstructured-address discontinuation). |
