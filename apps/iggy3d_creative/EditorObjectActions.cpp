#include "EditorObjectActions.hpp"

#include <span>
#include <string>
#include <vector>

#include "EditorAuthoredAssets.hpp"
#include "EditorAttachmentPlacement.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorEdits.hpp"
#include "EditorGroup.hpp"
#include "EditorObjectActionExecutor.hpp"
#include "EditorState.hpp"
#include "EditorTransform.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

cr::CreativeSemanticObjectActionFacts
resolveCreativeEditorObjectActionFacts(
    const cr::CreativeAppState& appState,
    const CreativeEditorWorldLayoutState* worldLayout) {
  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  std::vector<cr::CreativeObjectId> objectIds;
  objectIds.reserve(cr::selectedTargetCount(selection));
  for (cr::TargetRef target : cr::selectedTargetList(selection)) {
    if (target.value != cr::kInvalidId) {
      objectIds.push_back(
          static_cast<cr::CreativeObjectId>(target.value));
    }
  }
  if (objectIds.empty() &&
      selection.selectedTarget.value != cr::kInvalidId) {
    objectIds.push_back(static_cast<cr::CreativeObjectId>(
        selection.selectedTarget.value));
  }
  const cr::CreativeObjectId primaryObjectId =
      selection.selectedTarget.value == cr::kInvalidId
          ? cr::kInvalidObjectId
          : static_cast<cr::CreativeObjectId>(
                selection.selectedTarget.value);
  return cr::resolveCreativeSemanticObjectActionFacts(
      appState.facade.document(), objectIds, primaryObjectId,
      worldLayout != nullptr ? &worldLayout->source : nullptr,
      worldLayout != nullptr &&
          worldLayout->generatedRevision == worldLayout->revision);
}

const CreativeDesktopObjectActionContext&
refreshCreativeEditorDesktopObjectActionContext(
    CreativeEditorDesktopUiState& desktopUi,
    const cr::CreativeAppState& appState,
    const CreativeEditorWorldLayoutState* worldLayout) {
  CreativeDesktopObjectActionContext& context =
      desktopUi.objectActionContext;
  const cr::CreativeDocument& document = appState.facade.document();
  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  const bool worldLayoutAvailable = worldLayout != nullptr;
  const std::uint64_t worldLayoutRevision =
      worldLayoutAvailable ? worldLayout->revision : 0U;
  const std::uint64_t worldLayoutGeneratedRevision =
      worldLayoutAvailable ? worldLayout->generatedRevision : 0U;
  const std::uint64_t worldLayoutSourceEpoch =
      worldLayoutAvailable ? worldLayout->sourceEpoch : 0U;
  if (context.valid && context.documentId == document.id() &&
      context.documentRevision == document.revision() &&
      context.selectionRevision == selection.selectionRevision &&
      context.worldLayoutAvailable == worldLayoutAvailable &&
      context.worldLayoutRevision == worldLayoutRevision &&
      context.worldLayoutGeneratedRevision == worldLayoutGeneratedRevision &&
      context.worldLayoutSourceEpoch == worldLayoutSourceEpoch) {
    return context;
  }

  context.documentId = document.id();
  context.documentRevision = document.revision();
  context.selectionRevision = selection.selectionRevision;
  context.worldLayoutAvailable = worldLayoutAvailable;
  context.worldLayoutRevision = worldLayoutRevision;
  context.worldLayoutGeneratedRevision = worldLayoutGeneratedRevision;
  context.worldLayoutSourceEpoch = worldLayoutSourceEpoch;
  context.facts =
      resolveCreativeEditorObjectActionFacts(appState, worldLayout);
  context.admissions =
      cr::resolveCreativeSemanticObjectActionAdmissions(context.facts);
  ++context.rebuildCount;
  context.valid = true;
  return context;
}

CreativeEditorObjectActionCapability creativeEditorObjectActionCapability(
    const CreativeEditorObjectActionCapabilities& capabilities,
    cr::CreativeSemanticObjectAction action) noexcept {
  return cr::creativeSemanticObjectActionAdmission(capabilities, action);
}

bool creativeEditorObjectActionAvailable(
    const CreativeEditorObjectActionCapabilities& capabilities,
    cr::CreativeSemanticObjectAction action) noexcept {
  return creativeEditorObjectActionCapability(capabilities, action).allowed;
}

bool equipCreativeEditorAuthoredAssetToHotbar(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    const cr::CreativeAuthoredAssetDefinition& definition) {
  const std::size_t slot =
      static_cast<std::size_t>(editor.interaction.hotbar.selectedSlot);
  cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const cr::CreativeHeldItemKind previousKind = held.kind;
  held = {cr::CreativeHeldItemKind::Material,
          cr::CreativeObjectKind::PrefabInstance};
  const bool equipped = cr::setCreativeHotbarAsset(held, definition.assetId,
                                                   definition.sourceBounds);
  if (equipped) {
    if (previousKind != held.kind) {
      clearCreativeMaterialBrushPresetSlot(
          editor.interaction.materialBrushPresets, slot);
    }
    static_cast<void>(activateSelectedCreativeMaterialBrushPreset(
        editor.interaction.materialBrushPresets, editor.interaction.hotbar,
        editor.toolSettings));
    syncCreativeEditorHeldItem(appState, editor);
  }
  return equipped;
}

bool creativeEditorCommandIsObjectAction(
    CreativeEditorToolOptionsCommandId command) noexcept {
  return creativeEditorObjectActionDescriptor(command) != nullptr;
}

void refreshCreativeEditorObjectActionContext(
    const cr::CreativeAppState& appState,
    const CreativeEditorAuthoredAssetLibrary& authoredAssets,
    CreativeEditorToolOptionsState& state,
    const CreativeEditorWorldLayoutState* worldLayout) noexcept {
  state.contextDocumentId = appState.facade.document().id();
  state.contextDocumentRevision = appState.facade.document().revision();
  state.contextSelectionRevision =
      appState.facade.selectionState().selectionRevision;
  state.contextGroupId = cr::kInvalidObjectId;
  state.contextPrimaryObjectId = cr::kInvalidObjectId;
  state.contextPrimaryObjectKind = cr::CreativeObjectKind::Unknown;
  state.contextPrimaryAssetId.clear();
  state.contextContainerKind = cr::CreativeObjectKind::Unknown;
  state.contextContainerAssetId.clear();
  state.contextAttachmentParentId = cr::kInvalidObjectId;
  state.contextAttachmentSocket.clear();
  state.contextSelectionCount = 0U;
  state.contextSemanticSelection = {};
  state.contextActionAdmissions = {};
  state.contextWorldLayoutRevision =
      worldLayout != nullptr ? worldLayout->revision : 0U;
  state.contextWorldLayoutGeneratedRevision =
      worldLayout != nullptr ? worldLayout->generatedRevision : 0U;
  state.contextWorldLayoutSourceEpoch =
      worldLayout != nullptr ? worldLayout->sourceEpoch : 0U;
  state.contextWorldLayoutSynchronized =
      worldLayout != nullptr &&
      worldLayout->generatedRevision == worldLayout->revision;
  state.contextPrimaryVisible = true;
  state.contextPrimaryLocked = false;
  state.contextAllUnlocked = false;
  state.contextAllMovable = false;
  state.contextAllResettable = false;
  state.contextPrefabUpdateTransformSupported = false;
  state.contextPrefabSyncInspected = false;
  state.contextPrefabSyncState =
      cr::CreativeAuthoredAssetSyncState::Conflict;
  state.contextPrefabMatchedInstanceCount = 0U;
  state.contextPrefabCurrentInstanceCount = 0U;
  state.contextPrefabSourceChangedInstanceCount = 0U;
  state.contextPrefabLocallyModifiedInstanceCount = 0U;
  state.contextPrefabConflictInstanceCount = 0U;

  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  const cr::CreativeSemanticObjectActionFacts actionFacts =
      resolveCreativeEditorObjectActionFacts(appState, worldLayout);
  state.contextSelectionCount = actionFacts.selection.selectedCount;
  state.contextSemanticSelection = actionFacts.selection;
  state.contextAllUnlocked = actionFacts.allUnlocked;
  state.contextAllMovable =
      actionFacts.hierarchyResolved &&
      !actionFacts.hierarchyObjectIds.empty();
  state.contextAllResettable = state.contextAllMovable;
  for (cr::CreativeObjectId objectId : actionFacts.hierarchyObjectIds) {
    const cr::CreativeObject* object = appState.facade.findObject(objectId);
    if (object == nullptr) {
      state.contextAllMovable = false;
      state.contextAllResettable = false;
      continue;
    }
    state.contextAllMovable =
        state.contextAllMovable &&
        cr::descriptorAllowsMutation(object->kind, cr::CreativeMutationKind::Move);
    state.contextAllResettable =
        state.contextAllResettable &&
        cr::descriptorAllowsMutation(object->kind,
                                     cr::CreativeMutationKind::Rotate) &&
        cr::descriptorAllowsMutation(object->kind,
                                     cr::CreativeMutationKind::Scale);
  }
  state.contextActionAdmissions =
      cr::resolveCreativeSemanticObjectActionAdmissions(actionFacts);

  const cr::TargetRef primary = selection.selectedTarget;
  if (primary.value == cr::kInvalidId) {
    return;
  }
  state.contextPrimaryObjectId =
      static_cast<cr::CreativeObjectId>(primary.value);
  const cr::CreativeObject* object =
      appState.facade.findObject(state.contextPrimaryObjectId);
  if (object == nullptr) {
    return;
  }
  state.contextPrimaryObjectKind = object->kind;
  state.contextPrimaryAssetId = object->assetId;
  const cr::CreativeObjectHierarchyState primaryState =
      cr::resolveCreativeObjectHierarchyState(appState.facade.document(),
                                              object->id);
  state.contextPrimaryVisible =
      primaryState.resolved && primaryState.effectivelyVisible;
  state.contextPrimaryLocked =
      !primaryState.resolved || primaryState.effectivelyLocked;
  if (state.contextSelectionCount != 1U) {
    return;
  }
  if (object->parentId.has_value() && !object->attachmentSocket.empty()) {
    state.contextAttachmentParentId = *object->parentId;
    state.contextAttachmentSocket = object->attachmentSocket;
  }
  if (cr::creativeObjectIsHierarchyContainer(object->kind)) {
    state.contextGroupId = object->id;
    state.contextContainerKind = object->kind;
    state.contextContainerAssetId = object->assetId;
    state.contextPrefabUpdateTransformSupported =
        cr::creativeAuthoredAssetInstanceTransformSupported(*object);
  } else if (object->parentId.has_value()) {
    const cr::CreativeObject* parent =
        appState.facade.findObject(*object->parentId);
    if (parent != nullptr &&
        cr::creativeObjectIsHierarchyContainer(parent->kind)) {
      state.contextGroupId = parent->id;
      state.contextContainerKind = parent->kind;
      state.contextContainerAssetId = parent->assetId;
      state.contextPrefabUpdateTransformSupported =
          cr::creativeAuthoredAssetInstanceTransformSupported(*parent);
    }
  }

  if (state.contextContainerKind != cr::CreativeObjectKind::PrefabInstance) {
    return;
  }
  const cr::CreativeAuthoredAssetDefinition* definition =
      findCreativeEditorAuthoredAsset(authoredAssets,
                                     state.contextContainerAssetId);
  if (definition == nullptr) {
    return;
  }
  const cr::CreativeAuthoredAssetSyncReceipt inspected =
      cr::inspectCreativeAuthoredAssetInstanceSync(
          appState.facade.document(), *definition, state.contextGroupId);
  const cr::CreativeAuthoredAssetSyncSummary summary =
      cr::summarizeCreativeAuthoredAssetSync(appState.facade.document(),
                                             *definition);
  state.contextPrefabSyncInspected = true;
  state.contextPrefabSyncState =
      inspected.accepted ? inspected.state
                         : cr::CreativeAuthoredAssetSyncState::Conflict;
  state.contextPrefabMatchedInstanceCount = summary.matchedInstanceCount;
  state.contextPrefabCurrentInstanceCount = summary.currentInstanceCount;
  state.contextPrefabSourceChangedInstanceCount =
      summary.sourceChangedInstanceCount;
  state.contextPrefabLocallyModifiedInstanceCount =
      summary.locallyModifiedInstanceCount;
  state.contextPrefabConflictInstanceCount = summary.conflictInstanceCount;
}

bool creativeEditorObjectActionEnabled(
    const CreativeEditorState& editor,
    const CreativeEditorToolOptionsState& state,
    CreativeEditorToolOptionsCommandId command) noexcept {
  const CreativeEditorObjectActionDescriptor* descriptor =
      creativeEditorObjectActionDescriptor(command);
  if (descriptor == nullptr) {
    return false;
  }
  const bool hasAuthoredInstanceDefinition =
      state.contextGroupId != cr::kInvalidObjectId &&
      state.contextContainerKind == cr::CreativeObjectKind::PrefabInstance &&
      findCreativeEditorAuthoredAsset(
          editor.authoredAssets, state.contextContainerAssetId) != nullptr;
  const bool hasRefreshableAuthoredInstance =
      hasAuthoredInstanceDefinition &&
      state.contextPrefabUpdateTransformSupported;
  const auto capability =
      [&](cr::CreativeSemanticObjectAction action) {
        return creativeEditorObjectActionCapability(
            state.contextActionAdmissions, action);
      };
  switch (descriptor->activation) {
    case CreativeEditorObjectActionActivationKind::BeginTransformSession: {
      const CreativeEditorObjectActionCapability transform =
          capability(descriptor->semanticAction);
      return state.contextSelectionCount > 0U &&
             state.contextAllMovable && transform.allowed;
    }
    case CreativeEditorObjectActionActivationKind::ApplyResetTransform: {
      const CreativeEditorObjectActionCapability transform =
          capability(descriptor->semanticAction);
      return state.contextSelectionCount > 0U &&
             state.contextAllResettable && transform.allowed &&
             transform.route ==
                 cr::CreativeSemanticObjectActionRoute::Document;
    }
    case CreativeEditorObjectActionActivationKind::DuplicateSelection:
      return state.contextSelectionCount > 0U &&
             capability(descriptor->semanticAction).allowed;
    case CreativeEditorObjectActionActivationKind::DeleteSelection:
      return state.contextPrimaryObjectId != cr::kInvalidObjectId &&
             capability(descriptor->semanticAction).allowed;
    case CreativeEditorObjectActionActivationKind::ToggleSelectionVisibility:
      return state.contextPrimaryObjectId != cr::kInvalidObjectId &&
             capability(descriptor->semanticAction).allowed;
    case CreativeEditorObjectActionActivationKind::ToggleSelectionLocked:
      return state.contextPrimaryObjectId != cr::kInvalidObjectId &&
             capability(descriptor->semanticAction).allowed;
    case CreativeEditorObjectActionActivationKind::DetachAttachment:
      return state.contextSelectionCount == 1U &&
             state.contextPrimaryObjectId != cr::kInvalidObjectId &&
             state.contextAttachmentParentId != cr::kInvalidObjectId &&
             !state.contextAttachmentSocket.empty() &&
             capability(descriptor->semanticAction).allowed;
    case CreativeEditorObjectActionActivationKind::ReattachAttachment:
      return state.contextSelectionCount == 1U &&
             state.contextPrimaryObjectId != cr::kInvalidObjectId &&
             !state.contextPrimaryAssetId.empty() &&
             state.contextAttachmentAimAvailable &&
             state.contextAttachmentAimTargetId != cr::kInvalidObjectId &&
             state.contextAttachmentAimTargetId !=
                 state.contextPrimaryObjectId &&
             capability(descriptor->semanticAction).allowed;
    case CreativeEditorObjectActionActivationKind::GroupSelection:
      return state.contextSelectionCount > 1U &&
             capability(descriptor->semanticAction).allowed;
    case CreativeEditorObjectActionActivationKind::UngroupSelection:
      return state.contextGroupId != cr::kInvalidObjectId &&
             capability(descriptor->semanticAction).allowed;
    case CreativeEditorObjectActionActivationKind::SaveSelectionAsAsset:
      return state.contextSelectionCount > 0U && state.contextAllUnlocked &&
             !editor.authoredAssets.root.empty();
    case CreativeEditorObjectActionActivationKind::UpdateSavedAsset:
      return hasRefreshableAuthoredInstance;
    case CreativeEditorObjectActionActivationKind::RefreshSavedAssetInstance:
      return hasRefreshableAuthoredInstance &&
             state.contextPrefabSyncInspected &&
             state.contextPrefabSyncState !=
                 cr::CreativeAuthoredAssetSyncState::Current;
    case CreativeEditorObjectActionActivationKind::
        RefreshSafeSavedAssetInstances:
      return hasAuthoredInstanceDefinition &&
             state.contextPrefabSourceChangedInstanceCount > 0U;
    case CreativeEditorObjectActionActivationKind::
        ForceRefreshSavedAssetInstances:
      return hasAuthoredInstanceDefinition &&
             state.contextPrefabMatchedInstanceCount > 0U;
    case CreativeEditorObjectActionActivationKind::Count:
      return false;
  }
  return false;
}

std::string creativeEditorObjectActionValueLabel(
    const CreativeEditorToolOptionsState& state,
    CreativeEditorToolOptionsCommandId command) {
  const CreativeEditorObjectActionDescriptor* descriptor =
      creativeEditorObjectActionDescriptor(command);
  if (descriptor == nullptr) {
    return "INVALID";
  }
  switch (descriptor->activation) {
    case CreativeEditorObjectActionActivationKind::BeginTransformSession:
      return "MOVE ROTATE SCALE MIRROR";
    case CreativeEditorObjectActionActivationKind::ApplyResetTransform:
      return "ROTATION + SCALE";
    case CreativeEditorObjectActionActivationKind::DuplicateSelection:
      return state.contextSelectionCount == 0U
                 ? "SELECT OBJECT"
                 : std::to_string(state.contextSelectionCount) + " OBJECTS";
    case CreativeEditorObjectActionActivationKind::DeleteSelection:
      return state.contextPrimaryObjectId == cr::kInvalidObjectId
                 ? "SELECT OBJECT"
                 : "PRIMARY OBJECT";
    case CreativeEditorObjectActionActivationKind::ToggleSelectionVisibility:
      return state.contextPrimaryObjectId == cr::kInvalidObjectId
                 ? "SELECT OBJECT"
                 : state.contextPrimaryVisible ? "HIDE" : "SHOW";
    case CreativeEditorObjectActionActivationKind::ToggleSelectionLocked:
      return state.contextPrimaryObjectId == cr::kInvalidObjectId
                 ? "SELECT OBJECT"
                 : state.contextPrimaryLocked ? "UNLOCK" : "LOCK";
    case CreativeEditorObjectActionActivationKind::DetachAttachment:
      return state.contextAttachmentSocket.empty()
                 ? "NOT ATTACHED"
                 : "FROM " + state.contextAttachmentSocket;
    case CreativeEditorObjectActionActivationKind::ReattachAttachment:
      return state.contextAttachmentAimAvailable
                 ? "TO OBJECT #" +
                       std::to_string(state.contextAttachmentAimTargetId)
                 : "AIM AT SOCKET HOST";
    case CreativeEditorObjectActionActivationKind::GroupSelection:
      return state.contextSelectionCount > 1U
                 ? std::to_string(state.contextSelectionCount) + " OBJECTS"
                 : "SELECT MULTIPLE";
    case CreativeEditorObjectActionActivationKind::UngroupSelection:
      return state.contextGroupId != cr::kInvalidObjectId
                 ? state.contextContainerKind ==
                           cr::CreativeObjectKind::PrefabInstance
                       ? "UNPACK INSTANCE"
                       : "REMOVE CONTAINER"
                 : "SELECT GROUP";
    case CreativeEditorObjectActionActivationKind::SaveSelectionAsAsset:
      if (state.contextSelectionCount == 0U) {
        return "SELECT OBJECTS";
      }
      if (state.contextSelectionCount == 1U) {
        return "1 ROOT";
      }
      return std::to_string(state.contextSelectionCount) + " ROOTS";
    case CreativeEditorObjectActionActivationKind::UpdateSavedAsset:
      if (state.contextContainerKind !=
          cr::CreativeObjectKind::PrefabInstance) {
        return "SELECT ASSET INSTANCE";
      }
      if (!state.contextPrefabUpdateTransformSupported) {
        return "RESET INSTANCE SCALE + TILT";
      }
      return state.contextContainerAssetId;
    case CreativeEditorObjectActionActivationKind::RefreshSavedAssetInstance:
      if (!state.contextPrefabSyncInspected) {
        return "SELECT ASSET INSTANCE";
      }
      return std::string(cr::toString(state.contextPrefabSyncState));
    case CreativeEditorObjectActionActivationKind::
        RefreshSafeSavedAssetInstances:
      return state.contextPrefabSourceChangedInstanceCount == 0U
                 ? "NO SAFE UPDATES"
                 : std::to_string(
                       state.contextPrefabSourceChangedInstanceCount) +
                       " SOURCE CHANGED";
    case CreativeEditorObjectActionActivationKind::
        ForceRefreshSavedAssetInstances:
      return state.contextPrefabMatchedInstanceCount == 0U
                 ? "NO INSTANCES"
                 : std::to_string(state.contextPrefabMatchedInstanceCount) +
                       " INSTANCES";
    case CreativeEditorObjectActionActivationKind::Count:
      return "INVALID";
  }
  return "INVALID";
}

bool activateCreativeEditorObjectAction(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeEditorToolOptionsCommandId command,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog,
    const CreativePlacementClearanceCache* placementClearanceCache) {
  CreativeEditorToolOptionsState& state = editor.toolOptions;
  const CreativeEditorObjectActionDescriptor* descriptor =
      creativeEditorObjectActionDescriptor(command);
  if (descriptor == nullptr ||
      !creativeEditorObjectActionEnabled(editor, state, command)) {
    return false;
  }

  bool accepted = false;
  switch (descriptor->activation) {
    case CreativeEditorObjectActionActivationKind::BeginTransformSession:
      accepted = beginCreativeEditorSelectionTransformPreview(
          appState, editor.transform, "object_actions_transform",
          CreativeEditorTransformAnchorPolicy::FixedSource,
          &editor.worldLayout);
      break;
    case CreativeEditorObjectActionActivationKind::ApplyResetTransform: {
      cr::CreativeTransformCommandRequest reset;
      reset.kind = cr::CreativeTransformCommandKind::ResetRotationScale;
      const CreativeEditorObjectActionExecution execution =
          executeCreativeEditorSceneObjectAction(
              {appState, &editor.worldLayout},
              {CreativeEditorTransformSelectionAction{reset},
               "object_actions_reset_transform"});
      accepted =
          creativeEditorObjectActionOutcomeAccepted(execution.outcome);
      break;
    }
    case CreativeEditorObjectActionActivationKind::DuplicateSelection: {
      const CreativeEditorObjectActionExecution execution =
          executeCreativeEditorSceneObjectAction(
              {appState, &editor.worldLayout},
              {CreativeEditorDuplicateSelectionAction{},
               "object_actions_duplicate"});
      accepted =
          creativeEditorObjectActionOutcomeAccepted(execution.outcome);
      break;
    }
    case CreativeEditorObjectActionActivationKind::DeleteSelection: {
      const CreativeEditorObjectActionExecution execution =
          executeCreativeEditorSceneObjectAction(
              {appState, &editor.worldLayout},
              {CreativeEditorDeleteSelectionAction{},
               "object_actions_delete"});
      accepted =
          creativeEditorObjectActionOutcomeAccepted(execution.outcome);
      break;
    }
    case CreativeEditorObjectActionActivationKind::ToggleSelectionVisibility:
      accepted = toggleCreativeEditorSelectionVisibilityWithUndo(
                     appState, appState.history,
                     "object_actions_toggle_visibility",
                     &editor.worldLayout)
                     .accepted;
      break;
    case CreativeEditorObjectActionActivationKind::ToggleSelectionLocked:
      accepted = toggleCreativeEditorSelectionLockedWithUndo(
                     appState, appState.history,
                     "object_actions_toggle_locked", &editor.worldLayout)
                     .accepted;
      break;
    case CreativeEditorObjectActionActivationKind::DetachAttachment:
      accepted = detachCreativeEditorObjectWithUndo(
                     appState, appState.history,
                     state.contextPrimaryObjectId,
                     "object_actions_detach", &editor.worldLayout)
                     .accepted;
      break;
    case CreativeEditorObjectActionActivationKind::ReattachAttachment: {
      if (assetCatalog == nullptr) {
        editor.catalog.statusLabel = "REATTACH: ASSET CATALOG UNAVAILABLE";
        break;
      }
      const CreativeEditorObjectReattachmentPlan plan =
          planCreativeEditorObjectReattachment(
              appState.facade.document(), *assetCatalog,
              state.contextPrimaryObjectId,
              state.contextAttachmentAimTargetId,
              state.contextAttachmentAimPoint,
              cr::CreativeAssetAttachmentMode::AimSocket,
              placementClearanceCache);
      if (!plan.accepted) {
        editor.catalog.statusLabel =
            "REATTACH: " +
            std::string(plan.status ==
                                CreativeEditorObjectReattachmentStatus::SnapRejected
                            ? cr::toString(plan.snap.status)
                            : toString(plan.status));
        break;
      }
      const CreativeEditorObjectReattachmentReceipt receipt =
          reattachCreativeEditorObjectWithUndo(
              appState, appState.history, plan, "object_actions_reattach",
              &editor.worldLayout);
      accepted = receipt.accepted;
      editor.catalog.statusLabel =
          accepted ? "REATTACHED TO " + std::string(plan.snap.targetSocket)
                   : "REATTACH: " + std::string(toString(receipt.status));
      break;
    }
    case CreativeEditorObjectActionActivationKind::GroupSelection:
    case CreativeEditorObjectActionActivationKind::UngroupSelection:
      accepted = applyCreativeEditorGroupCommandWithHistory(
                     appState,
                     descriptor->activation ==
                             CreativeEditorObjectActionActivationKind::
                                 GroupSelection
                         ? "object_actions_group"
                         : "object_actions_ungroup")
                     .accepted;
      break;
    case CreativeEditorObjectActionActivationKind::SaveSelectionAsAsset: {
      const CreativeEditorAuthoredAssetSaveReceipt receipt =
          saveCreativeEditorSelectionAsAuthoredAsset(
              appState, editor.authoredAssets);
      accepted = receipt.accepted;
      if (accepted) {
        const cr::CreativeAuthoredAssetDefinition* definition =
            findCreativeEditorAuthoredAsset(editor.authoredAssets,
                                            receipt.assetId);
        if (definition == nullptr) {
          accepted = false;
          break;
        }
        if (!refreshCreativeEditorAuthoredAssetReferences(
                 editor, *definition)
                 .accepted) {
          accepted = false;
          break;
        }
        accepted = equipCreativeEditorAuthoredAssetToHotbar(appState, editor,
                                                            *definition);
        if (accepted) {
          editor.catalog.statusLabel = "SAVED + EQUIPPED " +
                                       definition->label;
        }
      }
      break;
    }
    case CreativeEditorObjectActionActivationKind::UpdateSavedAsset: {
      const CreativeEditorAuthoredAssetUpdateReceipt update =
          updateCreativeEditorAuthoredAssetFromInstance(
              appState, editor.authoredAssets, state.contextGroupId);
      accepted = update.accepted;
      if (accepted) {
        const cr::CreativeAuthoredAssetDefinition* definition =
            findCreativeEditorAuthoredAsset(editor.authoredAssets,
                                            update.assetId);
        if (definition == nullptr ||
            !refreshCreativeEditorAuthoredAssetReferences(
                 editor, *definition)
                 .accepted) {
          accepted = false;
          break;
        }
        syncCreativeEditorHeldItem(appState, editor);
        editor.catalog.statusLabel = "UPDATED ASSET " + definition->label;
      }
      break;
    }
    case CreativeEditorObjectActionActivationKind::RefreshSavedAssetInstance:
    case CreativeEditorObjectActionActivationKind::
        RefreshSafeSavedAssetInstances:
    case CreativeEditorObjectActionActivationKind::
        ForceRefreshSavedAssetInstances: {
      cr::CreativeAuthoredAssetRefreshMode mode =
          cr::CreativeAuthoredAssetRefreshMode::ForceAll;
      switch (descriptor->activation) {
        case CreativeEditorObjectActionActivationKind::
            RefreshSavedAssetInstance:
          mode = cr::CreativeAuthoredAssetRefreshMode::SelectedInstance;
          break;
        case CreativeEditorObjectActionActivationKind::
            RefreshSafeSavedAssetInstances:
          mode = cr::CreativeAuthoredAssetRefreshMode::SafeInstances;
          break;
        case CreativeEditorObjectActionActivationKind::
            ForceRefreshSavedAssetInstances:
          break;
        default:
          break;
      }
      const CreativeEditorAuthoredAssetInstanceRefreshReceipt refresh =
          refreshCreativeEditorAuthoredAssetInstances(
              appState, editor.authoredAssets, state.contextGroupId, mode);
      accepted = refresh.accepted;
      if (accepted) {
        switch (mode) {
          case cr::CreativeAuthoredAssetRefreshMode::SelectedInstance:
            editor.catalog.statusLabel = "REFRESHED THIS INSTANCE";
            break;
          case cr::CreativeAuthoredAssetRefreshMode::SafeInstances:
            editor.catalog.statusLabel =
                "REFRESHED " +
                std::to_string(refresh.refresh.refreshedInstanceCount) +
                " SAFE INSTANCES";
            break;
          case cr::CreativeAuthoredAssetRefreshMode::ForceAll:
            editor.catalog.statusLabel =
                "FORCE REFRESHED " +
                std::to_string(refresh.refresh.refreshedInstanceCount) +
                " INSTANCES";
            break;
        }
      } else {
        editor.catalog.statusLabel = refresh.reasonCode;
      }
      break;
    }
    case CreativeEditorObjectActionActivationKind::Count:
      break;
  }
  if (accepted) {
    state.open = false;
  }
  return accepted;
}

}  // namespace iggy3d_creative_app
