#include "EditorInteraction.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorPattern.hpp"
#include "EditorPlacement.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
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

struct HeldItemHandlerRow {
  cr::CreativeHeldItemKind kind = cr::CreativeHeldItemKind::Count;
  HeldItemActionHandlers handlers{};
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

[[nodiscard]] bool sameCell(cr::CreativeGridCoord3 lhs,
                            cr::CreativeGridCoord3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] bool strokeVisited(
    const CreativeMaterialStrokeState& stroke,
    CreativeMaterialStrokeKind kind,
    cr::CreativeGridCoord3 cell,
    cr::CreativeObjectId objectId) noexcept {
  for (std::size_t index = 0; index < stroke.visitedCount; ++index) {
    const CreativeMaterialStrokeVisitedKey& key = stroke.visited[index];
    if ((kind == CreativeMaterialStrokeKind::Place &&
         sameCell(key.cell, cell)) ||
        (kind == CreativeMaterialStrokeKind::Remove &&
         ((objectId != cr::kInvalidObjectId && key.objectId == objectId) ||
          (objectId == cr::kInvalidObjectId &&
           key.objectId == cr::kInvalidObjectId &&
           sameCell(key.cell, cell))))) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool rememberStrokeTarget(
    CreativeMaterialStrokeState& stroke,
    cr::CreativeGridCoord3 cell,
    cr::CreativeObjectId objectId) noexcept {
  if (stroke.visitedCount >= stroke.visited.size()) {
    stroke.capacityReached = true;
    return false;
  }
  stroke.visited[stroke.visitedCount++] = {cell, objectId};
  return true;
}

void rejectMaterialStroke(CreativeEditorState& editor,
                          cr::CreativeObjectKind objectKind) {
  editor.interaction.placementFeedback = {
      CreativeEditorPlacementFeedbackStatus::Rejected,
      cr::kInvalidObjectId, objectKind, editor.frameIndex};
}

[[nodiscard]] bool ensureMaterialStrokeTransaction(
    cr::CreativeAppState& appState,
    CreativeMaterialStrokeState& stroke,
    CreativeMaterialStrokeKind kind) {
  if (stroke.transaction.active) {
    return true;
  }
  stroke.transaction = beginEditTransaction(
      appState.facade,
      kind == CreativeMaterialStrokeKind::Remove
          ? "minecraft_primary_remove_stroke"
          : "minecraft_secondary_place_stroke");
  return stroke.transaction.active;
}

void applyMaterialStrokeMutation(cr::CreativeAppState& appState,
                                 CreativeEditorState& editor,
                                 const cr::CreativeHotbarEntry& held,
                                 CreativeMaterialStrokeKind kind) {
  CreativeMaterialStrokeState& stroke = editor.interaction.materialStroke;
  const CreativeEditorWorldTarget& target = editor.interaction.target;

  if (stroke.capacityReached) {
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }

  if (kind == CreativeMaterialStrokeKind::Remove) {
    if (!target.objectHit && !target.voxelHit) {
      rejectMaterialStroke(editor, target.objectKind);
      return;
    }
    const cr::CreativeGridCoord3 targetCell =
        target.voxelHit ? target.voxelCell : cr::CreativeGridCoord3{};
    const cr::CreativeObjectId targetObjectId =
        target.voxelHit ? cr::kInvalidObjectId : target.objectId;
    if (strokeVisited(stroke, kind, targetCell, targetObjectId)) {
      return;
    }
    if (!ensureMaterialStrokeTransaction(appState, stroke, kind)) {
      rejectMaterialStroke(editor, target.objectKind);
      return;
    }
    if (!rememberStrokeTarget(stroke, targetCell, targetObjectId)) {
      rejectMaterialStroke(editor, target.objectKind);
      return;
    }
    bool changed = false;
    if (target.voxelHit) {
      const cr::CreativeVoxelEdit edit{target.voxelCell,
                                       cr::CreativeObjectKind::Unknown};
      const cr::CreativeVoxelMutationReceipt receipt =
          appState.facade.applyVoxelEdits(std::span{&edit, 1U});
      changed = receipt.accepted && receipt.changed;
    } else {
      const cr::CreativeDocumentRemoveReceipt receipt =
          appState.facade.removeDocumentObject(target.objectId);
      changed = receipt.accepted && receipt.objectRemoved && receipt.changed;
    }
    if (changed) {
      ++stroke.acceptedMutationCount;
      editor.interaction.placementFeedback = {};
    } else {
      rejectMaterialStroke(editor, target.objectKind);
    }
    return;
  }

  const cr::CreativeGridTarget& grid = target.grid;
  if (!grid.valid || held.objectKind == cr::CreativeObjectKind::Unknown) {
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }
  const CreativeBrushPlacementAdmission admission =
      admitBrushPlacement(held.objectKind, grid);
  if (!admission.allowed) {
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }
  if (creativeBrushPlacementAlreadyExists(
          appState.facade.document(), admission.plan)) {
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }
  if (strokeVisited(stroke, kind, grid.adjacentCell,
                    cr::kInvalidObjectId)) {
    return;
  }
  if (!ensureMaterialStrokeTransaction(appState, stroke, kind)) {
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }
  if (!rememberStrokeTarget(stroke, grid.adjacentCell,
                            cr::kInvalidObjectId)) {
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }
  const std::uint64_t ordinal = editor.placedCount + 1U;
  const CreativeBrushPlacementMutationReceipt receipt = applyBrushPlacement(
      appState.facade, admission.plan, ordinal);
  if (receipt.accepted && receipt.changed &&
      (receipt.objectCreated || receipt.voxelCreated)) {
    editor.placedCount = ordinal;
    ++stroke.acceptedMutationCount;
    CreativeEditorPlacementFeedback feedback;
    feedback.status = CreativeEditorPlacementFeedbackStatus::Placed;
    feedback.objectId = receipt.objectId;
    feedback.objectKind = receipt.objectKind;
    feedback.frameIndex = editor.frameIndex;
    feedback.voxelPlaced = receipt.voxelCreated;
    feedback.voxelCell = receipt.voxelCell;
    feedback.voxelBounds = receipt.worldBounds;
    editor.interaction.placementFeedback = feedback;
  } else {
    rejectMaterialStroke(editor, held.objectKind);
  }
}

void processMaterialStroke(cr::CreativeAppState& appState,
                           CreativeEditorState& editor,
                           const cr::CreativeHotbarEntry& held,
                           const cr::CreativeWorldActionFrame& actions,
                           std::uint64_t monotonicTimeNanoseconds) {
  CreativeMaterialStrokeState& stroke = editor.interaction.materialStroke;
  CreativeMaterialRepeatRequest repeatRequest;
  repeatRequest.nowNanoseconds = monotonicTimeNanoseconds;
  repeatRequest.primaryPressed = cr::creativeWorldActionPressed(
      actions, cr::CreativeWorldActionId::Primary);
  repeatRequest.primaryDown = cr::creativeWorldActionDown(
      actions, cr::CreativeWorldActionId::Primary);
  repeatRequest.primaryReleased = cr::creativeWorldActionReleased(
      actions, cr::CreativeWorldActionId::Primary);
  repeatRequest.secondaryPressed = cr::creativeWorldActionPressed(
      actions, cr::CreativeWorldActionId::Secondary);
  repeatRequest.secondaryDown = cr::creativeWorldActionDown(
      actions, cr::CreativeWorldActionId::Secondary);
  repeatRequest.secondaryReleased = cr::creativeWorldActionReleased(
      actions, cr::CreativeWorldActionId::Secondary);

  const CreativeMaterialRepeatResult repeat =
      stepCreativeMaterialRepeat(stroke.repeat, repeatRequest);
  stroke.repeat = repeat.next;
  if (repeat.finalized) {
    finalizeCreativeMaterialStroke(appState, editor,
                                   "creative_material_stroke_released");
    return;
  }
  if (repeat.mutationDue) {
    applyMaterialStrokeMutation(appState, editor, held, repeat.dueKind);
  }
}

void sampleTargetMaterial(InteractionContext& context) {
  CreativeEditorState& editor = context.request.editor;
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  if ((!target.objectHit && !target.voxelHit) ||
      target.objectKind == cr::CreativeObjectKind::Unknown) {
    return;
  }
  if (cr::creativeHeldItemIsVolumeOperation(context.held.kind)) {
    if (cr::creativeVolumeBrushSupported(target.objectKind)) {
      editor.placeBrush = target.objectKind;
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar).objectKind =
          target.objectKind;
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
  editor.interaction.placementFeedback = {
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected,
      cr::kInvalidObjectId, editor.placeBrush, editor.frameIndex};
}

void beginHeldShapeVolume(InteractionContext& context) {
  CreativeEditorState& editor = context.request.editor;
  const CreativeEditorVolumeGestureReceipt receipt =
      stepCreativeEditorVolumeGesture(
          editor.volume, CreativeEditorVolumeGestureAction::Begin,
          editor.interaction.target.grid.valid,
          editor.interaction.target.grid.targetCell);
  if (receipt.accepted) {
    editor.interaction.placementFeedback = {};
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

void applyHeldLinearArray(InteractionContext& context) {
  static_cast<void>(applyCreativeEditorLinearArrayWithHistory(
      context.request.appState, context.request.editor.pattern,
      context.request.editor.toolSettings, context.request.editor.placeCellSize,
      "minecraft_secondary_linear_array"));
}

constexpr std::array<HeldItemHandlerRow, cr::kCreativeHeldItemKindCount>
    kHeldItemHandlers{{
        {cr::CreativeHeldItemKind::Material,
         {noInteraction, noInteraction, sampleTargetMaterial}},
        {cr::CreativeHeldItemKind::ObjectSelect,
         {selectObject, noInteraction, sampleTargetMaterial}},
        {cr::CreativeHeldItemKind::ObjectMove,
         {noInteraction, noInteraction, sampleTargetMaterial}},
        {cr::CreativeHeldItemKind::VolumeSelect,
         {setVolumeFirstCorner, setVolumeSecondCorner,
          expandVolumeSelection}},
        {cr::CreativeHeldItemKind::VolumeFill,
         {beginHeldShapeVolume, commitHeldShapeVolume, sampleTargetMaterial},
         true},
        {cr::CreativeHeldItemKind::VolumeHollow,
         {beginHeldShapeVolume, commitHeldShapeVolume, sampleTargetMaterial},
         true},
        {cr::CreativeHeldItemKind::VolumeReplace,
         {noInteraction, applyHeldVolumeOperation, sampleTargetMaterial}},
        {cr::CreativeHeldItemKind::VolumeErase,
         {noInteraction, applyHeldVolumeOperation, sampleTargetMaterial}},
        {cr::CreativeHeldItemKind::VolumeClone,
         {noInteraction, applyHeldVolumeOperation, sampleTargetMaterial}},
        {cr::CreativeHeldItemKind::LinearArray,
         {selectObject, applyHeldLinearArray, sampleTargetMaterial}},
    }};
static_assert(heldItemRowsMatchEnumOrder(kHeldItemHandlers));

void processMoveInteraction(
    const CreativeEditorWorldInteractionFrameRequest& request) {
  CreativeEditorState& editor = request.editor;
  const bool pressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Primary);
  const bool down = cr::creativeWorldActionDown(
      request.actions, cr::CreativeWorldActionId::Primary);
  const bool released = cr::creativeWorldActionReleased(
      request.actions, cr::CreativeWorldActionId::Primary);
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
    editor.interaction.moveTargetId = editor.interaction.target.objectId;
    static_cast<void>(request.appState.facade.setActiveTool(cr::Tool::Move));
    static_cast<void>(request.appState.facade.dispatchToolInput(
        selectionPacket(editor.interaction.target,
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
    const cr::CreativeToolSettings& settings) {
  if (cr::creativeHeldItemUsesDirectShapeGesture(entry.kind)) {
    return shapeHotbarLabel(entry.kind, settings);
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

}  // namespace

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
    case cr::CreativeHeldItemKind::VolumeSelect:
    case cr::CreativeHeldItemKind::VolumeFill:
    case cr::CreativeHeldItemKind::VolumeHollow:
    case cr::CreativeHeldItemKind::VolumeReplace:
    case cr::CreativeHeldItemKind::VolumeErase:
    case cr::CreativeHeldItemKind::VolumeClone:
      return document.gridSettings().cellSizeMeters;
    case cr::CreativeHeldItemKind::ObjectSelect:
    case cr::CreativeHeldItemKind::ObjectMove:
    case cr::CreativeHeldItemKind::LinearArray:
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
  if (cr::creativeHeldItemUsesDirectShapeGesture(held.kind)) {
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
    return output;
  }
  if (held.kind == cr::CreativeHeldItemKind::Material ||
      cr::creativeHeldItemUsesMaterial(held.kind)) {
    output.append(" | ");
    output.append(cr::toString(held.objectKind));
  }
  return output;
}

void finalizeCreativeMaterialStroke(cr::CreativeAppState& appState,
                                    CreativeEditorState& editor,
                                    std::string_view reasonCode) {
  CreativeMaterialStrokeState& stroke = editor.interaction.materialStroke;
  if (!stroke.repeat.active && !stroke.transaction.active) {
    stroke = {};
    return;
  }
  StandaloneEditTransaction transaction = std::move(stroke.transaction);
  const bool changed = stroke.acceptedMutationCount > 0U;
  stroke = {};
  if (transaction.active) {
    static_cast<void>(completeEditTransaction(
        appState.history, std::move(transaction), appState.facade, changed,
        reasonCode));
  }
}

void processCreativeMaterialStrokeFrame(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    const cr::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind != cr::CreativeHeldItemKind::Material) {
    finalizeCreativeMaterialStroke(appState, editor,
                                   "creative_material_stroke_non_material_tool");
    return;
  }
  processMaterialStroke(appState, editor, held, actions,
                        monotonicTimeNanoseconds);
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
  const bool objectInReach = pick.objectId != cr::kInvalidObjectId &&
                             pick.entryDistance <= kCreativeReachMeters;
  const bool voxelIsNearest =
      voxelPick.hit &&
      (!objectInReach || voxelPick.distance <= pick.entryDistance);

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

  if (objectInReach) {
    const ObjectVisualPickBounds* candidate =
        findCandidate(pickFrame, pick.objectId);
    if (candidate != nullptr) {
      const iggy3d::Vec3 point =
          target.ray.origin + target.ray.direction * pick.entryDistance;
      const iggy3d::Vec3 normal = candidate->orientedBounds.has_value()
                                      ? orientedFaceNormal(
                                            *candidate->orientedBounds, point)
                                      : aabbFaceNormal(candidate->bounds, point);
      target.grid = cr::resolveCreativeGridTargetFromHit(
          cr::creativeVec3FromCore(point), cr::creativeVec3FromCore(normal),
          cellSize,
          gridSettings.origin,
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
  finalizeCreativeMaterialStroke(appState, editor,
                                 "creative_material_stroke_tool_changed");
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
      (held.kind == cr::CreativeHeldItemKind::Material ||
       cr::creativeHeldItemIsVolumeOperation(held.kind))) {
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
  static_cast<void>(appState.facade.setActiveTool(behavior.facadeTool));
}

bool selectCreativeEditorHotbarSlot(cr::CreativeAppState& appState,
                                    CreativeEditorState& editor,
                                    std::size_t slot) {
  const bool changed =
      cr::selectCreativeHotbarSlot(editor.interaction.hotbar, slot);
  syncCreativeEditorHeldItem(appState, editor);
  return changed;
}

bool confirmCreativeEditorHeldItem(cr::CreativeAppState& appState,
                                   CreativeEditorState& editor,
                                   std::string_view source) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind == cr::CreativeHeldItemKind::LinearArray) {
    return applyCreativeEditorLinearArrayWithHistory(
               appState, editor.pattern, editor.toolSettings,
               editor.placeCellSize, source)
        .accepted;
  }
  if (cr::creativeHeldItemUsesDirectShapeGesture(held.kind)) {
    return false;
  }
  if (!cr::creativeHeldItemIsVolumeOperation(held.kind)) {
    return false;
  }
  const cr::CreativeVolumeOperationReceipt receipt =
      applyCreativeEditorVolumeOperationWithHistory(
          appState, editor.volume, editor.placeBrush,
          cr::creativeVolumeOperationForHeldItem(held.kind),
          editor.toolSettings, source);
  return receipt.accepted;
}

bool cancelCreativeEditorHeldItem(cr::CreativeAppState& appState,
                                  CreativeEditorState& editor) {
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
  editor.volume.cursorValid = editor.interaction.target.grid.valid;
  if (editor.volume.cursorValid) {
    editor.volume.cursorCell = editor.interaction.target.grid.targetCell;
  }
  if (request.captureMode) {
    finalizeCreativeMaterialStroke(request.appState, editor,
                                   "creative_material_stroke_capture");
    return;
  }
  if (editor.transform.active) {
    finalizeCreativeMaterialStroke(request.appState, editor,
                                   "creative_material_stroke_transform");
    const bool secondaryPressed = cr::creativeWorldActionPressed(
        request.actions, cr::CreativeWorldActionId::Secondary);
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
    finalizeCreativeMaterialStroke(request.appState, editor,
                                   "creative_material_stroke_hotbar");
    if (cr::cycleCreativeHotbar(editor.interaction.hotbar, hotbarSteps)) {
      syncCreativeEditorHeldItem(request.appState, editor);
    }
  }

  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind == cr::CreativeHeldItemKind::Material) {
    InteractionContext context{request, held};
    processCreativeMaterialStrokeFrame(
        request.appState, editor, request.actions,
        request.monotonicTimeNanoseconds);
    if (cr::creativeWorldActionPressed(request.actions,
                                       cr::CreativeWorldActionId::Pick)) {
      sampleTargetMaterial(context);
    }
    return;
  }
  finalizeCreativeMaterialStroke(request.appState, editor,
                                 "creative_material_stroke_non_material_tool");
  if (held.kind == cr::CreativeHeldItemKind::ObjectMove) {
    processMoveInteraction(request);
    if (cr::creativeWorldActionPressed(request.actions,
                                       cr::CreativeWorldActionId::Pick)) {
      InteractionContext context{request, held};
      sampleTargetMaterial(context);
    }
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
                                  editor.toolSettings),
                      x + 4, hotbarY + 22, drawableWidth, drawableHeight,
                      selected);
  }

  if (!inventoryModalOpen && !editor.transform.active) {
    const std::string heldLabel = creativeEditorHeldItemStatusLabel(editor);
    const std::int32_t heldLabelX = std::max(
        4, static_cast<std::int32_t>(drawableWidth / 2U) -
               static_cast<std::int32_t>(heldLabel.size() * 4U));
    appendColoredText(glyphs, heldLabel, heldLabelX, hotbarY - 22,
                      drawableWidth, drawableHeight, false);
  }

  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (!inventoryModalOpen && !editor.transform.active &&
      editor.interaction.target.grid.valid &&
      held.kind != cr::CreativeHeldItemKind::Material) {
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
