#include "EditorInteraction.hpp"

#include <array>
#include <span>
#include <string_view>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorPlacement.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/tools/ShapeBrush.hpp"
#include "app/iggy3d/creative/tools/Volume.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

static_assert(cr::kCreativeMaterialBrushStampCapacity <=
              kCreativeMaterialStrokeVisitedCapacity);

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

[[nodiscard]] std::string_view strokeTransactionSource(
    cr::CreativeHeldItemKind heldItem,
    CreativeMaterialStrokeKind kind) noexcept {
  if (heldItem == cr::CreativeHeldItemKind::MaterialBrush) {
    return kind == CreativeMaterialStrokeKind::Remove
               ? "creative_material_brush_erase_stroke"
               : "creative_material_brush_paint_stroke";
  }
  return kind == CreativeMaterialStrokeKind::Remove
             ? "minecraft_primary_remove_stroke"
             : "minecraft_secondary_place_stroke";
}

[[nodiscard]] bool ensureMaterialStrokeTransaction(
    cr::CreativeAppState& appState,
    CreativeMaterialStrokeState& stroke,
    cr::CreativeHeldItemKind heldItem,
    CreativeMaterialStrokeKind kind) {
  if (stroke.transaction.active) {
    return true;
  }
  stroke.transaction = beginEditTransaction(
      appState.facade, strokeTransactionSource(heldItem, kind));
  return stroke.transaction.active;
}

void applySingleMaterialMutation(cr::CreativeAppState& appState,
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
    if (!ensureMaterialStrokeTransaction(appState, stroke, held.kind, kind)) {
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
  const CreativeBrushPlacementAdmission admission = admitBrushPlacement(
      held.objectKind, grid, editor.toolSettings.placementYaw);
  if (!admission.allowed || creativeBrushPlacementAlreadyExists(
                                appState.facade.document(), admission.plan)) {
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }
  if (strokeVisited(stroke, kind, grid.adjacentCell,
                    cr::kInvalidObjectId)) {
    return;
  }
  if (!ensureMaterialStrokeTransaction(appState, stroke, held.kind, kind)) {
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

[[nodiscard]] cr::CreativeMaterialBrushStampPlan materialBrushStampPlan(
    const CreativeEditorState& editor,
    CreativeMaterialStrokeKind kind) noexcept {
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  if (!target.grid.valid ||
      (kind == CreativeMaterialStrokeKind::Remove && !target.voxelHit)) {
    return {};
  }
  cr::CreativeMaterialBrushStampRequest request;
  request.shape = editor.toolSettings.materialBrushShape;
  request.size = editor.toolSettings.materialBrushSize;
  request.centerCell = kind == CreativeMaterialStrokeKind::Remove
                           ? target.voxelCell
                           : target.grid.adjacentCell;
  return cr::planCreativeMaterialBrushStamp(request);
}

void applyMaterialBrushMutation(cr::CreativeAppState& appState,
                                CreativeEditorState& editor,
                                const cr::CreativeHotbarEntry& held,
                                CreativeMaterialStrokeKind kind) {
  CreativeMaterialStrokeState& stroke = editor.interaction.materialStroke;
  if (stroke.capacityReached ||
      (kind == CreativeMaterialStrokeKind::Place &&
       !cr::creativeVolumeBrushSupported(held.objectKind))) {
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }
  const cr::CreativeMaterialBrushStampPlan plan =
      materialBrushStampPlan(editor, kind);
  if (!plan.accepted) {
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }

  std::array<cr::CreativeVoxelEdit, cr::kCreativeMaterialBrushStampCapacity>
      edits{};
  std::size_t editCount = 0U;
  const cr::CreativeObjectKind material =
      kind == CreativeMaterialStrokeKind::Remove
          ? cr::CreativeObjectKind::Unknown
          : held.objectKind;
  const cr::CreativeVoxelField& field = appState.facade.document().voxelField();
  for (cr::CreativeGridCoord3 cell : plan.generatedCells()) {
    if (strokeVisited(stroke, kind, cell, cr::kInvalidObjectId) ||
        field.materialAt(cell) == material) {
      continue;
    }
    edits[editCount++] = {cell, material};
  }
  if (editCount == 0U) {
    return;
  }
  const std::size_t remainingCapacity =
      stroke.visited.size() - stroke.visitedCount;
  if (editCount > remainingCapacity ||
      !ensureMaterialStrokeTransaction(appState, stroke, held.kind, kind)) {
    stroke.capacityReached = editCount > remainingCapacity;
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }

  const cr::CreativeVoxelMutationReceipt receipt =
      appState.facade.applyVoxelEdits({edits.data(), editCount});
  if (!receipt.accepted || !receipt.changed) {
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }
  for (std::size_t index = 0U; index < editCount; ++index) {
    static_cast<void>(rememberStrokeTarget(
        stroke, edits[index].cell, cr::kInvalidObjectId));
  }
  ++stroke.acceptedMutationCount;
  if (kind == CreativeMaterialStrokeKind::Remove) {
    editor.interaction.placementFeedback = {};
    return;
  }

  const cr::CreativeGridSettings grid =
      appState.facade.document().gridSettings();
  const cr::CreativeBounds minBounds = cr::creativeVolumeCellBounds(
      plan.minCell, grid.cellSizeMeters, grid.origin);
  const cr::CreativeBounds maxBounds = cr::creativeVolumeCellBounds(
      plan.maxCell, grid.cellSizeMeters, grid.origin);
  CreativeEditorPlacementFeedback feedback;
  feedback.status = CreativeEditorPlacementFeedbackStatus::Placed;
  feedback.objectKind = held.objectKind;
  feedback.frameIndex = editor.frameIndex;
  feedback.voxelPlaced = true;
  feedback.voxelCell = plan.centerCell;
  feedback.voxelBounds = {minBounds.min, maxBounds.max};
  editor.interaction.placementFeedback = feedback;
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
      actions, cr::CreativeWorldActionId::Primary) ||
      cr::creativeWorldActionPressed(actions, cr::CreativeWorldActionId::Reject);
  repeatRequest.primaryDown = cr::creativeWorldActionDown(
      actions, cr::CreativeWorldActionId::Primary) ||
      cr::creativeWorldActionDown(actions, cr::CreativeWorldActionId::Reject);
  repeatRequest.primaryReleased = cr::creativeWorldActionReleased(
      actions, cr::CreativeWorldActionId::Primary) ||
      cr::creativeWorldActionReleased(actions,
                                      cr::CreativeWorldActionId::Reject);
  repeatRequest.secondaryPressed = cr::creativeWorldActionPressed(
      actions, cr::CreativeWorldActionId::Secondary) ||
      cr::creativeWorldActionPressed(actions, cr::CreativeWorldActionId::Accept);
  repeatRequest.secondaryDown = cr::creativeWorldActionDown(
      actions, cr::CreativeWorldActionId::Secondary) ||
      cr::creativeWorldActionDown(actions, cr::CreativeWorldActionId::Accept);
  repeatRequest.secondaryReleased = cr::creativeWorldActionReleased(
      actions, cr::CreativeWorldActionId::Secondary) ||
      cr::creativeWorldActionReleased(actions,
                                      cr::CreativeWorldActionId::Accept);

  const CreativeMaterialRepeatResult repeat =
      stepCreativeMaterialRepeat(stroke.repeat, repeatRequest);
  stroke.repeat = repeat.next;
  if (repeat.finalized) {
    finalizeCreativeMaterialStroke(appState, editor,
                                   "creative_material_stroke_released");
    return;
  }
  if (!repeat.mutationDue) {
    return;
  }
  if (held.kind == cr::CreativeHeldItemKind::MaterialBrush) {
    applyMaterialBrushMutation(appState, editor, held, repeat.dueKind);
  } else {
    applySingleMaterialMutation(appState, editor, held, repeat.dueKind);
  }
}

}  // namespace

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
  if (held.kind != cr::CreativeHeldItemKind::Material &&
      held.kind != cr::CreativeHeldItemKind::MaterialBrush) {
    finalizeCreativeMaterialStroke(appState, editor,
                                   "creative_material_stroke_non_material_tool");
    return;
  }
  processMaterialStroke(appState, editor, held, actions,
                        monotonicTimeNanoseconds);
}

}  // namespace iggy3d_creative_app
