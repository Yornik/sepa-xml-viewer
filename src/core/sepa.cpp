#include "sepa/sepa.h"

#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include <pugixml.hpp>

#include <string>
#include <vector>

namespace sepa {

namespace {

constexpr const char* kSupportedFamily = "pain.001";
constexpr const char* kSupportedVersion = "001.13";

// Parse "urn:iso:std:iso:20022:tech:xsd:pain.001.001.13" into a (family,
// version) pair. The family is everything before the final two .NN groups
// (i.e. "pain.001"); the version is the trailing ".001.13" without the
// leading dot. Returns ("", "") if the URI doesn't match the expected shape
// — we still try to render that case as UnrecognizedMessage rather than
// crashing, because catalogue URIs occasionally drift.
struct DetectedMessage {
    QString family;
    QString version;
};
DetectedMessage detectFromNamespace(const QString& xmlns) {
    static const QRegularExpression re(
        QStringLiteral(R"(urn:iso:std:iso:20022:tech:xsd:([a-z]+\.\d{3})\.(\d{3}\.\d{2}))"));
    const auto match = re.match(xmlns);
    if (!match.hasMatch()) {
        return {};
    }
    return {match.captured(1), match.captured(2)};
}

// Add (name, value) to `fields` only if the child element exists and has
// non-empty text. Keeps the detail pane free of "MsgId: " rows for fields
// the message simply doesn't carry.
void addField(std::vector<Field>& fields, const pugi::xml_node& parent, const char* childName) {
    const auto child = parent.child(childName);
    if (child.empty()) {
        return;
    }
    const auto value = QString::fromUtf8(child.child_value()).trimmed();
    if (value.isEmpty()) {
        return;
    }
    fields.push_back({QString::fromUtf8(childName), value});
}

// Walk down a path of element names and pull the leaf text into `fields`
// under `displayName`. e.g. addPath(fields, pmtInf, "ReqdExctnDt/Dt",
// "ReqdExctnDt"). Quietly skips when any step is missing.
void addPath(std::vector<Field>& fields,
             const pugi::xml_node& root,
             const char* path,
             const QString& displayName) {
    pugi::xml_node cursor = root;
    for (const auto& step : QString::fromUtf8(path).split(QChar('/'))) {
        cursor = cursor.child(step.toUtf8().constData());
        if (cursor.empty()) {
            return;
        }
    }
    const auto value = QString::fromUtf8(cursor.child_value()).trimmed();
    if (value.isEmpty()) {
        return;
    }
    fields.push_back({displayName, value});
}

// pain.001.001.13 wraps the payee/payer name + identifying account in two
// sibling elements (e.g. <Dbtr> + <DbtrAcct>, <Cdtr> + <CdtrAcct>). Flatten
// the most useful bits into the parent's field list so the detail pane
// doesn't require the user to expand four levels of nesting just to read
// "who and what IBAN".
void addPartyFields(std::vector<Field>& fields,
                    const pugi::xml_node& parent,
                    const char* partyTag,
                    const char* acctTag,
                    const QString& partyLabel,
                    const QString& acctLabel) {
    const auto party = parent.child(partyTag);
    if (!party.empty()) {
        addPath(fields, party, "Nm", partyLabel + QStringLiteral(" / Nm"));
        addPath(fields, party, "PstlAdr/Ctry", partyLabel + QStringLiteral(" / Ctry"));
        addPath(fields, party, "PstlAdr/TwnNm", partyLabel + QStringLiteral(" / TwnNm"));
    }
    const auto acct = parent.child(acctTag);
    if (!acct.empty()) {
        addPath(fields, acct, "Id/IBAN", acctLabel + QStringLiteral(" / IBAN"));
    }
    const auto agent = parent.child((QByteArray(partyTag) + "Agt").constData());
    if (!agent.empty()) {
        addPath(fields,
                agent,
                "FinInstnId/BICFI",
                QString::fromUtf8(partyTag) + QStringLiteral("Agt / BICFI"));
    }
}

Node buildTransaction(const pugi::xml_node& tx, int index) {
    Node n;
    n.name = QStringLiteral("Transaction %1").arg(index);

    addPath(n.fields, tx, "PmtId/InstrId", QStringLiteral("InstrId"));
    addPath(n.fields, tx, "PmtId/EndToEndId", QStringLiteral("EndToEndId"));
    addPath(n.fields, tx, "Amt/InstdAmt", QStringLiteral("InstdAmt"));
    // Amount currency lives in the InstdAmt @Ccy attribute, not a child element.
    if (const auto amt = tx.child("Amt").child("InstdAmt"); !amt.empty()) {
        const auto ccy = QString::fromUtf8(amt.attribute("Ccy").value()).trimmed();
        if (!ccy.isEmpty()) {
            n.fields.push_back({QStringLiteral("InstdAmt @Ccy"), ccy});
        }
    }
    addPartyFields(
        n.fields, tx, "Cdtr", "CdtrAcct", QStringLiteral("Cdtr"), QStringLiteral("CdtrAcct"));

    // Remittance info: unstructured vs structured (ISO 11649 SCOR ref).
    if (const auto rmt = tx.child("RmtInf"); !rmt.empty()) {
        addPath(n.fields, rmt, "Ustrd", QStringLiteral("RmtInf / Ustrd"));
        addPath(n.fields,
                rmt,
                "Strd/CdtrRefInf/Ref",
                QStringLiteral("RmtInf / Strd / CdtrRefInf / Ref"));
        addPath(
            n.fields, rmt, "Strd/CdtrRefInf/Tp/CdOrPrtry/Cd", QStringLiteral("RmtInf / Strd / Tp"));
    }
    return n;
}

Node buildPaymentInfo(const pugi::xml_node& pmtInf, int index) {
    Node n;
    n.name = QStringLiteral("Payment Info %1").arg(index);

    addField(n.fields, pmtInf, "PmtInfId");
    addField(n.fields, pmtInf, "PmtMtd");
    addField(n.fields, pmtInf, "BtchBookg");
    addField(n.fields, pmtInf, "NbOfTxs");
    addField(n.fields, pmtInf, "CtrlSum");
    addPath(n.fields, pmtInf, "PmtTpInf/SvcLvl/Cd", QStringLiteral("SvcLvl"));
    addPath(n.fields, pmtInf, "PmtTpInf/CtgyPurp/Cd", QStringLiteral("CtgyPurp"));
    addPath(n.fields, pmtInf, "ReqdExctnDt/Dt", QStringLiteral("ReqdExctnDt"));
    addField(n.fields, pmtInf, "ChrgBr");
    addPartyFields(
        n.fields, pmtInf, "Dbtr", "DbtrAcct", QStringLiteral("Dbtr"), QStringLiteral("DbtrAcct"));

    int txIndex = 1;
    for (auto tx = pmtInf.child("CdtTrfTxInf"); !tx.empty(); tx = tx.next_sibling("CdtTrfTxInf")) {
        n.children.push_back(buildTransaction(tx, txIndex++));
    }
    return n;
}

Node buildGroupHeader(const pugi::xml_node& grpHdr) {
    Node n;
    n.name = QStringLiteral("Group Header");

    addField(n.fields, grpHdr, "MsgId");
    addField(n.fields, grpHdr, "CreDtTm");
    addField(n.fields, grpHdr, "NbOfTxs");
    addField(n.fields, grpHdr, "CtrlSum");
    addPath(n.fields, grpHdr, "InitgPty/Nm", QStringLiteral("InitgPty / Nm"));
    addPath(n.fields, grpHdr, "InitgPty/Id/OrgId/Othr/Id", QStringLiteral("InitgPty / OrgId / Id"));
    return n;
}

Node buildPain001v13(const pugi::xml_node& cstmrInit) {
    Node root;
    root.name = QStringLiteral("pain.001.001.13 — Customer Credit Transfer Initiation");

    if (const auto grpHdr = cstmrInit.child("GrpHdr"); !grpHdr.empty()) {
        root.children.push_back(buildGroupHeader(grpHdr));
    }
    int pmtIndex = 1;
    for (auto p = cstmrInit.child("PmtInf"); !p.empty(); p = p.next_sibling("PmtInf")) {
        root.children.push_back(buildPaymentInfo(p, pmtIndex++));
    }
    return root;
}

}  // namespace

ParseResult parseFile(const QString& path) {
    ParseResult result;

    QFile file(path);
    if (!file.exists()) {
        result.status = ParseStatus::FileNotFound;
        result.errorMessage = QStringLiteral("File does not exist: %1").arg(path);
        return result;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        result.status = ParseStatus::FileNotFound;
        result.errorMessage = QStringLiteral("Cannot read file: %1").arg(file.errorString());
        return result;
    }
    const QByteArray bytes = file.readAll();
    file.close();
    result.rawXml = QString::fromUtf8(bytes);

    pugi::xml_document doc;
    const auto loadResult = doc.load_buffer(bytes.constData(), static_cast<size_t>(bytes.size()));
    if (!loadResult) {
        result.status = ParseStatus::MalformedXml;
        result.errorMessage = QStringLiteral("XML parse error at offset %1: %2")
                                  .arg(static_cast<qulonglong>(loadResult.offset))
                                  .arg(QString::fromUtf8(loadResult.description()));
        return result;
    }

    const auto docNode = doc.child("Document");
    if (docNode.empty()) {
        result.status = ParseStatus::UnrecognizedMessage;
        result.errorMessage = QStringLiteral(
            "Root element is not <Document>; this doesn't look like an ISO 20022 SEPA message.");
        return result;
    }

    const auto xmlns = QString::fromUtf8(docNode.attribute("xmlns").value());
    const auto detected = detectFromNamespace(xmlns);
    result.detectedFamily = detected.family;
    result.detectedVersion = detected.version;

    if (detected.family != QLatin1String(kSupportedFamily) ||
        detected.version != QLatin1String(kSupportedVersion)) {
        result.status = ParseStatus::UnrecognizedMessage;
        result.errorMessage =
            detected.family.isEmpty()
                ? QStringLiteral("Unrecognized message namespace: %1").arg(xmlns)
                : QStringLiteral(
                      "This file is %1 version %2. Currently only pain.001.001.13 is supported. "
                      "Multi-version coverage is on the roadmap — see "
                      "plan/02-multi-version-support.md.")
                      .arg(detected.family, detected.version);
        return result;
    }

    const auto cstmrInit = docNode.child("CstmrCdtTrfInitn");
    if (cstmrInit.empty()) {
        result.status = ParseStatus::MalformedXml;
        result.errorMessage = QStringLiteral(
            "pain.001.001.13 file is missing the required <CstmrCdtTrfInitn> element.");
        return result;
    }

    result.root = buildPain001v13(cstmrInit);
    result.status = ParseStatus::Success;
    return result;
}

}  // namespace sepa
