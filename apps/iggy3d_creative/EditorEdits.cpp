#include "EditorEdits.hpp"

#include "EditorAttachmentPlacement.hpp"
#include "EditorWorldLayout.hpp"
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

struct CreativeEditorResolvedAction {
  std::vector<creative::CreativeObjectId> objectIds;
  creative::CreativeSemanticSelectionSetResolution selection;
  creative::CreativeSemanticObjectActionPolicy policy;
};

struct CreativeEditorResolvedObjectAction {
  creative::CreativeSemanticSelectionResolution selection;
  creative::CreativeSemanticObjectActionPolicy policy;
};

[[nodiscard]] creative::CreativeDocumentMutationReceipt
renameDocumentObjectWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    std::string name,
    std::string_view source);

[[nodiscard]] CreativeStandaloneBatchEditReceipt
setDocumentObjectsVisibleWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    bool visible,
    std::string_view source);

[[nodiscard]] CreativeStandaloneBatchEditReceipt
setDocumentObjectsLockedWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    bool locked,
    std::string_view source);

[[nodiscard]] CreativeStandaloneBatchEditReceipt
setDocumentObjectTransformWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    const creative::CreativeTransform& transform,
    bool setPosition,
    bool setRotation,
    bool setScale,
    std::string_view source);

[[nodiscard]] CreativeEditorResolvedAction resolveEditorAction(
    const creative::CreativeAppState& appState,
    std::span<const creative::CreativeObjectId> explicitIds,
    const CreativeEditorWorldLayoutState* worldLayout,
    creative::CreativeSemanticObjectAction action) {
  CreativeEditorResolvedAction result;
  result.objectIds = gatherDesktopTargetIds(appState, explicitIds);
  creative::CreativeObjectId primaryObjectId = creative::kInvalidObjectId;
  const creative::Id selectedPrimary =
      appState.facade.selectionState().selectedTarget.value;
  if (selectedPrimary != creative::kInvalidId) {
    const creative::CreativeObjectId candidate =
        static_cast<creative::CreativeObjectId>(selectedPrimary);
    if (std::find(result.objectIds.begin(), result.objectIds.end(),
                  candidate) != result.objectIds.end()) {
      primaryObjectId = candidate;
    }
  }
  if (primaryObjectId == creative::kInvalidObjectId &&
      !result.objectIds.empty()) {
    primaryObjectId = result.objectIds.front();
  }
  result.selection = creative::resolveCreativeSemanticSelectionSet(
      appState.facade.document(), result.objectIds, primaryObjectId,
      worldLayout != nullptr ? &worldLayout->source : nullptr);
  if (action == creative::CreativeSemanticObjectAction::TransformSelection &&
      worldLayout != nullptr &&
      result.selection.worldLayoutOwnerCount > 0U &&
      result.selection.authoredOwnerCount == 0U &&
      result.selection.patternOwnerCount == 0U) {
    const creative::CreativeWorldLayoutSourceRef buildingSource =
        creative::resolveCompleteCreativeWorldLayoutBuildingSelectionSource(
            appState.facade.document(), result.objectIds,
            worldLayout->source);
    if (buildingSource.table ==
        creative::CreativeWorldLayoutTable::Building) {
      result.selection.commonWorldLayoutSource = buildingSource;
    }
  }
  result.policy =
      creative::resolveCreativeSemanticObjectAction(result.selection, action);
  return result;
}

[[nodiscard]] CreativeEditorResolvedObjectAction resolveEditorObjectAction(
    const creative::CreativeAppState& appState,
    creative::CreativeObjectId objectId,
    const CreativeEditorWorldLayoutState* worldLayout,
    creative::CreativeSemanticObjectAction action) {
  CreativeEditorResolvedObjectAction result;
  result.selection = creative::resolveCreativeSemanticSelection(
      appState.facade.document(), objectId,
      worldLayout != nullptr ? &worldLayout->source : nullptr);
  result.policy =
      creative::resolveCreativeSemanticObjectAction(result.selection, action);
  return result;
}

[[nodiscard]] bool worldLayoutSourceSynchronized(
    const CreativeEditorWorldLayoutState* worldLayout) noexcept {
  return worldLayout != nullptr &&
         worldLayout->generatedRevision == worldLayout->revision;
}

[[nodiscard]] CreativeEditorSemanticEditReceipt
applySemanticDocumentObjectMutationWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativeMutationKind mutationKind,
    creative::CreativeMutationPayload payload,
    std::string_view operation,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorSemanticEditReceipt outcome;
  const creative::CreativeStructuralMutationAdmission admission =
      creative::resolveCreativeStructuralMutationAdmission(
          appState.facade.document(), objectId,
          worldLayout != nullptr ? &worldLayout->source : nullptr);
  if (!admission.allowed) {
    outcome.reasonCode = std::string(admission.reasonCode);
    return outcome;
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  const creative::CreativeDocumentMutationReceipt receipt =
      appState.facade.mutateObject(objectId, mutationKind, std::move(payload));
  outcome.accepted = creative::documentMutationSucceeded(receipt.status);
  outcome.changed = outcome.accepted && receipt.changed;
  outcome.affectedObjectCount = outcome.changed ? 1U : 0U;
  outcome.reasonCode = receipt.message;
  static_cast<void>(completeEditTransaction(
      history, std::move(transaction), appState.facade, outcome.changed,
      outcome.reasonCode));
  SDL_Log("iggy3d_creative: SEMANTIC OBJECT MUTATION operation='%s' "
          "source='%s' objectId=%llu accepted=%d changed=%d "
          "revisionBefore=%llu revisionAfter=%llu reasonCode='%s'",
          std::string(operation).c_str(), std::string(source).c_str(),
          static_cast<unsigned long long>(objectId),
          outcome.accepted ? 1 : 0, outcome.changed ? 1 : 0,
          static_cast<unsigned long long>(receipt.revisionBefore),
          static_cast<unsigned long long>(receipt.revisionAfter),
          outcome.reasonCode.c_str());
  return outcome;
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

namespace {

creative::CreativeSemanticDeleteReceipt deleteSemanticObjectsWithUndo(
    creative::CreativeAppState& appState,
    std::span<const creative::CreativeObjectId> objectIds,
    std::string_view source,
    StandaloneEditHistory* history) {
  const std::uint64_t objectCountBefore =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  const std::uint64_t undoDepthBefore =
      history != nullptr ? creative::creativeUndoDepth(*history) : 0U;
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  creative::CreativeSemanticDeleteReceipt receipt =
      appState.facade.deleteDocumentObjectsSemantically(objectIds);
  if (history != nullptr) {
    (void)completeEditTransaction(*history, std::move(transaction),
                                  appState.facade,
                                  receipt.accepted && receipt.changed,
                                  receipt.reasonCode);
  }
  const std::uint64_t objectCountAfter =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  const creative::Id selectionAfter =
      appState.facade.selectionState().selectedTarget.value;
  SDL_Log("iggy3d_creative: DELETE selection requested=%llu removed=%llu "
          "recipes=%llu accepted=%d changed=%d status='%s' reasonCode='%s' "
          "objectCountBefore=%llu objectCountAfter=%llu selectionAfter=%u "
          "undoDepthBefore=%llu undoDepthAfter=%llu",
          static_cast<unsigned long long>(receipt.requestedObjectCount),
          static_cast<unsigned long long>(receipt.removedObjectCount),
          static_cast<unsigned long long>(receipt.removedPatternRecipeCount),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          receipt.reasonCode.c_str(),
          static_cast<unsigned long long>(objectCountBefore),
          static_cast<unsigned long long>(objectCountAfter), selectionAfter,
          static_cast<unsigned long long>(undoDepthBefore),
          static_cast<unsigned long long>(
              history != nullptr ? creative::creativeUndoDepth(*history)
                                 : 0U));
  return receipt;
}

}  // namespace

creative::CreativeSemanticDeleteReceipt deleteSelectedObjectsWithUndo(
    creative::CreativeAppState& appState,
    std::string_view source,
    StandaloneEditHistory* history) {
  const std::vector<creative::CreativeObjectId> selectedIds =
      gatherDesktopTargetIds(appState, {});
  if (selectedIds.empty()) {
    SDL_Log("iggy3d_creative: DELETE no selection source='%s' "
            "objectCount=%llu",
            std::string(source).c_str(),
            static_cast<unsigned long long>(
                appState.facade.document().objectCount()));
    return {};
  }
  return deleteSemanticObjectsWithUndo(appState, selectedIds, source, history);
}

CreativeEditorDeleteReceipt deleteCreativeEditorSelectionWithUndo(
    creative::CreativeAppState& appState,
    std::string_view source,
    StandaloneEditHistory* history,
    CreativeEditorWorldLayoutState* worldLayout) {
  return deleteCreativeEditorObjectsWithUndo(
      appState, {}, source, history, worldLayout);
}

CreativeEditorDeleteReceipt deleteCreativeEditorObjectsWithUndo(
    creative::CreativeAppState& appState,
    std::span<const creative::CreativeObjectId> objectIds,
    std::string_view source,
    StandaloneEditHistory* history,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorDeleteReceipt outcome;
  const CreativeEditorResolvedAction action = resolveEditorAction(
      appState, objectIds, worldLayout,
      creative::CreativeSemanticObjectAction::Delete);
  if (!action.policy.allowed) {
    outcome.reasonCode = std::string(action.policy.reasonCode);
    return outcome;
  }

  if (action.policy.route ==
      creative::CreativeSemanticObjectActionRoute::WorldLayoutSource) {
    if (!worldLayoutSourceSynchronized(worldLayout)) {
      outcome.reasonCode =
          "creative_editor_delete_world_layout_unsynchronized";
      return outcome;
    }
    const CreativeEditorWorldLayoutEditReceipt selected =
        selectCreativeEditorWorldLayoutSource(
            *worldLayout, action.selection.commonWorldLayoutSource.table,
            action.selection.commonWorldLayoutSource.index);
    if (!selected.accepted) {
      outcome.reasonCode = selected.reasonCode;
      return outcome;
    }
    const CreativeEditorWorldLayoutEditReceipt deleted =
        deleteCreativeEditorWorldLayoutSelection(*worldLayout);
    outcome.accepted = deleted.accepted;
    outcome.changed = deleted.changed;
    outcome.worldLayoutSourceDeleted = deleted.changed;
    outcome.affectedObjectCount = action.objectIds.size();
    outcome.reasonCode = deleted.reasonCode;
    return outcome;
  }

  if (action.policy.route ==
      creative::CreativeSemanticObjectActionRoute::SemanticDocument) {
    const creative::CreativeSemanticDeleteReceipt deleted =
        deleteSemanticObjectsWithUndo(appState, action.objectIds, source,
                                      history);
    outcome.accepted = deleted.accepted;
    outcome.changed = deleted.changed;
    outcome.affectedObjectCount = deleted.removedObjectCount;
    outcome.reasonCode = deleted.reasonCode;
    return outcome;
  }

  if (objectIds.empty()) {
    const creative::CreativeSemanticDeleteReceipt deleted =
        deleteSelectedObjectsWithUndo(appState, source, history);
    outcome.accepted = deleted.accepted;
    outcome.changed = deleted.changed;
    outcome.affectedObjectCount = deleted.removedObjectCount;
    outcome.reasonCode = deleted.reasonCode;
    return outcome;
  }
  if (history != nullptr) {
    const CreativeStandaloneBatchEditReceipt deleted =
        deleteObjectsWithUndo(appState, *history, action.objectIds, source);
    outcome.accepted = deleted.accepted;
    outcome.changed = deleted.changed;
    outcome.affectedObjectCount = deleted.affectedObjectCount;
    outcome.reasonCode = deleted.message;
    return outcome;
  }
  const creative::CreativeSemanticDeleteReceipt deleted =
      appState.facade.deleteDocumentObjectsSemantically(action.objectIds);
  outcome.accepted = deleted.accepted;
  outcome.changed = deleted.changed;
  outcome.affectedObjectCount = deleted.removedObjectCount;
  outcome.reasonCode = deleted.reasonCode;
  return outcome;
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

CreativeEditorDuplicateReceipt duplicateCreativeEditorSelectionWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const creative::CreativeDuplicateCommandRequest& request,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorDuplicateReceipt outcome;
  const CreativeEditorResolvedAction action = resolveEditorAction(
      appState, {}, worldLayout,
      creative::CreativeSemanticObjectAction::Duplicate);
  if (!action.policy.allowed) {
    outcome.reasonCode = std::string(action.policy.reasonCode);
    return outcome;
  }

  if (action.policy.route ==
      creative::CreativeSemanticObjectActionRoute::WorldLayoutSource) {
    if (!worldLayoutSourceSynchronized(worldLayout)) {
      outcome.reasonCode =
          "creative_editor_duplicate_world_layout_unsynchronized";
      return outcome;
    }
    const CreativeEditorWorldLayoutEditReceipt selected =
        selectCreativeEditorWorldLayoutSource(
            *worldLayout, action.selection.commonWorldLayoutSource.table,
            action.selection.commonWorldLayoutSource.index);
    if (!selected.accepted) {
      outcome.reasonCode = selected.reasonCode;
      return outcome;
    }
    const CreativeEditorWorldLayoutEditReceipt duplicated =
        duplicateCreativeEditorWorldLayoutSource(
            *worldLayout, action.selection.commonWorldLayoutSource.table,
            action.selection.commonWorldLayoutSource.index,
            appState.facade.document().gridSettings());
    outcome.accepted = duplicated.accepted;
    outcome.changed = duplicated.changed;
    outcome.worldLayoutSourceDuplicated = duplicated.changed;
    outcome.affectedObjectCount = action.objectIds.size();
    outcome.reasonCode = duplicated.reasonCode;
    return outcome;
  }

  const creative::CreativeDuplicateCommandReceipt duplicated =
      duplicateSelectedObjectsWithUndo(appState, history, request, source);
  outcome.accepted = duplicated.accepted;
  outcome.changed = duplicated.changed;
  outcome.affectedObjectCount = duplicated.duplicatedObjectCount;
  outcome.reasonCode = duplicated.message;
  return outcome;
}

CreativeEditorSemanticEditReceipt transformCreativeEditorSelectionWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const creative::CreativeTransformCommandRequest& request,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorSemanticEditReceipt outcome;
  const CreativeEditorResolvedAction action = resolveEditorAction(
      appState, {}, worldLayout,
      creative::CreativeSemanticObjectAction::TransformSelection);
  if (!action.policy.allowed) {
    outcome.reasonCode = std::string(action.policy.reasonCode);
    return outcome;
  }
  if (action.policy.route ==
      creative::CreativeSemanticObjectActionRoute::Document) {
    const creative::CreativeTransformCommandReceipt transformed =
        transformSelectedObjectsWithUndo(appState, history, request, source);
    outcome.accepted = transformed.accepted;
    outcome.changed = transformed.changed;
    outcome.affectedObjectCount = transformed.objectCount;
    outcome.reasonCode = transformed.message;
    return outcome;
  }
  if (action.policy.route ==
      creative::CreativeSemanticObjectActionRoute::PatternRecipe) {
    outcome.reasonCode =
        "creative_editor_transform_pattern_requires_transform_tool";
    return outcome;
  }
  if (action.policy.route !=
          creative::CreativeSemanticObjectActionRoute::WorldLayoutSource ||
      !worldLayoutSourceSynchronized(worldLayout)) {
    outcome.reasonCode =
        action.policy.route ==
                creative::CreativeSemanticObjectActionRoute::WorldLayoutSource
            ? "creative_editor_transform_world_layout_unsynchronized"
            : std::string(action.policy.reasonCode);
    return outcome;
  }
  if (action.selection.commonWorldLayoutSource.table !=
          creative::CreativeWorldLayoutTable::Building ||
      request.kind != creative::CreativeTransformCommandKind::RotateYaw ||
      std::fabs(std::fabs(request.yawDegrees) - 90.0) > 1.0e-6) {
    outcome.reasonCode =
        "creative_editor_transform_world_layout_use_transform_tool";
    worldLayout->statusMessage =
        "use Transform Selection for source-owned geometry";
    return outcome;
  }
  const CreativeEditorWorldLayoutEditReceipt selected =
      selectCreativeEditorWorldLayoutSource(
          *worldLayout, creative::CreativeWorldLayoutTable::Building,
          action.selection.commonWorldLayoutSource.index);
  if (!selected.accepted) {
    outcome.reasonCode = selected.reasonCode;
    return outcome;
  }
  const creative::CreativeWorldLayoutBuildingTransformOperation operation =
      request.yawDegrees > 0.0
          ? creative::CreativeWorldLayoutBuildingTransformOperation::
                RotateRight90
          : creative::CreativeWorldLayoutBuildingTransformOperation::
                RotateLeft90;
  const CreativeEditorWorldLayoutEditReceipt rotated =
      rotateCreativeEditorWorldLayoutBuildingSource(
          *worldLayout, action.selection.commonWorldLayoutSource.index,
          operation);
  outcome.accepted = rotated.accepted;
  outcome.changed = rotated.changed;
  outcome.worldLayoutSourceChanged = rotated.changed;
  outcome.affectedObjectCount = action.objectIds.size();
  outcome.reasonCode = rotated.reasonCode;
  return outcome;
}

CreativeEditorSemanticEditReceipt renameCreativeEditorObjectWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    std::string name,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorSemanticEditReceipt outcome;
  const CreativeEditorResolvedObjectAction action = resolveEditorObjectAction(
      appState, objectId, worldLayout,
      creative::CreativeSemanticObjectAction::Rename);
  if (!action.policy.allowed) {
    outcome.reasonCode = std::string(action.policy.reasonCode);
    return outcome;
  }
  if (action.policy.route ==
      creative::CreativeSemanticObjectActionRoute::WorldLayoutSource) {
    if (!worldLayoutSourceSynchronized(worldLayout)) {
      outcome.reasonCode =
          "creative_editor_rename_world_layout_unsynchronized";
      return outcome;
    }
    const CreativeEditorWorldLayoutEditReceipt renamed =
        renameCreativeEditorWorldLayoutSource(
            *worldLayout, action.selection.worldLayoutSource.table,
            action.selection.worldLayoutSource.index, std::move(name));
    outcome.accepted = renamed.accepted;
    outcome.changed = renamed.changed;
    outcome.worldLayoutSourceChanged = renamed.changed;
    outcome.affectedObjectCount = renamed.changed ? 1U : 0U;
    outcome.reasonCode = renamed.reasonCode;
    return outcome;
  }
  const creative::CreativeDocumentMutationReceipt renamed =
      renameDocumentObjectWithUndo(appState, history, objectId,
                                   std::move(name), source);
  outcome.accepted =
      creative::documentMutationSucceeded(renamed.status);
  outcome.changed = outcome.accepted && renamed.changed;
  outcome.affectedObjectCount = outcome.changed ? 1U : 0U;
  outcome.reasonCode = renamed.message;
  return outcome;
}

CreativeEditorSemanticEditReceipt setCreativeEditorObjectsVisibleWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    bool visible,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorSemanticEditReceipt outcome;
  const CreativeEditorResolvedAction action = resolveEditorAction(
      appState, objectIds, worldLayout,
      creative::CreativeSemanticObjectAction::SetVisible);
  if (!action.policy.allowed) {
    outcome.reasonCode = std::string(action.policy.reasonCode);
    return outcome;
  }
  if (action.policy.route ==
      creative::CreativeSemanticObjectActionRoute::WorldLayoutSource) {
    if (!worldLayoutSourceSynchronized(worldLayout)) {
      outcome.reasonCode =
          "creative_editor_visibility_world_layout_unsynchronized";
      return outcome;
    }
    const CreativeEditorWorldLayoutEditReceipt updated =
        setCreativeEditorWorldLayoutSourceVisible(
            *worldLayout, action.selection.commonWorldLayoutSource.table,
            action.selection.commonWorldLayoutSource.index, visible);
    outcome.accepted = updated.accepted;
    outcome.changed = updated.changed;
    outcome.worldLayoutSourceChanged = updated.changed;
    outcome.affectedObjectCount =
        updated.changed ? action.objectIds.size() : 0U;
    outcome.reasonCode = updated.reasonCode;
    return outcome;
  }
  const CreativeStandaloneBatchEditReceipt updated =
      setDocumentObjectsVisibleWithUndo(appState, history, action.objectIds,
                                        visible, source);
  outcome.accepted = updated.accepted;
  outcome.changed = updated.changed;
  outcome.affectedObjectCount = updated.affectedObjectCount;
  outcome.reasonCode = updated.message;
  return outcome;
}

CreativeEditorSemanticEditReceipt setCreativeEditorObjectsLockedWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    bool locked,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorSemanticEditReceipt outcome;
  const CreativeEditorResolvedAction action = resolveEditorAction(
      appState, objectIds, worldLayout,
      creative::CreativeSemanticObjectAction::SetLocked);
  if (!action.policy.allowed ||
      action.policy.route !=
          creative::CreativeSemanticObjectActionRoute::Document) {
    outcome.reasonCode = std::string(action.policy.reasonCode);
    return outcome;
  }
  const CreativeStandaloneBatchEditReceipt updated =
      setDocumentObjectsLockedWithUndo(appState, history, action.objectIds,
                                       locked, source);
  outcome.accepted = updated.accepted;
  outcome.changed = updated.changed;
  outcome.affectedObjectCount = updated.affectedObjectCount;
  outcome.reasonCode = updated.message;
  return outcome;
}

CreativeEditorSemanticEditReceipt setCreativeEditorObjectTransformWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    const creative::CreativeTransform& transform,
    bool setPosition,
    bool setRotation,
    bool setScale,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorSemanticEditReceipt outcome;
  const CreativeEditorResolvedObjectAction action = resolveEditorObjectAction(
      appState, objectId, worldLayout,
      creative::CreativeSemanticObjectAction::SetTransform);
  if (!action.policy.allowed) {
    outcome.reasonCode = std::string(action.policy.reasonCode);
    return outcome;
  }
  if (action.policy.route ==
          creative::CreativeSemanticObjectActionRoute::RefineThenAdopt &&
      !worldLayoutSourceSynchronized(worldLayout)) {
    outcome.reasonCode =
        "creative_editor_transform_world_layout_unsynchronized";
    return outcome;
  }
  const CreativeStandaloneBatchEditReceipt transformed =
      setDocumentObjectTransformWithUndo(
          appState, history, objectId, transform, setPosition, setRotation,
          setScale, source);
  outcome.accepted = transformed.accepted;
  outcome.changed = transformed.changed;
  outcome.requiresAdoption =
      transformed.changed &&
      action.policy.route ==
          creative::CreativeSemanticObjectActionRoute::RefineThenAdopt;
  outcome.affectedObjectCount = transformed.affectedObjectCount;
  outcome.reasonCode = transformed.message;
  return outcome;
}

CreativeEditorSemanticEditReceipt
toggleCreativeEditorSelectionVisibilityWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorSemanticEditReceipt outcome;
  const creative::Id selected =
      appState.facade.selectionState().selectedTarget.value;
  if (selected == creative::kInvalidId) {
    outcome.reasonCode = "creative_editor_visibility_selection_empty";
    return outcome;
  }
  const creative::CreativeObjectId objectId =
      static_cast<creative::CreativeObjectId>(selected);
  const creative::CreativeSemanticSelectionResolution selection =
      creative::resolveCreativeSemanticSelection(
          appState.facade.document(), objectId,
          worldLayout != nullptr ? &worldLayout->source : nullptr);
  if (!selection.accepted) {
    outcome.reasonCode = std::string(selection.reasonCode);
    return outcome;
  }
  const std::array targets{objectId};
  return setCreativeEditorObjectsVisibleWithUndo(
      appState, history, targets, !selection.objectVisible, source,
      worldLayout);
}

CreativeEditorSemanticEditReceipt toggleCreativeEditorSelectionLockedWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorSemanticEditReceipt outcome;
  const creative::Id selected =
      appState.facade.selectionState().selectedTarget.value;
  if (selected == creative::kInvalidId) {
    outcome.reasonCode = "creative_editor_lock_selection_empty";
    return outcome;
  }
  const creative::CreativeObjectId objectId =
      static_cast<creative::CreativeObjectId>(selected);
  const creative::CreativeSemanticSelectionResolution selection =
      creative::resolveCreativeSemanticSelection(
          appState.facade.document(), objectId,
          worldLayout != nullptr ? &worldLayout->source : nullptr);
  if (!selection.accepted) {
    outcome.reasonCode = std::string(selection.reasonCode);
    return outcome;
  }
  const std::array targets{objectId};
  return setCreativeEditorObjectsLockedWithUndo(
      appState, history, targets, !selection.objectLocked, source,
      worldLayout);
}

CreativeEditorSemanticEditReceipt detachCreativeEditorObjectWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  return applySemanticDocumentObjectMutationWithUndo(
      appState, history, objectId,
      creative::CreativeMutationKind::DetachFrom,
      creative::CreativeMutationPayload{}, "DETACH", source, worldLayout);
}

CreativeEditorObjectReattachmentReceipt
reattachCreativeEditorObjectWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const CreativeEditorObjectReattachmentPlan& plan,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  const creative::CreativeDocument& document = appState.facade.document();
  CreativeEditorObjectReattachmentReceipt receipt;
  if (!plan.accepted ||
      plan.status != CreativeEditorObjectReattachmentStatus::Ready ||
      document.id() != plan.documentId ||
      document.revision() != plan.documentRevision) {
    receipt = applyCreativeEditorObjectReattachment(appState.facade, plan);
  } else {
    const creative::CreativeStructuralMutationAdmission admission =
        creative::resolveCreativeStructuralMutationAdmission(
            document, plan.sourceObjectId,
            worldLayout != nullptr ? &worldLayout->source : nullptr);
    if (!admission.allowed) {
      receipt.status =
          CreativeEditorObjectReattachmentStatus::SourceOwned;
      receipt.sourceObjectId = plan.sourceObjectId;
      receipt.targetObjectId = plan.targetObjectId;
      receipt.revisionBefore = document.revision();
      receipt.revisionAfter = document.revision();
    } else {
      StandaloneEditTransaction transaction =
          beginEditTransaction(appState.facade, source);
      receipt = applyCreativeEditorObjectReattachment(appState.facade, plan);
      static_cast<void>(completeEditTransaction(
          history, std::move(transaction), appState.facade,
          receipt.accepted && receipt.changed, toString(receipt.status)));
    }
  }
  SDL_Log("iggy3d_creative: REATTACH source='%s' objectId=%llu targetId=%llu "
          "accepted=%d changed=%d revisionBefore=%llu revisionAfter=%llu "
          "status='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(receipt.sourceObjectId),
          static_cast<unsigned long long>(receipt.targetObjectId),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          static_cast<unsigned long long>(receipt.revisionBefore),
          static_cast<unsigned long long>(receipt.revisionAfter),
          std::string(toString(receipt.status)).c_str());
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

CreativeEditorSemanticEditReceipt
cutCreativeEditorSelectionToClipboardWithHistory(
    creative::CreativeAppState& appState,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorSemanticEditReceipt outcome;
  const CreativeEditorResolvedAction action = resolveEditorAction(
      appState, {}, worldLayout,
      creative::CreativeSemanticObjectAction::Cut);
  if (!action.policy.allowed ||
      (action.policy.route !=
           creative::CreativeSemanticObjectActionRoute::Document &&
       action.policy.route !=
           creative::CreativeSemanticObjectActionRoute::SemanticDocument)) {
    outcome.reasonCode = std::string(action.policy.reasonCode);
    return outcome;
  }
  const creative::CreativeClipboardCutReceipt cut =
      cutSelectionToClipboardWithHistory(appState, source);
  outcome.accepted = cut.accepted;
  outcome.changed = cut.changed;
  outcome.affectedObjectCount = cut.cutObjectCount;
  outcome.reasonCode = cut.reasonCode;
  return outcome;
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

  const std::uint64_t depthBefore = creative::creativeUndoDepth(history);
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  const creative::CreativeSemanticDeleteReceipt receipt =
      appState.facade.deleteDocumentObjectsSemantically(targets);
  outcome.accepted = receipt.accepted;
  outcome.changed = receipt.changed;
  outcome.affectedObjectCount = receipt.removedObjectCount;
  outcome.message = std::string(receipt.reasonCode);
  (void)completeEditTransaction(history, std::move(transaction),
                                appState.facade, outcome.changed,
                                outcome.message);
  SDL_Log("iggy3d_creative: DELETE MULTI source='%s' requested=%zu resolved=%zu "
          "removed=%llu undoBefore=%llu undoAfter=%llu",
          std::string(source).c_str(), targets.size(),
          receipt.removedObjectIds.size(),
          static_cast<unsigned long long>(outcome.affectedObjectCount),
          static_cast<unsigned long long>(depthBefore),
          static_cast<unsigned long long>(creative::creativeUndoDepth(history)));
  return outcome;
}

namespace {

creative::CreativeDocumentMutationReceipt renameDocumentObjectWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    std::string name,
    std::string_view source) {
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  creative::CreativeDocumentMutationReceipt receipt =
      appState.facade.mutateObject(
          objectId, creative::CreativeMutationKind::Rename,
          creative::makeRenamePayload(std::move(name)));
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

CreativeStandaloneBatchEditReceipt applyObjectsBoolStateWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    bool value,
    creative::CreativeMutationKind mutationKind,
    const char* verb,
    std::string_view source) {
  CreativeStandaloneBatchEditReceipt outcome;
  const std::vector<creative::CreativeObjectId> targets =
      gatherDesktopTargetIds(appState, objectIds);
  if (targets.empty()) {
    outcome.message = "no_targets";
    return outcome;
  }
  std::vector<creative::CreativeMutationRequest> requests;
  requests.reserve(targets.size());
  const creative::CreativeMutationPayload payload =
      mutationKind == creative::CreativeMutationKind::SetVisible
          ? creative::makeVisibilityPayload(value)
          : creative::makeLockPayload(value);
  for (creative::CreativeObjectId id : targets) {
    requests.push_back({0U, id, mutationKind, payload});
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  const creative::CreativeDocumentBatchMutationReceipt batch =
      appState.facade.mutateObjectsAtomically(requests);
  outcome.accepted = batch.committed &&
                     creative::documentMutationSucceeded(batch.status);
  outcome.changed = outcome.accepted && batch.changed;
  outcome.affectedObjectCount = outcome.changed ? batch.appliedCount : 0U;
  outcome.message = batch.message;
  (void)completeEditTransaction(history, std::move(transaction),
                                appState.facade, outcome.changed,
                                outcome.message);
  SDL_Log("iggy3d_creative: OBJECT SET %s source='%s' targets=%zu value=%d "
          "affected=%llu",
          verb, std::string(source).c_str(), targets.size(), value ? 1 : 0,
          static_cast<unsigned long long>(outcome.affectedObjectCount));
  return outcome;
}

CreativeStandaloneBatchEditReceipt setDocumentObjectsVisibleWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    bool visible,
    std::string_view source) {
  return applyObjectsBoolStateWithUndo(appState, history, objectIds, visible,
                                       creative::CreativeMutationKind::SetVisible,
                                       "VISIBLE", source);
}

CreativeStandaloneBatchEditReceipt setDocumentObjectsLockedWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    bool locked,
    std::string_view source) {
  return applyObjectsBoolStateWithUndo(appState, history, objectIds, locked,
                                       creative::CreativeMutationKind::SetLocked,
                                       "LOCKED", source);
}

CreativeStandaloneBatchEditReceipt setDocumentObjectTransformWithUndo(
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
  const creative::CreativeObject* object = appState.facade.findObject(objectId);
  if (object == nullptr) {
    outcome.message = "missing_object";
    return outcome;
  }
  if ((setPosition &&
       !creative::isFiniteCreativeVec3(transform.position)) ||
      (setRotation &&
       !creative::isFiniteCreativeVec3(transform.rotationEulerRadians)) ||
      (setScale && !creative::isPositiveCreativeVec3(transform.scale))) {
    outcome.message = "invalid_transform";
    return outcome;
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  const creative::CreativeHierarchyTransformReceipt receipt =
      appState.facade.transformObjectHierarchyAtomically(
          {objectId, transform, setPosition, setRotation, setScale});
  outcome.accepted = receipt.accepted;
  outcome.changed = receipt.changed;
  outcome.affectedObjectCount = receipt.changed ? receipt.hierarchyObjectCount : 0U;
  outcome.message = receipt.message;
  (void)completeEditTransaction(history, std::move(transaction),
                                appState.facade, outcome.changed,
                                outcome.message);
  SDL_Log("iggy3d_creative: SET TRANSFORM objectId=%llu pos=%d rot=%d scale=%d "
          "changed=%d source='%s'",
          static_cast<unsigned long long>(objectId), setPosition ? 1 : 0,
          setRotation ? 1 : 0, setScale ? 1 : 0,
          outcome.changed ? 1 : 0,
          std::string(source).c_str());
  return outcome;
}

}  // namespace

CreativeEditorSemanticEditReceipt
setCreativeEditorMovingPlatformSettingsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativeMovingPlatformSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  return applySemanticDocumentObjectMutationWithUndo(
      appState, history, objectId,
      creative::CreativeMutationKind::SetMovingPlatformSettings,
      creative::makeMovingPlatformSettingsPayload(settings),
      "SET MOVING PLATFORM SETTINGS", source, worldLayout);
}

CreativeEditorSemanticEditReceipt setCreativeEditorPlayerSpawnSettingsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativePlayerSpawnSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  return applySemanticDocumentObjectMutationWithUndo(
      appState, history, objectId,
      creative::CreativeMutationKind::SetPlayerSpawnSettings,
      creative::makePlayerSpawnSettingsPayload(std::move(settings)),
      "SET PLAYER SPAWN SETTINGS", source, worldLayout);
}

CreativeEditorSemanticEditReceipt setCreativeEditorNpcSpawnSettingsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativeNpcSpawnSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  return applySemanticDocumentObjectMutationWithUndo(
      appState, history, objectId,
      creative::CreativeMutationKind::SetNpcSpawnSettings,
      creative::makeNpcSpawnSettingsPayload(std::move(settings)),
      "SET NPC SPAWN SETTINGS", source, worldLayout);
}

CreativeEditorSemanticEditReceipt setCreativeEditorLootPointSettingsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativeLootPointSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  return applySemanticDocumentObjectMutationWithUndo(
      appState, history, objectId,
      creative::CreativeMutationKind::SetLootPointSettings,
      creative::makeLootPointSettingsPayload(std::move(settings)),
      "SET LOOT POINT SETTINGS", source, worldLayout);
}

CreativeEditorSemanticEditReceipt setCreativeEditorExitPointSettingsWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativeExitPointSettings settings,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  return applySemanticDocumentObjectMutationWithUndo(
      appState, history, objectId,
      creative::CreativeMutationKind::SetExitPointSettings,
      creative::makeExitPointSettingsPayload(std::move(settings)),
      "SET EXIT POINT SETTINGS", source, worldLayout);
}

}  // namespace iggy3d_creative_app
