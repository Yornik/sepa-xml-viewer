# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Init phase plan at [`plan/00-init-phase.md`](plan/00-init-phase.md) covering Phase 0 scope, technology decisions, target audience (HR / non-technical office users), offline-only design constraint, and definition of done.
- [`CLAUDE.md`](CLAUDE.md) onboarding doc for contributors and AI agents.
- Repository scaffolding dotfiles: [`.editorconfig`](.editorconfig), [`.clang-format`](.clang-format), [`.clang-tidy`](.clang-tidy), [`.gitattributes`](.gitattributes).
- This `CHANGELOG.md` (Keep-a-Changelog format).
- Example SEPA XML directory at [`tests/example-xml/`](tests/example-xml/) with a `pain.001.001.13` Customer Credit Transfer Initiation file using checksum-valid synthetic IBANs and clearly-marked placeholder values, plus a README documenting the placeholder convention.
- Phase 1 architectural commitment to multi-version SEPA reading at [`plan/01-multi-version-support.md`](plan/01-multi-version-support.md): adapter-per-version pattern feeding a canonical internal model, supporting every published version of `pain.001`, `pain.002`, `pain.007`, `pain.008`, the SRTP family, and `camt.052`/`053`/`054`/`055`. Competitive framing and graceful "unknown version" fallback included.

### Changed

- `README.md` expanded with project tagline, target audience, status, tech stack summary, offline-only guarantee, and links to the plan and example data.
- `README.md` early-alpha warning callout added at the top — explicit "not for production, no warranty, cross-check before real-money decisions" notice. Reinforces the GPL `AS IS` clause in plain language for non-technical readers.
