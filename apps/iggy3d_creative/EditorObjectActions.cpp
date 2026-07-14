#include "EditorObjectActions.hpp"

#include <span>
#include <string>
#include <vector>

#include "EditorAuthoredAssets.hpp"
#include "EditorEdits.hpp"
#include "EditorGroup.hpp"
#include "EditorState.hpp"
#include "EditorTransform.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

bool creativeEditorCommandIsObjectAction(
    CreativeEditorToolOptionsCommandId command) noexcept {
  switch (command) {
    case CreativeEditorToolOptionsCommandId::TransformSelection:
    case CreativeEditorToolOptionsCommandId::ResetSelectionTransform:
    case CreativeEditorToolOptionsCommandId::DuplicateSelection:
    case CreativeEditorToolOptionsCommandId::DeleteSelection:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionVisibility:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionLocked:
    case CreativeEditorToolOptionsCommandId::GroupSelection:
    case CreativeEditorToolOptionsCommandId::UngroupSelection:
    case CreativeEditorToolOptionsCommandId::SaveSelectionAsAsset:
    case CreativeEditorToolOptionsCommandId::UpdateSavedAsset:
    case CreativeEditorToolOptionsCommandId::RefreshSavedAssetInstances:
      return true;
    case CreativeEditorToolOptionsCommandId::SetMaterialBrushSymmetryPivot:
    case CreativeEditorToolOptionsCommandId::ClearMaterialBrushSymmetryPivot:
    case CreativeEditorToolOptionsCommandId::EditGroupContents:
    case CreativeEditorToolOptionsCommandId::Count:
      return false;
  }
  return false;
}

void refreshCreativeEditorObjectActionContext(
    const cr::CreativeAppState& appState,
    CreativeEditorToolOptionsState& state) noexcept {
  state.contextGroupId = cr::kInvalidObjectId;
  state.contextPrimaryObjectId = cr::kInvalidObjectId;
  state.contextPrimaryObjectKind = cr::CreativeObjectKind::Unknown;
  state.contextContainerKind = cr::CreativeObjectKind::Unknown;
  state.contextContainerAssetId.clear();
  state.contextSelectionCount = 0U;
  state.contextPrimaryVisible = true;
  state.contextPrimaryLocked = false;
  state.contextAllUnlocked = true;
  state.contextAllMovable = true;
  state.contextAllResettable = true;
  state.contextPrefabUpdateTransformSupported = false;

  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  std::vector<cr::CreativeObjectId> selectedObjectIds;
  selectedObjectIds.reserve(cr::selectedTargetCount(selection));
  for (cr::TargetRef target : cr::selectedTargetList(selection)) {
    if (target.value != cr::kInvalidId) {
      selectedObjectIds.push_back(
          static_cast<cr::CreativeObjectId>(target.value));
    }
  }
  if (selectedObjectIds.empty() &&
      selection.selectedTarget.value != cr::kInvalidId) {
    selectedObjectIds.push_back(static_cast<cr::CreativeObjectId>(
        selection.selectedTarget.value));
  }
  state.contextSelectionCount = selectedObjectIds.size();

  const cr::CreativeHierarchySelection hierarchy =
      cr::resolveCreativeObjectHierarchy(appState.facade.document(),
                                         selectedObjectIds);
  const std::span<const cr::CreativeObjectId> capabilityObjectIds =
      hierarchy.accepted
          ? std::span<const cr::CreativeObjectId>{hierarchy.objectIds}
          : std::span<const cr::CreativeObjectId>{selectedObjectIds};
  for (cr::CreativeObjectId objectId : capabilityObjectIds) {
    const cr::CreativeObject* object = appState.facade.findObject(objectId);
    if (object == nullptr) {
      state.contextAllUnlocked = false;
      state.contextAllMovable = false;
      state.contextAllResettable = false;
      continue;
    }
    state.contextAllUnlocked = state.contextAllUnlocked && !object->locked;
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
  if (state.contextSelectionCount == 0U) {
    state.contextAllUnlocked = false;
    state.contextAllMovable = false;
    state.contextAllResettable = false;
  }

  const cr::TargetRef primary = selection.selectedTarget;
  if (primary.value == cr::kInvalidId) {
    return;
  }
  const cr::CreativeObject* object = appState.facade.findObject(
      static_cast<cr::CreativeObjectId>(primary.value));
  if (object == nullptr) {
    return;
  }
  state.contextPrimaryObjectId = object->id;
  state.contextPrimaryObjectKind = object->kind;
  state.contextPrimaryVisible = object->visible;
  state.contextPrimaryLocked = object->locked;
  if (state.contextSelectionCount != 1U) {
    return;
  }
  if (cr::creativeObjectIsHierarchyContainer(object->kind)) {
    state.contextGroupId = object->id;
    state.contextContainerKind = object->kind;
    state.contextContainerAssetId = object->assetId;
    state.contextPrefabUpdateTransformSupported =
        cr::creativeAuthoredAssetInstanceTransformSupported(*object);
    return;
  }
  if (!object->parentId.has_value()) {
    return;
  }
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

bool creativeEditorObjectActionEnabled(
    const CreativeEditorState& editor,
    const CreativeEditorToolOptionsState& state,
    CreativeEditorToolOptionsCommandId command) noexcept {
  switch (command) {
    case CreativeEditorToolOptionsCommandId::TransformSelection:
      return state.contextSelectionCount > 0U && state.contextAllUnlocked &&
             state.contextAllMovable;
    case CreativeEditorToolOptionsCommandId::ResetSelectionTransform:
      return state.contextSelectionCount > 0U && state.contextAllUnlocked &&
             state.contextAllResettable;
    case CreativeEditorToolOptionsCommandId::DuplicateSelection:
      return state.contextSelectionCount > 0U && state.contextAllUnlocked;
    case CreativeEditorToolOptionsCommandId::DeleteSelection:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionVisibility:
      return state.contextPrimaryObjectId != cr::kInvalidObjectId &&
             !state.contextPrimaryLocked;
    case CreativeEditorToolOptionsCommandId::ToggleSelectionLocked:
      return state.contextPrimaryObjectId != cr::kInvalidObjectId;
    case CreativeEditorToolOptionsCommandId::GroupSelection:
      return state.contextSelectionCount > 1U && state.contextAllUnlocked;
    case CreativeEditorToolOptionsCommandId::UngroupSelection:
      return state.contextGroupId != cr::kInvalidObjectId;
    case CreativeEditorToolOptionsCommandId::SaveSelectionAsAsset:
      return state.contextSelectionCount > 0U && state.contextAllUnlocked &&
             !editor.authoredAssets.root.empty();
    case CreativeEditorToolOptionsCommandId::UpdateSavedAsset:
    case CreativeEditorToolOptionsCommandId::RefreshSavedAssetInstances:
      return state.contextGroupId != cr::kInvalidObjectId &&
             state.contextContainerKind ==
                 cr::CreativeObjectKind::PrefabInstance &&
             state.contextPrefabUpdateTransformSupported &&
             findCreativeEditorAuthoredAsset(
                 editor.authoredAssets,
                 state.contextContainerAssetId) != nullptr;
    case CreativeEditorToolOptionsCommandId::SetMaterialBrushSymmetryPivot:
    case CreativeEditorToolOptionsCommandId::ClearMaterialBrushSymmetryPivot:
    case CreativeEditorToolOptionsCommandId::EditGroupContents:
    case CreativeEditorToolOptionsCommandId::Count:
      return false;
  }
  return false;
}

std::string_view creativeEditorObjectActionLabel(
    CreativeEditorToolOptionsCommandId command) noexcept {
  switch (command) {
    case CreativeEditorToolOptionsCommandId::TransformSelection:
      return "TRANSFORM";
    case CreativeEditorToolOptionsCommandId::ResetSelectionTransform:
      return "RESET TRANSFORM";
    case CreativeEditorToolOptionsCommandId::DuplicateSelection:
      return "DUPLICATE";
    case CreativeEditorToolOptionsCommandId::DeleteSelection:
      return "DELETE";
    case CreativeEditorToolOptionsCommandId::ToggleSelectionVisibility:
      return "VISIBILITY";
    case CreativeEditorToolOptionsCommandId::ToggleSelectionLocked:
      return "LOCK";
    case CreativeEditorToolOptionsCommandId::GroupSelection:
      return "GROUP";
    case CreativeEditorToolOptionsCommandId::UngroupSelection:
      return "UNGROUP";
    case CreativeEditorToolOptionsCommandId::SaveSelectionAsAsset:
      return "SAVE AS ASSET";
    case CreativeEditorToolOptionsCommandId::UpdateSavedAsset:
      return "UPDATE SAVED ASSET";
    case CreativeEditorToolOptionsCommandId::RefreshSavedAssetInstances:
      return "REFRESH ALL INSTANCES";
    case CreativeEditorToolOptionsCommandId::SetMaterialBrushSymmetryPivot:
    case CreativeEditorToolOptionsCommandId::ClearMaterialBrushSymmetryPivot:
    case CreativeEditorToolOptionsCommandId::EditGroupContents:
    case CreativeEditorToolOptionsCommandId::Count:
      return "INVALID ACTION";
  }
  return "INVALID ACTION";
}

std::string creativeEditorObjectActionValueLabel(
    const CreativeEditorToolOptionsState& state,
    CreativeEditorToolOptionsCommandId command) {
  switch (command) {
    case CreativeEditorToolOptionsCommandId::TransformSelection:
      return "MOVE ROTATE SCALE MIRROR";
    case CreativeEditorToolOptionsCommandId::ResetSelectionTransform:
      return "ROTATION + SCALE";
    case CreativeEditorToolOptionsCommandId::DuplicateSelection:
      return state.contextSelectionCount == 0U
                 ? "SELECT OBJECT"
                 : std::to_string(state.contextSelectionCount) + " OBJECTS";
    case CreativeEditorToolOptionsCommandId::DeleteSelection:
      return state.contextPrimaryObjectId == cr::kInvalidObjectId
                 ? "SELECT OBJECT"
                 : "PRIMARY OBJECT";
    case CreativeEditorToolOptionsCommandId::ToggleSelectionVisibility:
      return state.contextPrimaryObjectId == cr::kInvalidObjectId
                 ? "SELECT OBJECT"
                 : state.contextPrimaryVisible ? "HIDE" : "SHOW";
    case CreativeEditorToolOptionsCommandId::ToggleSelectionLocked:
      return state.contextPrimaryObjectId == cr::kInvalidObjectId
                 ? "SELECT OBJECT"
                 : state.contextPrimaryLocked ? "UNLOCK" : "LOCK";
    case CreativeEditorToolOptionsCommandId::GroupSelection:
      return state.contextSelectionCount > 1U
                 ? std::to_string(state.contextSelectionCount) + " OBJECTS"
                 : "SELECT MULTIPLE";
    case CreativeEditorToolOptionsCommandId::UngroupSelection:
      return state.contextGroupId != cr::kInvalidObjectId
                 ? state.contextContainerKind ==
                           cr::CreativeObjectKind::PrefabInstance
                       ? "UNPACK INSTANCE"
                       : "REMOVE CONTAINER"
                 : "SELECT GROUP";
    case CreativeEditorToolOptionsCommandId::SaveSelectionAsAsset:
      if (state.contextSelectionCount == 0U) {
        return "SELECT OBJECTS";
      }
      if (state.contextSelectionCount == 1U) {
        return "1 ROOT";
      }
      return std::to_string(state.contextSelectionCount) + " ROOTS";
    case CreativeEditorToolOptionsCommandId::UpdateSavedAsset:
    case CreativeEditorToolOptionsCommandId::RefreshSavedAssetInstances:
      if (state.contextContainerKind !=
          cr::CreativeObjectKind::PrefabInstance) {
        return "SELECT ASSET INSTANCE";
      }
      if (!state.contextPrefabUpdateTransformSupported) {
        return "RESET INSTANCE SCALE + TILT";
      }
      return state.contextContainerAssetId;
    case CreativeEditorToolOptionsCommandId::SetMaterialBrushSymmetryPivot:
    case CreativeEditorToolOptionsCommandId::ClearMaterialBrushSymmetryPivot:
    case CreativeEditorToolOptionsCommandId::EditGroupContents:
    case CreativeEditorToolOptionsCommandId::Count:
      return "INVALID";
  }
  return "INVALID";
}

bool activateCreativeEditorObjectAction(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeEditorToolOptionsCommandId command) {
  CreativeEditorToolOptionsState& state = editor.toolOptions;
  if (!creativeEditorObjectActionEnabled(editor, state, command)) {
    return false;
  }

  bool accepted = false;
  switch (command) {
    case CreativeEditorToolOptionsCommandId::TransformSelection:
      accepted = beginCreativeEditorSelectionTransformPreview(
          appState, editor.transform, "object_actions_transform",
          CreativeEditorTransformAnchorPolicy::FixedSource);
      break;
    case CreativeEditorToolOptionsCommandId::ResetSelectionTransform: {
      cr::CreativeTransformCommandRequest reset;
      reset.kind = cr::CreativeTransformCommandKind::ResetRotationScale;
      accepted = transformSelectedObjectsWithUndo(
                     appState, appState.history, reset,
                     "object_actions_reset_transform")
                     .accepted;
      break;
    }
    case CreativeEditorToolOptionsCommandId::DuplicateSelection:
      accepted = duplicateSelectedObjectsWithUndo(
                     appState, appState.history,
                     cr::CreativeDuplicateCommandRequest{},
                     "object_actions_duplicate")
                     .accepted;
      break;
    case CreativeEditorToolOptionsCommandId::DeleteSelection:
      accepted = deleteSelectedObject(appState, "object_actions_delete",
                                      &appState.history)
                     .accepted;
      break;
    case CreativeEditorToolOptionsCommandId::ToggleSelectionVisibility:
      accepted = toggleSelectedObjectVisibilityWithUndo(
                     appState, appState.history,
                     "object_actions_toggle_visibility")
                     .accepted;
      break;
    case CreativeEditorToolOptionsCommandId::ToggleSelectionLocked:
      accepted = toggleSelectedObjectLockedWithUndo(
                     appState, appState.history,
                     "object_actions_toggle_locked")
                     .accepted;
      break;
    case CreativeEditorToolOptionsCommandId::GroupSelection:
    case CreativeEditorToolOptionsCommandId::UngroupSelection:
      accepted = applyCreativeEditorGroupCommandWithHistory(
                     appState,
                     command == CreativeEditorToolOptionsCommandId::GroupSelection
                         ? "object_actions_group"
                         : "object_actions_ungroup")
                     .accepted;
      break;
    case CreativeEditorToolOptionsCommandId::SaveSelectionAsAsset: {
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
        const std::size_t slot = static_cast<std::size_t>(
            editor.interaction.hotbar.selectedSlot);
        cr::CreativeHotbarEntry& held =
            cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
        const cr::CreativeHeldItemKind previousKind = held.kind;
        held = {cr::CreativeHeldItemKind::Material,
                cr::CreativeObjectKind::PrefabInstance};
        accepted = cr::setCreativeHotbarAsset(
            held, definition->assetId, definition->sourceBounds);
        if (accepted) {
          if (previousKind != held.kind) {
            clearCreativeMaterialBrushPresetSlot(
                editor.interaction.materialBrushPresets, slot);
          }
          static_cast<void>(activateSelectedCreativeMaterialBrushPreset(
              editor.interaction.materialBrushPresets,
              editor.interaction.hotbar, editor.toolSettings));
          syncCreativeEditorHeldItem(appState, editor);
          editor.catalog.statusLabel = "SAVED + EQUIPPED " +
                                       definition->label;
        }
      }
      break;
    }
    case CreativeEditorToolOptionsCommandId::UpdateSavedAsset: {
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
    case CreativeEditorToolOptionsCommandId::RefreshSavedAssetInstances: {
      const CreativeEditorAuthoredAssetInstanceRefreshReceipt refresh =
          refreshCreativeEditorAuthoredAssetInstances(
              appState, editor.authoredAssets, state.contextGroupId);
      accepted = refresh.accepted;
      if (accepted) {
        editor.catalog.statusLabel =
            "REFRESHED " +
            std::to_string(refresh.refresh.refreshedInstanceCount) +
            " INSTANCES";
      } else {
        editor.catalog.statusLabel = refresh.reasonCode;
      }
      break;
    }
    case CreativeEditorToolOptionsCommandId::SetMaterialBrushSymmetryPivot:
    case CreativeEditorToolOptionsCommandId::ClearMaterialBrushSymmetryPivot:
    case CreativeEditorToolOptionsCommandId::EditGroupContents:
    case CreativeEditorToolOptionsCommandId::Count:
      break;
  }
  if (accepted) {
    state.open = false;
  }
  return accepted;
}

}  // namespace iggy3d_creative_app
