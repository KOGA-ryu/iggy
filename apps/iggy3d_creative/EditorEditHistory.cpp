#include "EditorEdits.hpp"
#include "EditorEditsInternal.hpp"

#include "EditorAttachmentPlacement.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "EditorWorldLayoutSources.hpp"
#include "EditorWorldLayoutHistory.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/mutation/Mutation.hpp"
#include "app/iggy3d/creative/recipes/PatternRecipe.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

namespace iggy3d_creative_app {
namespace creative = iggy3d::creative;
namespace {
bool applyHistoryDirection(creative::CreativeAppState& appState,
                           creative::CreativeHistoryDirection direction,
                           std::string_view commandSource,
                           CreativeEditorWorldLayoutState* worldLayout) {
  const creative::CreativeHistoryApplyReceipt receipt =
      worldLayout != nullptr
          ? applyCreativeEditorWorldLayoutHistory(*worldLayout, appState,
                                                  direction)
          : creative::applyCreativeHistory(appState.facade, appState.history,
                                           direction);
  const creative::Id selectionAfter =
      appState.facade.selectionState().selectedTarget.value;
  SDL_Log("iggy3d_creative: HISTORY %s commandSource='%s' editSource='%s' "
          "accepted=%d changed=%d status='%s' undoBefore=%llu undoAfter=%llu "
          "redoBefore=%llu redoAfter=%llu objectCountBefore=%llu "
          "objectCountAfter=%llu revisionAfter=%llu selectionAfter=%u "
          "reasonCode='%s'",
          std::string(creative::toString(direction)).c_str(),
          std::string(commandSource).c_str(), receipt.source.c_str(),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.undoDepthBefore),
          static_cast<unsigned long long>(receipt.undoDepthAfter),
          static_cast<unsigned long long>(receipt.redoDepthBefore),
          static_cast<unsigned long long>(receipt.redoDepthAfter),
          static_cast<unsigned long long>(receipt.objectCountBefore),
          static_cast<unsigned long long>(receipt.objectCountAfter),
          static_cast<unsigned long long>(receipt.revisionAfter), selectionAfter,
          receipt.reasonCode.c_str());
  return receipt.accepted;
}

}  // namespace

void clearEditHistory(StandaloneEditHistory& history,
                      std::string_view source) {
  const std::uint64_t undoBefore = creative::creativeUndoDepth(history);
  const std::uint64_t redoBefore = creative::creativeRedoDepth(history);
  creative::clearCreativeHistory(history);
  if (undoBefore > 0U || redoBefore > 0U) {
    SDL_Log("iggy3d_creative: HISTORY cleared source='%s' undoBefore=%llu "
            "redoBefore=%llu",
            std::string(source).c_str(),
            static_cast<unsigned long long>(undoBefore),
            static_cast<unsigned long long>(redoBefore));
  }
}

StandaloneEditTransaction beginEditTransaction(const creative::Facade& facade,
                                                std::string_view source) {
  return creative::beginCreativeHistoryTransaction(facade, source);
}

StandaloneEditTransaction beginEditTransaction(
    const creative::Facade& facade,
    std::string_view source,
    creative::CreativeHistorySidecar beforeSidecar) {
  return creative::beginCreativeHistoryTransaction(
      facade, source, std::move(beforeSidecar));
}

StandaloneEditTransaction beginEditTransaction(
    const creative::Facade& facade,
    std::string_view source,
    creative::CreativeAuthoringOperationRecord operation) {
  return creative::beginCreativeHistoryTransaction(facade, source,
                                                    std::move(operation));
}

StandaloneEditTransaction beginEditTransaction(
    const creative::Facade& facade,
    std::string_view source,
    std::optional<creative::CreativeHistorySidecar> beforeSidecar,
    std::optional<creative::CreativeAuthoringOperationRecord> operation) {
  return creative::beginCreativeHistoryTransaction(
      facade, source, std::move(beforeSidecar), std::move(operation));
}

bool setEditTransactionOperation(
    StandaloneEditTransaction& transaction,
    creative::CreativeAuthoringFamily family,
    creative::CreativeAuthoringOperationKind kind,
    std::string_view action,
    std::uint64_t requestFingerprint,
    std::uint64_t affectedMemberCount) {
  return creative::setCreativeHistoryTransactionOperation(
      transaction, family, kind, action, requestFingerprint,
      affectedMemberCount);
}

void cancelEditTransaction(StandaloneEditTransaction& transaction) noexcept {
  creative::cancelCreativeHistoryTransaction(transaction);
}

creative::CreativeHistoryRecordReceipt completeEditTransaction(
    StandaloneEditHistory& history,
    StandaloneEditTransaction transaction,
    const creative::Facade& facade,
    bool changed,
    std::string_view reasonCode) {
  const std::string source = transaction.source;
  const creative::CreativeDocument& current = facade.document();
  const bool documentChanged =
      transaction.active && transaction.before.isValid() &&
      current.isValid() &&
      transaction.before.id() == current.id() &&
      transaction.before.revision() != current.revision();
  if (!changed && documentChanged) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "iggy3d_creative: HISTORY changed-result mismatch "
                "source='%s' reportedChanged=0 revisionBefore=%llu "
                "revisionAfter=%llu reasonCode='%s'",
                source.c_str(),
                static_cast<unsigned long long>(
                    transaction.before.revision()),
                static_cast<unsigned long long>(current.revision()),
                std::string(reasonCode).c_str());
  }
  if (!changed && !documentChanged) {
    cancelEditTransaction(transaction);
    SDL_Log("iggy3d_creative: HISTORY cancelled source='%s' undo=%llu redo=%llu "
            "reasonCode='%s'",
            source.c_str(),
            static_cast<unsigned long long>(creative::creativeUndoDepth(history)),
            static_cast<unsigned long long>(creative::creativeRedoDepth(history)),
            std::string(reasonCode).c_str());
    return {};
  }

  creative::CreativeHistoryRecordReceipt receipt =
      creative::commitCreativeHistoryTransaction(history, std::move(transaction),
                                                  facade);
  SDL_Log("iggy3d_creative: HISTORY recorded source='%s' accepted=%d "
          "status='%s' undoBefore=%llu undoAfter=%llu redoCleared=%llu "
          "trimmed=%d reasonCode='%s'",
          source.c_str(), receipt.accepted ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.undoDepthBefore),
          static_cast<unsigned long long>(receipt.undoDepthAfter),
          static_cast<unsigned long long>(receipt.clearedRedoCount),
          receipt.trimmedOldestUndo ? 1 : 0,
          std::string(receipt.reasonCode).c_str());
  return receipt;
}

bool undoLastEdit(creative::CreativeAppState& appState,
                  std::string_view source,
                  CreativeEditorWorldLayoutState* worldLayout) {
  return applyHistoryDirection(appState, creative::CreativeHistoryDirection::Undo,
                               source, worldLayout);
}

bool redoLastEdit(creative::CreativeAppState& appState,
                  std::string_view source,
                  CreativeEditorWorldLayoutState* worldLayout) {
  return applyHistoryDirection(appState, creative::CreativeHistoryDirection::Redo,
                               source, worldLayout);
}


}  // namespace iggy3d_creative_app
