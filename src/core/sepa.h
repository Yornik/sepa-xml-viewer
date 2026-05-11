#pragma once

// MVP parser for pain.001.001.13. One version, no adapter pattern.
//
// Phase 2 (plan/02-multi-version-support.md) refactors this into the
// adapter-per-version pattern with a canonical model. For the MVP, the
// goal is to put a parsed tree on screen, not architect for hypothetical
// future code.

#include <QString>

#include <vector>

namespace sepa {

enum class ParseStatus {
    Success,
    FileNotFound,
    MalformedXml,
    UnrecognizedMessage,  // Recognized XML but not a SEPA family/version we handle.
};

struct Field {
    QString name;
    QString value;
};

// One node in the parsed tree (Group Header, Payment Info block, Transaction).
// Children form the nested structure pain.001 messages have.
struct Node {
    QString name;
    std::vector<Field> fields;
    std::vector<Node> children;
};

struct ParseResult {
    ParseStatus status = ParseStatus::FileNotFound;

    // Human-readable diagnostic for the failure UI. Empty when status == Success.
    QString errorMessage;

    // Populated whenever parseFile() reads enough to identify the message.
    // For Success, both fields are set. For UnrecognizedMessage, family/version
    // reflect what we found in the root xmlns even though we can't parse it.
    QString detectedFamily;
    QString detectedVersion;

    // Root of the parsed tree. Only meaningful when status == Success.
    Node root;

    // Full file bytes, decoded as UTF-8, for the Raw XML tab. Populated whenever
    // the file could be read (even on parse failure).
    QString rawXml;
};

// Load and parse the SEPA file at `path`. Never throws — all failure modes
// surface through ParseResult::status. Safe to call from the GUI thread for
// files up to a few MB; for the 2500-transaction stress fixture this returns
// in well under a second on any modern machine.
ParseResult parseFile(const QString& path);

}  // namespace sepa
