// Unit tests for the sepa::parseFile() API. Runs against the example XML
// fixtures under tests/example-xml/. The fixture paths come from a compile-
// time define so the test doesn't have to discover them at runtime.

#include "sepa/sepa.h"

#include <QString>

#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#ifndef SEPA_FIXTURE_DIR
#error "SEPA_FIXTURE_DIR must be defined by the build system"
#endif

namespace {
QString fixture(const char* name) {
    return QString::fromUtf8(SEPA_FIXTURE_DIR) + QStringLiteral("/") + QString::fromUtf8(name);
}
}  // namespace

TEST_CASE("parseFile reports FileNotFound for a missing path", "[parser]") {
    const auto r = sepa::parseFile(QStringLiteral("/no/such/path/__definitely_missing.xml"));
    REQUIRE(r.status == sepa::ParseStatus::FileNotFound);
    REQUIRE_FALSE(r.errorMessage.isEmpty());
}

TEST_CASE("parseFile reports MalformedXml for non-XML content", "[parser]") {
    // Use the README as a deliberately non-XML input. The exact wording
    // pugixml returns isn't important; we just want a MalformedXml status.
    const auto r = sepa::parseFile(fixture("README.md"));
    REQUIRE(r.status == sepa::ParseStatus::MalformedXml);
    REQUIRE_FALSE(r.errorMessage.isEmpty());
    // The file existed, so rawXml should still be populated for the UI's
    // Raw XML tab.
    REQUIRE_FALSE(r.rawXml.isEmpty());
}

TEST_CASE("parseFile recognizes a single-tx pain.001.001.13", "[parser]") {
    const auto r = sepa::parseFile(fixture("pain.001.001.13-credit-transfer.xml"));
    REQUIRE(r.status == sepa::ParseStatus::Success);
    REQUIRE(r.detectedFamily == QStringLiteral("pain.001"));
    REQUIRE(r.detectedVersion == QStringLiteral("001.13"));
    // Root → GroupHeader + PaymentInfo
    REQUIRE(r.root.children.size() == 2);
    // GroupHeader carries MsgId / CreDtTm / NbOfTxs / CtrlSum / InitgPty
    const auto& grp = r.root.children.front();
    REQUIRE(grp.name.startsWith(QStringLiteral("Group Header")));
    REQUIRE(grp.fields.size() >= 4);
    // PaymentInfo contains exactly one transaction in this fixture
    const auto& pmt = r.root.children.back();
    REQUIRE(pmt.children.size() == 1);
}

TEST_CASE("parseFile recognizes the multi-batch payroll fixture", "[parser]") {
    const auto r = sepa::parseFile(fixture("pain.001.001.13-payroll-and-suppliers.xml"));
    REQUIRE(r.status == sepa::ParseStatus::Success);
    // 1 group header + 2 PmtInf blocks
    REQUIRE(r.root.children.size() == 3);
    // PmtInf #1 = payroll, 3 txs ; PmtInf #2 = suppliers, 2 txs
    const auto& pmt1 = r.root.children[1];
    const auto& pmt2 = r.root.children[2];
    REQUIRE(pmt1.children.size() == 3);
    REQUIRE(pmt2.children.size() == 2);
    // The structured creditor reference (RF18539007547034) lives on the
    // first supplier transaction; verify the parser surfaced it.
    const auto& firstSupplierTx = pmt2.children.front();
    const bool hasStructuredRef = std::any_of(
        firstSupplierTx.fields.begin(), firstSupplierTx.fields.end(), [](const sepa::Field& f) {
            return f.value == QStringLiteral("RF18539007547034");
        });
    REQUIRE(hasStructuredRef);
}

TEST_CASE("parseFile handles the 2500-tx stress fixture", "[parser][stress]") {
    const auto r = sepa::parseFile(fixture("pain.001.001.13-payroll-2500-stress.xml"));
    REQUIRE(r.status == sepa::ParseStatus::Success);
    // 1 GroupHeader + 1 PmtInf block with 2500 children
    REQUIRE(r.root.children.size() == 2);
    const auto& pmt = r.root.children.back();
    REQUIRE(pmt.children.size() == 2500);
}
