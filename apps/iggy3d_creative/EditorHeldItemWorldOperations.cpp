#include "EditorInteraction.hpp"
#include "EditorInteractionInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <span>
#include <vector>

#include "EditorFrame.hpp"
#include "EditorConnectedFill.hpp"
#include "EditorGizmo.hpp"
#include "EditorGroup.hpp"
#include "EditorLogicLinks.hpp"
#include "EditorMeasurement.hpp"
#include "EditorPathEditing.hpp"
#include "EditorPattern.hpp"
#include "EditorRoomPlacement.hpp"
#include "EditorState.hpp"
#include "EditorStructuralPlacement.hpp"
#include "EditorSurfaceExtrude.hpp"
#include "EditorTerrain.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"
#include "EditorWorldLayout.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

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

[[nodiscard]] cr::CreativeSelectionPlacementAxis placementAxisForGizmo(
    GizmoAxis axis) noexcept {
  switch (axis) {
    case GizmoAxis::X: return cr::CreativeSelectionPlacementAxis::X;
    case GizmoAxis::Y: return cr::CreativeSelectionPlacementAxis::Y;
    case GizmoAxis::Z: return cr::CreativeSelectionPlacementAxis::Z;
    case GizmoAxis::None: return cr::CreativeSelectionPlacementAxis::Count;
  }
  return cr::CreativeSelectionPlacementAxis::Count;
}

[[nodiscard]] bool beginTransformFromGizmoAxis(
    const CreativeEditorWorldInteractionFrameRequest& request,
    float targetX,
    float targetY) {
  if (request.gizmoFrame == nullptr ||
      !request.editor.interaction.target.ray.valid) {
    return false;
  }
  const GizmoAxisPickResult picked = pickCreativeEditorGizmoAxisAtPixel(
      request.gizmoFrame->axisHandles, targetX, targetY);
  if (!picked.hit) {
    return false;
  }

  CreativeEditorSelectionTransformState& transform = request.editor.transform;
  if (!beginCreativeEditorSelectionTransformPreview(
          request.appState, transform, "gizmo_axis_transform_begin",
          CreativeEditorTransformAnchorPolicy::FixedSource,
          &request.editor.worldLayout)) {
    return true;
  }
  const cr::CreativeSelectionPlacementAxis axis =
      placementAxisForGizmo(picked.axis);
  const auto shaft = std::find_if(
      request.gizmoFrame->shafts.begin(), request.gizmoFrame->shafts.end(),
      [picked](const GizmoAxisShaft& candidate) {
        return candidate.axis == picked.axis;
      });
  const cr::CreativeVec3 rayOrigin =
      cr::creativeVec3FromCore(request.editor.interaction.target.ray.origin);
  const cr::CreativeVec3 rayDirection =
      cr::creativeVec3FromCore(request.editor.interaction.target.ray.direction);
  const cr::CreativeVec3 axisOrigin =
      cr::creativeVec3FromCore(request.gizmoFrame->center);
  const cr::CreativeVec3 axisDirection =
      shaft != request.gizmoFrame->shafts.end()
          ? cr::creativeVec3FromCore(shaft->tip - request.gizmoFrame->center)
          : cr::CreativeVec3{};
  if (!beginCreativeEditorAxisTransformPointerGesture(
          request.appState, transform, axis, axisOrigin, axisDirection,
          rayOrigin, rayDirection)) {
    static_cast<void>(cancelCreativeEditorSelectionTransformPreview(
        transform, "gizmo_axis_transform_invalid"));
  }
  return true;
}

[[nodiscard]] bool selectionContainsObject(
    const cr::CreativeSelectionState& selection,
    cr::CreativeObjectId objectId) noexcept {
  if (selection.selectedTarget.value == static_cast<cr::Id>(objectId)) {
    return true;
  }
  const std::span<const cr::TargetRef> targets =
      cr::selectedTargetList(selection);
  return std::any_of(
      targets.begin(), targets.end(),
      [objectId](cr::TargetRef target) {
        return target.value == static_cast<cr::Id>(objectId);
      });
}

[[nodiscard]] bool beginFreeTransformFromTarget(
    const CreativeEditorWorldInteractionFrameRequest& request) {
  CreativeEditorState& editor = request.editor;
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  if (!target.objectHit || !target.grid.valid) {
    return false;
  }

  const cr::CreativeSelectionState& selection =
      request.appState.facade.selectionState();
  std::vector<cr::CreativeObjectId> selectedObjectIds;
  const std::span<const cr::TargetRef> selectedTargets =
      cr::selectedTargetList(selection);
  selectedObjectIds.reserve(selectedTargets.size());
  for (cr::TargetRef selected : selectedTargets) {
    if (selected.value != cr::kInvalidId) {
      selectedObjectIds.push_back(
          static_cast<cr::CreativeObjectId>(selected.value));
    }
  }
  const cr::CreativeObjectId interactionRoot =
      cr::resolveCreativeHierarchyInteractionRoot(
          request.appState.facade.document(), selectedObjectIds,
          target.objectId);
  if (interactionRoot == cr::kInvalidObjectId) {
    return false;
  }
  if (!selectionContainsObject(selection, interactionRoot)) {
    const std::array selected{interactionRoot};
    if (!request.appState.facade
             .selectTargets(selected, interactionRoot)
             .accepted) {
      return false;
    }
  }
  if (!beginCreativeEditorSelectionTransformPreview(
          request.appState, editor.transform, "pointer_transform_begin",
          CreativeEditorTransformAnchorPolicy::FixedSource,
          &editor.worldLayout)) {
    return false;
  }
  if (!beginCreativeEditorFreeTransformPointerGesture(
          request.appState, editor.transform,
          target.grid.placementAnchor)) {
    static_cast<void>(cancelCreativeEditorSelectionTransformPreview(
        editor.transform, "pointer_transform_invalid"));
    return false;
  }
  return true;
}

void selectObject(InteractionContext& context) {
  const CreativeEditorWorldTarget& aimedTarget =
      context.request.editor.interaction.target;
  const cr::CreativeToolModifierFlags modifiers =
      toolModifiers(context.request.modifiers);
  CreativeEditorWorldTarget target = aimedTarget;
  cr::CreativeWorldLayoutSourceRef preferredSource;
  if (target.objectHit &&
      (modifiers & cr::kCreativeToolModifierShift) == 0U &&
      target.objectPickStack.count > 0U) {
    const cr::CreativeObjectId currentObjectId =
        context.request.appState.facade.selectionState().selectedTarget.value ==
                cr::kInvalidId
            ? cr::kInvalidObjectId
            : static_cast<cr::CreativeObjectId>(
                  context.request.appState.facade.selectionState()
                      .selectedTarget.value);
    const cr::CreativeObjectId cycled =
        cycleObjectVisualPick(target.objectPickStack, currentObjectId);
    if (cycled != cr::kInvalidObjectId) {
      target.objectId = cycled;
      for (const ObjectVisualPickHit& hit : target.objectPickStack.hits()) {
        if (hit.objectId == cycled) {
          target.distanceMeters = hit.entryDistance;
          break;
        }
      }
    }
  }
  if (target.objectHit) {
    const cr::CreativeDocument& document =
        context.request.appState.facade.document();
    CreativeEditorWorldLayoutState& worldLayout =
        context.request.editor.worldLayout;
    cr::CreativeSemanticSelectionResolution resolved;
    const cr::CreativeGridSettings grid = document.gridSettings();
    if (std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0) {
      cr::CreativeVec3 worldPoint = target.grid.hitPoint;
      if (target.ray.valid && std::isfinite(target.distanceMeters)) {
        worldPoint = cr::creativeVec3FromCore(
            target.ray.origin +
            target.ray.direction * target.distanceMeters);
      }
      const cr::CreativeVec3 sourcePointCells{
          (worldPoint.x - grid.origin.x) / grid.cellSizeMeters,
          (worldPoint.y - grid.origin.y) / grid.cellSizeMeters,
          (worldPoint.z - grid.origin.z) / grid.cellSizeMeters,
      };
      resolved = cr::resolveCreativeSemanticSelection(
          document, target.objectId, worldLayout.source, sourcePointCells);
    } else {
      resolved = cr::resolveCreativeSemanticSelection(
          document, target.objectId, &worldLayout.source);
    }
    if (worldLayout.generatedRevision == worldLayout.revision &&
        resolved.worldLayoutSource.owned) {
      preferredSource = {resolved.worldLayoutSource.table,
                         resolved.worldLayoutSource.index};
    }
  }
  static_cast<void>(
      context.request.appState.facade.setActiveTool(cr::Tool::Select));
  const cr::CreativeFacadeToolDispatchReceipt selection =
      context.request.appState.facade.dispatchToolInput(
          selectionPacket(target, modifiers));
  if (selection.accepted) {
    static_cast<void>(synchronizeCreativeEditorWorldLayoutSelection(
        context.request.editor.worldLayout,
        context.request.appState.facade.document(),
        context.request.appState.facade.selectionState(), preferredSource));
  }
}

void clearObjectSelection(InteractionContext& context) {
  const cr::CreativeSelectionReceipt cleared =
      context.request.appState.facade.selectTargets(
          std::span<const cr::CreativeObjectId>{}, cr::kInvalidObjectId);
  if (cleared.accepted) {
    static_cast<void>(synchronizeCreativeEditorWorldLayoutSelection(
        context.request.editor.worldLayout,
        context.request.appState.facade.document(),
        context.request.appState.facade.selectionState()));
  }
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

void completeHeldShapeVolume(InteractionContext& context) {
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
  clearCreativeEditorPlacementFeedback(editor.interaction);
}

void applyHeldShapeVolume(InteractionContext& context) {
  CreativeEditorState& editor = context.request.editor;
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
  switch (context.request.editor.volume.selection.phase) {
    case cr::CreativeVolumeSelectionPhase::Empty:
      beginHeldShapeVolume(context);
      return;
    case cr::CreativeVolumeSelectionPhase::FirstCorner:
      completeHeldShapeVolume(context);
      return;
    case cr::CreativeVolumeSelectionPhase::Complete:
      if (context.request.editor.volume.lastReceipt.requested &&
          context.request.editor.volume.lastReceipt.accepted) {
        beginHeldShapeVolume(context);
        return;
      }
      applyHeldShapeVolume(context);
      return;
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
  const CreativeEditorTerrainRegionReceipt selected =
      selectCreativeEditorTerrainRegionOperationAtPointer(
          context.request.appState.facade.document(), context.request.editor);
  if (selected.accepted) {
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

void advanceBuildingRoom(InteractionContext& context) {
  static_cast<void>(advanceCreativeEditorRoomPlacement(
      context.request.appState, context.request.editor,
      "creative_viewport_room_apply"));
}

void cancelBuildingRoom(InteractionContext& context) {
  static_cast<void>(
      cancelCreativeEditorRoomPlacement(context.request.editor));
}

void appendMeasurementPoint(InteractionContext& context) {
  static_cast<void>(
      appendCreativeEditorMeasurementPoint(context.request));
}

void completeMeasurement(InteractionContext& context) {
  static_cast<void>(completeCreativeEditorMeasurement(context.request));
}

void cancelMeasurement(InteractionContext& context) {
  static_cast<void>(cancelCreativeEditorMeasurement(context.request));
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
    case cr::CreativeHeldItemWorldOperation::ClearSelection:
      clearObjectSelection(context);
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
      advanceHeldShapeVolume(context);
      return;
    case cr::CreativeHeldItemWorldOperation::CommitShapeVolume:
      advanceHeldShapeVolume(context);
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
    case cr::CreativeHeldItemWorldOperation::AdvanceBuildingRoom:
      advanceBuildingRoom(context);
      return;
    case cr::CreativeHeldItemWorldOperation::CancelBuildingRoom:
      cancelBuildingRoom(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AppendMeasurementPoint:
      appendMeasurementPoint(context);
      return;
    case cr::CreativeHeldItemWorldOperation::CompleteMeasurement:
      completeMeasurement(context);
      return;
    case cr::CreativeHeldItemWorldOperation::CancelMeasurement:
      cancelMeasurement(context);
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
          "minecraft_secondary_transform_begin",
          CreativeEditorTransformAnchorPolicy::FollowAim,
          &editor.worldLayout)) {
    static_cast<void>(processCreativeEditorSelectionTransformPreview(
        request.appState, editor.transform,
        editor.interaction.target.grid.valid,
        editor.interaction.target.grid.placementAnchor, false,
        "minecraft_secondary_transform_begin",
        cr::creativeSnapIncrementMeters(editor.toolSettings.snapIncrement),
        &editor.worldLayout, request.placementClearanceCache));
    return;
  }

  if (pressed) {
    if (beginTransformFromGizmoAxis(request, targetX, targetY)) {
      return;
    }
    static_cast<void>(beginFreeTransformFromTarget(request));
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
