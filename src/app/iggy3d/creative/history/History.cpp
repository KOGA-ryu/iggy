#include "app/iggy3d/creative/history/History.hpp"

#include <utility>

namespace iggy3d::creative {
namespace {

void appendBounded(std::vector<CreativeDocumentHistorySnapshot>& snapshots,
                   CreativeDocumentHistorySnapshot snapshot,
                   std::size_t maxDepth,
                   bool* trimmed = nullptr) {
  if (snapshots.size() >= maxDepth) {
    snapshots.erase(snapshots.begin());
    if (trimmed != nullptr) {
      *trimmed = true;
    }
  }
  snapshots.push_back(std::move(snapshot));
}

void setRecordStatus(CreativeHistoryRecordReceipt& receipt,
                     CreativeHistoryStatus status,
                     std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
}

}  // namespace

std::string_view toString(CreativeHistoryDirection direction) noexcept {
  switch (direction) {
    case CreativeHistoryDirection::Undo:
      return "Undo";
    case CreativeHistoryDirection::Redo:
      return "Redo";
  }
  return "Unknown";
}

std::string_view toString(CreativeHistoryStatus status) noexcept {
  switch (status) {
    case CreativeHistoryStatus::NotRequested:
      return "NotRequested";
    case CreativeHistoryStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeHistoryStatus::InactiveTransaction:
      return "InactiveTransaction";
    case CreativeHistoryStatus::Disabled:
      return "Disabled";
    case CreativeHistoryStatus::NoChange:
      return "NoChange";
    case CreativeHistoryStatus::Recorded:
      return "Recorded";
    case CreativeHistoryStatus::Empty:
      return "Empty";
    case CreativeHistoryStatus::InstallRejected:
      return "InstallRejected";
    case CreativeHistoryStatus::Applied:
      return "Applied";
  }
  return "Unknown";
}

bool creativeUndoAvailable(const CreativeDocumentHistory& history) noexcept {
  return !history.undoSnapshots.empty();
}

bool creativeRedoAvailable(const CreativeDocumentHistory& history) noexcept {
  return !history.redoSnapshots.empty();
}

std::uint64_t creativeUndoDepth(
    const CreativeDocumentHistory& history) noexcept {
  return history.undoSnapshots.size();
}

std::uint64_t creativeRedoDepth(
    const CreativeDocumentHistory& history) noexcept {
  return history.redoSnapshots.size();
}

void clearCreativeHistory(CreativeDocumentHistory& history) noexcept {
  history.undoSnapshots.clear();
  history.redoSnapshots.clear();
}

CreativeDocumentHistoryTransaction beginCreativeHistoryTransaction(
    const Facade& facade,
    std::string_view source) {
  return beginCreativeHistoryTransaction(facade, source, std::nullopt,
                                          std::nullopt);
}

CreativeDocumentHistoryTransaction beginCreativeHistoryTransaction(
    const Facade& facade,
    std::string_view source,
    std::optional<CreativeHistorySidecar> beforeSidecar) {
  return beginCreativeHistoryTransaction(facade, source,
                                          std::move(beforeSidecar),
                                          std::nullopt);
}

CreativeDocumentHistoryTransaction beginCreativeHistoryTransaction(
    const Facade& facade,
    std::string_view source,
    CreativeAuthoringOperationRecord operation) {
  return beginCreativeHistoryTransaction(facade, source, std::nullopt,
                                          std::move(operation));
}

CreativeDocumentHistoryTransaction beginCreativeHistoryTransaction(
    const Facade& facade,
    std::string_view source,
    std::optional<CreativeHistorySidecar> beforeSidecar,
    std::optional<CreativeAuthoringOperationRecord> operation) {
  CreativeDocumentHistoryTransaction transaction;
  const CreativeDocument& document = facade.document();
  if (!document.isValid() || document.id() == kInvalidDocumentId) {
    return transaction;
  }
  if (operation.has_value() &&
      !validateCreativeAuthoringOperationRecord(*operation)) {
    return transaction;
  }
  transaction.active = true;
  transaction.before = document;
  transaction.source = source;
  transaction.beforeSidecar = std::move(beforeSidecar);
  transaction.operation = std::move(operation);
  return transaction;
}

bool setCreativeHistoryTransactionOperation(
    CreativeDocumentHistoryTransaction& transaction,
    CreativeAuthoringFamily family,
    CreativeAuthoringOperationKind kind,
    std::string_view action,
    std::uint64_t requestFingerprint,
    std::uint64_t affectedMemberCount) {
  if (!transaction.active) {
    return false;
  }
  std::optional<CreativeAuthoringOperationRecord> operation =
      makeCreativeAuthoringOperationRecord(
          family, kind, action, requestFingerprint, affectedMemberCount);
  if (!operation.has_value()) {
    return false;
  }
  transaction.operation = std::move(operation);
  return true;
}

CreativeHistoryRecordReceipt commitCreativeHistoryTransaction(
    CreativeDocumentHistory& history,
    CreativeDocumentHistoryTransaction transaction,
    const Facade& facade) {
  CreativeHistoryRecordReceipt receipt;
  receipt.requested = true;
  receipt.undoDepthBefore = creativeUndoDepth(history);
  receipt.redoDepthBefore = creativeRedoDepth(history);
  receipt.undoDepthAfter = receipt.undoDepthBefore;
  receipt.redoDepthAfter = receipt.redoDepthBefore;

  if (!transaction.active) {
    setRecordStatus(receipt, CreativeHistoryStatus::InactiveTransaction,
                    "creative_history_transaction_inactive");
    return receipt;
  }
  if (transaction.operation.has_value() &&
      !validateCreativeAuthoringOperationRecord(*transaction.operation)) {
    setRecordStatus(receipt, CreativeHistoryStatus::InactiveTransaction,
                    "creative_history_operation_invalid");
    return receipt;
  }

  const CreativeDocument& current = facade.document();
  if (!transaction.before.isValid() || !current.isValid() ||
      transaction.before.id() == kInvalidDocumentId ||
      transaction.before.id() != current.id()) {
    setRecordStatus(receipt, CreativeHistoryStatus::InvalidDocument,
                    "creative_history_document_invalid");
    return receipt;
  }
  if (transaction.before.revision() == current.revision()) {
    setRecordStatus(receipt, CreativeHistoryStatus::NoChange,
                    "creative_history_document_unchanged");
    return receipt;
  }
  if (history.maxDepth == 0U) {
    setRecordStatus(receipt, CreativeHistoryStatus::Disabled,
                    "creative_history_disabled");
    return receipt;
  }

  receipt.clearedRedoCount = creativeRedoDepth(history);
  history.redoSnapshots.clear();
  appendBounded(history.undoSnapshots,
                {std::move(transaction.before), std::move(transaction.source),
                 std::move(transaction.beforeSidecar),
                 std::move(transaction.operation)},
                history.maxDepth, &receipt.trimmedOldestUndo);
  receipt.accepted = true;
  receipt.recorded = true;
  receipt.undoDepthAfter = creativeUndoDepth(history);
  receipt.redoDepthAfter = creativeRedoDepth(history);
  setRecordStatus(receipt, CreativeHistoryStatus::Recorded,
                  "creative_history_recorded");
  return receipt;
}

void cancelCreativeHistoryTransaction(
    CreativeDocumentHistoryTransaction& transaction) noexcept {
  transaction = {};
}

CreativeHistoryApplyReceipt applyCreativeHistory(
    Facade& facade,
    CreativeDocumentHistory& history,
    CreativeHistoryDirection direction,
    std::optional<CreativeHistorySidecar> currentSidecar) {
  CreativeHistoryApplyReceipt receipt;
  receipt.requested = true;
  receipt.direction = direction;
  receipt.revisionBefore = facade.document().revision();
  receipt.objectCountBefore = facade.document().objectCount();
  receipt.undoDepthBefore = creativeUndoDepth(history);
  receipt.redoDepthBefore = creativeRedoDepth(history);
  receipt.undoDepthAfter = receipt.undoDepthBefore;
  receipt.redoDepthAfter = receipt.redoDepthBefore;

  if (history.maxDepth == 0U) {
    receipt.status = CreativeHistoryStatus::Disabled;
    receipt.reasonCode = "creative_history_disabled";
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.objectCountAfter = receipt.objectCountBefore;
    return receipt;
  }

  const bool undo = direction == CreativeHistoryDirection::Undo;
  const auto& sourceSnapshots =
      undo ? history.undoSnapshots : history.redoSnapshots;
  if (sourceSnapshots.empty()) {
    receipt.status = CreativeHistoryStatus::Empty;
    receipt.reasonCode = undo ? "creative_history_undo_empty"
                              : "creative_history_redo_empty";
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.objectCountAfter = receipt.objectCountBefore;
    return receipt;
  }

  receipt.hadSnapshot = true;
  receipt.documentId = sourceSnapshots.back().document.id();
  receipt.source = sourceSnapshots.back().source;
  receipt.targetSidecar = sourceSnapshots.back().sidecar;
  receipt.targetOperation = sourceSnapshots.back().operation;

  CreativeDocumentHistory staged = history;
  auto& stagedSource =
      undo ? staged.undoSnapshots : staged.redoSnapshots;
  auto& stagedDestination =
      undo ? staged.redoSnapshots : staged.undoSnapshots;
  CreativeDocument target = stagedSource.back().document;
  const std::string source = stagedSource.back().source;
  std::optional<CreativeAuthoringOperationRecord> operation =
      stagedSource.back().operation;
  stagedSource.pop_back();
  if (staged.maxDepth > 0U) {
    appendBounded(stagedDestination,
                  {facade.document(), source, std::move(currentSidecar),
                   std::move(operation)},
                  staged.maxDepth);
  }

  receipt.installReceipt = facade.installDocument(std::move(target));
  receipt.accepted = receipt.installReceipt.accepted;
  receipt.changed = receipt.installReceipt.changed;
  if (!receipt.accepted) {
    receipt.status = CreativeHistoryStatus::InstallRejected;
    receipt.reasonCode = std::string(receipt.installReceipt.reasonCode);
  } else {
    history = std::move(staged);
    receipt.status = CreativeHistoryStatus::Applied;
    receipt.reasonCode = undo ? "creative_history_undo_applied"
                              : "creative_history_redo_applied";
  }
  receipt.revisionAfter = facade.document().revision();
  receipt.objectCountAfter = facade.document().objectCount();
  receipt.undoDepthAfter = creativeUndoDepth(history);
  receipt.redoDepthAfter = creativeRedoDepth(history);
  return receipt;
}

const CreativeHistorySidecar* creativeHistoryTargetSidecar(
    const CreativeDocumentHistory& history,
    CreativeHistoryDirection direction) noexcept {
  const auto& snapshots = direction == CreativeHistoryDirection::Undo
                              ? history.undoSnapshots
                              : history.redoSnapshots;
  if (snapshots.empty() || !snapshots.back().sidecar.has_value()) {
    return nullptr;
  }
  return &*snapshots.back().sidecar;
}

const CreativeAuthoringOperationRecord* creativeHistoryTargetOperation(
    const CreativeDocumentHistory& history,
    CreativeHistoryDirection direction) noexcept {
  const auto& snapshots = direction == CreativeHistoryDirection::Undo
                              ? history.undoSnapshots
                              : history.redoSnapshots;
  if (snapshots.empty() || !snapshots.back().operation.has_value()) {
    return nullptr;
  }
  return &*snapshots.back().operation;
}

}  // namespace iggy3d::creative
