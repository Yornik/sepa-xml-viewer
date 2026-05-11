#include "sepa_controller.h"

#include "sepa/sepa.h"

#include <QFileInfo>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace sepa::ui {

namespace {
// Custom Qt::ItemDataRole for stashing the field list on each tree item.
// Anything >= Qt::UserRole is safe for app-defined roles per QAbstractItemModel.
constexpr int kFieldsRole = Qt::UserRole + 1;
}  // namespace

SepaController::SepaController(QObject* parent) : QObject(parent) {
    // One column, no header label — TreeView in QML hides it anyway and the
    // default Qt::DisplayRole on each row is enough.
    model_.setColumnCount(1);
}

QAbstractItemModel* SepaController::treeModel() {
    return &model_;
}

QStandardItem* SepaController::buildItem(const sepa::Node& node) {
    auto* item = new QStandardItem(node.name);
    item->setEditable(false);

    QVariantList fieldList;
    fieldList.reserve(static_cast<int>(node.fields.size()));
    for (const auto& f : node.fields) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), f.name);
        row.insert(QStringLiteral("value"), f.value);
        fieldList.push_back(row);
    }
    item->setData(fieldList, kFieldsRole);

    for (const auto& child : node.children) {
        item->appendRow(buildItem(child));
    }
    return item;
}

void SepaController::rebuildModelFrom(const sepa::Node& root) {
    model_.clear();
    model_.setColumnCount(1);
    model_.appendRow(buildItem(root));
}

QVariantList SepaController::fieldsForIndex(const QModelIndex& index) const {
    if (!index.isValid()) {
        return {};
    }
    // The QModelIndex from QML's TreeView may be from a proxy — fall back to
    // looking up by row/parent within our model directly when needed.
    const auto data = model_.data(index, kFieldsRole);
    if (data.isValid()) {
        return data.toList();
    }
    return {};
}

void SepaController::loadFile(const QUrl& fileUrl) {
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    currentFilePath_ = path;

    const auto result = sepa::parseFile(path);
    rawXml_ = result.rawXml;

    if (result.status == sepa::ParseStatus::Success) {
        rebuildModelFrom(result.root);
        hasContent_ = true;
        isError_ = false;
        statusMessage_ =
            QStringLiteral("Loaded %1 (%2 %3).")
                .arg(QFileInfo(path).fileName(), result.detectedFamily, result.detectedVersion);

        // Compute a one-line summary for the status bar: total transactions,
        // batch count, file size. Walks the parsed tree (cheap — already in
        // memory). Layout: "5 txs across 2 batches · 14.0 KB".
        int txCount = 0;
        int batchCount = 0;
        if (!result.root.children.empty()) {
            // root.children = [GrpHdr, PmtInf, PmtInf, ...]; the PmtInf
            // entries each have CdtTrfTxInf children. Skip the GrpHdr.
            for (size_t i = 1; i < result.root.children.size(); ++i) {
                ++batchCount;
                txCount += static_cast<int>(result.root.children[i].children.size());
            }
        }
        // Format the file size as B / KB / MB. Q_INT64 multiplications stay
        // in qint64 (the LL suffix avoids the int-overflow lint), and the
        // / 1024 fan-out uses an explicit cast to double rather than
        // mixed-type arithmetic.
        const qint64 fileSize = QFileInfo(path).size();
        const auto sizeAsDouble = static_cast<double>(fileSize);
        QString humanSize;
        if (fileSize < 1024LL) {
            humanSize = QStringLiteral("%1 B").arg(fileSize);
        } else if (fileSize < 1024LL * 1024LL) {
            humanSize = QStringLiteral("%1 KB").arg(sizeAsDouble / 1024.0, 0, 'f', 1);
        } else {
            humanSize = QStringLiteral("%1 MB").arg(sizeAsDouble / (1024.0 * 1024.0), 0, 'f', 1);
        }
        summary_ = QStringLiteral("%1 transaction%2 across %3 batch%4 · %5")
                       .arg(txCount)
                       .arg(txCount == 1 ? QString() : QStringLiteral("s"))
                       .arg(batchCount)
                       .arg(batchCount == 1 ? QString() : QStringLiteral("es"))
                       .arg(humanSize);
    } else {
        // FileNotFound / MalformedXml / UnrecognizedMessage all surface the
        // same way to the user: clear the tree, show the parser's message
        // in red. The status enum still distinguishes them for diagnostics.
        model_.clear();
        hasContent_ = false;
        isError_ = true;
        statusMessage_ = result.errorMessage;
        summary_.clear();
    }
    emit stateChanged();
}

}  // namespace sepa::ui
