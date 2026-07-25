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
  if (!action.admission.allowed) {
    outcome.reasonCode = std::string(action.admission.reasonCode);
    return outcome;
  }

  if (action.admission.route ==
      creative::CreativeSemanticObjectActionRoute::WorldLayoutSource) {
    const CreativeEditorWorldLayoutEditReceipt selected =
        selectCreativeEditorWorldLayoutSource(
            *worldLayout,
            action.facts.selection.commonWorldLayoutSource.table,
            action.facts.selection.commonWorldLayoutSource.index);
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

  if (action.admission.route ==
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
  if (!action.admission.allowed) {
    outcome.reasonCode = std::string(action.admission.reasonCode);
    return outcome;
  }

  if (action.admission.route ==
      creative::CreativeSemanticObjectActionRoute::WorldLayoutSource) {
    const CreativeEditorWorldLayoutEditReceipt selected =
        selectCreativeEditorWorldLayoutSource(
            *worldLayout,
            action.facts.selection.commonWorldLayoutSource.table,
            action.facts.selection.commonWorldLayoutSource.index);
    if (!selected.accepted) {
      outcome.reasonCode = selected.reasonCode;
      return outcome;
    }
    const CreativeEditorWorldLayoutEditReceipt duplicated =
        duplicateCreativeEditorWorldLayoutSource(
            *worldLayout,
            action.facts.selection.commonWorldLayoutSource.table,
            action.facts.selection.commonWorldLayoutSource.index,
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
  if (!action.admission.allowed) {
    outcome.reasonCode = std::string(action.admission.reasonCode);
    return outcome;
  }
  if (action.admission.route ==
      creative::CreativeSemanticObjectActionRoute::Document) {
    const creative::CreativeTransformCommandReceipt transformed =
        transformSelectedObjectsWithUndo(appState, history, request, source);
    outcome.accepted = transformed.accepted;
    outcome.changed = transformed.changed;
    outcome.affectedObjectCount = transformed.objectCount;
    outcome.reasonCode = transformed.message;
    return outcome;
  }
  if (action.admission.route ==
      creative::CreativeSemanticObjectActionRoute::PatternRecipe) {
    outcome.reasonCode =
        "creative_editor_transform_pattern_requires_transform_tool";
    return outcome;
  }
  if (action.admission.route !=
      creative::CreativeSemanticObjectActionRoute::WorldLayoutSource) {
    outcome.reasonCode = std::string(action.admission.reasonCode);
    return outcome;
  }
  if (action.facts.selection.commonWorldLayoutSource.table !=
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
          action.facts.selection.commonWorldLayoutSource.index);
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
          *worldLayout,
          action.facts.selection.commonWorldLayoutSource.index,
          operation);
  outcome.accepted = rotated.accepted;
  outcome.changed = rotated.changed;
  outcome.worldLayoutSourceChanged = rotated.changed;
  outcome.affectedObjectCount = action.objectIds.size();
  outcome.reasonCode = rotated.reasonCode;
  return outcome;
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

}  // namespace iggy3d_creative_app
