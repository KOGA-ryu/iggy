#include "EditorInteraction.hpp"
#include "EditorInteractionInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

#include "EditorFrame.hpp"
#include "EditorConnectedFill.hpp"
#include "EditorGizmo.hpp"
#include "EditorGroup.hpp"
#include "EditorPattern.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "EditorSurfaceExtrude.hpp"
#include "EditorTerrain.hpp"
#include "EditorTerrainPaint.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"
#include "app/iggy3d/creative/tools/ShapeBrush.hpp"
#include "core/math/EulerRotation.hpp"
#include "core/math/Transform3.hpp"
#include "render/debug/DebugHudText.hpp"

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
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar).objectKind =
          target.objectKind;
      syncCreativeEditorQuickEdit(editor);
    }
    return;
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
    case cr::CreativeHeldItemWorldOperation::Count:
      return;
  }
}

struct HeldItemCommandResult {
  bool handled = false;
  bool changed = false;
};

HeldItemCommandResult confirmArrayCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorArrayWithHistory(
                    appState, editor.pattern, editor.toolSettings,
                    editor.placeCellSize,
                    editor.interaction.target.grid.valid,
                    editor.interaction.target.grid.placementAnchor, source)};
}

HeldItemCommandResult confirmConnectedFillCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorConnectedFillWithHistory(
                    appState, editor, CreativeConnectedFillEditKind::Paint,
                    source)
                    .accepted};
}

HeldItemCommandResult confirmSurfaceExtrudeCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorSurfaceExtrudeWithHistory(
                    appState, editor,
                    cr::CreativeSurfaceExtrudeKind::Extrude, source)
                    .accepted};
}

HeldItemCommandResult confirmTerrainControlCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorTerrainEditWithHistory(
                    appState, editor,
                    CreativeEditorTerrainEditKind::Upsert, source)
                    .accepted};
}

HeldItemCommandResult confirmTerrainGradeCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorTerrainGradeWithHistory(
                    appState, editor, source)
                    .accepted};
}

HeldItemCommandResult confirmTerrainSculptCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorTerrainSculptWithHistory(
                    appState, editor, source)
                    .accepted};
}

HeldItemCommandResult confirmTerrainProfileCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorTerrainProfileWithHistory(
                    appState, editor, source)
                    .accepted};
}

HeldItemCommandResult confirmTerrainPathCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  return {true, applyCreativeEditorTerrainPathWithHistory(
                    appState, editor, source)
                    .accepted};
}

HeldItemCommandResult confirmTerrainRegionCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  if (editor.terrain.region.stamp.active) {
    const CreativeEditorTerrainStampReceipt receipt =
        applyCreativeEditorTerrainStampWithHistory(appState, editor, source);
    return {true, receipt.accepted};
  }
  return {true, applyCreativeEditorTerrainRegionWithHistory(
                    appState, editor, source)
                    .accepted};
}

HeldItemCommandResult confirmVolumeCommand(
    cr::CreativeHeldItemKind kind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  const cr::CreativeVolumeOperationReceipt receipt =
      applyCreativeEditorVolumeOperationWithHistory(
          appState, editor.volume, editor.placeBrush,
          cr::creativeVolumeOperationForHeldItem(kind), editor.toolSettings,
          source);
  return {true, receipt.accepted};
}

HeldItemCommandResult confirmGroupCommand(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState& appState,
    CreativeEditorState&,
    std::string_view source) {
  const cr::CreativeGroupCommandReceipt receipt =
      applyCreativeEditorGroupCommandWithHistory(appState, source);
  return {true, receipt.accepted && receipt.changed};
}

HeldItemCommandResult cancelTerrainControlCommand(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor) {
  if (!editor.terrain.selectionValid) {
    return {};
  }
  return {true, applyCreativeEditorTerrainEditWithHistory(
                    appState, editor, CreativeEditorTerrainEditKind::Remove,
                    "creative_terrain_cancel_active_tool")
                    .accepted};
}

HeldItemCommandResult cancelTerrainGradeCommand(
    cr::CreativeAppState&,
    CreativeEditorState& editor) {
  return {true, cancelCreativeEditorTerrainGrade(editor).accepted};
}

HeldItemCommandResult cancelTerrainSculptCommand(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor) {
  finalizeCreativeTerrainSculptStroke(
      appState, editor, "creative_terrain_sculpt_cancel_active_tool");
  return {true, cancelCreativeEditorTerrainSculpt(editor).accepted};
}

HeldItemCommandResult cancelTerrainProfileCommand(
    cr::CreativeAppState&,
    CreativeEditorState& editor) {
  return {true, unlockCreativeEditorTerrainProfileBase(editor).accepted};
}

HeldItemCommandResult cancelTerrainPathCommand(
    cr::CreativeAppState&,
    CreativeEditorState& editor) {
  return {true, removeCreativeEditorTerrainPathPoint(editor).accepted};
}

HeldItemCommandResult cancelTerrainRegionCommand(
    cr::CreativeAppState&,
    CreativeEditorState& editor) {
  if (editor.terrain.region.stamp.active) {
    return {true, cancelCreativeEditorTerrainStamp(editor).changed};
  }
  return {true, cancelCreativeEditorTerrainRegion(editor).changed};
}

HeldItemCommandResult dispatchHeldItemCommand(
    cr::CreativeHeldItemCommandOperation operation,
    cr::CreativeHeldItemKind kind,
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  switch (operation) {
    case cr::CreativeHeldItemCommandOperation::None:
      return {};
    case cr::CreativeHeldItemCommandOperation::ConfirmArray:
      return confirmArrayCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmConnectedFill:
      return confirmConnectedFillCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmSurfaceExtrude:
      return confirmSurfaceExtrudeCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmTerrainControl:
      return confirmTerrainControlCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmTerrainGrade:
      return confirmTerrainGradeCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmTerrainSculpt:
      return confirmTerrainSculptCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmTerrainProfile:
      return confirmTerrainProfileCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmTerrainPath:
      return confirmTerrainPathCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmTerrainRegion:
      return confirmTerrainRegionCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmVolume:
      return confirmVolumeCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::ConfirmGroup:
      return confirmGroupCommand(kind, appState, editor, source);
    case cr::CreativeHeldItemCommandOperation::CancelTerrainControl:
      return cancelTerrainControlCommand(appState, editor);
    case cr::CreativeHeldItemCommandOperation::CancelTerrainGrade:
      return cancelTerrainGradeCommand(appState, editor);
    case cr::CreativeHeldItemCommandOperation::CancelTerrainSculpt:
      return cancelTerrainSculptCommand(appState, editor);
    case cr::CreativeHeldItemCommandOperation::CancelTerrainProfile:
      return cancelTerrainProfileCommand(appState, editor);
    case cr::CreativeHeldItemCommandOperation::CancelTerrainPath:
      return cancelTerrainPathCommand(appState, editor);
    case cr::CreativeHeldItemCommandOperation::CancelTerrainRegion:
      return cancelTerrainRegionCommand(appState, editor);
    case cr::CreativeHeldItemCommandOperation::Count:
      return {};
  }
  return {};
}

void processMoveInteraction(
    const CreativeEditorWorldInteractionFrameRequest& request) {
  CreativeEditorState& editor = request.editor;
  const bool rejectPressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Reject);
  if (rejectPressed) {
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

void refreshHeldItemPreview(
    const CreativeEditorWorldInteractionFrameRequest& request,
    cr::CreativeHeldItemKind kind);

[[nodiscard]] bool processExclusiveHeldItemFrame(
    const CreativeEditorWorldInteractionFrameRequest& request,
    const cr::CreativeHotbarEntry& held) {
  CreativeEditorState& editor = request.editor;
  const cr::CreativeHeldItemDefinition& definition =
      cr::describeCreativeHeldItem(held.kind);
  if (definition.frameMode != cr::CreativeHeldItemFrameMode::TerrainPaint) {
    finalizeCreativeEditorTerrainPaintStroke(
        request.appState, editor, "creative_terrain_paint_non_paint_tool");
  }
  switch (definition.frameMode) {
    case cr::CreativeHeldItemFrameMode::MaterialStroke: {
      finalizeCreativeTerrainStroke(request.appState, editor,
                                    "creative_terrain_stroke_material_tool");
      InteractionContext context{request, held};
      processCreativeMaterialStrokeFrame(
          request.appState, editor, request.actions,
          request.monotonicTimeNanoseconds);
      if (cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        sampleTargetMaterial(context);
      }
      return true;
    }
    case cr::CreativeHeldItemFrameMode::TerrainControlStroke: {
      finalizeCreativeMaterialStroke(
          request.appState, editor,
          "creative_material_stroke_non_material_tool");
      finalizeCreativeTerrainSculptStroke(
          request.appState, editor, "creative_terrain_sculpt_non_sculpt_tool");
      InteractionContext context{request, held};
      processCreativeTerrainStrokeFrame(
          request.appState, editor, request.actions,
          request.monotonicTimeNanoseconds);
      if (!editor.terrain.stroke.repeat.active &&
          cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        sampleTerrainControl(context);
      }
      return true;
    }
    case cr::CreativeHeldItemFrameMode::TerrainPaint:
      finalizeCreativeMaterialStroke(
          request.appState, editor,
          "creative_material_stroke_terrain_paint_tool");
      finalizeCreativeTerrainStroke(
          request.appState, editor,
          "creative_terrain_stroke_terrain_paint_tool");
      finalizeCreativeTerrainSculptStroke(
          request.appState, editor,
          "creative_terrain_sculpt_terrain_paint_tool");
      processCreativeEditorTerrainPaintFrame(
          request.appState, editor, request.actions,
          request.monotonicTimeNanoseconds);
      if (!editor.terrainPaint.repeat.active &&
          cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        static_cast<void>(sampleCreativeEditorTerrainMaterial(
            request.appState.facade.document(), editor));
      }
      return true;
    case cr::CreativeHeldItemFrameMode::TerrainSculpt:
      finalizeCreativeMaterialStroke(
          request.appState, editor,
          "creative_material_stroke_non_material_tool");
      finalizeCreativeTerrainStroke(
          request.appState, editor,
          "creative_terrain_stroke_non_terrain_tool");
      processCreativeTerrainSculptStrokeFrame(
          request.appState, editor, request.actions,
          request.monotonicTimeNanoseconds);
      if (!editor.terrain.sculpt.stroke.repeat.active &&
          cr::creativeTerrainSculptUsesTargetHeight(
              editor.toolSettings.terrainSculptMode) &&
          cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        static_cast<void>(sampleCreativeEditorTerrainSculptHeight(
            request.appState.facade.document(), editor));
      }
      static_cast<void>(refreshCreativeEditorTerrainSculptPreview(
          editor.terrain, request.appState.facade.document(), editor));
      return true;
    case cr::CreativeHeldItemFrameMode::ObjectMove: {
      finalizeCreativeMaterialStroke(
          request.appState, editor,
          "creative_material_stroke_non_material_tool");
      finalizeCreativeTerrainStroke(
          request.appState, editor,
          "creative_terrain_stroke_non_terrain_tool");
      finalizeCreativeTerrainSculptStroke(
          request.appState, editor,
          "creative_terrain_sculpt_non_sculpt_tool");
      processMoveInteraction(request);
      if (cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        InteractionContext context{request, held};
        sampleTargetMaterial(context);
      }
      return true;
    }
    case cr::CreativeHeldItemFrameMode::Standard:
    case cr::CreativeHeldItemFrameMode::Count:
      break;
  }
  finalizeCreativeMaterialStroke(
      request.appState, editor, "creative_material_stroke_non_material_tool");
  finalizeCreativeTerrainStroke(
      request.appState, editor, "creative_terrain_stroke_non_terrain_tool");
  finalizeCreativeTerrainSculptStroke(
      request.appState, editor, "creative_terrain_sculpt_non_sculpt_tool");
  refreshHeldItemPreview(request, held.kind);
  return false;
}

void refreshHeldItemPreview(
    const CreativeEditorWorldInteractionFrameRequest& request,
    cr::CreativeHeldItemKind kind) {
  switch (cr::describeCreativeHeldItem(kind).previewMode) {
    case cr::CreativeHeldItemPreviewMode::TerrainProfile:
      static_cast<void>(refreshCreativeEditorTerrainProfilePreview(
          request.editor.terrain, request.appState.facade.document(),
          request.editor));
      return;
    case cr::CreativeHeldItemPreviewMode::TerrainPath:
      static_cast<void>(refreshCreativeEditorTerrainPathPreview(
          request.editor.terrain, request.appState.facade.document(),
          request.editor));
      return;
    case cr::CreativeHeldItemPreviewMode::TerrainRegion:
      if (request.editor.terrain.region.stamp.active) {
        static_cast<void>(refreshCreativeEditorTerrainStampPreview(
            request.editor.terrain, request.appState.facade.document(),
            request.editor, request.appState.terrainStamp));
      } else {
        static_cast<void>(refreshCreativeEditorTerrainRegionPreview(
            request.editor.terrain, request.appState.facade.document(),
            request.editor));
      }
      return;
    case cr::CreativeHeldItemPreviewMode::None:
    case cr::CreativeHeldItemPreviewMode::Count:
      return;
  }
}

}  // namespace

bool confirmCreativeEditorHeldItem(cr::CreativeAppState& appState,
                                   CreativeEditorState& editor,
                                   std::string_view source) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const cr::CreativeHeldItemDefinition& definition =
      cr::describeCreativeHeldItem(held.kind);
  return dispatchHeldItemCommand(definition.confirmCommand, held.kind,
                                 appState, editor, source)
      .changed;
}

bool cancelCreativeEditorHeldItem(cr::CreativeAppState& appState,
                                  CreativeEditorState& editor) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const cr::CreativeHeldItemDefinition& definition =
      cr::describeCreativeHeldItem(held.kind);
  const HeldItemCommandResult command = dispatchHeldItemCommand(
      definition.cancelCommand, held.kind, appState, editor, {});
  if (command.handled) {
    return command.changed;
  }
  if (editor.volume.active &&
      editor.volume.selection.phase != cr::CreativeVolumeSelectionPhase::Empty) {
    cr::clearCreativeVolumeSelection(editor.volume.selection);
    editor.volume.lastReceipt = {};
    return true;
  }
  cr::CreativeToolInputPacket cancel;
  cancel.kind = cr::CreativeToolInputKind::Cancel;
  const cr::CreativeFacadeToolDispatchReceipt receipt =
      appState.facade.dispatchToolInput(cancel);
  editor.interaction.moveTargetId = cr::kInvalidObjectId;
  return receipt.changed;
}

void processCreativeEditorHeldItemFrame(
    const CreativeEditorWorldInteractionFrameRequest& request) {
  CreativeEditorState& editor = request.editor;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (processExclusiveHeldItemFrame(request, held)) {
    return;
  }

  InteractionContext context{request, held};
  const cr::CreativeHeldItemDefinition& definition =
      cr::describeCreativeHeldItem(held.kind);
  constexpr std::array actions{
      cr::CreativeWorldActionId::Primary,
      cr::CreativeWorldActionId::Secondary,
      cr::CreativeWorldActionId::Pick,
  };
  const bool rejectPressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Reject);
  const bool acceptPressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Accept);
  if (rejectPressed) {
    dispatchHeldItemWorldOperation(definition.rejectOperation, context);
  } else if (acceptPressed) {
    dispatchHeldItemWorldOperation(definition.acceptOperation, context);
  }
  const bool primaryPressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Primary);
  for (std::size_t index = 0; index < actions.size(); ++index) {
    if (definition.primaryWinsSimultaneous && index == 1U && primaryPressed) {
      continue;
    }
    if (cr::creativeWorldActionPressed(request.actions, actions[index])) {
      dispatchHeldItemWorldOperation(definition.worldOperations[index],
                                     context);
    }
  }
}

}  // namespace iggy3d_creative_app
