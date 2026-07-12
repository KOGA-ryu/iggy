#include "EditorInteraction.hpp"

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
#include "app/iggy3d/creative/tools/ShapeBrush.hpp"
#include "core/math/EulerRotation.hpp"
#include "core/math/Transform3.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr float kCreativeReachMeters = 128.0F;

struct HeldItemBehavior {
  cr::CreativeHeldItemKind kind = cr::CreativeHeldItemKind::Count;
  cr::Tool facadeTool = cr::Tool::Select;
  bool placeMode = false;
  bool volumeMode = false;
  cr::CreativeVolumeOperationKind volumeOperation =
      cr::CreativeVolumeOperationKind::Fill;
};

template <typename Row, std::size_t Size>
consteval bool heldItemRowsMatchEnumOrder(
    const std::array<Row, Size>& rows) {
  if (Size != cr::kCreativeHeldItemKindCount) {
    return false;
  }
  for (std::size_t index = 0; index < rows.size(); ++index) {
    if (static_cast<std::size_t>(rows[index].kind) != index) {
      return false;
    }
  }
  return true;
}

constexpr std::array kHeldItemBehaviors{
    HeldItemBehavior{cr::CreativeHeldItemKind::Material, cr::Tool::Select, true,
                     false},
    HeldItemBehavior{cr::CreativeHeldItemKind::MaterialBrush, cr::Tool::Select,
                     false, false},
    HeldItemBehavior{cr::CreativeHeldItemKind::ObjectSelect, cr::Tool::Select,
                     false, false},
    HeldItemBehavior{cr::CreativeHeldItemKind::ObjectMove, cr::Tool::Move, false,
                     false},
    HeldItemBehavior{cr::CreativeHeldItemKind::VolumeSelect, cr::Tool::Select,
                     false, true},
    HeldItemBehavior{cr::CreativeHeldItemKind::VolumeFill, cr::Tool::Select,
                     false, true,
                     cr::CreativeVolumeOperationKind::Fill},
    HeldItemBehavior{cr::CreativeHeldItemKind::VolumeHollow, cr::Tool::Select,
                     false, true,
                     cr::CreativeVolumeOperationKind::Hollow},
    HeldItemBehavior{cr::CreativeHeldItemKind::VolumeReplace, cr::Tool::Select,
                     false, true,
                     cr::CreativeVolumeOperationKind::Replace},
    HeldItemBehavior{cr::CreativeHeldItemKind::VolumeErase, cr::Tool::Select,
                     false, true,
                     cr::CreativeVolumeOperationKind::Erase},
    HeldItemBehavior{cr::CreativeHeldItemKind::VolumeClone, cr::Tool::Select,
                     false, true,
                     cr::CreativeVolumeOperationKind::Clone},
    HeldItemBehavior{cr::CreativeHeldItemKind::LinearArray, cr::Tool::Select,
                     false, false},
    HeldItemBehavior{cr::CreativeHeldItemKind::ConnectedFill,
                     cr::Tool::Select, false, false},
    HeldItemBehavior{cr::CreativeHeldItemKind::SurfaceExtrude,
                     cr::Tool::Select, false, false},
    HeldItemBehavior{cr::CreativeHeldItemKind::TerrainControl,
                     cr::Tool::Select, false, false},
    HeldItemBehavior{cr::CreativeHeldItemKind::TerrainPaint,
                     cr::Tool::Select, false, false},
    HeldItemBehavior{cr::CreativeHeldItemKind::TerrainGrade,
                     cr::Tool::Select, false, false},
    HeldItemBehavior{cr::CreativeHeldItemKind::TerrainSculpt,
                     cr::Tool::Select, false, false},
    HeldItemBehavior{cr::CreativeHeldItemKind::TerrainProfile,
                     cr::Tool::Select, false, false},
    HeldItemBehavior{cr::CreativeHeldItemKind::TerrainPath,
                     cr::Tool::Select, false, false},
    HeldItemBehavior{cr::CreativeHeldItemKind::TerrainRegion,
                     cr::Tool::Select, false, true},
    HeldItemBehavior{cr::CreativeHeldItemKind::ObjectGroup,
                     cr::Tool::Select, false, false},
};
static_assert(heldItemRowsMatchEnumOrder(kHeldItemBehaviors));

[[nodiscard]] iggy3d::Vec3 aabbFaceNormal(VisualBounds bounds,
                                          iggy3d::Vec3 point) noexcept {
  const std::array distances{
      std::fabs(point.x - bounds.min.x), std::fabs(point.x - bounds.max.x),
      std::fabs(point.y - bounds.min.y), std::fabs(point.y - bounds.max.y),
      std::fabs(point.z - bounds.min.z), std::fabs(point.z - bounds.max.z),
  };
  const std::size_t face = static_cast<std::size_t>(
      std::distance(distances.begin(),
                    std::min_element(distances.begin(), distances.end())));
  constexpr std::array normals{
      iggy3d::Vec3{-1.0F, 0.0F, 0.0F}, iggy3d::Vec3{1.0F, 0.0F, 0.0F},
      iggy3d::Vec3{0.0F, -1.0F, 0.0F}, iggy3d::Vec3{0.0F, 1.0F, 0.0F},
      iggy3d::Vec3{0.0F, 0.0F, -1.0F}, iggy3d::Vec3{0.0F, 0.0F, 1.0F},
  };
  return normals[face];
}

[[nodiscard]] iggy3d::Vec3 orientedFaceNormal(
    const iggy3d::OrientedBox& box,
    iggy3d::Vec3 point) noexcept {
  const iggy3d::Vec3 localCenter = iggy3d::center(box.localBounds);
  constexpr std::array localNormals{
      iggy3d::Vec3{-1.0F, 0.0F, 0.0F}, iggy3d::Vec3{1.0F, 0.0F, 0.0F},
      iggy3d::Vec3{0.0F, -1.0F, 0.0F}, iggy3d::Vec3{0.0F, 1.0F, 0.0F},
      iggy3d::Vec3{0.0F, 0.0F, -1.0F}, iggy3d::Vec3{0.0F, 0.0F, 1.0F},
  };
  std::array<float, localNormals.size()> distances{};
  std::array<iggy3d::Vec3, localNormals.size()> worldNormals{};
  for (std::size_t face = 0; face < localNormals.size(); ++face) {
    iggy3d::Vec3 faceCenter = localCenter;
    if (localNormals[face].x < 0.0F) faceCenter.x = box.localBounds.min.x;
    if (localNormals[face].x > 0.0F) faceCenter.x = box.localBounds.max.x;
    if (localNormals[face].y < 0.0F) faceCenter.y = box.localBounds.min.y;
    if (localNormals[face].y > 0.0F) faceCenter.y = box.localBounds.max.y;
    if (localNormals[face].z < 0.0F) faceCenter.z = box.localBounds.min.z;
    if (localNormals[face].z > 0.0F) faceCenter.z = box.localBounds.max.z;
    const iggy3d::Vec3 worldCenter =
        iggy3d::transformPointTrs(box.transform, faceCenter);
    worldNormals[face] = iggy3d::normalizedOr(
        iggy3d::rotateEulerXyz(localNormals[face],
                              box.transform.rotationEulerRadians),
        localNormals[face]);
    distances[face] =
        std::fabs(iggy3d::dot(point - worldCenter, worldNormals[face]));
  }
  const std::size_t face = static_cast<std::size_t>(
      std::distance(distances.begin(),
                    std::min_element(distances.begin(), distances.end())));
  return worldNormals[face];
}

[[nodiscard]] const ObjectVisualPickBounds* findCandidate(
    const CreativeEditorPickFrame& pickFrame,
    cr::CreativeObjectId objectId) noexcept {
  const auto found = std::find_if(
      pickFrame.objectPickCandidates.begin(),
      pickFrame.objectPickCandidates.end(),
      [objectId](const ObjectVisualPickBounds& candidate) {
        return candidate.id == objectId;
      });
  return found == pickFrame.objectPickCandidates.end() ? nullptr : &*found;
}

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

using InteractionHandler = void (*)(InteractionContext&);
using HeldItemActionHandlers = std::array<InteractionHandler, 3>;

void noInteraction(InteractionContext&);

struct HeldItemHandlerRow {
  cr::CreativeHeldItemKind kind = cr::CreativeHeldItemKind::Count;
  HeldItemActionHandlers handlers{};
  InteractionHandler accept = noInteraction;
  InteractionHandler reject = noInteraction;
  bool primaryWinsSimultaneous = false;
};

void noInteraction(InteractionContext&) {}

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

constexpr std::array<HeldItemHandlerRow, cr::kCreativeHeldItemKindCount>
    kHeldItemHandlers{{
        {cr::CreativeHeldItemKind::Material,
         {noInteraction, noInteraction, sampleTargetMaterial},
         noInteraction, noInteraction},
        {cr::CreativeHeldItemKind::MaterialBrush,
         {noInteraction, noInteraction, sampleTargetMaterial},
         noInteraction, noInteraction},
        {cr::CreativeHeldItemKind::ObjectSelect,
         {selectObject, noInteraction, sampleTargetMaterial},
         selectObject, noInteraction},
        {cr::CreativeHeldItemKind::ObjectMove,
         {noInteraction, noInteraction, sampleTargetMaterial},
         noInteraction, rejectActiveInteraction},
        {cr::CreativeHeldItemKind::VolumeSelect,
         {setVolumeFirstCorner, setVolumeSecondCorner,
          expandVolumeSelection},
         advanceVolumeSelection, rejectActiveInteraction},
        {cr::CreativeHeldItemKind::VolumeFill,
         {beginHeldShapeVolume, commitHeldShapeVolume, sampleTargetMaterial},
         advanceHeldShapeVolume, rejectActiveInteraction,
         true},
        {cr::CreativeHeldItemKind::VolumeHollow,
         {beginHeldShapeVolume, commitHeldShapeVolume, sampleTargetMaterial},
         advanceHeldShapeVolume, rejectActiveInteraction,
         true},
        {cr::CreativeHeldItemKind::VolumeReplace,
         {noInteraction, applyHeldVolumeOperation, sampleTargetMaterial},
         applyHeldVolumeOperation, rejectActiveInteraction},
        {cr::CreativeHeldItemKind::VolumeErase,
         {noInteraction, applyHeldVolumeOperation, sampleTargetMaterial},
         applyHeldVolumeOperation, rejectActiveInteraction},
        {cr::CreativeHeldItemKind::VolumeClone,
         {noInteraction, applyHeldVolumeOperation, sampleTargetMaterial},
         applyHeldVolumeOperation, rejectActiveInteraction},
        {cr::CreativeHeldItemKind::LinearArray,
         {selectObject, applyHeldArray, sampleTargetMaterial},
         acceptHeldArray, rejectActiveInteraction},
        {cr::CreativeHeldItemKind::ConnectedFill,
         {eraseConnectedFill, paintConnectedFill, sampleTargetMaterial},
         paintConnectedFill, eraseConnectedFill, true},
        {cr::CreativeHeldItemKind::SurfaceExtrude,
         {insetSurface, extrudeSurface, sampleTargetMaterial},
         extrudeSurface, insetSurface, true},
        {cr::CreativeHeldItemKind::TerrainControl,
         {removeTerrainControl, upsertTerrainControl, sampleTerrainControl},
         upsertTerrainControl, removeTerrainControl, true},
        {cr::CreativeHeldItemKind::TerrainPaint,
         {noInteraction, noInteraction, noInteraction},
         noInteraction, noInteraction},
        {cr::CreativeHeldItemKind::TerrainGrade,
         {cancelTerrainGrade, applyTerrainGrade, beginTerrainGrade},
         applyTerrainGrade, cancelTerrainGrade, true},
        {cr::CreativeHeldItemKind::TerrainSculpt,
         {noInteraction, noInteraction, noInteraction},
         noInteraction, noInteraction},
        {cr::CreativeHeldItemKind::TerrainProfile,
         {unlockTerrainProfileBase, applyTerrainProfile,
          lockTerrainProfileBase},
         applyTerrainProfile, unlockTerrainProfileBase, true},
        {cr::CreativeHeldItemKind::TerrainPath,
         {removeTerrainPathPoint, applyTerrainPath, addTerrainPathPoint},
         applyTerrainPath, removeTerrainPathPoint, true},
        {cr::CreativeHeldItemKind::TerrainRegion,
         {cancelTerrainRegion, advanceTerrainRegion,
          sampleTerrainRegionHeight},
         advanceTerrainRegion, cancelTerrainRegion, true},
        {cr::CreativeHeldItemKind::ObjectGroup,
         {selectObject, applyObjectGroup, noInteraction},
         applyObjectGroup, noInteraction},
    }};
static_assert(heldItemRowsMatchEnumOrder(kHeldItemHandlers));

struct HeldItemCommandResult {
  bool handled = false;
  bool changed = false;
};

using HeldItemConfirmCommand = HeldItemCommandResult (*)(
    cr::CreativeHeldItemKind,
    cr::CreativeAppState&,
    CreativeEditorState&,
    std::string_view);
using HeldItemCancelCommand = HeldItemCommandResult (*)(
    cr::CreativeAppState&,
    CreativeEditorState&);

struct HeldItemCommandRow {
  cr::CreativeHeldItemKind kind = cr::CreativeHeldItemKind::Count;
  HeldItemConfirmCommand confirm = nullptr;
  HeldItemCancelCommand cancel = nullptr;
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

constexpr std::array<HeldItemCommandRow, cr::kCreativeHeldItemKindCount>
    kHeldItemCommands{{
        {cr::CreativeHeldItemKind::Material},
        {cr::CreativeHeldItemKind::MaterialBrush},
        {cr::CreativeHeldItemKind::ObjectSelect},
        {cr::CreativeHeldItemKind::ObjectMove},
        {cr::CreativeHeldItemKind::VolumeSelect},
        {cr::CreativeHeldItemKind::VolumeFill},
        {cr::CreativeHeldItemKind::VolumeHollow},
        {cr::CreativeHeldItemKind::VolumeReplace, confirmVolumeCommand},
        {cr::CreativeHeldItemKind::VolumeErase, confirmVolumeCommand},
        {cr::CreativeHeldItemKind::VolumeClone, confirmVolumeCommand},
        {cr::CreativeHeldItemKind::LinearArray, confirmArrayCommand},
        {cr::CreativeHeldItemKind::ConnectedFill,
         confirmConnectedFillCommand},
        {cr::CreativeHeldItemKind::SurfaceExtrude,
         confirmSurfaceExtrudeCommand},
        {cr::CreativeHeldItemKind::TerrainControl,
         confirmTerrainControlCommand, cancelTerrainControlCommand},
        {cr::CreativeHeldItemKind::TerrainPaint},
        {cr::CreativeHeldItemKind::TerrainGrade,
         confirmTerrainGradeCommand, cancelTerrainGradeCommand},
        {cr::CreativeHeldItemKind::TerrainSculpt,
         confirmTerrainSculptCommand, cancelTerrainSculptCommand},
        {cr::CreativeHeldItemKind::TerrainProfile,
         confirmTerrainProfileCommand, cancelTerrainProfileCommand},
        {cr::CreativeHeldItemKind::TerrainPath,
         confirmTerrainPathCommand, cancelTerrainPathCommand},
        {cr::CreativeHeldItemKind::TerrainRegion,
         confirmTerrainRegionCommand, cancelTerrainRegionCommand},
        {cr::CreativeHeldItemKind::ObjectGroup, confirmGroupCommand},
    }};
static_assert(heldItemRowsMatchEnumOrder(kHeldItemCommands));

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
  if (held.kind != cr::CreativeHeldItemKind::TerrainPaint) {
    finalizeCreativeEditorTerrainPaintStroke(
        request.appState, editor, "creative_terrain_paint_non_paint_tool");
  }
  switch (held.kind) {
    case cr::CreativeHeldItemKind::Material:
    case cr::CreativeHeldItemKind::MaterialBrush: {
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
    case cr::CreativeHeldItemKind::TerrainControl: {
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
    case cr::CreativeHeldItemKind::TerrainPaint:
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
    case cr::CreativeHeldItemKind::TerrainSculpt:
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
    case cr::CreativeHeldItemKind::ObjectMove: {
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
    case cr::CreativeHeldItemKind::ObjectSelect:
    case cr::CreativeHeldItemKind::VolumeSelect:
    case cr::CreativeHeldItemKind::VolumeFill:
    case cr::CreativeHeldItemKind::VolumeHollow:
    case cr::CreativeHeldItemKind::VolumeReplace:
    case cr::CreativeHeldItemKind::VolumeErase:
    case cr::CreativeHeldItemKind::VolumeClone:
    case cr::CreativeHeldItemKind::LinearArray:
    case cr::CreativeHeldItemKind::ConnectedFill:
    case cr::CreativeHeldItemKind::SurfaceExtrude:
    case cr::CreativeHeldItemKind::TerrainGrade:
    case cr::CreativeHeldItemKind::TerrainProfile:
    case cr::CreativeHeldItemKind::TerrainPath:
    case cr::CreativeHeldItemKind::TerrainRegion:
    case cr::CreativeHeldItemKind::ObjectGroup:
    case cr::CreativeHeldItemKind::Count:
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
  switch (kind) {
    case cr::CreativeHeldItemKind::TerrainProfile:
      static_cast<void>(refreshCreativeEditorTerrainProfilePreview(
          request.editor.terrain, request.appState.facade.document(),
          request.editor));
      return;
    case cr::CreativeHeldItemKind::TerrainPath:
      static_cast<void>(refreshCreativeEditorTerrainPathPreview(
          request.editor.terrain, request.appState.facade.document(),
          request.editor));
      return;
    case cr::CreativeHeldItemKind::TerrainRegion:
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
    case cr::CreativeHeldItemKind::Material:
    case cr::CreativeHeldItemKind::MaterialBrush:
    case cr::CreativeHeldItemKind::ObjectSelect:
    case cr::CreativeHeldItemKind::ObjectMove:
    case cr::CreativeHeldItemKind::VolumeSelect:
    case cr::CreativeHeldItemKind::VolumeFill:
    case cr::CreativeHeldItemKind::VolumeHollow:
    case cr::CreativeHeldItemKind::VolumeReplace:
    case cr::CreativeHeldItemKind::VolumeErase:
    case cr::CreativeHeldItemKind::VolumeClone:
    case cr::CreativeHeldItemKind::LinearArray:
    case cr::CreativeHeldItemKind::ConnectedFill:
    case cr::CreativeHeldItemKind::SurfaceExtrude:
    case cr::CreativeHeldItemKind::TerrainControl:
    case cr::CreativeHeldItemKind::TerrainPaint:
    case cr::CreativeHeldItemKind::TerrainGrade:
    case cr::CreativeHeldItemKind::TerrainSculpt:
    case cr::CreativeHeldItemKind::ObjectGroup:
    case cr::CreativeHeldItemKind::Count:
      return;
  }
}

[[nodiscard]] std::string shapeHotbarLabel(
    cr::CreativeHeldItemKind kind,
    const cr::CreativeToolSettings& settings) {
  std::string output =
      kind == cr::CreativeHeldItemKind::VolumeFill ? "F" : "H";
  switch (settings.shapeBrushKind) {
    case cr::CreativeShapeBrushKind::Box: return output + "B";
    case cr::CreativeShapeBrushKind::Line: return output + "L";
    case cr::CreativeShapeBrushKind::Ellipsoid: return output + "E";
    case cr::CreativeShapeBrushKind::Cylinder:
      output.push_back('C');
      output.append(cr::toString(settings.shapeBrushAxis));
      return output;
    case cr::CreativeShapeBrushKind::Count: return output + "?";
  }
  return output + "?";
}

[[nodiscard]] std::string hotbarLabel(
    const cr::CreativeHotbarEntry& entry,
    const cr::CreativeToolSettings& settings,
    const CreativeMaterialBrushPresetBank& brushPresets,
    std::size_t slot) {
  if (cr::creativeHeldItemUsesDirectShapeGesture(entry.kind)) {
    return shapeHotbarLabel(entry.kind, settings);
  }
  if (entry.kind == cr::CreativeHeldItemKind::MaterialBrush) {
    CreativeMaterialBrushGestureConfig config =
        creativeMaterialBrushGestureConfig(settings);
    static_cast<void>(
        creativeMaterialBrushPresetForSlot(brushPresets, slot, config));
    return creativeMaterialBrushPresetHotbarLabel(config);
  }
  if (entry.kind != cr::CreativeHeldItemKind::Material) {
    return std::string(cr::toString(entry.kind)).substr(0, 4);
  }
  const std::string name(cr::toString(entry.objectKind));
  return name.substr(0, std::min<std::size_t>(4U, name.size()));
}

void appendColoredText(std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
                       std::string_view text,
                       std::int32_t x,
                       std::int32_t y,
                       std::uint32_t width,
                       std::uint32_t height,
                       bool selected) {
  iggy3d::DebugHudLayoutResult layout =
      iggy3d::layoutDebugHudTextAt(text, x, y, width, height);
  for (iggy3d::DebugHudGlyphQuad& quad : layout.quads) {
    quad.r = selected ? 0.08F : 0.88F;
    quad.g = selected ? 0.08F : 0.90F;
    quad.b = selected ? 0.08F : 0.94F;
    quad.a = 1.0F;
  }
  glyphs.insert(glyphs.end(), layout.quads.begin(), layout.quads.end());
}

void appendHeldItemStatusText(
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
    std::string_view text,
    std::int32_t x,
    std::int32_t y,
    std::uint32_t width,
    std::uint32_t height) {
  iggy3d::DebugHudLayoutResult layout =
      iggy3d::layoutDebugHudTextAt(text, x, y, width, height);
  bool quickEditActive = false;
  for (iggy3d::DebugHudGlyphQuad& quad : layout.quads) {
    quickEditActive = quickEditActive || quad.source == '[';
    quad.r = quickEditActive ? 0.24F : 0.88F;
    quad.g = quickEditActive ? 1.0F : 0.90F;
    quad.b = quickEditActive ? 0.34F : 0.94F;
    quad.a = 1.0F;
  }
  glyphs.insert(glyphs.end(), layout.quads.begin(), layout.quads.end());
}

void appendHeldQuickEditStatus(std::string& output,
                               const CreativeEditorState& editor) {
  const std::string quickEdit = creativeEditorQuickEditStatusLabel(editor);
  if (!quickEdit.empty()) {
    output.append(" | [");
    output.append(quickEdit);
    output.push_back(']');
  }
}

void appendDirectShapeStatus(std::string& output,
                             const CreativeEditorState& editor) {
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.shapeBrushKind));
  if (editor.toolSettings.shapeBrushKind ==
      cr::CreativeShapeBrushKind::Cylinder) {
    output.push_back(' ');
    output.append(cr::toString(editor.toolSettings.shapeBrushAxis));
  }
  output.append(" | ");
  output.append(cr::toString(editor.placeBrush));
  if (editor.volume.selection.phase ==
      cr::CreativeVolumeSelectionPhase::FirstCorner) {
    output.append(" | Corner 1");
  } else if (editor.volume.selection.phase ==
             cr::CreativeVolumeSelectionPhase::Complete) {
    output.append(" | Ready");
  }
  appendHeldQuickEditStatus(output, editor);
}

void appendMaterialBrushStatus(std::string& output,
                               const CreativeEditorState& editor,
                               const cr::CreativeHotbarEntry& held) {
  const CreativeMaterialBrushGestureConfig brushConfig =
      editor.interaction.materialStroke.hasBrushAnchor
          ? editor.interaction.materialStroke.brushConfig
          : creativeMaterialBrushGestureConfig(editor.toolSettings);
  output.append(" | ");
  output.append(cr::toString(held.objectKind));
  output.append(" | ");
  output.append(cr::toString(brushConfig.shape));
  if (brushConfig.shape == cr::CreativeMaterialBrushShape::Cylinder) {
    output.push_back(' ');
    output.append(cr::toString(brushConfig.axis));
  }
  output.append(" | ");
  output.append(cr::toString(brushConfig.size));
  output.append(" | ");
  output.append(cr::toString(brushConfig.fill));
  output.append(" | ");
  output.append(cr::toString(brushConfig.guide));
  if (brushConfig.symmetry != cr::CreativeMaterialBrushSymmetry::Off) {
    output.append(" | ");
    output.append(cr::toString(brushConfig.symmetry));
    const CreativeMaterialBrushPivotState& pivot =
        editor.interaction.materialBrushPivot;
    if (pivot.locked) {
      output.append(" | PIVOT LOCKED ");
      output.append(std::to_string(pivot.lockedCell.x));
      output.push_back(' ');
      output.append(std::to_string(pivot.lockedCell.y));
      output.push_back(' ');
      output.append(std::to_string(pivot.lockedCell.z));
    }
  }
  output.append(" | ");
  output.append(cr::toString(brushConfig.mask));
  if (brushConfig.mask == cr::CreativeMaterialBrushMask::Replace) {
    output.push_back(' ');
    output.append(
        brushConfig.replaceSourceKind == cr::CreativeObjectKind::Unknown
            ? std::string_view{"ANY"}
            : cr::toString(brushConfig.replaceSourceKind));
  }
  const cr::CreativeMaterialBrushStampPlan stamp =
      cr::planCreativeMaterialBrushStamp(
          creativeMaterialBrushStampRequest(brushConfig));
  if (stamp.accepted) {
    output.append(" | ");
    output.append(std::to_string(stamp.cellCount));
    output.append(" VOXELS");
  }
  appendHeldQuickEditStatus(output, editor);
}

void appendLinearArrayStatus(std::string& output,
                             const CreativeEditorState& editor) {
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.arrayMode));
  if (editor.toolSettings.arrayMode == cr::CreativeArrayMode::Radial) {
    output.append(" | ");
    output.append(cr::toString(editor.toolSettings.radialArrayAxis));
    output.append(" | ");
    output.append(cr::toString(editor.toolSettings.radialArrayInstanceCount));
    output.append(" | ");
    output.append(cr::toString(editor.toolSettings.radialArraySweep));
  }
  appendHeldQuickEditStatus(output, editor);
}

void appendTerrainControlStatus(std::string& output,
                                const CreativeEditorState& editor) {
  const bool seedMode = editor.toolSettings.terrainRodStampMode ==
                        cr::CreativeTerrainRodStampMode::Seed;
  if (!seedMode && editor.terrain.selectionValid) {
    output.append(" | EDIT ");
    output.append(std::to_string(editor.terrain.selectedCoord.x));
    output.push_back(' ');
    output.append(std::to_string(editor.terrain.selectedCoord.z));
  } else if (!seedMode && editor.terrain.hoverValid) {
    output.append(" | ROD ");
    output.append(std::to_string(editor.terrain.hoverCoord.x));
    output.push_back(' ');
    output.append(std::to_string(editor.terrain.hoverCoord.z));
  }
  if (seedMode) {
    output.append(" | SEED RADIUS ");
    output.append(std::to_string(cr::creativeTerrainSeedRadiusCells(
        editor.toolSettings.terrainSeedRadius)));
    output.append(" | SPACING ");
    output.append(std::to_string(cr::creativeTerrainSeedSpacingCells(
        editor.toolSettings.terrainSeedSpacing)));
  }
  appendHeldQuickEditStatus(output, editor);
}

void appendTerrainGradeStatus(std::string& output,
                              const CreativeEditorState& editor) {
  if (editor.terrain.grade.anchorValid) {
    output.append(" | START ");
    output.append(std::to_string(editor.terrain.grade.anchorCoord.x));
    output.push_back(' ');
    output.append(std::to_string(editor.terrain.grade.anchorCoord.z));
  } else {
    output.append(" | SET START ROD");
  }
  appendHeldQuickEditStatus(output, editor);
}

void appendTerrainSculptStatus(std::string& output,
                               const CreativeEditorState& editor) {
  output.append(" | ");
  output.append(creativeEditorTerrainSculptQuickEditLabel(editor));
  if (editor.terrain.sculpt.preview.valid &&
      !editor.terrain.sculpt.preview.plan.accepted) {
    output.append(" | NO RODS");
  }
}

void appendTerrainProfileStatus(std::string& output,
                                const CreativeEditorState& editor) {
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.terrainProfileKind));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.terrainProfileBlend));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.terrainProfileRodPolicy));
  output.append(" | ");
  output.append(creativeEditorTerrainProfileQuickEditLabel(editor));
  if (editor.terrain.profile.preview.valid &&
      !editor.terrain.profile.preview.plan.accepted) {
    output.append(" | ");
    output.append(cr::toString(editor.terrain.profile.preview.plan.status));
  }
}

void appendTerrainPathStatus(std::string& output,
                             const CreativeEditorState& editor) {
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.terrainPathKind));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.terrainPathElevation));
  output.append(" | ");
  output.append(creativeEditorTerrainPathQuickEditLabel(editor));
  output.append(" | POINTS ");
  output.append(std::to_string(editor.terrain.path.pointCount));
  if (editor.terrain.path.preview.valid &&
      editor.terrain.path.preview.pointCount >= 2U &&
      !editor.terrain.path.preview.plan.accepted) {
    output.append(" | ");
    output.append(cr::toString(editor.terrain.path.preview.plan.status));
  }
}

void appendTerrainRegionStatus(std::string& output,
                               const CreativeEditorState& editor) {
  output.append(" | ");
  output.append(creativeEditorTerrainRegionQuickEditLabel(editor));
  if (editor.terrain.region.stamp.active) {
    const CreativeTerrainStampPreviewCache& preview =
        editor.terrain.region.stamp.preview;
    if (preview.valid) {
      output.append(" | ");
      output.append(std::to_string(preview.plan.finalControlCount));
      output.append(" RODS");
      if (!preview.plan.accepted) {
        output.append(" | ");
        output.append(cr::toString(preview.plan.status));
      }
    }
    return;
  }
  const CreativeTerrainRegionPreviewCache& preview =
      editor.terrain.region.preview;
  if (preview.valid) {
    output.append(" | ");
    output.append(std::to_string(preview.plan.affectedControlCount));
    output.append(" RODS");
    if (!preview.plan.accepted) {
      output.append(" | ");
      output.append(cr::toString(preview.plan.status));
    }
  }
}

void appendConnectedFillStatus(std::string& output,
                               const CreativeEditorState& editor,
                               const cr::CreativeHotbarEntry& held) {
  output.append(" | ");
  output.append(cr::toString(held.objectKind));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.connectedFillLimit));
  const CreativeEditorConnectedFillCache& cache =
      editor.interaction.connectedFill;
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  const bool cacheMatchesTarget =
      cache.valid && target.voxelHit && cache.seedCell == target.voxelCell &&
      cache.limit == editor.toolSettings.connectedFillLimit;
  if (cacheMatchesTarget) {
    output.append(" | ");
    if (cache.plan.accepted) {
      output.append(std::to_string(cache.plan.cellCount));
      output.append(" CELLS");
    } else if (cache.plan.status ==
               cr::CreativeConnectedFillStatus::CapacityExceeded) {
      output.append("TOO LARGE");
    } else {
      output.append(cr::toString(cache.plan.status));
    }
  }
  appendHeldQuickEditStatus(output, editor);
}

void appendSurfaceExtrudeStatus(std::string& output,
                                const CreativeEditorState& editor,
                                const cr::CreativeHotbarEntry& held) {
  output.append(" | ");
  output.append(cr::toString(held.objectKind));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.surfaceExtrudeDepth));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.surfaceExtrudeLimit));
  const CreativeEditorSurfaceExtrudeCache& cache =
      editor.interaction.surfaceExtrude;
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  cr::CreativeGridCoord3 outward{};
  const bool faceValid =
      creativeSurfaceFaceOffset(target.grid.faceNormal, outward);
  const bool cacheMatchesTarget =
      cache.valid && target.voxelHit && faceValid &&
      cache.seedCell == target.voxelCell && cache.outward == outward &&
      cache.kind == cr::CreativeSurfaceExtrudeKind::Extrude &&
      cache.depth == editor.toolSettings.surfaceExtrudeDepth &&
      cache.affectedCellLimit == editor.toolSettings.surfaceExtrudeLimit;
  if (cacheMatchesTarget) {
    output.append(" | ");
    if (cache.plan.accepted) {
      output.append(std::to_string(cache.plan.surfaceCellCount));
      output.append(" FACE / ");
      output.append(std::to_string(cache.plan.mutationCellCount));
      output.append(" CELLS");
    } else if (cache.plan.status ==
               cr::CreativeSurfaceExtrudeStatus::CapacityExceeded) {
      output.append("TOO LARGE");
    } else {
      output.append(cr::toString(cache.plan.status));
    }
  }
  appendHeldQuickEditStatus(output, editor);
}

}  // namespace

void clearCreativeEditorPlacementFeedback(
    CreativeEditorInteractionState& interaction) noexcept {
  interaction.placementFeedback = {};
}

void setCreativeEditorPlacementFeedback(
    CreativeEditorInteractionState& interaction,
    CreativeEditorPlacementFeedbackStatus status,
    std::uint64_t frameIndex,
    cr::CreativeObjectKind objectKind,
    cr::CreativeObjectId objectId) noexcept {
  interaction.placementFeedback = {};
  interaction.placementFeedback.status = status;
  interaction.placementFeedback.objectId = objectId;
  interaction.placementFeedback.objectKind = objectKind;
  interaction.placementFeedback.frameIndex = frameIndex;
}

void setCreativeEditorVoxelPlacementFeedback(
    CreativeEditorInteractionState& interaction,
    std::uint64_t frameIndex,
    cr::CreativeObjectKind objectKind,
    cr::CreativeGridCoord3 voxelCell,
    cr::CreativeBounds voxelBounds) noexcept {
  setCreativeEditorPlacementFeedback(
      interaction, CreativeEditorPlacementFeedbackStatus::Placed, frameIndex,
      objectKind);
  interaction.placementFeedback.voxelPlaced = true;
  interaction.placementFeedback.voxelCell = voxelCell;
  interaction.placementFeedback.voxelBounds = voxelBounds;
}

void resetCreativeMaterialBrushPivot(
    CreativeMaterialBrushPivotState& state,
    cr::CreativeDocumentId documentId) noexcept {
  state = {};
  state.documentId = documentId;
}

void synchronizeCreativeMaterialBrushPivotDocument(
    CreativeMaterialBrushPivotState& state,
    cr::CreativeDocumentId documentId) noexcept {
  if (state.documentId != documentId) {
    resetCreativeMaterialBrushPivot(state, documentId);
  }
}

void updateCreativeMaterialBrushPivotAim(
    CreativeMaterialBrushPivotState& state,
    cr::CreativeDocumentId documentId,
    bool aimAvailable,
    cr::CreativeGridCoord3 aimCell) noexcept {
  synchronizeCreativeMaterialBrushPivotDocument(state, documentId);
  state.aimAvailable =
      aimAvailable && documentId != cr::kInvalidDocumentId;
  if (state.aimAvailable) {
    state.aimCell = aimCell;
  }
}

bool lockCreativeMaterialBrushPivotFromAim(
    CreativeMaterialBrushPivotState& state) noexcept {
  if (!state.aimAvailable ||
      state.documentId == cr::kInvalidDocumentId) {
    return false;
  }
  state.locked = true;
  state.lockedCell = state.aimCell;
  return true;
}

bool clearCreativeMaterialBrushPivot(
    CreativeMaterialBrushPivotState& state) noexcept {
  if (!state.locked) {
    return false;
  }
  state.locked = false;
  state.lockedCell = {};
  return true;
}

bool creativeMaterialBrushLockedPivot(
    const CreativeMaterialBrushPivotState& state,
    cr::CreativeDocumentId documentId,
    cr::CreativeGridCoord3& pivot) noexcept {
  if (!state.locked || documentId == cr::kInvalidDocumentId ||
      state.documentId != documentId) {
    return false;
  }
  pivot = state.lockedCell;
  return true;
}

bool storeSelectedCreativeMaterialBrushPreset(
    CreativeMaterialBrushPresetBank& presets,
    const cr::CreativeHotbarState& hotbar,
    const cr::CreativeToolSettings& settings) noexcept {
  const std::size_t slot = std::min<std::size_t>(
      hotbar.selectedSlot, cr::kCreativeHotbarSlotCount - 1U);
  if (hotbar.entries[slot].kind !=
          cr::CreativeHeldItemKind::MaterialBrush ||
      !cr::isValidCreativeToolSettings(settings)) {
    return false;
  }
  presets.slots[slot] = creativeMaterialBrushGestureConfig(settings);
  presets.initialized[slot] = 1U;
  return true;
}

bool activateSelectedCreativeMaterialBrushPreset(
    CreativeMaterialBrushPresetBank& presets,
    const cr::CreativeHotbarState& hotbar,
    cr::CreativeToolSettings& settings) noexcept {
  const std::size_t slot = std::min<std::size_t>(
      hotbar.selectedSlot, cr::kCreativeHotbarSlotCount - 1U);
  if (hotbar.entries[slot].kind !=
      cr::CreativeHeldItemKind::MaterialBrush) {
    return false;
  }
  if (presets.initialized[slot] == 0U) {
    return storeSelectedCreativeMaterialBrushPreset(presets, hotbar,
                                                    settings);
  }
  cr::CreativeToolSettings adjusted = settings;
  applyCreativeMaterialBrushGestureConfig(adjusted, presets.slots[slot]);
  if (!cr::isValidCreativeToolSettings(adjusted)) {
    return false;
  }
  settings = adjusted;
  return true;
}

void clearCreativeMaterialBrushPresetSlot(
    CreativeMaterialBrushPresetBank& presets,
    std::size_t slot) noexcept {
  if (slot >= cr::kCreativeHotbarSlotCount) {
    return;
  }
  presets.slots[slot] = {};
  presets.initialized[slot] = 0U;
}

bool creativeMaterialBrushPresetForSlot(
    const CreativeMaterialBrushPresetBank& presets,
    std::size_t slot,
    CreativeMaterialBrushGestureConfig& preset) noexcept {
  if (slot >= cr::kCreativeHotbarSlotCount ||
      presets.initialized[slot] == 0U) {
    return false;
  }
  preset = presets.slots[slot];
  return true;
}

std::string creativeMaterialBrushPresetHotbarLabel(
    const CreativeMaterialBrushGestureConfig& preset) {
  std::string output;
  switch (preset.shape) {
    case cr::CreativeMaterialBrushShape::Cube:
      output = "C";
      break;
    case cr::CreativeMaterialBrushShape::Sphere:
      output = "S";
      break;
    case cr::CreativeMaterialBrushShape::Cylinder:
      output = "C";
      switch (preset.axis) {
        case cr::CreativeAxis3::X:
          output.append("X");
          break;
        case cr::CreativeAxis3::Y:
          output.append("Y");
          break;
        case cr::CreativeAxis3::Z:
          output.append("Z");
          break;
        case cr::CreativeAxis3::Count:
          return "B?";
      }
      break;
    case cr::CreativeMaterialBrushShape::Count:
      return "B?";
  }

  switch (preset.size) {
    case cr::CreativeMaterialBrushSize::OneCell:
      output.append("1");
      break;
    case cr::CreativeMaterialBrushSize::ThreeCells:
      output.append("3");
      break;
    case cr::CreativeMaterialBrushSize::FiveCells:
      output.append("5");
      break;
    case cr::CreativeMaterialBrushSize::Count:
      return "B?";
  }

  switch (preset.fill) {
    case cr::CreativeMaterialBrushFill::Solid:
      break;
    case cr::CreativeMaterialBrushFill::Shell:
      output.append("H");
      break;
    case cr::CreativeMaterialBrushFill::Count:
      return "B?";
  }

  switch (preset.symmetry) {
    case cr::CreativeMaterialBrushSymmetry::Off:
      break;
    case cr::CreativeMaterialBrushSymmetry::MirrorX:
    case cr::CreativeMaterialBrushSymmetry::MirrorY:
    case cr::CreativeMaterialBrushSymmetry::MirrorZ:
    case cr::CreativeMaterialBrushSymmetry::MirrorXZ:
      output.append("M");
      break;
    case cr::CreativeMaterialBrushSymmetry::Count:
      return "B?";
  }
  return output;
}

double creativeEditorTargetCellSize(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor) noexcept {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  switch (held.kind) {
    case cr::CreativeHeldItemKind::Material:
      switch (cr::describeObject(held.objectKind)
                  .placementPolicy.storagePolicy) {
        case cr::CreativePlacementStoragePolicy::AuthoredObject:
          return editor.placeCellSize;
        case cr::CreativePlacementStoragePolicy::VoxelCell:
          return document.gridSettings().cellSizeMeters;
      }
      return editor.placeCellSize;
    case cr::CreativeHeldItemKind::MaterialBrush:
      return document.gridSettings().cellSizeMeters;
    case cr::CreativeHeldItemKind::VolumeSelect:
    case cr::CreativeHeldItemKind::VolumeFill:
    case cr::CreativeHeldItemKind::VolumeHollow:
    case cr::CreativeHeldItemKind::VolumeReplace:
    case cr::CreativeHeldItemKind::VolumeErase:
    case cr::CreativeHeldItemKind::VolumeClone:
    case cr::CreativeHeldItemKind::ConnectedFill:
    case cr::CreativeHeldItemKind::SurfaceExtrude:
    case cr::CreativeHeldItemKind::TerrainControl:
    case cr::CreativeHeldItemKind::TerrainPaint:
    case cr::CreativeHeldItemKind::TerrainGrade:
    case cr::CreativeHeldItemKind::TerrainSculpt:
    case cr::CreativeHeldItemKind::TerrainProfile:
    case cr::CreativeHeldItemKind::TerrainPath:
    case cr::CreativeHeldItemKind::TerrainRegion:
      return document.gridSettings().cellSizeMeters;
    case cr::CreativeHeldItemKind::ObjectSelect:
    case cr::CreativeHeldItemKind::ObjectMove:
    case cr::CreativeHeldItemKind::LinearArray:
    case cr::CreativeHeldItemKind::ObjectGroup:
    case cr::CreativeHeldItemKind::Count:
      return editor.placeCellSize;
  }
  return editor.placeCellSize;
}

std::string creativeEditorHeldItemStatusLabel(
    const CreativeEditorState& editor) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  std::string output(cr::toString(held.kind));
  switch (held.kind) {
    case cr::CreativeHeldItemKind::MaterialBrush:
      appendMaterialBrushStatus(output, editor, held);
      break;
    case cr::CreativeHeldItemKind::VolumeFill:
    case cr::CreativeHeldItemKind::VolumeHollow:
      appendDirectShapeStatus(output, editor);
      break;
    case cr::CreativeHeldItemKind::LinearArray:
      appendLinearArrayStatus(output, editor);
      break;
    case cr::CreativeHeldItemKind::TerrainControl:
      appendTerrainControlStatus(output, editor);
      break;
    case cr::CreativeHeldItemKind::TerrainPaint:
      output.append(" | ");
      output.append(cr::toString(editor.toolSettings.terrainPaintMode));
      output.append(" | ");
      output.append(cr::toString(editor.toolSettings.terrainPaintMaterial));
      switch (editor.toolSettings.terrainPaintMode) {
        case cr::CreativeTerrainPaintMode::Brush:
          output.append(" | RADIUS ");
          output.append(cr::toString(editor.toolSettings.terrainPaintRadius));
          break;
        case cr::CreativeTerrainPaintMode::Connected:
          break;
        case cr::CreativeTerrainPaintMode::Region:
          output.append(" | FROM ");
          output.append(cr::toString(editor.toolSettings.terrainPaintSource));
          switch (editor.terrainPaint.regionPhase) {
            case CreativeEditorTerrainPaintRegionPhase::Empty:
              output.append(" | CORNER 1");
              break;
            case CreativeEditorTerrainPaintRegionPhase::FirstCorner:
              output.append(" | CORNER 2");
              break;
            case CreativeEditorTerrainPaintRegionPhase::Complete:
              output.append(" | READY");
              break;
          }
          break;
        case cr::CreativeTerrainPaintMode::Count:
          break;
      }
      appendHeldQuickEditStatus(output, editor);
      break;
    case cr::CreativeHeldItemKind::TerrainGrade:
      appendTerrainGradeStatus(output, editor);
      break;
    case cr::CreativeHeldItemKind::TerrainSculpt:
      appendTerrainSculptStatus(output, editor);
      break;
    case cr::CreativeHeldItemKind::TerrainProfile:
      appendTerrainProfileStatus(output, editor);
      break;
    case cr::CreativeHeldItemKind::TerrainPath:
      appendTerrainPathStatus(output, editor);
      break;
    case cr::CreativeHeldItemKind::TerrainRegion:
      appendTerrainRegionStatus(output, editor);
      break;
    case cr::CreativeHeldItemKind::ConnectedFill:
      appendConnectedFillStatus(output, editor, held);
      break;
    case cr::CreativeHeldItemKind::SurfaceExtrude:
      appendSurfaceExtrudeStatus(output, editor, held);
      break;
    case cr::CreativeHeldItemKind::Material:
    case cr::CreativeHeldItemKind::VolumeReplace:
      output.append(" | ");
      output.append(cr::toString(held.objectKind));
      appendHeldQuickEditStatus(output, editor);
      break;
    case cr::CreativeHeldItemKind::ObjectSelect:
    case cr::CreativeHeldItemKind::ObjectMove:
    case cr::CreativeHeldItemKind::VolumeSelect:
    case cr::CreativeHeldItemKind::VolumeErase:
    case cr::CreativeHeldItemKind::VolumeClone:
    case cr::CreativeHeldItemKind::ObjectGroup:
    case cr::CreativeHeldItemKind::Count:
      appendHeldQuickEditStatus(output, editor);
      break;
  }
  if (creativeEditorGroupFocusActive(editor.groupFocus)) {
    output.append(" | EDIT GROUP ");
    output.append(std::to_string(editor.groupFocus.depth));
    output.append(" | CIRCLE EXIT");
  }
  return output;
}

CreativeEditorWorldTarget resolveCreativeEditorWorldTarget(
    const cr::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    const CreativeEditorPickFrame& pickFrame,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    double cellSize) {
  CreativeEditorWorldTarget target;
  target.ray = worldRayFromPixel(
      camera, static_cast<float>(drawableWidth) * 0.5F,
      static_cast<float>(drawableHeight) * 0.5F, drawableWidth, drawableHeight);
  if (!target.ray.valid) {
    return target;
  }

  const ObjectVisualPickResult pick = pickNearestVisualBoundsObject(
      pickFrame.objectPickCandidates, target.ray);
  const cr::CreativeGridSettings gridSettings = document.gridSettings();
  cr::CreativeVoxelRaycastRequest voxelRequest;
  voxelRequest.rayOrigin = cr::creativeVec3FromCore(target.ray.origin);
  voxelRequest.rayDirection = cr::creativeVec3FromCore(target.ray.direction);
  voxelRequest.gridOrigin = gridSettings.origin;
  voxelRequest.cellSize = cellSize;
  voxelRequest.maxDistance = kCreativeReachMeters;
  const cr::CreativeVoxelRaycastReceipt voxelPick =
      cr::raycastCreativeVoxelField(document.voxelField(), voxelRequest);
  cr::CreativeTerrainRaycastRequest terrainRequest;
  terrainRequest.rayOrigin = cr::creativeVec3FromCore(target.ray.origin);
  terrainRequest.rayDirection = cr::creativeVec3FromCore(target.ray.direction);
  terrainRequest.gridOrigin = gridSettings.origin;
  terrainRequest.cellSize = gridSettings.cellSizeMeters;
  terrainRequest.maxDistance = kCreativeReachMeters;
  const cr::CreativeTerrainRaycastReceipt terrainPick =
      cr::raycastCreativeTerrainField(document.terrainField(), terrainRequest);
  const ObjectVisualPickBounds* objectCandidate =
      findCandidate(pickFrame, pick.objectId);
  const bool objectInReach = objectCandidate != nullptr &&
                             pick.entryDistance <= kCreativeReachMeters;
  const bool terrainInReach =
      terrainPick.hit && terrainPick.distance <= kCreativeReachMeters;
  const bool voxelIsNearest =
      voxelPick.hit &&
      (!objectInReach || voxelPick.distance <= pick.entryDistance) &&
      (!terrainInReach || voxelPick.distance <= terrainPick.distance);

  if (voxelIsNearest) {
    target.grid = cr::resolveCreativeGridTargetFromHit(
        voxelPick.hitPoint, voxelPick.faceNormal, cellSize,
        gridSettings.origin, cr::creativeVec3FromCore(target.ray.direction));
    target.valid = target.grid.valid;
    target.voxelHit = true;
    target.voxelCell = voxelPick.cell;
    target.objectKind = voxelPick.material;
    target.distanceMeters = static_cast<float>(voxelPick.distance);
    return target;
  }

  const bool objectIsNearest =
      objectInReach &&
      (!terrainInReach || pick.entryDistance <= terrainPick.distance);
  if (objectIsNearest) {
    const iggy3d::Vec3 point =
        target.ray.origin + target.ray.direction * pick.entryDistance;
    const iggy3d::Vec3 normal = objectCandidate->orientedBounds.has_value()
                                    ? orientedFaceNormal(
                                          *objectCandidate->orientedBounds,
                                          point)
                                    : aabbFaceNormal(objectCandidate->bounds,
                                                     point);
    target.grid = cr::resolveCreativeGridTargetFromHit(
        cr::creativeVec3FromCore(point), cr::creativeVec3FromCore(normal),
        cellSize, gridSettings.origin,
        cr::creativeVec3FromCore(target.ray.direction));
    target.valid = target.grid.valid;
    target.objectHit = true;
    target.objectId = pick.objectId;
    target.distanceMeters = pick.entryDistance;
    if (const cr::CreativeObject* object = document.findObject(pick.objectId);
        object != nullptr) {
      target.objectKind = object->kind;
    }
    return target;
  }

  if (terrainInReach) {
    target.grid = cr::resolveCreativeGridTargetFromHit(
        terrainPick.hitPoint, terrainPick.faceNormal, cellSize,
        gridSettings.origin, cr::creativeVec3FromCore(target.ray.direction));
    target.valid = target.grid.valid;
    target.terrainHit = true;
    target.terrainCell = terrainPick.cell;
    target.objectKind = cr::CreativeObjectKind::TerrainPatch;
    target.distanceMeters = static_cast<float>(terrainPick.distance);
    return target;
  }

  if (!terrainPick.accepted) {
    return target;
  }

  if (std::fabs(target.ray.direction.y) <= 1.0e-5F) {
    return target;
  }
  const cr::CreativeCoreVec3Conversion coreGridOrigin =
      cr::creativeVec3ToCoreChecked(gridSettings.origin);
  if (!coreGridOrigin.converted) {
    return target;
  }
  const float distance =
      (coreGridOrigin.value.y - target.ray.origin.y) /
      target.ray.direction.y;
  if (!std::isfinite(distance) || distance < 0.0F ||
      distance > kCreativeReachMeters) {
    return target;
  }
  const iggy3d::Vec3 point =
      target.ray.origin + target.ray.direction * distance;
  target.grid = cr::resolveCreativeGridTargetFromHit(
      cr::creativeVec3FromCore(point), {0.0, 1.0, 0.0}, cellSize,
      gridSettings.origin,
      cr::creativeVec3FromCore(target.ray.direction));
  target.valid = target.grid.valid;
  target.distanceMeters = distance;
  return target;
}

void syncCreativeEditorHeldItem(cr::CreativeAppState& appState,
                                CreativeEditorState& editor) {
  finalizeCreativeEditorContinuousGestures(
      appState, editor, "creative_continuous_gesture_tool_changed");
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const std::size_t behaviorIndex = static_cast<std::size_t>(held.kind);
  if (behaviorIndex >= kHeldItemBehaviors.size()) {
    editor.placeMode = false;
    deactivateCreativeEditorVolumeMode(editor.volume);
    editor.interaction.moveTargetId = cr::kInvalidObjectId;
    static_cast<void>(appState.facade.setActiveTool(cr::Tool::Select));
    return;
  }
  const HeldItemBehavior& behavior = kHeldItemBehaviors[behaviorIndex];
  editor.placeMode = behavior.placeMode;
  if (held.objectKind != cr::CreativeObjectKind::Unknown &&
      cr::creativeHeldItemUsesMaterial(held.kind)) {
    editor.placeBrush = held.objectKind;
  }
  if (behavior.volumeMode) {
    const cr::CreativeGridSettings grid =
        appState.facade.document().gridSettings();
    activateCreativeEditorVolumeMode(
        editor.volume, creativeEditorTargetCellSize(
                           appState.facade.document(), editor),
        grid.origin);
    editor.volume.operation = behavior.volumeOperation;
  } else {
    deactivateCreativeEditorVolumeMode(editor.volume);
  }
  if (held.kind != cr::CreativeHeldItemKind::ObjectMove) {
    editor.interaction.moveTargetId = cr::kInvalidObjectId;
  }
  if (held.kind != cr::CreativeHeldItemKind::TerrainControl) {
    clearCreativeEditorTerrainInteraction(editor.terrain,
                                          appState.facade.document().id());
  } else {
    static_cast<void>(cancelCreativeEditorTerrainGrade(editor));
  }
  syncCreativeEditorQuickEdit(editor);
  static_cast<void>(appState.facade.setActiveTool(behavior.facadeTool));
}

bool selectCreativeEditorHotbarSlot(cr::CreativeAppState& appState,
                                    CreativeEditorState& editor,
                                    std::size_t slot) {
  static_cast<void>(storeSelectedCreativeMaterialBrushPreset(
      editor.interaction.materialBrushPresets,
      editor.interaction.hotbar, editor.toolSettings));
  const bool changed =
      cr::selectCreativeHotbarSlot(editor.interaction.hotbar, slot);
  if (changed) {
    static_cast<void>(activateSelectedCreativeMaterialBrushPreset(
        editor.interaction.materialBrushPresets,
        editor.interaction.hotbar, editor.toolSettings));
  }
  syncCreativeEditorHeldItem(appState, editor);
  return changed;
}

bool confirmCreativeEditorHeldItem(cr::CreativeAppState& appState,
                                   CreativeEditorState& editor,
                                   std::string_view source) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const std::size_t row = static_cast<std::size_t>(held.kind);
  if (row >= kHeldItemCommands.size()) {
    return false;
  }
  const HeldItemConfirmCommand command = kHeldItemCommands[row].confirm;
  if (command == nullptr) {
    return false;
  }
  return command(held.kind, appState, editor, source).changed;
}

bool cancelCreativeEditorHeldItem(cr::CreativeAppState& appState,
                                  CreativeEditorState& editor) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const std::size_t row = static_cast<std::size_t>(held.kind);
  if (row < kHeldItemCommands.size() &&
      kHeldItemCommands[row].cancel != nullptr) {
    const HeldItemCommandResult command =
        kHeldItemCommands[row].cancel(appState, editor);
    if (command.handled) {
      return command.changed;
    }
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

void processCreativeEditorWorldInteractionFrame(
    const CreativeEditorWorldInteractionFrameRequest& request) {
  CreativeEditorState& editor = request.editor;
  const cr::CreativeDocument& document = request.appState.facade.document();
  static_cast<void>(syncCreativeEditorGroupFocus(editor.groupFocus, document));
  const double targetCellSize =
      creativeEditorTargetCellSize(document, editor);
  const cr::CreativeGridSettings documentGrid = document.gridSettings();
  const cr::CreativeVolumeSelection& volumeSelection =
      editor.volume.selection;
  const bool volumeGridChanged =
      volumeSelection.cellSize != targetCellSize ||
      volumeSelection.origin.x != documentGrid.origin.x ||
      volumeSelection.origin.y != documentGrid.origin.y ||
      volumeSelection.origin.z != documentGrid.origin.z;
  if (editor.volume.active && volumeGridChanged) {
    activateCreativeEditorVolumeMode(editor.volume, targetCellSize,
                                     documentGrid.origin);
  }
  editor.interaction.target = resolveCreativeEditorWorldTarget(
      document, request.camera, request.pickFrame, request.drawableWidth,
      request.drawableHeight, targetCellSize);
  const cr::CreativeHotbarEntry& aimedHeld =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const bool hierarchySelectionTool =
      aimedHeld.kind == cr::CreativeHeldItemKind::ObjectSelect ||
      aimedHeld.kind == cr::CreativeHeldItemKind::ObjectMove ||
      aimedHeld.kind == cr::CreativeHeldItemKind::ObjectGroup ||
      aimedHeld.kind == cr::CreativeHeldItemKind::LinearArray;
  if (hierarchySelectionTool && editor.interaction.target.objectHit) {
    const cr::CreativeObjectId resolvedObjectId =
        resolveCreativeEditorGroupSelectionTarget(
            document, editor.groupFocus, editor.interaction.target.objectId);
    const cr::CreativeObject* resolvedObject =
        document.findObject(resolvedObjectId);
    if (resolvedObject == nullptr) {
      editor.interaction.target.objectHit = false;
      editor.interaction.target.objectId = cr::kInvalidObjectId;
      editor.interaction.target.objectKind = cr::CreativeObjectKind::Unknown;
    } else {
      editor.interaction.target.objectId = resolvedObject->id;
      editor.interaction.target.objectKind = resolvedObject->kind;
    }
  }
  if (cr::creativeHeldItemIsTerrainTool(aimedHeld.kind)) {
    updateCreativeEditorTerrainAim(
        editor.terrain, document, editor.interaction.target.ray,
        editor.interaction.target.valid ? editor.interaction.target.distanceMeters
                                        : kCreativeReachMeters);
  } else {
    clearCreativeEditorTerrainInteraction(editor.terrain, document.id());
  }
  cr::CreativeGridTarget brushPivotAim;
  if (editor.interaction.target.grid.valid) {
    const cr::CreativeGridTarget& targetGrid = editor.interaction.target.grid;
    brushPivotAim = cr::resolveCreativeGridTargetFromHit(
        targetGrid.hitPoint, targetGrid.faceNormal,
        documentGrid.cellSizeMeters, documentGrid.origin,
        targetGrid.placerForward);
  }
  updateCreativeMaterialBrushPivotAim(
      editor.interaction.materialBrushPivot, document.id(),
      brushPivotAim.valid, brushPivotAim.adjacentCell);
  editor.volume.cursorValid = editor.interaction.target.grid.valid;
  if (editor.volume.cursorValid) {
    editor.volume.cursorCell = editor.interaction.target.grid.targetCell;
  }
  if (request.captureMode) {
    finalizeCreativeEditorContinuousGestures(
        request.appState, editor, "creative_continuous_gesture_capture");
    return;
  }
  if (creativeEditorGroupFocusActive(editor.groupFocus) &&
      cr::creativeWorldActionPressed(
          request.actions, cr::CreativeWorldActionId::Reject)) {
    finalizeCreativeEditorContinuousGestures(
        request.appState, editor, "creative_group_focus_exit");
    static_cast<void>(exitCreativeEditorGroupFocus(
        request.appState, editor.groupFocus));
    editor.interaction.target = {};
    return;
  }
  if (editor.transform.active) {
    finalizeCreativeEditorContinuousGestures(
        request.appState, editor, "creative_continuous_gesture_transform");
    const bool secondaryPressed =
        cr::creativeWorldActionPressed(
            request.actions, cr::CreativeWorldActionId::Secondary) ||
        cr::creativeWorldActionPressed(
            request.actions, cr::CreativeWorldActionId::Accept);
    static_cast<void>(processCreativeEditorSelectionTransformPreview(
        request.appState, editor.transform,
        editor.interaction.target.grid.valid,
        editor.interaction.target.grid.placementAnchor, secondaryPressed,
        "selection_transform_commit",
        cr::creativeSnapIncrementMeters(editor.toolSettings.snapIncrement)));
    return;
  }

  std::int32_t hotbarSteps = -request.actions.hotbarWheelSteps;
  if (cr::creativeWorldActionPressed(
          request.actions, cr::CreativeWorldActionId::HotbarPrevious)) {
    --hotbarSteps;
  }
  if (cr::creativeWorldActionPressed(
          request.actions, cr::CreativeWorldActionId::HotbarNext)) {
    ++hotbarSteps;
  }
  if (hotbarSteps != 0) {
    finalizeCreativeEditorContinuousGestures(
        request.appState, editor, "creative_continuous_gesture_hotbar");
    static_cast<void>(storeSelectedCreativeMaterialBrushPreset(
        editor.interaction.materialBrushPresets,
        editor.interaction.hotbar, editor.toolSettings));
    if (cr::cycleCreativeHotbar(editor.interaction.hotbar, hotbarSteps)) {
      static_cast<void>(activateSelectedCreativeMaterialBrushPreset(
          editor.interaction.materialBrushPresets,
          editor.interaction.hotbar, editor.toolSettings));
      syncCreativeEditorHeldItem(request.appState, editor);
    }
  }

  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (processExclusiveHeldItemFrame(request, held)) {
    return;
  }

  InteractionContext context{request, held};
  constexpr std::array actions{
      cr::CreativeWorldActionId::Primary,
      cr::CreativeWorldActionId::Secondary,
      cr::CreativeWorldActionId::Pick,
  };
  const std::size_t handlerRow = static_cast<std::size_t>(held.kind);
  if (handlerRow >= kHeldItemHandlers.size()) {
    return;
  }
  const HeldItemHandlerRow& handler = kHeldItemHandlers[handlerRow];
  const bool rejectPressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Reject);
  const bool acceptPressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Accept);
  if (rejectPressed) {
    handler.reject(context);
  } else if (acceptPressed) {
    handler.accept(context);
  }
  const bool primaryPressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Primary);
  for (std::size_t index = 0; index < actions.size(); ++index) {
    if (handler.primaryWinsSimultaneous && index == 1U && primaryPressed) {
      continue;
    }
    if (cr::creativeWorldActionPressed(request.actions, actions[index])) {
      handler.handlers[index](context);
    }
  }
}

void finalizeCreativeEditorContinuousGestures(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view reasonCode) {
  finalizeCreativeMaterialStroke(appState, editor, reasonCode);
  finalizeCreativeTerrainStroke(appState, editor, reasonCode);
  finalizeCreativeTerrainSculptStroke(appState, editor, reasonCode);
  finalizeCreativeEditorTerrainPaintStroke(appState, editor, reasonCode);
}

void appendCreativeEditorInteractionOverlay(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    float wireThickness,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  if (drawableWidth == 0U || drawableHeight == 0U) {
    return;
  }

  const std::int32_t centerX = static_cast<std::int32_t>(drawableWidth / 2U);
  const std::int32_t centerY = static_cast<std::int32_t>(drawableHeight / 2U);
  const bool inventoryModalOpen = editor.catalog.model.open ||
                                  editor.catalog.toolWheel.open ||
                                  editor.toolOptions.open ||
                                  editor.transform.controlsOpen;
  if (!inventoryModalOpen) {
    const CreativeEditorPlacementFeedback& feedback =
        editor.interaction.placementFeedback;
    const bool feedbackVisible = creativeEditorPlacementFeedbackVisible(
        feedback, editor.frameIndex);
    const bool placed = feedbackVisible &&
                        feedback.status ==
                            CreativeEditorPlacementFeedbackStatus::Placed;
    const bool rejected = feedbackVisible &&
                          feedback.status ==
                              CreativeEditorPlacementFeedbackStatus::Rejected;
    const float crosshairR = rejected ? 1.0F : placed ? 0.25F : 0.95F;
    const float crosshairG = rejected ? 0.18F : placed ? 1.0F : 0.95F;
    const float crosshairB = rejected ? 0.14F : placed ? 0.35F : 0.95F;
    uiRects.push_back({centerX - 8, centerY - 1, 17, 3,
                       crosshairR, crosshairG, crosshairB, 0.92F});
    uiRects.push_back({centerX - 1, centerY - 8, 3, 17,
                       crosshairR, crosshairG, crosshairB, 0.92F});
    if (feedbackVisible) {
      uiRects.push_back({centerX - 13, centerY - 13, 11, 3,
                         crosshairR, crosshairG, crosshairB, 0.94F});
      uiRects.push_back({centerX + 3, centerY - 13, 11, 3,
                         crosshairR, crosshairG, crosshairB, 0.94F});
      uiRects.push_back({centerX - 13, centerY + 11, 11, 3,
                         crosshairR, crosshairG, crosshairB, 0.94F});
      uiRects.push_back({centerX + 3, centerY + 11, 11, 3,
                         crosshairR, crosshairG, crosshairB, 0.94F});
    }
  }

  constexpr std::int32_t slotSize = 44;
  constexpr std::int32_t gap = 4;
  constexpr std::int32_t totalWidth =
      static_cast<std::int32_t>(cr::kCreativeHotbarSlotCount) * slotSize +
      static_cast<std::int32_t>(cr::kCreativeHotbarSlotCount - 1U) * gap;
  const std::int32_t hotbarX =
      std::max(4, (static_cast<std::int32_t>(drawableWidth) - totalWidth) / 2);
  const std::int32_t hotbarY =
      std::max(4, static_cast<std::int32_t>(drawableHeight) - slotSize - 12);
  for (std::size_t slot = 0; slot < cr::kCreativeHotbarSlotCount; ++slot) {
    const bool selected = editor.interaction.hotbar.selectedSlot == slot;
    const std::int32_t x =
        hotbarX + static_cast<std::int32_t>(slot) * (slotSize + gap);
    uiRects.push_back({x, hotbarY,
                       static_cast<std::uint32_t>(slotSize),
                       static_cast<std::uint32_t>(slotSize),
                       selected ? 0.92F : 0.08F,
                       selected ? 0.92F : 0.09F,
                       selected ? 0.88F : 0.11F,
                       selected ? 0.96F : 0.86F});
    char slotLabel[4];
    std::snprintf(slotLabel, sizeof(slotLabel), "%zu", slot + 1U);
    appendColoredText(glyphs, slotLabel, x + 4, hotbarY + 3,
                      drawableWidth, drawableHeight, selected);
    appendColoredText(glyphs,
                      hotbarLabel(editor.interaction.hotbar.entries[slot],
                                  editor.toolSettings,
                                  editor.interaction.materialBrushPresets,
                                  slot),
                      x + 4, hotbarY + 22, drawableWidth, drawableHeight,
                      selected);
  }

  if (!inventoryModalOpen && !editor.transform.active) {
    const std::string heldLabel = creativeEditorHeldItemStatusLabel(editor);
    const std::int32_t heldLabelX = std::max(
        4, static_cast<std::int32_t>(drawableWidth / 2U) -
               static_cast<std::int32_t>(heldLabel.size() * 6U));
    appendHeldItemStatusText(glyphs, heldLabel, heldLabelX, hotbarY - 22,
                             drawableWidth, drawableHeight);
  }

  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (!inventoryModalOpen && !editor.transform.active &&
      editor.interaction.target.grid.valid &&
      held.kind != cr::CreativeHeldItemKind::Material &&
      held.kind != cr::CreativeHeldItemKind::MaterialBrush) {
    const cr::CreativeBounds& bounds =
        editor.interaction.target.grid.targetCellBounds;
    const cr::CreativeCoreVec3Conversion boxMin =
        cr::creativeVec3ToCoreChecked(bounds.min);
    const cr::CreativeCoreVec3Conversion boxMax =
        cr::creativeVec3ToCoreChecked(bounds.max);
    if (boxMin.converted && boxMax.converted) {
      appendStandaloneWireframeBoxEdges(
          wireLines, boxMin.value, boxMax.value,
          iggy3d::RenderLineColor{0.96F, 0.96F, 0.96F, 1.0F},
          std::max(0.025F, wireThickness * 0.7F));
    }
  }
}

}  // namespace iggy3d_creative_app
