#include "EditorInteraction.hpp"
#include "EditorInteractionInternal.hpp"

#include <span>
#include <vector>

#include "EditorFrame.hpp"
#include "EditorConnectedFill.hpp"
#include "EditorGizmo.hpp"
#include "EditorGroup.hpp"
#include "EditorLogicLinks.hpp"
#include "EditorPathEditing.hpp"
#include "EditorPattern.hpp"
#include "EditorState.hpp"
#include "EditorStructuralPlacement.hpp"
#include "EditorSurfaceExtrude.hpp"
#include "EditorTerrain.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] cr::CreativeToolModifierFlags toolModifiers(
    cr::CreativeInputModifierMask modifiers) noexcept {
  cr::CreativeToolModifierFlags output = cr::kCreativeToolModifierNone;
  if ((modifiers & cr::kCreativeInputModifierShift) != 0U) {
    output |= cr::kCreativeToolModifierShift;
  }
  if ((modifiers & cr::kCreativeInputModifierControl) != 0U) {
    output |= cr::kCreativeToolModifierControl;
  }
  if ((modifiers & cr::kCreativeInputModifierCommand) != 0U) {
    output |= cr::kCreativeToolModifierCommand;
  }
  return output;
}

[[nodiscard]] cr::CreativeToolInputPacket selectionPacket(
    const CreativeEditorWorldTarget& target,
    cr::CreativeToolModifierFlags modifiers) noexcept {
  cr::CreativeToolInputPacket packet;
  packet.kind = cr::CreativeToolInputKind::PointerPress;
  packet.pointer.button = cr::CreativeToolPointerButton::Primary;
  packet.pointer.modifiers = modifiers;
  if (target.objectHit) {
    packet.pointer.target =
        cr::TargetRef{static_cast<cr::Id>(target.objectId)};
  }
  return packet;
}

struct InteractionContext {
  const CreativeEditorWorldInteractionFrameRequest& request;
  const cr::CreativeHotbarEntry& held;
};

void selectObject(InteractionContext& context) {
  static_cast<void>(
      context.request.appState.facade.setActiveTool(cr::Tool::Select));
  static_cast<void>(context.request.appState.facade.dispatchToolInput(
      selectionPacket(context.request.editor.interaction.target,
                      toolModifiers(context.request.modifiers))));
}

void sampleTargetMaterial(InteractionContext& context) {
  CreativeEditorState& editor = context.request.editor;
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  if ((!target.objectHit && !target.voxelHit) ||
      target.objectKind == cr::CreativeObjectKind::Unknown) {
    return;
  }
  if (context.held.kind != cr::CreativeHeldItemKind::Material &&
      cr::creativeHeldItemUsesMaterial(context.held.kind)) {
    if (cr::creativeVolumeBrushSupported(target.objectKind)) {
      editor.placeBrush = target.objectKind;
      cr::CreativeHotbarEntry& selected =
          cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
      selected.objectKind = target.objectKind;
      cr::clearCreativeHotbarAsset(selected);
      syncCreativeEditorQuickEdit(editor);
    }
    return;
  }
  if (target.objectHit) {
    const cr::CreativeObject* object =
        context.request.appState.facade.document().findObject(target.objectId);
    if (object != nullptr &&
        cr::assignCreativeHotbarFromObject(editor.interaction.hotbar,
                                           *object)) {
      syncCreativeEditorHeldItem(context.request.appState, editor);
      return;
    }
  }
  static_cast<void>(
      cr::assignCreativeHotbarMaterial(editor.interaction.hotbar,
                                       target.objectKind));
  syncCreativeEditorHeldItem(context.request.appState, editor);
}

void setVolumeFirstCorner(InteractionContext& context) {
  CreativeEditorState& editor = context.request.editor;
  if (!editor.interaction.target.grid.valid) {
    return;
  }
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::First,
      editor.interaction.target.grid.targetCell));
  editor.volume.lastReceipt = {};
}

void setVolumeSecondCorner(InteractionContext& context) {
  CreativeEditorState& editor = context.request.editor;
  if (!editor.interaction.target.grid.valid) {
    return;
  }
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::Second,
      editor.interaction.target.grid.targetCell));
  editor.volume.lastReceipt = {};
}

void advanceVolumeSelection(InteractionContext& context) {
  CreativeEditorState& editor = context.request.editor;
  if (!editor.interaction.target.grid.valid) {
    return;
  }
  static_cast<void>(cr::advanceCreativeVolumeSelection(
      editor.volume.selection,
      editor.interaction.target.grid.targetCell));
  editor.volume.lastReceipt = {};
}

void rejectActiveInteraction(InteractionContext& context) {
  static_cast<void>(cancelCreativeEditorHeldItem(
      context.request.appState, context.request.editor));
}

void expandVolumeSelection(InteractionContext& context) {
  CreativeEditorState& editor = context.request.editor;
  if (!editor.interaction.target.grid.valid) {
    return;
  }
  if (cr::expandCreativeVolumeSelectionToCell(
          editor.volume.selection,
          editor.interaction.target.grid.targetCell)) {
    editor.volume.lastReceipt = {};
  }
}

void applyHeldVolumeOperation(InteractionContext& context) {
  const cr::CreativeVolumeOperationKind operation =
      cr::creativeVolumeOperationForHeldItem(context.held.kind);
  context.request.editor.volume.operation = operation;
  static_cast<void>(applyCreativeEditorVolumeOperationWithHistory(
      context.request.appState, context.request.editor.volume,
      context.request.editor.placeBrush, operation,
      context.request.editor.toolSettings,
      "minecraft_secondary_volume_apply"));
}

void setVolumeGestureFeedback(CreativeEditorState& editor,
                              bool accepted) noexcept {
  setCreativeEditorPlacementFeedback(
      editor.interaction,
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex, editor.placeBrush);
}

void beginHeldShapeVolume(InteractionContext& context) {
  CreativeEditorState& editor = context.request.editor;
  const CreativeEditorVolumeGestureReceipt receipt =
      stepCreativeEditorVolumeGesture(
          editor.volume, CreativeEditorVolumeGestureAction::Begin,
          editor.interaction.target.grid.valid,
          editor.interaction.target.grid.targetCell);
  if (receipt.accepted) {
    clearCreativeEditorPlacementFeedback(editor.interaction);
  } else {
    setVolumeGestureFeedback(editor, false);
  }
}

void commitHeldShapeVolume(InteractionContext& context) {
  CreativeEditorState& editor = context.request.editor;
  const CreativeEditorVolumeGestureReceipt gesture =
      stepCreativeEditorVolumeGesture(
          editor.volume, CreativeEditorVolumeGestureAction::Commit,
          editor.interaction.target.grid.valid,
          editor.interaction.target.grid.targetCell);
  if (!gesture.accepted) {
    setVolumeGestureFeedback(editor, false);
    return;
  }
  const cr::CreativeVolumeOperationKind operation =
      cr::creativeVolumeOperationForHeldItem(context.held.kind);
  editor.volume.operation = operation;
  const cr::CreativeVolumeOperationReceipt receipt =
      applyCreativeEditorVolumeOperationWithHistory(
          context.request.appState, editor.volume, editor.placeBrush, operation,
          editor.toolSettings, "minecraft_shape_tool_commit");
  setVolumeGestureFeedback(editor, receipt.accepted);
}

void advanceHeldShapeVolume(InteractionContext& context) {
  if (context.request.editor.volume.selection.phase ==
      cr::CreativeVolumeSelectionPhase::FirstCorner) {
    commitHeldShapeVolume(context);
  } else {
    beginHeldShapeVolume(context);
  }
}

void applyHeldArray(InteractionContext& context) {
  static_cast<void>(applyCreativeEditorArrayWithHistory(
      context.request.appState, context.request.editor.pattern,
      context.request.editor.toolSettings, context.request.editor.placeCellSize,
      context.request.editor.interaction.target.grid.valid,
      context.request.editor.interaction.target.grid.placementAnchor,
      "minecraft_secondary_array"));
}

void acceptHeldArray(InteractionContext& context) {
  const cr::CreativeSelectionState& selection =
      context.request.appState.facade.selectionState();
  const CreativeEditorWorldTarget& target =
      context.request.editor.interaction.target;
  const bool targetAlreadySelected =
      target.objectHit &&
      cr::selectionContainsTarget(
          selection, cr::TargetRef{static_cast<cr::Id>(target.objectId)});
  if (cr::selectedTargetList(selection).empty() ||
      (target.objectHit &&
       (!targetAlreadySelected ||
        context.request.modifiers != cr::kCreativeInputModifierNone))) {
    selectObject(context);
  } else {
    applyHeldArray(context);
  }
}

void paintConnectedFill(InteractionContext& context) {
  static_cast<void>(applyCreativeEditorConnectedFillWithHistory(
      context.request.appState, context.request.editor,
      CreativeConnectedFillEditKind::Paint,
      "minecraft_connected_fill_paint"));
}

void eraseConnectedFill(InteractionContext& context) {
  static_cast<void>(applyCreativeEditorConnectedFillWithHistory(
      context.request.appState, context.request.editor,
      CreativeConnectedFillEditKind::Erase,
      "minecraft_connected_fill_erase"));
}

void extrudeSurface(InteractionContext& context) {
  static_cast<void>(applyCreativeEditorSurfaceExtrudeWithHistory(
      context.request.appState, context.request.editor,
      cr::CreativeSurfaceExtrudeKind::Extrude,
      "minecraft_surface_extrude"));
}

void insetSurface(InteractionContext& context) {
  static_cast<void>(applyCreativeEditorSurfaceExtrudeWithHistory(
      context.request.appState, context.request.editor,
      cr::CreativeSurfaceExtrudeKind::Inset,
      "minecraft_surface_inset"));
}

void upsertTerrainControl(InteractionContext& context) {
  static_cast<void>(applyCreativeEditorTerrainEditWithHistory(
      context.request.appState, context.request.editor,
      CreativeEditorTerrainEditKind::Upsert, "minecraft_terrain_rod_upsert"));
}

void removeTerrainControl(InteractionContext& context) {
  static_cast<void>(applyCreativeEditorTerrainEditWithHistory(
      context.request.appState, context.request.editor,
      CreativeEditorTerrainEditKind::Remove, "minecraft_terrain_rod_remove"));
}

void sampleTerrainControl(InteractionContext& context) {
  const CreativeEditorTerrainEditReceipt receipt =
      applyCreativeEditorTerrainEditWithHistory(
      context.request.appState, context.request.editor,
      CreativeEditorTerrainEditKind::Sample, "minecraft_terrain_rod_sample");
  if (receipt.accepted &&
      context.request.editor.toolSettings.terrainRodStampMode ==
          cr::CreativeTerrainRodStampMode::Seed) {
    context.request.editor.terrain.selectionValid = false;
  }
}

void beginTerrainGrade(InteractionContext& context) {
  static_cast<void>(beginCreativeEditorTerrainGrade(
      context.request.appState, context.request.editor));
}

void applyTerrainGrade(InteractionContext& context) {
  static_cast<void>(applyCreativeEditorTerrainGradeWithHistory(
      context.request.appState, context.request.editor,
      "minecraft_terrain_grade_apply"));
}

void cancelTerrainGrade(InteractionContext& context) {
  static_cast<void>(cancelCreativeEditorTerrainGrade(context.request.editor));
}

void applyTerrainProfile(InteractionContext& context) {
  static_cast<void>(applyCreativeEditorTerrainProfileWithHistory(
      context.request.appState, context.request.editor,
      "minecraft_terrain_profile_apply"));
}

void lockTerrainProfileBase(InteractionContext& context) {
  static_cast<void>(lockCreativeEditorTerrainProfileBase(
      context.request.appState.facade.document(), context.request.editor));
}

void unlockTerrainProfileBase(InteractionContext& context) {
  static_cast<void>(
      unlockCreativeEditorTerrainProfileBase(context.request.editor));
}

void addTerrainPathPoint(InteractionContext& context) {
  static_cast<void>(addCreativeEditorTerrainPathPoint(
      context.request.appState.facade.document(), context.request.editor));
}

void applyTerrainPath(InteractionContext& context) {
  static_cast<void>(applyCreativeEditorTerrainPathWithHistory(
      context.request.appState, context.request.editor,
      "minecraft_terrain_path_apply"));
}

void removeTerrainPathPoint(InteractionContext& context) {
  static_cast<void>(
      removeCreativeEditorTerrainPathPoint(context.request.editor));
}

void applyTerrainRegion(InteractionContext& context) {
  if (context.request.editor.terrain.region.stamp.active) {
    static_cast<void>(applyCreativeEditorTerrainStampWithHistory(
        context.request.appState, context.request.editor,
        "minecraft_terrain_stamp_apply"));
    return;
  }
  static_cast<void>(applyCreativeEditorTerrainRegionWithHistory(
      context.request.appState, context.request.editor,
      "minecraft_terrain_region_apply"));
}

void advanceTerrainRegion(InteractionContext& context) {
  CreativeEditorState& editor = context.request.editor;
  if (cr::creativeVolumeSelectionComplete(editor.volume.selection)) {
    applyTerrainRegion(context);
    return;
  }
  advanceVolumeSelection(context);
}

void sampleTerrainRegionHeight(InteractionContext& context) {
  if (context.request.editor.terrain.region.stamp.active) {
    return;
  }
  static_cast<void>(sampleCreativeEditorTerrainRegionHeight(
      context.request.appState.facade.document(), context.request.editor));
}

void cancelTerrainRegion(InteractionContext& context) {
  if (context.request.editor.terrain.region.stamp.active) {
    static_cast<void>(
        cancelCreativeEditorTerrainStamp(context.request.editor));
    return;
  }
  static_cast<void>(
      cancelCreativeEditorTerrainRegion(context.request.editor));
}

void applyObjectGroup(InteractionContext& context) {
  if (cr::selectedTargetCount(
          context.request.appState.facade.selectionState()) == 0U &&
      context.request.editor.interaction.target.objectHit) {
    static_cast<void>(context.request.appState.facade.dispatchToolInput(
        selectionPacket(context.request.editor.interaction.target,
                        cr::kCreativeToolModifierNone)));
  }
  static_cast<void>(applyCreativeEditorGroupCommandWithHistory(
      context.request.appState, "creative_group_world_action"));
}

void advanceLogicLink(InteractionContext& context) {
  CreativeEditorState& editor = context.request.editor;
  if (!editor.interaction.target.objectHit) {
    editor.logicLinks.status = CreativeEditorLogicLinkStatus::InvalidTarget;
    return;
  }
  static_cast<void>(advanceCreativeEditorLogicLink(
      context.request.appState, editor.logicLinks,
      editor.interaction.target.objectId, "creative_logic_link_world_action"));
}

void clearLogicLinkSource(InteractionContext& context) {
  static_cast<void>(
      clearCreativeEditorLogicLinkSource(context.request.editor.logicLinks));
}

void dispatchHeldItemWorldOperation(
    cr::CreativeHeldItemWorldOperation operation,
    InteractionContext& context) {
  switch (operation) {
    case cr::CreativeHeldItemWorldOperation::None:
      return;
    case cr::CreativeHeldItemWorldOperation::SelectObject:
      selectObject(context);
      return;
    case cr::CreativeHeldItemWorldOperation::SampleTargetMaterial:
      sampleTargetMaterial(context);
      return;
    case cr::CreativeHeldItemWorldOperation::RejectActiveInteraction:
      rejectActiveInteraction(context);
      return;
    case cr::CreativeHeldItemWorldOperation::SetVolumeFirstCorner:
      setVolumeFirstCorner(context);
      return;
    case cr::CreativeHeldItemWorldOperation::SetVolumeSecondCorner:
      setVolumeSecondCorner(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ExpandVolumeSelection:
      expandVolumeSelection(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AdvanceVolumeSelection:
      advanceVolumeSelection(context);
      return;
    case cr::CreativeHeldItemWorldOperation::BeginShapeVolume:
      beginHeldShapeVolume(context);
      return;
    case cr::CreativeHeldItemWorldOperation::CommitShapeVolume:
      commitHeldShapeVolume(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AdvanceShapeVolume:
      advanceHeldShapeVolume(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyVolumeOperation:
      applyHeldVolumeOperation(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyArray:
      applyHeldArray(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AcceptArray:
      acceptHeldArray(context);
      return;
    case cr::CreativeHeldItemWorldOperation::PaintConnectedFill:
      paintConnectedFill(context);
      return;
    case cr::CreativeHeldItemWorldOperation::EraseConnectedFill:
      eraseConnectedFill(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ExtrudeSurface:
      extrudeSurface(context);
      return;
    case cr::CreativeHeldItemWorldOperation::InsetSurface:
      insetSurface(context);
      return;
    case cr::CreativeHeldItemWorldOperation::UpsertTerrainControl:
      upsertTerrainControl(context);
      return;
    case cr::CreativeHeldItemWorldOperation::RemoveTerrainControl:
      removeTerrainControl(context);
      return;
    case cr::CreativeHeldItemWorldOperation::SampleTerrainControl:
      sampleTerrainControl(context);
      return;
    case cr::CreativeHeldItemWorldOperation::BeginTerrainGrade:
      beginTerrainGrade(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyTerrainGrade:
      applyTerrainGrade(context);
      return;
    case cr::CreativeHeldItemWorldOperation::CancelTerrainGrade:
      cancelTerrainGrade(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyTerrainProfile:
      applyTerrainProfile(context);
      return;
    case cr::CreativeHeldItemWorldOperation::LockTerrainProfileBase:
      lockTerrainProfileBase(context);
      return;
    case cr::CreativeHeldItemWorldOperation::UnlockTerrainProfileBase:
      unlockTerrainProfileBase(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AddTerrainPathPoint:
      addTerrainPathPoint(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyTerrainPath:
      applyTerrainPath(context);
      return;
    case cr::CreativeHeldItemWorldOperation::RemoveTerrainPathPoint:
      removeTerrainPathPoint(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyTerrainRegion:
      applyTerrainRegion(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AdvanceTerrainRegion:
      advanceTerrainRegion(context);
      return;
    case cr::CreativeHeldItemWorldOperation::SampleTerrainRegionHeight:
      sampleTerrainRegionHeight(context);
      return;
    case cr::CreativeHeldItemWorldOperation::CancelTerrainRegion:
      cancelTerrainRegion(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyObjectGroup:
      applyObjectGroup(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AdvanceLogicLink:
      advanceLogicLink(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ClearLogicLinkSource:
      clearLogicLinkSource(context);
      return;
    case cr::CreativeHeldItemWorldOperation::Count:
      return;
  }
}

void processMoveInteraction(
    const CreativeEditorWorldInteractionFrameRequest& request) {
  CreativeEditorState& editor = request.editor;
  const float targetX = static_cast<float>(request.contentRegion.x) +
                        static_cast<float>(request.contentRegion.width) * 0.5F;
  const float targetY = static_cast<float>(request.contentRegion.y) +
                        static_cast<float>(request.contentRegion.height) * 0.5F;
  if (processCreativeEditorStructuralSpanEditInput(
          request.appState, editor, request.actions,
          request.pickFrame.structuralSpanEndpointHandles.items(), targetX,
          targetY)) {
    return;
  }
  CreativeMovingPlatformPathEditState& pathEdit =
      editor.interaction.movingPlatformPathEdit;
  const bool rejectPressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Reject);
  if (rejectPressed) {
    if (clearCreativeMovingPlatformPathPointSelection(pathEdit)) {
      return;
    }
    static_cast<void>(cancelCreativeEditorHeldItem(request.appState, editor));
    return;
  }
  const bool pressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Primary) ||
      cr::creativeWorldActionPressed(request.actions,
                                     cr::CreativeWorldActionId::Accept);
  const bool down = cr::creativeWorldActionDown(
      request.actions, cr::CreativeWorldActionId::Primary) ||
      cr::creativeWorldActionDown(request.actions,
                                  cr::CreativeWorldActionId::Accept);
  const bool released = cr::creativeWorldActionReleased(
      request.actions, cr::CreativeWorldActionId::Primary) ||
      cr::creativeWorldActionReleased(request.actions,
                                      cr::CreativeWorldActionId::Accept);
  const bool secondaryPressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Secondary);

  if (secondaryPressed && pathEdit.pointSelected) {
    static_cast<void>(clearCreativeMovingPlatformPathPointSelection(pathEdit));
    return;
  }

  if (pressed && pathEdit.available) {
    const PathPointHandlePickResult handle = pickPathPointHandleAtPixel(
        request.pickFrame.pathPointHandleHits, targetX, targetY);
    if (handle.hit && handle.objectId == pathEdit.objectId &&
        selectCreativeMovingPlatformPathPoint(pathEdit, handle.pointIndex)) {
      return;
    }
    if (pathEdit.pointSelected &&
        queueCreativeMovingPlatformPathEdit(
            request.appState, pathEdit,
            CreativeMovingPlatformPathEditCommand::MoveSelectedToTarget)) {
      const CreativeMovingPlatformPathEditReceipt receipt =
          consumeCreativeMovingPlatformPathEdit(
              request.appState, pathEdit,
              editor.interaction.target.grid.valid,
              editor.interaction.target.grid.placementAnchor,
              "creative_platform_path_point_move",
              editor.toolSettings.moveConstraint);
      setCreativeEditorPlacementFeedback(
          editor.interaction,
          receipt.accepted ? CreativeEditorPlacementFeedbackStatus::Placed
                           : CreativeEditorPlacementFeedbackStatus::Rejected,
          editor.frameIndex, cr::CreativeObjectKind::MovingPlatform,
          receipt.objectId);
      return;
    }
  }

  if (pathEdit.pointSelected) {
    return;
  }

  if (secondaryPressed && !pressed &&
      beginCreativeEditorSelectionTransformPreview(
          request.appState, editor.transform,
          "minecraft_secondary_transform_begin")) {
    static_cast<void>(processCreativeEditorSelectionTransformPreview(
        request.appState, editor.transform,
        editor.interaction.target.grid.valid,
        editor.interaction.target.grid.placementAnchor, false,
        "minecraft_secondary_transform_begin",
        cr::creativeSnapIncrementMeters(editor.toolSettings.snapIncrement)));
    return;
  }

  if (pressed && editor.interaction.target.objectHit) {
    std::vector<cr::CreativeObjectId> selectedObjectIds;
    const cr::CreativeSelectionState& selection =
        request.appState.facade.selectionState();
    const std::span<const cr::TargetRef> selectedTargets =
        cr::selectedTargetList(selection);
    selectedObjectIds.reserve(selectedTargets.empty()
                                  ? 1U
                                  : selectedTargets.size());
    for (cr::TargetRef target : selectedTargets) {
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
    editor.interaction.moveTargetId =
        cr::resolveCreativeHierarchyInteractionRoot(
            request.appState.facade.document(), selectedObjectIds,
            editor.interaction.target.objectId);
    CreativeEditorWorldTarget moveTarget = editor.interaction.target;
    moveTarget.objectId = editor.interaction.moveTargetId;
    static_cast<void>(request.appState.facade.setActiveTool(cr::Tool::Move));
    static_cast<void>(request.appState.facade.dispatchToolInput(
        selectionPacket(moveTarget,
                        cr::kCreativeToolModifierNone)));
  }

  const cr::CreativeToolWorldPoint destination =
      resolveCreativeEditorGroundPoint(request.camera);
  if (down && editor.interaction.moveTargetId != cr::kInvalidObjectId) {
    cr::CreativeToolInputPacket move;
    move.kind = cr::CreativeToolInputKind::PointerMove;
    move.pointer.button = cr::CreativeToolPointerButton::Primary;
    move.pointer.hasWorldDestination = true;
    move.pointer.worldDestination = destination;
    move.pointer.moveHeldAxis = cr::CreativeToolMoveHeldAxis::Y;
    move.pointer.moveConstraint = editor.toolSettings.moveConstraint;
    move.pointer.hasMoveSnapStepOverride = true;
    move.pointer.moveSnapStepOverride =
        cr::creativeSnapIncrementMeters(editor.toolSettings.snapIncrement);
    static_cast<void>(request.appState.facade.dispatchToolInput(move));
  }

  if (released && editor.interaction.moveTargetId != cr::kInvalidObjectId) {
    cr::CreativeToolInputPacket release;
    release.kind = cr::CreativeToolInputKind::PointerRelease;
    release.pointer.button = cr::CreativeToolPointerButton::Primary;
    release.pointer.hasWorldDestination = true;
    release.pointer.worldDestination = destination;
    release.pointer.moveHeldAxis = cr::CreativeToolMoveHeldAxis::Y;
    release.pointer.moveConstraint = editor.toolSettings.moveConstraint;
    release.pointer.hasMoveSnapStepOverride = true;
    release.pointer.moveSnapStepOverride =
        cr::creativeSnapIncrementMeters(editor.toolSettings.snapIncrement);
    static_cast<void>(dispatchMoveReleaseWithUndo(
        request.appState, request.appState.history, release,
        editor.interaction.moveTargetId, "minecraft_primary_move_release"));
    editor.interaction.moveTargetId = cr::kInvalidObjectId;
  }
}

}  // namespace

void dispatchCreativeEditorHeldItemWorldOperation(
    cr::CreativeHeldItemWorldOperation operation,
    const CreativeEditorWorldInteractionFrameRequest& request,
    const cr::CreativeHotbarEntry& held) {
  InteractionContext context{request, held};
  dispatchHeldItemWorldOperation(operation, context);
}

void processCreativeEditorMoveInteraction(
    const CreativeEditorWorldInteractionFrameRequest& request) {
  processMoveInteraction(request);
}

}  // namespace iggy3d_creative_app
