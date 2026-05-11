# Signature Verification and Decryption — Phase 4 Capability Plan

Status: draft, 2026-05-08
Owner: @yornik
Scope: read-side cryptographic operations for SEPA messages — verify XML Digital Signatures, decrypt XML-Encrypted and EBICS-enveloped payloads. The viewer never signs and never encrypts.

---

## 1. Goal

A user can drag a **signed** SEPA message into the viewer and see, in plain language, who signed it, when, and whether the signature is valid. A user can drag an **encrypted** SEPA message into the viewer, supply their private key file from disk, and see the decrypted contents in the same view they would see for any other SEPA message. Both operations stay fully offline.

Concretely, three operations land in this phase:

1. **Signature verification** — W3C XML Digital Signature (`xmldsig`, `http://www.w3.org/2000/09/xmldsig#`). Used routinely for inter-PSP SEPA messages and increasingly for corporate-to-bank communication.
2. **Decryption (W3C)** — W3C XML Encryption (`xmlenc`, `http://www.w3.org/2001/04/xmlenc#`). Element-level encryption embedded in an otherwise-readable XML file.
3. **Decryption (EBICS envelope)** — the EBICS-specific framing where the SEPA payload is compressed, AES-CBC-encrypted with a random session key, and the session key is RSA-wrapped with the bank's E002 public key. EBICS does *not* use W3C `xmlenc`; it has its own format (see §9).

### Out of scope

- **Signing.** This is a viewer. We never sign anything. No private signing keys are touched.
- **Encrypting.** Same reasoning. The viewer does not produce encrypted output; export to clear `.xlsx` / CSV / JSON / PDF (per [`plan/00-init-phase.md`](00-init-phase.md) §13 Phase 4) is sufficient when a user wants to share decrypted contents.
- **Key management beyond per-file load.** No key-store integration, no HSM support, no per-user keychain integration. The user supplies a private key file (PEM or PKCS#12) per decryption operation, which is loaded into memory, used, and zeroed on close.
- **OCSP / CRL fetch.** Certificate-revocation checking that requires network calls. The viewer is offline by design (see [`plan/00-init-phase.md`](00-init-phase.md) §1 Goal #9) — revocation status is not consulted at runtime. A "this certificate's expiry / not-before window" check is the strongest temporal validation we do.
- **Certificate transparency, pinning, or any other network-dependent assurance mechanism.**
- **Modifying signed files.** Touching the bytes invalidates the signature; the viewer is read-only and sidesteps this entirely.

---

## 2. Why this matters

The audience defined in [`plan/00-init-phase.md`](00-init-phase.md) §1 *Target audience* — HR, payroll, accounting, compliance — increasingly receives signed and encrypted SEPA artifacts as a normal part of their workflow:

- **Inter-PSP SEPA messages** between banks are routinely signed; the corporate user receiving them has no way to verify the signature without specialized tooling.
- **EBICS bank-to-customer downloads** (account statements `camt.053`, debit/credit notifications `camt.054`, payment status reports `pain.002`) arrive AES-encrypted inside an EBICS envelope. Without decryption support, the user sees an unintelligible blob.
- **Some corporate payroll systems** sign their `pain.001` outputs before handing them to the bank, and an HR person reviewing the file before submission needs to know the signature is intact.
- The audience does not have access to enterprise tooling like a dedicated EBICS client; they want to *read* what their bank or payroll system produced, not send it.

Filling this need at the desktop-viewer level — with the signature/decryption result rendered in plain language alongside the rest of the viewer's UX — is one more place where this project does what the multi-thousand-euro tools do, in a corner those tools mostly leave to specialists.

---

## 3. Standards reference

We do not invent crypto, even less than we invent SEPA message formats. The published standards we build against:

| Standard                              | Where                                                    | Used for                                          |
| ------------------------------------- | -------------------------------------------------------- | ------------------------------------------------- |
| XML Digital Signature (xmldsig)       | https://www.w3.org/TR/xmldsig-core2/                     | Verifying signed SEPA messages                    |
| XML Encryption (xmlenc)               | https://www.w3.org/TR/xmlenc-core1/                      | W3C-style encrypted XML                           |
| Canonical XML 1.1                     | https://www.w3.org/TR/xml-c14n11/                        | Required for signature verification               |
| Exclusive XML Canonicalization        | https://www.w3.org/TR/xml-exc-c14n/                      | Common for SEPA / banking signatures              |
| EBICS specification                   | https://www.ebics.org/en/home (current production version pinned at Phase 4 entry) | EBICS envelope decryption framing |
| ISO 20022 BAH (Business Application Header) | https://www.iso20022.org                          | Often carries the signature in inter-PSP messages |

Concrete EBICS version and BAH version pinning is a Phase 4 entry decision; this document does not pre-commit to one.

---

## 4. Threat model

Cryptographic features change the threat model, so call it out explicitly before architecture.

**Assets:**
- The user's private key material (decryption RSA key, stored on user's disk as PEM or PKCS#12).
- The cleartext of decrypted SEPA messages (after decryption operates).
- The trust roots used for signature verification (bundled certificates plus user-imported additions).

**Adversaries:**
- **A malicious file** the user opens. The file may contain XSLT transforms, very large `<KeyInfo>` URIs, decompression bombs in EBICS payloads, malformed ASN.1 in embedded certificates, or signature elements crafted to mis-route verification. The viewer must not execute arbitrary code, must not perform any network I/O regardless of file content, and must bound resource usage during cryptographic operations.
- **A compromise of the user's machine.** Out of our threat model — once the attacker controls the OS, key material in our process is also compromised. We do not pretend to defend against this.

**Defenses we commit to:**

1. **No network I/O, ever.** xmlsec1 is configured to never dereference `<KeyInfo>` URIs, never fetch certificates, never perform OCSP / CRL checks. Reinforced at the dependency level by the offline-only Qt config in [`plan/00-init-phase.md`](00-init-phase.md) §3.4 — `QNetworkAccessManager` is not linked into the binary, so even if a code path tried, it could not.
2. **No XSLT transforms in signature processing.** XSLT signature transforms (`http://www.w3.org/TR/1999/REC-xslt-19991116`) are a known arbitrary-code-execution vector. xmlsec1 supports disabling them; we do.
3. **Whitelisted transform set.** Only c14n11, exclusive-c14n, base64, and enveloped-signature transforms are allowed. Anything else fails verification with a plain-language explanation.
4. **Bounded resource usage.** Decompression sizes (EBICS) and KeyInfo / signature element sizes are capped at sensible defaults. Anything exceeding the cap fails the operation with a plain-language explanation.
5. **Private keys in memory only.** Loaded from disk, used, and explicitly zeroed (via `OPENSSL_cleanse`) when the document closes or the application quits. Never written back to disk by us.
6. **Passphrase prompts go through Qt's secure password dialog.** No echo, cleared from any intermediate buffer on dismissal.
7. **Trust roots are read-only at runtime.** The bundled set lives in the installer's read-only resource area; user-imported additions go to the user's profile dir but cannot be modified by the running app — they are only read.

**Defenses we explicitly do NOT commit to:**

- **Side-channel resistance.** A user running the viewer on a shared system has the same exposure as any other app processing keys on that system. The OS, not the viewer, mitigates side-channel attacks.
- **Memory locking (`mlock`).** Out of scope for v1; fine to add later if a real user requests it.
- **Tamper-evident binaries.** That is a code-signing concern (see [`plan/00-init-phase.md`](00-init-phase.md) §10), not a runtime concern.

---

## 5. Architecture

```
+-----------------------------+
|  XML file on disk           |
+-------------+---------------+
              |
              v
+-----------------------------+
|  Detection / classification |  src/core/detection/
|  - is this XMLDSig signed?  |  + new: signed_or_encrypted.{h,cpp}
|  - is this XMLEnc?          |
|  - is this an EBICS envelope?|
+-------------+---------------+
              |
              v
+-----------------------------+
|  src/core/security/         |  new module
|  - signature_verifier.{h,cpp}|  - wraps xmlsec1 sig verify
|  - xmlenc_decryptor.{h,cpp}  |  - wraps xmlsec1 decrypt
|  - ebics_decryptor.{h,cpp}   |  - bespoke EBICS framing
|  - trust_store.{h,cpp}       |  - bundled + user-imported trust anchors
|  - key_loader.{h,cpp}        |  - PEM / PKCS#12 from disk
+-------------+---------------+
              |
              v
+-----------------------------+
|  Per-version SEPA adapter   |  src/core/messages/...
|  (canonical model fields    |  unchanged from plan/01
|   plus security annotations:|  + Message gains:
|   signature, encryption,    |    - std::optional<SignatureSummary>
|   verification result)      |    - std::optional<DecryptionSummary>
+-------------+---------------+
              |
              v
+-----------------------------+
|  UI                         |  knows nothing about xmlsec1
|                             |  renders SignatureSummary /
|                             |  DecryptionSummary in plain language
+-----------------------------+
```

**Why this shape:**

- `src/core/security/` is a new module that encapsulates all crypto. It depends on xmlsec1 + libxml2 + OpenSSL; nothing else in the codebase does. If we ever swap libraries, the change is local.
- The canonical Message gains two optional summary fields (`SignatureSummary`, `DecryptionSummary`). Files that arrive unsigned/unencrypted simply don't populate them, and the UI hides the corresponding panels. No new "if signed: ..." branches in the UI.
- Detection is the routing layer. A file that starts with `<EbicsRequest>` goes to EBICS decryption; a file with a `<Signature>` element under `<Document>` goes to verification before parsing the inner content; a file with `<EncryptedData>` goes to xmlenc decryption.

This keeps the existing parser pipeline (plan/01 adapters → canonical model) intact. Crypto sits as a pre-stage that produces cleartext + summary, which then feeds the same adapter pipeline.

---

## 6. Library choices

**xmlsec1** (vcpkg `xmlsec`) — the canonical XML Security library. C, MIT-licensed, mature (active since ~2002, used by GNOME, Apache, every banking integration in C/C++ that handles XML signatures). Supports OpenSSL / NSS / MSCrypto backends; we pick OpenSSL for portability.

- Handles xmldsig verification, c14n / exc-c14n, transform chains, KeyInfo parsing.
- Handles xmlenc decryption.
- We **do not** use its XML transformation features for anything beyond the whitelisted signature transform set.

**OpenSSL** (vcpkg `openssl`) — pulled in by xmlsec1 anyway. We use OpenSSL primitives directly for EBICS envelope decryption (RSA private-key decrypt, AES-CBC decrypt, zlib inflate via the OpenSSL-bundled `libz`).

**libxml2** — pulled in by xmlsec1. Coexists with pugixml — pugixml drives the friendly tree view (small, fast, simple), libxml2 does the cryptographic operations (xmlsec1's hard requirement).

**Qt's QSslCertificate / QSslKey / QCryptographicHash** — used for cert / key inspection in the UI layer (display "issuer," "subject," "expires") without going through xmlsec1. Qt already ships with these and they require no extra dependency.

**Explicitly NOT used:**

- **Crypto++, Botan, libsodium, mbedTLS** — duplicating OpenSSL doubles the attack surface for no gain. xmlsec1 + OpenSSL is the de-facto SEPA stack.
- **GnuPG / OpenPGP libraries.** Not relevant to SEPA — banks use X.509 certificates, not PGP keys.
- **JOSE / JWS / JWE libraries.** SEPA uses XML signatures, not JWS.

---

## 7. Trust roots

Two sources, both read-only at runtime:

1. **Bundled trust anchors.** A small set of CA certificates ships with the installer. Sourced from the public bank-CA distribution channels (e.g. Bundesverband deutscher Banken for German EBICS, equivalent for FR / NL / ES). Stored under the installer's read-only resource path. License check before bundling — same pattern as the SEPA XSDs in [`plan/02-multi-version-support.md`](02-multi-version-support.md) §5.
2. **User-imported additions.** The user can drag a PEM-format CA certificate into the viewer's "Trust Anchors" dialog. The certificate is copied (not symlinked) into the user's profile dir (`~/.config/sepa-xml-viewer/trust/` or platform equivalent) and used on subsequent verifications. The dialog shows fingerprint, issuer, subject, expiry; the user explicitly consents to trust it.

If verification succeeds against the union of (1) and (2), the result is "valid." If neither set contains the signing certificate's chain, the result is "valid signature, but the signer is not in your trust list" — surfaced clearly so the user understands the difference between a forged signature and an unknown-but-correctly-signed message.

---

## 8. UI flow — for the non-technical audience

The audience must not be required to understand X.509 to use this. The default experience for **unsigned, unencrypted** files stays exactly as designed in [`plan/00-init-phase.md`](00-init-phase.md) §1 *Target audience*: drag-and-drop, plain-language summary, no crypto UI clutter.

When the file **is** signed or encrypted, the UI adapts:

**Signed file** (banner above the summary, green-tinted in the UI):
> Signed by **Bank XYZ AG** on 8 May 2026 at 10:30 CET. Signature is valid.
>
> *(subdued link)* Show technical details — issuer chain, signature algorithm, certificate validity window.

If the signer is not in the trust list (yellow-tinted):
> This file has a valid signature, but the signer (**Some Other Bank**) is not in your trust list. The signature was not forged, but the viewer cannot confirm the signer is who they claim to be without a trust anchor. *(link)* Add **Some Other Bank** to your trust list…

If the signature is broken (red-tinted):
> This file's signature is **not valid**. The file may have been modified after it was signed, or the signature was corrupted. The contents below are shown for inspection but should not be trusted.

**Encrypted file** (file-open dialog flows into):
> This file is encrypted. To read it, supply the private key from your bank or payroll system.
>
> *(file picker for PEM / PKCS#12)*
>
> *(passphrase prompt if PKCS#12 password-protected)*

After decryption (banner above the summary, neutral-tinted):
> Decrypted in memory. Closing this file clears the key from memory.
>
> *(subdued link)* Show technical details — encryption algorithm, key fingerprint.

The cleartext is rendered through the normal SEPA viewer pipeline. The user sees the same summary they would see for an unencrypted file — they don't need to learn a different UI.

**Visual styling** for the four banner states (valid / untrusted / broken / decrypted) uses Qt Quick's built-in theming colour roles plus an inline icon (Material Design or equivalent — final asset choice is a Phase 4 UI task). The banner copy is also exposed as plain-text accessible name so screen readers announce the state correctly without depending on the icon glyph.

---

## 9. EBICS envelope — the bespoke piece

W3C `xmlenc` covers most encrypted SEPA cases corporate users encounter. EBICS is the exception, and it is common enough (German / French banking) that we cannot skip it.

EBICS request and response bodies wrap the SEPA payload in a custom structure. Decryption flow:

1. Parse the EBICS XML envelope (`<ebicsRequest>` or `<ebicsResponse>` root). Extract:
   - `<DataEncryptionInfo>/<EncryptionPubKeyDigest>` — fingerprint of the bank's E002 key the file was encrypted to. Sanity-check: should match the user's currently-loaded bank key.
   - `<DataEncryptionInfo>/<TransactionKey>` — base64-encoded AES session key, RSA-PKCS1-v1.5-encrypted with the user's E002 RSA private key.
   - `<DataTransfer>/<OrderData>` — base64-encoded compressed AES-128-CBC ciphertext of the SEPA file. IV is the first 16 bytes of the ciphertext (per EBICS convention) or zero (older versions); both supported.
2. Base64-decode `<TransactionKey>`, RSA-decrypt with the user's private key → 16-byte AES session key.
3. Base64-decode `<OrderData>`, AES-128-CBC decrypt with the session key and the appropriate IV → compressed cleartext.
4. zlib-inflate → SEPA XML cleartext.
5. Hand the cleartext to the normal multi-version SEPA adapter pipeline.

We bound the inflate output size to avoid decompression-bomb DoS. The bound is configurable but defaults to 100 MB — far larger than any plausible SEPA file, far smaller than what a malicious file could try to balloon to.

xmlsec1 does **not** handle EBICS framing — this is bespoke OpenSSL code in `src/core/security/ebics_decryptor.cpp`. It is a small, focused, well-tested piece. Test fixtures (synthetic EBICS envelopes encrypted to a synthetic test key) live in `tests/example-xml/ebics/`.

---

## 10. Testing strategy

Three layers, mirroring the rest of the project ([`plan/00-init-phase.md`](00-init-phase.md) §6):

**Unit tests (`tests/unit/security/`):**
- Each crypto operation (signature verify, xmlenc decrypt, EBICS decrypt) gets its own Catch2 test against synthetic fixtures.
- Negative tests: tampered signature → fails with the expected error. Wrong key for decryption → fails cleanly. Decompression-bomb fixture → bounded and rejected.
- Trust-store tests: signer in bundled set → valid. Signer in user-imported set → valid. Signer not in either → "valid signature, untrusted signer" result.

**Integration tests (`tests/integration/`):**
- Full pipeline: drop a signed encrypted EBICS file in, decrypt, verify, parse, populate canonical model. Assert summary fields and `SignatureSummary` / `DecryptionSummary` annotations are correct.
- Cross-version: a `pain.001.001.13` and a `pain.001.001.03` both wrapped in EBICS envelopes — both decrypt and produce equivalent canonical-model output.

**GUI smoke tests (`tests/gui/`):**
- Drag a signed file in; assert the signature banner appears with the correct text.
- Drag an encrypted file in; the key prompt appears; provide a key from the test fixture; cleartext renders.

**Fixtures (`tests/example-xml/ebics/`, `tests/example-xml/signed/`):**
- Synthetic EBICS envelopes encrypted to a generated test RSA key (committed alongside as `test-bank-e002.pem`, **clearly marked as synthetic**).
- Synthetic signed pain.001 files signed with a generated test signing key (also committed, also marked as synthetic).
- Generation script `tools/generate-crypto-fixtures.sh` so anyone can regenerate the fixtures and verify they are synthetic.

No real bank keys, no real customer keys, no real EBICS sessions. The placeholder convention from `tests/example-xml/README.md` extends to crypto material.

---

## 11. Phasing

- **Phase 4** — W3C `xmldsig` verification + W3C `xmlenc` decryption. Trust-store UI. The "signed/encrypted file" UX. This is the bulk of the work and covers the corporate-to-bank cases that don't use EBICS framing.
- **Phase 4.5** — EBICS envelope decryption, layered on top of the Phase 4 module. Adds the EBICS-specific detection, framing parser, and bespoke decryption pipeline. Reuses the Phase 4 trust store and key-loading UI; adds nothing else to the user-visible interface.
- **Phase 5 onward** — no further crypto features planned. If real-world demand emerges for, e.g., signing an existing file before re-submission to a bank, that is a substantial scope expansion and gets its own plan document.

---

## 12. Definition of Done

A user can:

1. Drag a signed pain.002 from their bank into the viewer; see "Signed by [Bank], valid" banner; see the message contents below it; technical details available behind one click.
2. Drag an EBICS-enveloped camt.053 from their bank; supply their E002 private key file; see the bank statement decrypted and rendered as if it had arrived in cleartext.
3. Drag a tampered signed file; see a clear "signature not valid" banner; see the contents anyway, marked as untrusted.
4. Add a CA certificate they received from their bank to the trust list via a clearly-labeled dialog; subsequent files signed by that bank verify cleanly.
5. Close a decrypted file; receive subtle confirmation that the key was cleared; verify (via OS tooling, since we do not provide our own confirmation beyond UI text) that the application's memory does not retain the key.

Across all of these, the offline guarantee from [`plan/00-init-phase.md`](00-init-phase.md) §1 Goal #9 holds: zero network I/O at any point, including during signature verification (no OCSP, no CRL fetch, no KeyInfo URL resolution).

---

## 13. Open Questions

1. **Bundled trust anchors — which set?** Need to pick a defensible default. Probably empty in v1 (user must explicitly add their bank's CA), opt-in to "include common European bank CAs" via a setting. Empty default is the safest position; bundling implicitly endorses a set of authorities.
2. **Where do user-imported certs live?** Recommend `~/.config/sepa-xml-viewer/trust/` (Linux), `~/Library/Application Support/sepa-xml-viewer/trust/` (macOS), `%APPDATA%\sepa-xml-viewer\trust\` (Windows). Confirm before implementation; not load-bearing until then.
3. **Passphrase caching for the duration of a session?** Tempting (re-prompting on every encrypted file is annoying) but expands the in-memory key window. Default: no caching; each file prompts. User opt-in via setting if real demand appears.
4. **PKCS#11 / hardware-token support?** Out of scope for v1. Some EBICS users keep their E002 key on a smart card. If that audience is meaningful, add a separate Phase 5+ plan; do not bake it in now.
5. **Validation chain depth and policy for self-signed CAs.** SEPA banks sometimes use self-signed CAs internally. Need a clear policy: do we trust a self-signed CA the user explicitly imported (yes — by design they're saying "trust this"), or require a chain to a recognized root (no — would force users to disable verification entirely)? Recommendation: explicit-import = explicit-trust, document this clearly in the trust-store UI.
