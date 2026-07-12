#include "EditorInteraction.hpp"

#include <algorithm>
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

struct MaterialBrushTargetSample {
  bool valid = false;
  cr::CreativeGridCoord3 center{};
};

[[nodiscard]] MaterialBrushTargetSample materialBrushTargetSample(
    const CreativeEditorState& editor,
    CreativeMaterialStrokeKind kind) noexcept {
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  if (!target.grid.valid ||
      (kind == CreativeMaterialStrokeKind::Remove && !target.voxelHit)) {
    return {};
  }
  return {true, kind == CreativeMaterialStrokeKind::Remove
                    ? target.voxelCell
                    : target.grid.adjacentCell};
}

[[nodiscard]] cr::CreativeMaterialBrushStampPlan materialBrushStampPlan(
    const CreativeEditorState& editor,
    cr::CreativeGridCoord3 center) noexcept {
  cr::CreativeMaterialBrushStampRequest request;
  request.shape = editor.toolSettings.materialBrushShape;
  request.size = editor.toolSettings.materialBrushSize;
  request.centerCell = center;
  return cr::planCreativeMaterialBrushStamp(request);
}

[[nodiscard]] bool editBatchContainsCell(
    const std::array<cr::CreativeVoxelEdit,
                     kCreativeMaterialStrokeVisitedCapacity>& edits,
    std::size_t editCount,
    cr::CreativeGridCoord3 cell) noexcept {
  for (std::size_t index = 0U; index < editCount; ++index) {
    if (sameCell(edits[index].cell, cell)) {
      return true;
    }
  }
  return false;
}

void includeStampBounds(cr::CreativeGridCoord3 stampMin,
                        cr::CreativeGridCoord3 stampMax,
                        bool& initialized,
                        cr::CreativeGridCoord3& aggregateMin,
                        cr::CreativeGridCoord3& aggregateMax) noexcept {
  if (!initialized) {
    initialized = true;
    aggregateMin = stampMin;
    aggregateMax = stampMax;
    return;
  }
  aggregateMin.x = std::min(aggregateMin.x, stampMin.x);
  aggregateMin.y = std::min(aggregateMin.y, stampMin.y);
  aggregateMin.z = std::min(aggregateMin.z, stampMin.z);
  aggregateMax.x = std::max(aggregateMax.x, stampMax.x);
  aggregateMax.y = std::max(aggregateMax.y, stampMax.y);
  aggregateMax.z = std::max(aggregateMax.z, stampMax.z);
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
  const MaterialBrushTargetSample sample =
      materialBrushTargetSample(editor, kind);
  if (!sample.valid) {
    stroke.hasLastBrushCenter = false;
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }
  cr::CreativeMaterialBrushPathRequest pathRequest;
  pathRequest.fromCell =
      stroke.hasLastBrushCenter ? stroke.lastBrushCenter : sample.center;
  pathRequest.toCell = sample.center;
  const cr::CreativeMaterialBrushPathPlan path =
      cr::planCreativeMaterialBrushPath(pathRequest);
  stroke.hasLastBrushCenter = true;
  stroke.lastBrushCenter = sample.center;
  if (!path.accepted) {
    stroke.capacityReached =
        path.status == cr::CreativeMaterialBrushPathStatus::CapacityExceeded;
    rejectMaterialStroke(editor, held.objectKind);
    return;
  }

  std::array<cr::CreativeVoxelEdit,
             kCreativeMaterialStrokeVisitedCapacity>
      edits{};
  std::size_t editCount = 0U;
  const std::size_t remainingCapacity =
      stroke.visited.size() - stroke.visitedCount;
  const cr::CreativeObjectKind material =
      kind == CreativeMaterialStrokeKind::Remove
          ? cr::CreativeObjectKind::Unknown
          : held.objectKind;
  const cr::CreativeVoxelField& field = appState.facade.document().voxelField();
  bool boundsInitialized = false;
  cr::CreativeGridCoord3 aggregateMin{};
  cr::CreativeGridCoord3 aggregateMax{};
  for (cr::CreativeGridCoord3 center : path.generatedCenters()) {
    const cr::CreativeMaterialBrushStampPlan stamp =
        materialBrushStampPlan(editor, center);
    if (!stamp.accepted) {
      rejectMaterialStroke(editor, held.objectKind);
      return;
    }
    includeStampBounds(stamp.minCell, stamp.maxCell, boundsInitialized,
                       aggregateMin, aggregateMax);
    for (cr::CreativeGridCoord3 cell : stamp.generatedCells()) {
      const cr::CreativeObjectKind currentMaterial = field.materialAt(cell);
      const bool maskAllows =
          kind == CreativeMaterialStrokeKind::Remove ||
          cr::creativeMaterialBrushMaskAllows(
              editor.toolSettings.materialBrushMask,
              currentMaterial != cr::CreativeObjectKind::Unknown);
      if (strokeVisited(stroke, kind, cell, cr::kInvalidObjectId) ||
          editBatchContainsCell(edits, editCount, cell) ||
          currentMaterial == material || !maskAllows) {
        continue;
      }
      if (editCount >= remainingCapacity) {
        stroke.capacityReached = true;
        rejectMaterialStroke(editor, held.objectKind);
        return;
      }
      edits[editCount++] = {cell, material};
    }
  }
  if (editCount == 0U) {
    return;
  }
  if (!ensureMaterialStrokeTransaction(appState, stroke, held.kind, kind)) {
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
    stroke.visited[stroke.visitedCount++] = {
        edits[index].cell, cr::kInvalidObjectId};
  }
  ++stroke.acceptedMutationCount;
  if (kind == CreativeMaterialStrokeKind::Remove) {
    editor.interaction.placementFeedback = {};
    return;
  }

  const cr::CreativeGridSettings grid =
      appState.facade.document().gridSettings();
  const cr::CreativeBounds minBounds = cr::creativeVolumeCellBounds(
      aggregateMin, grid.cellSizeMeters, grid.origin);
  const cr::CreativeBounds maxBounds = cr::creativeVolumeCellBounds(
      aggregateMax, grid.cellSizeMeters, grid.origin);
  CreativeEditorPlacementFeedback feedback;
  feedback.status = CreativeEditorPlacementFeedbackStatus::Placed;
  feedback.objectKind = held.objectKind;
  feedback.frameIndex = editor.frameIndex;
  feedback.voxelPlaced = true;
  feedback.voxelCell = sample.center;
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
  if (held.kind != cr::CreativeHeldItemKind::MaterialBrush) {
    stroke.hasLastBrushCenter = false;
  } else if (stroke.repeat.active &&
             !materialBrushTargetSample(editor, stroke.repeat.kind).valid) {
    stroke.hasLastBrushCenter = false;
  }
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
