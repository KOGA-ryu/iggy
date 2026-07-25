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
  if (!action.admission.allowed) {
    outcome.reasonCode = std::string(action.admission.reasonCode);
    return outcome;
  }
  if (action.admission.route ==
      creative::CreativeSemanticObjectActionRoute::WorldLayoutSource) {
    const CreativeEditorWorldLayoutEditReceipt renamed =
        renameCreativeEditorWorldLayoutSource(
            *worldLayout,
            action.facts.singleSelection.worldLayoutSource.table,
            action.facts.singleSelection.worldLayoutSource.index,
            std::move(name));
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
  if (!action.admission.allowed) {
    outcome.reasonCode = std::string(action.admission.reasonCode);
    return outcome;
  }
  if (action.admission.route ==
      creative::CreativeSemanticObjectActionRoute::WorldLayoutSource) {
    const CreativeEditorWorldLayoutEditReceipt updated =
        setCreativeEditorWorldLayoutSourceVisible(
            *worldLayout,
            action.facts.selection.commonWorldLayoutSource.table,
            action.facts.selection.commonWorldLayoutSource.index, visible);
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
  if (!action.admission.allowed ||
      action.admission.route !=
          creative::CreativeSemanticObjectActionRoute::Document) {
    outcome.reasonCode = std::string(action.admission.reasonCode);
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
  if (!action.admission.allowed) {
    outcome.reasonCode = std::string(action.admission.reasonCode);
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
      action.admission.route ==
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

}  // namespace iggy3d_creative_app
