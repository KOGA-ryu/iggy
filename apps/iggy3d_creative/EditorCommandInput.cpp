#include "EditorFrame.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

#include "app/iggy3d/creative/tools/Tools.hpp"

#include "EditorConnectedFill.hpp"
#include "EditorDesktopCommandsInternal.hpp"
#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorPathEditing.hpp"
#include "EditorPersistence.hpp"
#include "EditorSurfaceExtrude.hpp"
#include "EditorTerrain.hpp"
#include "EditorTerrainGeneration.hpp"
#include "EditorToolOptions.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"
#include "EditorWorldLayout.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

void resetCreativeEditorForDocumentReplacement(
    CreativeEditorState& editor,
    creative::CreativeDocumentId documentId) noexcept {
  editor.interaction.synchronizedHeldItemKind =
      creative::CreativeHeldItemKind::Count;
  creative::clearCreativeVolumeSelection(editor.volume.selection);
  editor.terrain.region.stamp.active = false;
  editor.terrain.region.stamp.preview.valid = false;
  editor.terrain.region.stamp.preview.renderAccepted = false;
  editor.terrain.region.stamp.preview.patches.clear();
  resetCreativeEditorTerrainGeneration(editor.terrainGeneration);
  invalidateCreativeEditorConnectedFillCache(editor.interaction.connectedFill);
  invalidateCreativeEditorSurfaceExtrudeCache(
      editor.interaction.surfaceExtrude);
  resetCreativeMaterialBrushPivot(editor.interaction.materialBrushPivot,
                                  documentId);
  editor.interaction.movingPlatformPathEdit = {};
  editor.interaction.structuralSpanEdit = {};
  editor.movingPlatformPreview = {};
  resetCreativeEditorWorldLayout(editor.worldLayout);
}

namespace {

[[nodiscard]] bool isControllerTransformShortcut(
    const CreativeEditorState& editor,
    const creative::CreativeInputActionEvent& event) noexcept {
  if (event.action != creative::CreativeInputActionId::QuickEditNext) {
    return false;
  }
  const creative::CreativeControlProfile& profile = editor.controlProfile;
  std::uint16_t shortcutGroup =
      static_cast<std::uint16_t>(profile.groupCount);
  for (std::size_t index = 0U; index < profile.bindingCount; ++index) {
    const creative::CreativeInputBinding& binding = profile.bindings[index];
    if (binding.action == event.action && binding.trigger == event.trigger &&
        binding.context == creative::CreativeInputContext::EditorViewport &&
        creative::creativeInputKeyIsGamepad(binding.trigger)) {
      shortcutGroup = profile.bindingGroups[index];
      break;
    }
  }
  if (shortcutGroup >= profile.groupCount) {
    return false;
  }
  for (std::size_t index = 0U; index < profile.bindingCount; ++index) {
    const creative::CreativeInputBinding& binding = profile.bindings[index];
    if (profile.bindingGroups[index] == shortcutGroup &&
        binding.action == creative::CreativeInputActionId::QuickEditNext &&
        binding.context == creative::CreativeInputContext::TransformPreview) {
      return true;
    }
  }
  return false;
}

}  // namespace

void applyCreativeEditorCommandInput(
    const creative::CreativeInputRouteResult& routedInput,
    creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId) {
  if (!routedInput.actionEvents().empty()) {
    finalizeCreativeEditorContinuousGestures(
        appState, editor, "creative_continuous_gesture_command");
  }
  if (routedInput.context ==
          creative::CreativeInputContext::TransformPreview ||
      routedInput.context ==
          creative::CreativeInputContext::TransformControls) {
    return;
  }
  if (routedInput.context != creative::CreativeInputContext::EditorViewport) {
    return;
  }
  const auto terrainRegionHeld = [&editor] {
    return creative::selectedCreativeHotbarEntry(editor.interaction.hotbar).kind ==
           creative::CreativeHeldItemKind::TerrainRegion;
  };
  for (const creative::CreativeInputActionEvent& event :
       routedInput.actionEvents()) {
    switch (event.action) {
      case creative::CreativeInputActionId::HotbarSlot1:
      case creative::CreativeInputActionId::HotbarSlot2:
      case creative::CreativeInputActionId::HotbarSlot3:
      case creative::CreativeInputActionId::HotbarSlot4:
      case creative::CreativeInputActionId::HotbarSlot5:
      case creative::CreativeInputActionId::HotbarSlot6:
      case creative::CreativeInputActionId::HotbarSlot7:
      case creative::CreativeInputActionId::HotbarSlot8:
      case creative::CreativeInputActionId::HotbarSlot9: {
        const std::size_t slot =
            static_cast<std::size_t>(event.action) -
            static_cast<std::size_t>(
                creative::CreativeInputActionId::HotbarSlot1);
        static_cast<void>(
            selectCreativeEditorHotbarSlot(appState, editor, slot));
        break;
      }
      case creative::CreativeInputActionId::FrameContext3D:
        if (!focusEditorCameraOnSelection(
                editor, appState.facade.document(),
                appState.facade.selectionState())) {
          static_cast<void>(focusEditorCameraOnDocument(
              editor, appState.facade.document()));
        }
        break;
      case creative::CreativeInputActionId::ToggleCatalog:
      case creative::CreativeInputActionId::CatalogPrevious:
      case creative::CreativeInputActionId::CatalogNext:
      case creative::CreativeInputActionId::CatalogPreviousVariant:
      case creative::CreativeInputActionId::CatalogNextVariant:
      case creative::CreativeInputActionId::CatalogPreviousPage:
      case creative::CreativeInputActionId::CatalogNextPage:
      case creative::CreativeInputActionId::CatalogConfirm:
      case creative::CreativeInputActionId::CatalogContextAction:
      case creative::CreativeInputActionId::CatalogClose:
      case creative::CreativeInputActionId::ToggleToolWheel:
      case creative::CreativeInputActionId::ToolWheelPrevious:
      case creative::CreativeInputActionId::ToolWheelNext:
      case creative::CreativeInputActionId::ToolWheelConfirm:
      case creative::CreativeInputActionId::ToolWheelOptions:
      case creative::CreativeInputActionId::ToolWheelClose:
      case creative::CreativeInputActionId::ToolOptionsPrevious:
      case creative::CreativeInputActionId::ToolOptionsNext:
      case creative::CreativeInputActionId::ToolOptionsDecrease:
      case creative::CreativeInputActionId::ToolOptionsIncrease:
      case creative::CreativeInputActionId::ToolOptionsConfirm:
      case creative::CreativeInputActionId::ToolOptionsClose:
      case creative::CreativeInputActionId::ToggleTransformControls:
      case creative::CreativeInputActionId::TransformControlPrevious:
      case creative::CreativeInputActionId::TransformControlNext:
      case creative::CreativeInputActionId::TransformConstraintX:
      case creative::CreativeInputActionId::TransformConstraintY:
      case creative::CreativeInputActionId::TransformConstraintZ:
      case creative::CreativeInputActionId::TransformNudgeNegative:
      case creative::CreativeInputActionId::TransformNudgePositive:
      case creative::CreativeInputActionId::MoveForward:
      case creative::CreativeInputActionId::MoveBackward:
      case creative::CreativeInputActionId::MoveLeft:
      case creative::CreativeInputActionId::MoveRight:
      case creative::CreativeInputActionId::FlyUp:
      case creative::CreativeInputActionId::FlyDown:
      case creative::CreativeInputActionId::Sprint:
      case creative::CreativeInputActionId::RuntimeAttack:
      case creative::CreativeInputActionId::RuntimeInteract:
      case creative::CreativeInputActionId::PrimaryAction:
      case creative::CreativeInputActionId::SecondaryAction:
      case creative::CreativeInputActionId::AcceptAction:
      case creative::CreativeInputActionId::RejectAction:
      case creative::CreativeInputActionId::PickAction:
      case creative::CreativeInputActionId::HotbarPrevious:
      case creative::CreativeInputActionId::HotbarNext:
      case creative::CreativeInputActionId::ToggleControls:
      case creative::CreativeInputActionId::ControlsPrevious:
      case creative::CreativeInputActionId::ControlsNext:
      case creative::CreativeInputActionId::ControlsDecrease:
      case creative::CreativeInputActionId::ControlsIncrease:
      case creative::CreativeInputActionId::ControlsActivate:
      case creative::CreativeInputActionId::ControlsClose:
      case creative::CreativeInputActionId::ControlsResetDefaults:
      case creative::CreativeInputActionId::Count:
        break;
      case creative::CreativeInputActionId::QuickEditNext: {
        const creative::CreativeHeldItemKind held =
            creative::selectedCreativeHotbarEntry(
                editor.interaction.hotbar).kind;
        if (held == creative::CreativeHeldItemKind::ObjectMove &&
            isControllerTransformShortcut(editor, event)) {
          static_cast<void>(clearCreativeMovingPlatformPathPointSelection(
              editor.interaction.movingPlatformPathEdit));
          if (creative::selectedTargetCount(
                  appState.facade.selectionState()) > 0U) {
            static_cast<void>(beginCreativeEditorSelectionTransformPreview(
                appState, editor.transform,
                "controller_selection_transform_begin",
                CreativeEditorTransformAnchorPolicy::FixedSource,
                &editor.worldLayout));
          }
          break;
        }
        if (held == creative::CreativeHeldItemKind::ObjectMove) {
          syncCreativeMovingPlatformPathEditState(
              appState, editor.interaction.movingPlatformPathEdit);
          if (cycleCreativeMovingPlatformPathPoint(
                  editor.interaction.movingPlatformPathEdit, 1)) {
            break;
          }
        }
        static_cast<void>(
            processCreativeEditorQuickEditAction(editor, event.action));
        break;
      }
      case creative::CreativeInputActionId::QuickEditPrevious: {
        const creative::CreativeHeldItemKind held =
            creative::selectedCreativeHotbarEntry(
                editor.interaction.hotbar).kind;
        if (held == creative::CreativeHeldItemKind::ObjectMove) {
          syncCreativeMovingPlatformPathEditState(
              appState, editor.interaction.movingPlatformPathEdit);
          if (cycleCreativeMovingPlatformPathPoint(
                  editor.interaction.movingPlatformPathEdit, -1)) {
            break;
          }
        }
        static_cast<void>(
            processCreativeEditorQuickEditAction(editor, event.action));
        break;
      }
      case creative::CreativeInputActionId::QuickEditDecrease:
      case creative::CreativeInputActionId::QuickEditIncrease: {
        const creative::CreativeHeldItemKind held =
            creative::selectedCreativeHotbarEntry(
                editor.interaction.hotbar).kind;
        if (held == creative::CreativeHeldItemKind::ObjectMove &&
            (event.action ==
                 creative::CreativeInputActionId::QuickEditDecrease ||
             event.action ==
                 creative::CreativeInputActionId::QuickEditIncrease)) {
          const CreativeMovingPlatformPathEditCommand command =
              event.action ==
                      creative::CreativeInputActionId::QuickEditIncrease
                  ? CreativeMovingPlatformPathEditCommand::AppendAtTarget
                  : editor.interaction.movingPlatformPathEdit.pointSelected
                        ? CreativeMovingPlatformPathEditCommand::RemoveSelected
                        : CreativeMovingPlatformPathEditCommand::RemoveLast;
          if (queueCreativeMovingPlatformPathEdit(
                  appState, editor.interaction.movingPlatformPathEdit,
                  command)) {
            break;
          }
        }
        static_cast<void>(
            processCreativeEditorQuickEditAction(editor, event.action));
        break;
      }
      case creative::CreativeInputActionId::ConfirmActiveTool:
        if (editor.interaction.movingPlatformPathEdit.pointSelected &&
            queueCreativeMovingPlatformPathEdit(
                appState, editor.interaction.movingPlatformPathEdit,
                CreativeMovingPlatformPathEditCommand::MoveSelectedToTarget)) {
          break;
        }
        static_cast<void>(confirmCreativeEditorHeldItem(
            appState, editor, "keyboard_confirm"));
        break;
      case creative::CreativeInputActionId::CancelActiveTool:
        if (clearCreativeMovingPlatformPathPointSelection(
                editor.interaction.movingPlatformPathEdit)) {
          break;
        }
        static_cast<void>(cancelCreativeEditorHeldItem(appState, editor));
        break;
      case creative::CreativeInputActionId::DeleteSelection:
        if (terrainRegionHeld()) {
          break;
        }
        if (editor.volume.active) {
          static_cast<void>(applyCreativeEditorVolumeOperationWithHistory(
              appState, editor.volume, editor.placeBrush,
              creative::CreativeVolumeOperationKind::Erase,
              editor.toolSettings,
              event.trigger == creative::CreativeInputKey::Backspace
                  ? "volume_backspace_erase"
                  : "volume_delete_erase"));
        } else if (!(editor.interaction.movingPlatformPathEdit.pointSelected &&
                     queueCreativeMovingPlatformPathEdit(
                       appState, editor.interaction.movingPlatformPathEdit,
                       CreativeMovingPlatformPathEditCommand::RemoveSelected))) {
          (void)deleteCreativeEditorSelectionWithUndo(
              appState,
              event.trigger == creative::CreativeInputKey::Backspace
                  ? "backspace_key"
                  : "delete_key",
              &appState.history, &editor.worldLayout);
        }
        break;
      case creative::CreativeInputActionId::Undo:
        (void)undoLastEdit(appState, "keyboard_undo",
                           editor.assetEdit.active ? nullptr
                                                   : &editor.worldLayout);
        break;
      case creative::CreativeInputActionId::Redo:
        (void)redoLastEdit(appState, "keyboard_redo",
                           editor.assetEdit.active ? nullptr
                                                   : &editor.worldLayout);
        break;
      case creative::CreativeInputActionId::CopySelection:
        if (terrainRegionHeld()) {
          static_cast<void>(
              copyCreativeEditorTerrainRegionToStamp(appState, editor));
        } else {
          (void)copySelectionToClipboard(appState, "keyboard_copy");
        }
        break;
      case creative::CreativeInputActionId::CutSelection:
        if (!terrainRegionHeld()) {
          (void)cutSelectionToClipboardWithHistory(appState, "keyboard_cut");
        }
        break;
      case creative::CreativeInputActionId::PasteClipboard:
        if (terrainRegionHeld()) {
          static_cast<void>(
              beginCreativeEditorTerrainStampPreview(appState, editor));
        } else {
          static_cast<void>(beginCreativeEditorClipboardTransformPreview(
              appState, appState.clipboard, editor.transform,
              "keyboard_paste", &editor.worldLayout));
        }
        break;
      case creative::CreativeInputActionId::DuplicateSelection:
        if (terrainRegionHeld()) {
          const CreativeEditorTerrainStampReceipt copied =
              copyCreativeEditorTerrainRegionToStamp(appState, editor);
          if (copied.accepted) {
            static_cast<void>(
                beginCreativeEditorTerrainStampPreview(appState, editor));
          }
        } else {
          (void)duplicateSelectedObjectsWithUndo(
              appState, appState.history,
              creative::CreativeDuplicateCommandRequest{},
              "keyboard_duplicate");
        }
        break;
      case creative::CreativeInputActionId::RotateYawNegative:
      case creative::CreativeInputActionId::RotateYawPositive: {
        creative::CreativeTransformCommandRequest request;
        request.kind = creative::CreativeTransformCommandKind::RotateYaw;
        const double rotationStep =
            creative::creativeRotationStepDegrees(
                editor.toolSettings.rotationStep);
        request.yawDegrees =
            event.action == creative::CreativeInputActionId::RotateYawNegative
                ? -rotationStep
                : rotationStep;
        (void)transformSelectedObjectsWithUndo(
            appState, appState.history, request,
            request.yawDegrees < 0.0 ? "keyboard_rotate_yaw_negative"
                                     : "keyboard_rotate_yaw_positive");
        break;
      }
      case creative::CreativeInputActionId::ScaleDown:
      case creative::CreativeInputActionId::ScaleUp: {
        creative::CreativeTransformCommandRequest request;
        request.kind = creative::CreativeTransformCommandKind::Scale;
        const double factor =
            event.action == creative::CreativeInputActionId::ScaleDown ? 0.9
                                                                        : 1.1;
        request.scaleFactor = {factor, factor, factor};
        (void)transformSelectedObjectsWithUndo(
            appState, appState.history, request,
            factor < 1.0 ? "keyboard_scale_down" : "keyboard_scale_up");
        break;
      }
      case creative::CreativeInputActionId::Save: {
        const iggy3d::CreativeWorldSaveResult saveResult =
            saveStandaloneScene(appState.facade, saveRoot, saveId,
                                &editor.worldLayout.source,
                                editor.worldLayout.generatedRevision ==
                                    editor.worldLayout.revision);
        if (saveResult.accepted && saveResult.saved) {
          clearEditHistory(appState.history, "save_success");
          markCreativeEditorWorldLayoutSaved(editor.worldLayout);
          markCreativeEditorDocumentSaved(
              editor.persistence, appState.facade.document());
        }
        break;
      }
      case creative::CreativeInputActionId::NewDocument: {
        const creative::CreativeFacadeDocumentInstallReceipt replaced =
            clearToBlankScene(appState);
        if (replaced.accepted) {
          clearEditHistory(appState.history, "new_clear");
          resetCreativeEditorForDocumentReplacement(
              editor, appState.facade.document().id());
          clearCreativeEditorDocumentSavePoint(
              editor.persistence, appState.facade.document().id());
        }
        break;
      }
      case creative::CreativeInputActionId::Load: {
        creative::CreativeWorldLayout loadedLayout;
        const bool loaded = loadStandaloneScene(appState, saveRoot, saveId,
                                                &loadedLayout);
        if (loaded) {
          clearEditHistory(appState.history, "load_success");
          resetCreativeEditorForDocumentReplacement(
              editor, appState.facade.document().id());
          installCreativeEditorWorldLayout(editor.worldLayout,
                                           std::move(loadedLayout));
          markCreativeEditorDocumentSaved(
              editor.persistence, appState.facade.document());
        }
        break;
      }
    }
  }
}


}  // namespace iggy3d_creative_app
