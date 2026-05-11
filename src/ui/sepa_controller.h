#pragma once

// Bridge between the pure-C++ sepa::parseFile() and the QML UI.
//
// The controller owns the parsed result, exposes the tree to QML via a
// QStandardItemModel, and answers "what fields go in the detail pane for
// this tree node" via fieldsForIndex(). One controller instance per
// QQmlApplicationEngine — registered as a QML_SINGLETON in
// sepa.viewer.

#include "sepa/sepa.h"

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QObject>
#include <qqmlintegration.h>
#include <QStandardItemModel>
#include <QString>
#include <QUrl>
#include <QVariantList>

namespace sepa::ui {

class SepaController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // Tree model the QML TreeView binds to. Rebuilt on every loadFile() call.
    Q_PROPERTY(QAbstractItemModel* treeModel READ treeModel CONSTANT)

    // Banner shown at the top of the window. Empty means "no message — we're
    // either showing a parsed file or the initial drop-here screen".
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY stateChanged)

    // True iff the most recent load produced a parsed tree. Drives whether
    // the tree / detail / raw XML tabs are visible vs the drop-here screen.
    Q_PROPERTY(bool hasContent READ hasContent NOTIFY stateChanged)

    // True iff statusMessage is an error (file not found / malformed XML /
    // unsupported version) as opposed to informational. Affects banner color.
    Q_PROPERTY(bool isError READ isError NOTIFY stateChanged)

    // Full text of the loaded file, populated whenever bytes could be read
    // (including when parsing then failed). Bound by the Raw XML tab.
    Q_PROPERTY(QString rawXml READ rawXml NOTIFY stateChanged)

    // Path of the currently-loaded file, "" if nothing is loaded. Used in
    // the window title.
    Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY stateChanged)

    // One-line summary the status bar shows: "5 transactions across 2 batches
    // · 14.0 KB". Empty when no file is loaded.
    Q_PROPERTY(QString summary READ summary NOTIFY stateChanged)

public:
    explicit SepaController(QObject* parent = nullptr);

    [[nodiscard]] QAbstractItemModel* treeModel();
    [[nodiscard]] QString statusMessage() const { return statusMessage_; }
    [[nodiscard]] bool hasContent() const { return hasContent_; }
    [[nodiscard]] bool isError() const { return isError_; }
    [[nodiscard]] QString rawXml() const { return rawXml_; }
    [[nodiscard]] QString currentFilePath() const { return currentFilePath_; }
    [[nodiscard]] QString summary() const { return summary_; }

    // Called from QML on drop / File → Open. Parses synchronously; for the
    // 2500-transaction stress fixture this is well under a second on any
    // modern machine, so a worker thread would just add complexity.
    Q_INVOKABLE void loadFile(const QUrl& fileUrl);

    // Look up the (name, value) pairs for a given tree-model index so the
    // detail pane can render them. Returns a QVariantList of QVariantMap
    // entries with "name" and "value" string keys. Empty if the index is
    // invalid or has no associated fields.
    [[nodiscard]] Q_INVOKABLE QVariantList fieldsForIndex(const QModelIndex& index) const;

signals:
    void stateChanged();

private:
    void rebuildModelFrom(const sepa::Node& root);
    static QStandardItem* buildItem(const sepa::Node& node);

    QStandardItemModel model_;
    QString statusMessage_;
    bool hasContent_ = false;
    bool isError_ = false;
    QString rawXml_;
    QString currentFilePath_;
    QString summary_;
};

}  // namespace sepa::ui
