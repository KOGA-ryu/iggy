#include "EditorEdits.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/mutation/Mutation.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"

namespace iggy3d_creative_app {
namespace creative = iggy3d::creative;
namespace {

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

creative::CreativeFacadeMutationReceipt toggleSelectedObjectStateWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeMutationKind mutationKind,
    std::string_view source) {
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  creative::CreativeFacadeMutationReceipt receipt =
      mutationKind == creative::CreativeMutationKind::SetLocked
          ? appState.facade.toggleSelectedObjectLocked()
          : appState.facade.toggleSelectedObjectVisibility();
  static_cast<void>(completeEditTransaction(
      history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.message));
  SDL_Log("iggy3d_creative: OBJECT STATE source='%s' kind='%s' accepted=%d "
          "changed=%d objectId=%llu revisionBefore=%llu revisionAfter=%llu",
          std::string(source).c_str(),
          std::string(creative::toString(receipt.mutationKind)).c_str(),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          static_cast<unsigned long long>(receipt.objectId),
          static_cast<unsigned long long>(receipt.revisionBefore),
          static_cast<unsigned long long>(receipt.revisionAfter));
  return receipt;
}

// Resolves the object ids a batch desktop edit should act on: the explicit span
// when non-empty, otherwise the current selection (mirroring the facade's
// selectedObjectIds fallback — the ordered list, or the primary alone).
[[nodiscard]] std::vector<creative::CreativeObjectId> gatherDesktopTargetIds(
    const creative::CreativeAppState& appState,
    std::span<const creative::CreativeObjectId> explicitIds) {
  std::vector<creative::CreativeObjectId> ids;
  if (!explicitIds.empty()) {
    ids.assign(explicitIds.begin(), explicitIds.end());
    return ids;
  }
  const creative::CreativeSelectionState& selection =
      appState.facade.selectionState();
  const std::span<const creative::TargetRef> targets =
      creative::selectedTargetList(selection);
  if (targets.empty()) {
    if (selection.selectedTarget.value != creative::kInvalidId) {
      ids.push_back(static_cast<creative::CreativeObjectId>(
          selection.selectedTarget.value));
    }
    return ids;
  }
  ids.reserve(targets.size());
  for (const creative::TargetRef& target : targets) {
    if (target.value != creative::kInvalidId) {
      ids.push_back(static_cast<creative::CreativeObjectId>(target.value));
    }
  }
  return ids;
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

creative::CreativeFacadeMutationReceipt
toggleSelectedObjectVisibilityWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::string_view source) {
  return toggleSelectedObjectStateWithUndo(
      appState, history, creative::CreativeMutationKind::SetVisible, source);
}

creative::CreativeFacadeMutationReceipt toggleSelectedObjectLockedWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::string_view source) {
  return toggleSelectedObjectStateWithUndo(
      appState, history, creative::CreativeMutationKind::SetLocked, source);
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
  return pasteClipboardWithHistory(appState, appState.clipboard, request, source);
}

creative::CreativeClipboardPasteReceipt pasteClipboardWithHistory(
    creative::CreativeAppState& appState,
    const creative::CreativeClipboard& clipboard,
    const creative::CreativeClipboardPasteRequest& request,
    std::string_view source) {
  const std::uint64_t depthBefore =
      creative::creativeUndoDepth(appState.history);
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  creative::CreativeClipboardPasteReceipt receipt =
      appState.facade.pasteClipboard(clipboard, request);
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

CreativeStandaloneBatchEditReceipt deleteObjectsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    std::string_view source) {
  CreativeStandaloneBatchEditReceipt outcome;
  std::vector<creative::CreativeObjectId> targets =
      gatherDesktopTargetIds(appState, objectIds);
  if (targets.empty()) {
    SDL_Log("iggy3d_creative: DELETE MULTI no targets source='%s'",
            std::string(source).c_str());
    outcome.message = "no_delete_targets";
    return outcome;
  }

  // Expand each selected root to its full hierarchy so a group root takes its
  // descendants with it, then remove deepest-first so a parent is only removed
  // once its children are gone (avoids ParentHasChildren).
  const creative::CreativeHierarchySelection hierarchy =
      creative::resolveCreativeObjectHierarchy(appState.facade.document(),
                                               targets);
  std::vector<creative::CreativeObjectId> ordered =
      hierarchy.accepted ? hierarchy.objectIds : targets;
  const auto depthOf = [&appState](creative::CreativeObjectId id) {
    std::uint32_t depth = 0U;
    const creative::CreativeObject* object = appState.facade.findObject(id);
    while (object != nullptr && object->parentId.has_value()) {
      ++depth;
      object = appState.facade.findObject(*object->parentId);
    }
    return depth;
  };
  std::stable_sort(ordered.begin(), ordered.end(),
                   [&depthOf](creative::CreativeObjectId a,
                              creative::CreativeObjectId b) {
                     return depthOf(a) > depthOf(b);
                   });

  const std::uint64_t depthBefore = creative::creativeUndoDepth(history);
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  std::uint64_t removed = 0U;
  for (const creative::CreativeObjectId id : ordered) {
    const creative::CreativeDocumentRemoveReceipt receipt =
        appState.facade.removeDocumentObject(id);
    if (receipt.accepted && receipt.objectRemoved && receipt.changed) {
      ++removed;
    }
  }
  outcome.accepted = true;
  outcome.changed = removed > 0U;
  outcome.affectedObjectCount = removed;
  outcome.message = outcome.changed ? "deleted_objects" : "delete_no_change";
  (void)completeEditTransaction(history, std::move(transaction),
                                appState.facade, outcome.changed,
                                outcome.message);
  SDL_Log("iggy3d_creative: DELETE MULTI source='%s' requested=%zu resolved=%zu "
          "removed=%llu undoBefore=%llu undoAfter=%llu",
          std::string(source).c_str(), targets.size(), ordered.size(),
          static_cast<unsigned long long>(removed),
          static_cast<unsigned long long>(depthBefore),
          static_cast<unsigned long long>(creative::creativeUndoDepth(history)));
  return outcome;
}

creative::CreativeDocumentMutationReceipt renameObjectWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    std::string name,
    std::string_view source) {
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  creative::CreativeDocumentMutationReceipt receipt =
      creative::renameDocumentObject(appState.facade.documentForPersistence(),
                                     objectId, std::move(name));
  const bool applied =
      receipt.status == creative::CreativeDocumentMutationStatus::Applied;
  (void)completeEditTransaction(history, std::move(transaction),
                                appState.facade, applied && receipt.changed,
                                receipt.message);
  SDL_Log("iggy3d_creative: RENAME objectId=%llu accepted=%d changed=%d "
          "status=%d source='%s'",
          static_cast<unsigned long long>(objectId), applied ? 1 : 0,
          receipt.changed ? 1 : 0, static_cast<int>(receipt.status),
          std::string(source).c_str());
  return receipt;
}

namespace {

using DocBoolMutation = creative::CreativeDocumentMutationReceipt (*)(
    creative::CreativeDocument&, creative::CreativeObjectId, bool,
    const creative::CreativeDocumentMutationOptions&);

CreativeStandaloneBatchEditReceipt applyObjectsBoolStateWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    bool value,
    DocBoolMutation apply,
    const char* verb,
    std::string_view source) {
  CreativeStandaloneBatchEditReceipt outcome;
  const std::vector<creative::CreativeObjectId> targets =
      gatherDesktopTargetIds(appState, objectIds);
  if (targets.empty()) {
    outcome.message = "no_targets";
    return outcome;
  }
  creative::CreativeDocument& document =
      appState.facade.documentForPersistence();
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  std::uint64_t affected = 0U;
  for (const creative::CreativeObjectId id : targets) {
    const creative::CreativeDocumentMutationReceipt receipt =
        apply(document, id, value, creative::CreativeDocumentMutationOptions{});
    if (receipt.status == creative::CreativeDocumentMutationStatus::Applied &&
        receipt.changed) {
      ++affected;
    }
  }
  outcome.accepted = true;
  outcome.changed = affected > 0U;
  outcome.affectedObjectCount = affected;
  outcome.message = outcome.changed ? "state_changed" : "state_no_change";
  (void)completeEditTransaction(history, std::move(transaction),
                                appState.facade, outcome.changed,
                                outcome.message);
  SDL_Log("iggy3d_creative: OBJECT SET %s source='%s' targets=%zu value=%d "
          "affected=%llu",
          verb, std::string(source).c_str(), targets.size(), value ? 1 : 0,
          static_cast<unsigned long long>(affected));
  return outcome;
}

}  // namespace

CreativeStandaloneBatchEditReceipt setObjectsVisibleWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    bool visible,
    std::string_view source) {
  return applyObjectsBoolStateWithUndo(appState, history, objectIds, visible,
                                       &creative::setDocumentObjectVisible,
                                       "VISIBLE", source);
}

CreativeStandaloneBatchEditReceipt setObjectsLockedWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    bool locked,
    std::string_view source) {
  return applyObjectsBoolStateWithUndo(appState, history, objectIds, locked,
                                       &creative::setDocumentObjectLocked,
                                       "LOCKED", source);
}

CreativeStandaloneBatchEditReceipt setObjectTransformWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    const creative::CreativeTransform& transform,
    bool setPosition,
    bool setRotation,
    bool setScale,
    std::string_view source) {
  CreativeStandaloneBatchEditReceipt outcome;
  if (!setPosition && !setRotation && !setScale) {
    outcome.message = "no_transform_components";
    return outcome;
  }
  if (appState.facade.findObject(objectId) == nullptr) {
    outcome.message = "missing_object";
    return outcome;
  }

  creative::CreativeDocument& document =
      appState.facade.documentForPersistence();
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  bool anyChanged = false;
  const auto record =
      [&anyChanged](const creative::CreativeDocumentMutationReceipt& receipt) {
        if (receipt.status ==
                creative::CreativeDocumentMutationStatus::Applied &&
            receipt.changed) {
          anyChanged = true;
        }
      };
  if (setPosition && setRotation && setScale) {
    // All three components collapse to a single whole-transform mutation.
    record(creative::applyDocumentMutation(
        document, objectId, creative::CreativeMutationKind::SetTransform,
        creative::makeSetTransformPayload(transform)));
  } else {
    if (setPosition) {
      record(creative::applyDocumentMutation(
          document, objectId, creative::CreativeMutationKind::Move,
          creative::makeMovePayload(transform.position)));
    }
    if (setRotation) {
      record(creative::applyDocumentMutation(
          document, objectId, creative::CreativeMutationKind::Rotate,
          creative::makeRotatePayload(transform.rotationEulerRadians)));
    }
    if (setScale) {
      record(creative::applyDocumentMutation(
          document, objectId, creative::CreativeMutationKind::Scale,
          creative::makeScalePayload(transform.scale)));
    }
  }
  outcome.accepted = true;
  outcome.changed = anyChanged;
  outcome.affectedObjectCount = anyChanged ? 1U : 0U;
  outcome.message = anyChanged ? "set_transform" : "transform_no_change";
  (void)completeEditTransaction(history, std::move(transaction),
                                appState.facade, anyChanged, outcome.message);
  SDL_Log("iggy3d_creative: SET TRANSFORM objectId=%llu pos=%d rot=%d scale=%d "
          "changed=%d source='%s'",
          static_cast<unsigned long long>(objectId), setPosition ? 1 : 0,
          setRotation ? 1 : 0, setScale ? 1 : 0, anyChanged ? 1 : 0,
          std::string(source).c_str());
  return outcome;
}

}  // namespace iggy3d_creative_app
