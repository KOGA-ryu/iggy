#include "EditorEdits.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#include "EditorPreviewProxies.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Object.hpp"

namespace iggy3d_creative_app {
namespace creative = iggy3d::creative;
namespace {

constexpr std::size_t kDetailedClipboardPreviewObjectCapacity = 512U;

[[nodiscard]] bool finiteVec3(creative::CreativeVec3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

[[nodiscard]] creative::CreativeVec3 subtract(
    creative::CreativeVec3 lhs,
    creative::CreativeVec3 rhs) noexcept {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] bool renderOffset(creative::CreativeVec3 value,
                                iggy3d::Vec3& out) noexcept {
  constexpr double kMax = std::numeric_limits<float>::max();
  if (!finiteVec3(value) || std::fabs(value.x) > kMax ||
      std::fabs(value.y) > kMax || std::fabs(value.z) > kMax) {
    return false;
  }
  out = {static_cast<float>(value.x), static_cast<float>(value.y),
         static_cast<float>(value.z)};
  return true;
}

[[nodiscard]] bool finiteBounds(VisualBounds bounds) noexcept {
  return std::isfinite(bounds.min.x) && std::isfinite(bounds.min.y) &&
         std::isfinite(bounds.min.z) && std::isfinite(bounds.max.x) &&
         std::isfinite(bounds.max.y) && std::isfinite(bounds.max.z);
}

[[nodiscard]] VisualBounds translatedBounds(VisualBounds bounds,
                                            iggy3d::Vec3 offset) noexcept {
  return {bounds.min + offset, bounds.max + offset};
}

bool applyHistoryDirection(creative::CreativeAppState& appState,
                           creative::CreativeHistoryDirection direction,
                           std::string_view commandSource) {
  const creative::CreativeHistoryApplyReceipt receipt =
      creative::applyCreativeHistory(appState.facade, appState.history,
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

creative::CreativeHistoryRecordReceipt completeEditTransaction(
    StandaloneEditHistory& history,
    StandaloneEditTransaction transaction,
    const creative::Facade& facade,
    bool changed,
    std::string_view reasonCode) {
  const std::string source = transaction.source;
  if (!changed) {
    creative::cancelCreativeHistoryTransaction(transaction);
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
                  std::string_view source) {
  return applyHistoryDirection(appState, creative::CreativeHistoryDirection::Undo,
                               source);
}

bool redoLastEdit(creative::CreativeAppState& appState,
                  std::string_view source) {
  return applyHistoryDirection(appState, creative::CreativeHistoryDirection::Redo,
                               source);
}

creative::CreativeDocumentRemoveReceipt deleteSelectedObject(
    creative::CreativeAppState& appState,
    std::string_view source,
    StandaloneEditHistory* history) {
  const creative::Id selectedId =
      appState.facade.selectionState().selectedTarget.value;
  const std::uint64_t objectCountBefore =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  if (selectedId == 0U) {
    SDL_Log("iggy3d_creative: DELETE no selection source='%s' "
            "objectCount=%llu",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectCountBefore));
    return {};
  }

  const auto objectId = static_cast<creative::CreativeObjectId>(selectedId);
  const creative::CreativeObject* object = appState.facade.findObject(objectId);
  if (object == nullptr) {
    SDL_Log("iggy3d_creative: DELETE missing selection source='%s' "
            "objectId=%llu objectCount=%llu",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId),
            static_cast<unsigned long long>(objectCountBefore));
    return {};
  }

  const creative::CreativeObjectKind kind = object->kind;
  const std::uint64_t undoDepthBefore =
      history != nullptr ? creative::creativeUndoDepth(*history) : 0U;
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  creative::CreativeDocumentRemoveReceipt receipt =
      appState.facade.removeDocumentObject(objectId);
  if (history != nullptr) {
    (void)completeEditTransaction(*history, std::move(transaction),
                                  appState.facade,
                                  receipt.accepted && receipt.objectRemoved &&
                                      receipt.changed,
                                  receipt.reasonCode);
  }
  const std::uint64_t objectCountAfter =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  const creative::Id selectionAfter =
      appState.facade.selectionState().selectedTarget.value;
  SDL_Log("iggy3d_creative: DELETE removed objectId=%llu kind='%s' "
          "accepted=%d changed=%d removed=%d status='%s' reasonCode='%s' "
          "objectCountBefore=%llu objectCountAfter=%llu selectionAfter=%u "
          "undoDepthBefore=%llu undoDepthAfter=%llu",
          static_cast<unsigned long long>(objectId),
          std::string(creative::toString(kind)).c_str(),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          receipt.objectRemoved ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          std::string(receipt.reasonCode).c_str(),
          static_cast<unsigned long long>(objectCountBefore),
          static_cast<unsigned long long>(objectCountAfter), selectionAfter,
          static_cast<unsigned long long>(undoDepthBefore),
          static_cast<unsigned long long>(
              history != nullptr ? creative::creativeUndoDepth(*history) : 0U));
  return receipt;
}

creative::CreativeTransformCommandReceipt transformSelectedObjectsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const creative::CreativeTransformCommandRequest& request,
    std::string_view source) {
  const std::uint64_t depthBefore = creative::creativeUndoDepth(history);
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  creative::CreativeTransformCommandReceipt receipt =
      appState.facade.transformSelectedObjects(request);
  (void)completeEditTransaction(history, std::move(transaction), appState.facade,
                                receipt.accepted && receipt.changed,
                                receipt.message);
  SDL_Log("iggy3d_creative: TRANSFORM source='%s' kind='%s' status='%s' "
          "accepted=%d changed=%d objects=%llu revisionBefore=%llu "
          "revisionAfter=%llu undoDepthBefore=%llu undoDepthAfter=%llu",
          std::string(source).c_str(),
          std::string(creative::toString(receipt.kind)).c_str(),
          std::string(creative::toString(receipt.status)).c_str(),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          static_cast<unsigned long long>(receipt.objectCount),
          static_cast<unsigned long long>(receipt.revisionBefore),
          static_cast<unsigned long long>(receipt.revisionAfter),
          static_cast<unsigned long long>(depthBefore),
          static_cast<unsigned long long>(creative::creativeUndoDepth(history)));
  return receipt;
}

creative::CreativeDuplicateCommandReceipt duplicateSelectedObjectsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const creative::CreativeDuplicateCommandRequest& request,
    std::string_view source) {
  const std::uint64_t depthBefore = creative::creativeUndoDepth(history);
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  creative::CreativeDuplicateCommandReceipt receipt =
      appState.facade.duplicateSelectedObjects(request);
  (void)completeEditTransaction(history, std::move(transaction), appState.facade,
                                receipt.accepted && receipt.changed,
                                receipt.message);
  SDL_Log("iggy3d_creative: DUPLICATE source='%s' status='%s' accepted=%d "
          "changed=%d objects=%llu revisionBefore=%llu revisionAfter=%llu "
          "undoDepthBefore=%llu undoDepthAfter=%llu",
          std::string(source).c_str(),
          std::string(creative::toString(receipt.status)).c_str(),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          static_cast<unsigned long long>(receipt.duplicatedObjectCount),
          static_cast<unsigned long long>(receipt.revisionBefore),
          static_cast<unsigned long long>(receipt.revisionAfter),
          static_cast<unsigned long long>(depthBefore),
          static_cast<unsigned long long>(creative::creativeUndoDepth(history)));
  return receipt;
}

creative::CreativeClipboardCopyReceipt copySelectionToClipboard(
    creative::CreativeAppState& appState,
    std::string_view source) {
  const creative::CreativeClipboardCopyReceipt receipt =
      appState.facade.copySelectedObjectsToClipboard(appState.clipboard);
  SDL_Log("iggy3d_creative: CLIPBOARD copy source='%s' accepted=%d "
          "status='%s' requested=%llu copied=%llu failedObjectId=%llu "
          "reasonCode='%s'",
          std::string(source).c_str(), receipt.accepted ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.requestedObjectCount),
          static_cast<unsigned long long>(receipt.copiedObjectCount),
          static_cast<unsigned long long>(receipt.failedObjectId),
          receipt.reasonCode.c_str());
  return receipt;
}

creative::CreativeClipboardCutReceipt cutSelectionToClipboardWithHistory(
    creative::CreativeAppState& appState,
    std::string_view source) {
  const std::uint64_t depthBefore =
      creative::creativeUndoDepth(appState.history);
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  creative::CreativeClipboardCutReceipt receipt =
      appState.facade.cutSelectedObjectsToClipboard(appState.clipboard);
  (void)completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.reasonCode);
  SDL_Log("iggy3d_creative: CLIPBOARD cut source='%s' accepted=%d changed=%d "
          "status='%s' requested=%llu cut=%llu failedObjectId=%llu "
          "undoBefore=%llu undoAfter=%llu reasonCode='%s'",
          std::string(source).c_str(), receipt.accepted ? 1 : 0,
          receipt.changed ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.requestedObjectCount),
          static_cast<unsigned long long>(receipt.cutObjectCount),
          static_cast<unsigned long long>(receipt.failedObjectId),
          static_cast<unsigned long long>(depthBefore),
          static_cast<unsigned long long>(
              creative::creativeUndoDepth(appState.history)),
          receipt.reasonCode.c_str());
  return receipt;
}

creative::CreativeClipboardPasteReceipt pasteClipboardWithHistory(
    creative::CreativeAppState& appState,
    const creative::CreativeClipboardPasteRequest& request,
    std::string_view source) {
  const std::uint64_t depthBefore =
      creative::creativeUndoDepth(appState.history);
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  creative::CreativeClipboardPasteReceipt receipt =
      appState.facade.pasteClipboard(appState.clipboard, request);
  (void)completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.reasonCode);
  SDL_Log("iggy3d_creative: CLIPBOARD paste source='%s' accepted=%d changed=%d "
          "status='%s' requested=%llu pasted=%llu failedObjectId=%llu "
          "undoBefore=%llu undoAfter=%llu reasonCode='%s'",
          std::string(source).c_str(), receipt.accepted ? 1 : 0,
          receipt.changed ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.requestedObjectCount),
          static_cast<unsigned long long>(receipt.pastedObjectCount),
          static_cast<unsigned long long>(receipt.failedObjectId),
          static_cast<unsigned long long>(depthBefore),
          static_cast<unsigned long long>(
              creative::creativeUndoDepth(appState.history)),
          receipt.reasonCode.c_str());
  return receipt;
}

bool beginCreativeEditorClipboardPastePreview(
    const creative::CreativeClipboard& clipboard,
    CreativeEditorClipboardPasteState& state,
    std::string_view source) {
  state = {};
  if (creative::creativeClipboardEmpty(clipboard) ||
      !clipboard.hasPlacementAnchor ||
      !finiteVec3(clipboard.placementAnchor)) {
    SDL_Log("iggy3d_creative: CLIPBOARD preview rejected source='%s' "
            "objectCount=%zu hasAnchor=%d",
            std::string(source).c_str(), clipboard.objects.size(),
            clipboard.hasPlacementAnchor ? 1 : 0);
    return false;
  }
  state.active = true;
  SDL_Log("iggy3d_creative: CLIPBOARD preview begun source='%s' "
          "objectCount=%zu anchor=(%.3f, %.3f, %.3f)",
          std::string(source).c_str(), clipboard.objects.size(),
          clipboard.placementAnchor.x, clipboard.placementAnchor.y,
          clipboard.placementAnchor.z);
  return true;
}

bool requestCreativeEditorClipboardPasteCommit(
    CreativeEditorClipboardPasteState& state) noexcept {
  if (!state.active || state.commitRequested) {
    return false;
  }
  state.commitRequested = true;
  return true;
}

bool cancelCreativeEditorClipboardPastePreview(
    CreativeEditorClipboardPasteState& state,
    std::string_view source) {
  if (!state.active) {
    return false;
  }
  SDL_Log("iggy3d_creative: CLIPBOARD preview cancelled source='%s'",
          std::string(source).c_str());
  state = {};
  return true;
}

creative::CreativeClipboardPasteReceipt
processCreativeEditorClipboardPastePreview(
    creative::CreativeAppState& appState,
    CreativeEditorClipboardPasteState& state,
    bool targetValid,
    creative::CreativeVec3 targetAnchor,
    bool secondaryPressed,
    std::string_view source) {
  if (!state.active) {
    return {};
  }

  state.targetValid = targetValid &&
                      appState.clipboard.hasPlacementAnchor &&
                      finiteVec3(targetAnchor);
  if (state.targetValid) {
    const creative::CreativeVec3 offset =
        subtract(targetAnchor, appState.clipboard.placementAnchor);
    state.targetValid = finiteVec3(offset);
    if (state.targetValid) {
      state.targetAnchor = targetAnchor;
      state.request.offset = offset;
    }
  }
  if (secondaryPressed) {
    static_cast<void>(requestCreativeEditorClipboardPasteCommit(state));
  }
  if (!state.commitRequested) {
    return {};
  }
  state.commitRequested = false;
  if (!state.targetValid) {
    return {};
  }

  state.lastReceipt = pasteClipboardWithHistory(
      appState, state.request, source);
  if (state.lastReceipt.accepted && state.lastReceipt.changed) {
    state.active = false;
    state.targetValid = false;
  }
  return state.lastReceipt;
}

std::size_t appendCreativeEditorClipboardPastePreview(
    const creative::CreativeClipboard& clipboard,
    const CreativeEditorClipboardPasteState& state,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  if (!state.active || !state.targetValid ||
      creative::creativeClipboardEmpty(clipboard)) {
    return 0U;
  }
  iggy3d::Vec3 offset;
  if (!renderOffset(state.request.offset, offset)) {
    return 0U;
  }

  const std::size_t before = wireLines.size();
  const bool failedCommit = state.lastReceipt.requested &&
                            !state.lastReceipt.accepted;
  const iggy3d::RenderLineColor color =
      failedCommit ? iggy3d::RenderLineColor{1.0F, 0.24F, 0.20F, 0.94F}
                   : iggy3d::RenderLineColor{0.18F, 1.0F, 0.70F, 0.92F};
  const float thickness = std::isfinite(wireThickness)
                              ? std::max(0.025F, wireThickness * 0.8F)
                              : 0.025F;
  if (clipboard.objects.size() <= kDetailedClipboardPreviewObjectCapacity) {
    wireLines.reserve(wireLines.size() + clipboard.objects.size() * 12U);
    for (const creative::CreativeObject& object : clipboard.objects) {
      const VisualBounds bounds = translatedBounds(
          visualBoundsForObject(object), offset);
      if (!finiteBounds(bounds)) {
        continue;
      }
      appendStandaloneWireframeBoxEdges(
          wireLines, bounds.min, bounds.max, color, thickness);
    }
    return wireLines.size() - before;
  }

  VisualBounds aggregate{};
  bool haveAggregate = false;
  for (const creative::CreativeObject& object : clipboard.objects) {
    const VisualBounds bounds = translatedBounds(
        visualBoundsForObject(object), offset);
    if (!finiteBounds(bounds)) {
      continue;
    }
    if (!haveAggregate) {
      aggregate = bounds;
      haveAggregate = true;
      continue;
    }
    aggregate.min.x = std::min(aggregate.min.x, bounds.min.x);
    aggregate.min.y = std::min(aggregate.min.y, bounds.min.y);
    aggregate.min.z = std::min(aggregate.min.z, bounds.min.z);
    aggregate.max.x = std::max(aggregate.max.x, bounds.max.x);
    aggregate.max.y = std::max(aggregate.max.y, bounds.max.y);
    aggregate.max.z = std::max(aggregate.max.z, bounds.max.z);
  }
  if (haveAggregate) {
    appendStandaloneWireframeBoxEdges(
        wireLines, aggregate.min, aggregate.max, color, thickness);
  }
  return wireLines.size() - before;
}

}  // namespace iggy3d_creative_app
