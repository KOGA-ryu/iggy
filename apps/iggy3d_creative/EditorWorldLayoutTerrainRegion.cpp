#include "EditorWorldLayoutTopography.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] bool terrainCellFromPoint(
    double xCells,
    double zCells,
    cr::CreativeTerrainCoord2& output) noexcept {
  if (!std::isfinite(xCells) || !std::isfinite(zCells)) {
    return false;
  }
  const double x = std::floor(xCells);
  const double z = std::floor(zCells);
  if (x < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      x > static_cast<double>(std::numeric_limits<std::int32_t>::max()) ||
      z < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      z > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)};
  return true;
}

[[nodiscard]] bool terrainRegionBoundsFromEdges(
    std::int64_t minimumX,
    std::int64_t minimumZ,
    std::int64_t maximumXExclusive,
    std::int64_t maximumZExclusive,
    cr::CreativeTerrainHeightFieldBounds& output) noexcept {
  const std::int64_t width = maximumXExclusive - minimumX;
  const std::int64_t depth = maximumZExclusive - minimumZ;
  if (minimumX < std::numeric_limits<std::int32_t>::min() ||
      minimumX > std::numeric_limits<std::int32_t>::max() ||
      minimumZ < std::numeric_limits<std::int32_t>::min() ||
      minimumZ > std::numeric_limits<std::int32_t>::max() || width <= 0 ||
      depth <= 0 || width > std::numeric_limits<std::uint16_t>::max() ||
      depth > std::numeric_limits<std::uint16_t>::max()) {
    return false;
  }
  output = {{static_cast<std::int32_t>(minimumX),
             static_cast<std::int32_t>(minimumZ)},
            static_cast<std::uint16_t>(width),
            static_cast<std::uint16_t>(depth)};
  return cr::isValidCreativeTerrainHeightFieldBounds(output);
}

[[nodiscard]] bool terrainRegionBounds(
    cr::CreativeTerrainCoord2 first,
    cr::CreativeTerrainCoord2 second,
    cr::CreativeTerrainHeightFieldBounds& output) noexcept {
  const std::int64_t minimumX = std::min(first.x, second.x);
  const std::int64_t minimumZ = std::min(first.z, second.z);
  const std::int64_t maximumXExclusive =
      static_cast<std::int64_t>(std::max(first.x, second.x)) + 1;
  const std::int64_t maximumZExclusive =
      static_cast<std::int64_t>(std::max(first.z, second.z)) + 1;
  return terrainRegionBoundsFromEdges(minimumX, minimumZ, maximumXExclusive,
                                      maximumZExclusive, output);
}

[[nodiscard]] bool handleMovesMinimumX(
    CreativeEditorWorldLayoutTerrainRegionHandle handle) noexcept {
  return handle == CreativeEditorWorldLayoutTerrainRegionHandle::MinimumX ||
         handle ==
             CreativeEditorWorldLayoutTerrainRegionHandle::MinimumXMinimumZ ||
         handle ==
             CreativeEditorWorldLayoutTerrainRegionHandle::MinimumXMaximumZ;
}

[[nodiscard]] bool handleMovesMaximumX(
    CreativeEditorWorldLayoutTerrainRegionHandle handle) noexcept {
  return handle == CreativeEditorWorldLayoutTerrainRegionHandle::MaximumX ||
         handle ==
             CreativeEditorWorldLayoutTerrainRegionHandle::MaximumXMinimumZ ||
         handle ==
             CreativeEditorWorldLayoutTerrainRegionHandle::MaximumXMaximumZ;
}

[[nodiscard]] bool handleMovesMinimumZ(
    CreativeEditorWorldLayoutTerrainRegionHandle handle) noexcept {
  return handle == CreativeEditorWorldLayoutTerrainRegionHandle::MinimumZ ||
         handle ==
             CreativeEditorWorldLayoutTerrainRegionHandle::MinimumXMinimumZ ||
         handle ==
             CreativeEditorWorldLayoutTerrainRegionHandle::MaximumXMinimumZ;
}

[[nodiscard]] bool handleMovesMaximumZ(
    CreativeEditorWorldLayoutTerrainRegionHandle handle) noexcept {
  return handle == CreativeEditorWorldLayoutTerrainRegionHandle::MaximumZ ||
         handle ==
             CreativeEditorWorldLayoutTerrainRegionHandle::MinimumXMaximumZ ||
         handle ==
             CreativeEditorWorldLayoutTerrainRegionHandle::MaximumXMaximumZ;
}

}  // namespace

bool setCreativeEditorWorldLayoutTerrainRegionBounds(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    cr::CreativeTerrainHeightFieldBounds bounds) noexcept {
  if (!cr::isValidCreativeTerrainHeightFieldBounds(bounds)) {
    return false;
  }
  const bool changed = !state.regionValid || state.recipe.bounds != bounds;
  state.recipe.bounds = bounds;
  state.regionValid = true;
  return changed;
}

bool beginCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept {
  cr::CreativeTerrainCoord2 coord;
  if (!state.editingEnabled || !terrainCellFromPoint(xCells, zCells, coord)) {
    return false;
  }
  cr::CreativeTerrainHeightFieldBounds bounds;
  if (!terrainRegionBounds(coord, coord, bounds)) {
    return false;
  }
  state.selecting = true;
  state.manipulation = {};
  state.editingOperationId = cr::kInvalidCreativeTerrainOperationId;
  state.anchor = coord;
  state.cursor = coord;
  static_cast<void>(setCreativeEditorWorldLayoutTerrainRegionBounds(state,
                                                                    bounds));
  return true;
}

bool updateCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept {
  cr::CreativeTerrainCoord2 coord;
  if (!state.selecting || !terrainCellFromPoint(xCells, zCells, coord)) {
    return false;
  }
  cr::CreativeTerrainHeightFieldBounds bounds;
  if (!terrainRegionBounds(state.anchor, coord, bounds)) {
    state.regionValid = false;
    return false;
  }
  state.cursor = coord;
  static_cast<void>(setCreativeEditorWorldLayoutTerrainRegionBounds(state,
                                                                    bounds));
  return true;
}

bool finishCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept {
  if (!state.selecting) {
    return false;
  }
  static_cast<void>(updateCreativeEditorWorldLayoutTerrainRegion(
      state, xCells, zCells));
  state.selecting = false;
  return state.regionValid;
}

void clearCreativeEditorWorldLayoutTerrainRegionSelection(
    CreativeEditorWorldLayoutTerrainRegionState& state) noexcept {
  state.selecting = false;
  state.regionValid = false;
  state.anchor = {};
  state.cursor = {};
  state.manipulation = {};
  state.recipe.bounds = {{0, 0}, 1U, 1U};
}

CreativeEditorWorldLayoutTerrainRegionHandle
hitCreativeEditorWorldLayoutTerrainRegionHandle(
    const CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells,
    double toleranceCells) noexcept {
  if (!state.regionValid || state.selecting ||
      !std::isfinite(xCells) || !std::isfinite(zCells) ||
      !std::isfinite(toleranceCells) || toleranceCells < 0.0) {
    return CreativeEditorWorldLayoutTerrainRegionHandle::None;
  }
  const cr::CreativeTerrainHeightFieldBounds bounds = state.recipe.bounds;
  const double minimumX = bounds.minimum.x;
  const double minimumZ = bounds.minimum.z;
  const double maximumX = minimumX + bounds.widthCells;
  const double maximumZ = minimumZ + bounds.depthCells;
  const bool nearMinimumX = std::abs(xCells - minimumX) <= toleranceCells;
  const bool nearMaximumX = std::abs(xCells - maximumX) <= toleranceCells;
  const bool nearMinimumZ = std::abs(zCells - minimumZ) <= toleranceCells;
  const bool nearMaximumZ = std::abs(zCells - maximumZ) <= toleranceCells;
  if (nearMinimumX && nearMinimumZ) {
    return CreativeEditorWorldLayoutTerrainRegionHandle::MinimumXMinimumZ;
  }
  if (nearMaximumX && nearMinimumZ) {
    return CreativeEditorWorldLayoutTerrainRegionHandle::MaximumXMinimumZ;
  }
  if (nearMinimumX && nearMaximumZ) {
    return CreativeEditorWorldLayoutTerrainRegionHandle::MinimumXMaximumZ;
  }
  if (nearMaximumX && nearMaximumZ) {
    return CreativeEditorWorldLayoutTerrainRegionHandle::MaximumXMaximumZ;
  }
  const bool insideX = xCells >= minimumX - toleranceCells &&
                       xCells <= maximumX + toleranceCells;
  const bool insideZ = zCells >= minimumZ - toleranceCells &&
                       zCells <= maximumZ + toleranceCells;
  if (nearMinimumX && insideZ) {
    return CreativeEditorWorldLayoutTerrainRegionHandle::MinimumX;
  }
  if (nearMaximumX && insideZ) {
    return CreativeEditorWorldLayoutTerrainRegionHandle::MaximumX;
  }
  if (nearMinimumZ && insideX) {
    return CreativeEditorWorldLayoutTerrainRegionHandle::MinimumZ;
  }
  if (nearMaximumZ && insideX) {
    return CreativeEditorWorldLayoutTerrainRegionHandle::MaximumZ;
  }
  if (xCells >= minimumX && xCells <= maximumX && zCells >= minimumZ &&
      zCells <= maximumZ) {
    return CreativeEditorWorldLayoutTerrainRegionHandle::Body;
  }
  return CreativeEditorWorldLayoutTerrainRegionHandle::None;
}

bool beginCreativeEditorWorldLayoutTerrainRegionManipulation(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorWorldLayoutTerrainRegionHandle handle,
    double xCells,
    double zCells) noexcept {
  cr::CreativeTerrainCoord2 pointer;
  if (!state.editingEnabled || !state.regionValid || state.selecting ||
      handle == CreativeEditorWorldLayoutTerrainRegionHandle::None ||
      handle >= CreativeEditorWorldLayoutTerrainRegionHandle::Count ||
      !terrainCellFromPoint(xCells, zCells, pointer)) {
    return false;
  }
  state.manipulation.active = true;
  state.manipulation.changed = false;
  state.manipulation.handle = handle;
  state.manipulation.pointerStart = pointer;
  state.manipulation.boundsStart = state.recipe.bounds;
  return true;
}

bool updateCreativeEditorWorldLayoutTerrainRegionManipulation(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept {
  cr::CreativeTerrainCoord2 pointer;
  if (!state.manipulation.active ||
      !terrainCellFromPoint(xCells, zCells, pointer)) {
    return false;
  }
  const CreativeEditorWorldLayoutTerrainRegionManipulation& manipulation =
      state.manipulation;
  const std::int64_t deltaX =
      static_cast<std::int64_t>(pointer.x) - manipulation.pointerStart.x;
  const std::int64_t deltaZ =
      static_cast<std::int64_t>(pointer.z) - manipulation.pointerStart.z;
  const cr::CreativeTerrainHeightFieldBounds start = manipulation.boundsStart;
  std::int64_t minimumX = start.minimum.x;
  std::int64_t minimumZ = start.minimum.z;
  std::int64_t maximumX = minimumX + start.widthCells;
  std::int64_t maximumZ = minimumZ + start.depthCells;
  if (manipulation.handle ==
      CreativeEditorWorldLayoutTerrainRegionHandle::Body) {
    minimumX += deltaX;
    maximumX += deltaX;
    minimumZ += deltaZ;
    maximumZ += deltaZ;
  } else {
    minimumX += handleMovesMinimumX(manipulation.handle) ? deltaX : 0;
    maximumX += handleMovesMaximumX(manipulation.handle) ? deltaX : 0;
    minimumZ += handleMovesMinimumZ(manipulation.handle) ? deltaZ : 0;
    maximumZ += handleMovesMaximumZ(manipulation.handle) ? deltaZ : 0;
  }
  cr::CreativeTerrainHeightFieldBounds candidate;
  if (!terrainRegionBoundsFromEdges(minimumX, minimumZ, maximumX, maximumZ,
                                    candidate)) {
    return false;
  }
  const bool changed = state.recipe.bounds != candidate;
  static_cast<void>(setCreativeEditorWorldLayoutTerrainRegionBounds(state,
                                                                    candidate));
  state.manipulation.changed =
      state.recipe.bounds != state.manipulation.boundsStart;
  return changed;
}

bool finishCreativeEditorWorldLayoutTerrainRegionManipulation(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept {
  if (!state.manipulation.active) {
    return false;
  }
  static_cast<void>(updateCreativeEditorWorldLayoutTerrainRegionManipulation(
      state, xCells, zCells));
  const bool changed =
      state.recipe.bounds != state.manipulation.boundsStart;
  state.manipulation = {};
  return changed;
}

bool cancelCreativeEditorWorldLayoutTerrainRegionManipulation(
    CreativeEditorWorldLayoutTerrainRegionState& state) noexcept {
  if (!state.manipulation.active) {
    return false;
  }
  const bool changed =
      state.recipe.bounds != state.manipulation.boundsStart;
  static_cast<void>(setCreativeEditorWorldLayoutTerrainRegionBounds(
      state, state.manipulation.boundsStart));
  state.manipulation = {};
  return changed;
}

CreativeEditorWorldLayoutTerrainRegionRecipePlan
planCreativeEditorWorldLayoutTerrainRegion(
    const CreativeEditorWorldLayoutTerrainRegionState& state) noexcept {
  CreativeEditorWorldLayoutTerrainRegionRecipePlan plan;
  plan.requested = true;
  if (!state.editingEnabled || !state.regionValid ||
      !cr::isValidCreativeTerrainRegionRecipe(state.recipe)) {
    plan.reasonCode =
        "creative_editor_world_layout_terrain_region_request_invalid";
    return plan;
  }
  plan.recipe = state.recipe;
  plan.accepted = true;
  plan.reasonCode = "creative_editor_world_layout_terrain_region_ready";
  return plan;
}

bool selectCreativeEditorWorldLayoutTerrainRegionOperation(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorTerrainGenerationState& terrainGeneration,
    const cr::CreativeDocument& document,
    cr::CreativeTerrainOperationId operationId) {
  const cr::CreativeTerrainOperation* operation =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       operationId);
  if (operation == nullptr ||
      operation->kind != cr::CreativeTerrainOperationKind::Region ||
      !cr::isValidCreativeTerrainRegionRecipe(operation->region) ||
      (!selectCreativeEditorTerrainOperation(terrainGeneration, document,
                                            operationId) &&
       (terrainGeneration.editingOperationId != operationId ||
        terrainGeneration.operationKind !=
            cr::CreativeTerrainOperationKind::Region))) {
    state.statusMessage = "Terrain region operation unavailable";
    return false;
  }

  state.editingEnabled = true;
  state.selecting = false;
  state.regionValid = true;
  state.ownsPreview = false;
  state.manipulation = {};
  state.editingOperationId = operationId;
  state.recipe = operation->region;
  static_cast<void>(setCreativeEditorWorldLayoutTerrainRegionBounds(
      state, operation->region.bounds));
  state.anchor = operation->region.bounds.minimum;
  state.cursor = {
      static_cast<std::int32_t>(
          static_cast<std::int64_t>(operation->region.bounds.minimum.x) +
          operation->region.bounds.widthCells - 1),
      static_cast<std::int32_t>(
          static_cast<std::int64_t>(operation->region.bounds.minimum.z) +
          operation->region.bounds.depthCells - 1),
  };
  state.statusMessage = "Terrain region selected";
  return true;
}

CreativeEditorTerrainGenerationPreviewReceipt
previewCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorTerrainGenerationState& terrainGeneration,
    const cr::CreativeDocument& document) {
  CreativeEditorTerrainGenerationPreviewReceipt receipt;
  receipt.requested = true;
  const CreativeEditorWorldLayoutTerrainRegionRecipePlan plan =
      planCreativeEditorWorldLayoutTerrainRegion(state);
  if (!plan.accepted) {
    if (state.ownsPreview) {
      static_cast<void>(cancelCreativeEditorTerrainGeneration(
          terrainGeneration, "Terrain region preview rejected"));
      state.ownsPreview = false;
    }
    receipt.reasonCode = plan.reasonCode;
    state.statusMessage = "Terrain region preview rejected";
    return receipt;
  }
  if (terrainGeneration.previewActive && !state.ownsPreview) {
    receipt.reasonCode =
        "creative_editor_world_layout_terrain_region_preview_owned_elsewhere";
    state.statusMessage = "Another terrain preview is active";
    return receipt;
  }

  terrainGeneration.operationKind = cr::CreativeTerrainOperationKind::Region;
  terrainGeneration.editingOperationId = state.editingOperationId;
  terrainGeneration.regionRecipe = plan.recipe;
  terrainGeneration.draftDirty = true;
  receipt = previewCreativeEditorTerrainGeneration(
      terrainGeneration, document, false);
  state.ownsPreview = receipt.accepted;
  if (!receipt.accepted) {
    static_cast<void>(cancelCreativeEditorTerrainGeneration(
        terrainGeneration, "Terrain region preview rejected"));
  }
  state.statusMessage = receipt.accepted ? "Terrain region preview ready"
                                         : "Terrain region preview rejected";
  return receipt;
}

CreativeEditorTerrainGenerationApplyReceipt
applyCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorTerrainGenerationState& terrainGeneration,
    cr::CreativeAppState& appState) {
  CreativeEditorTerrainGenerationApplyReceipt receipt;
  receipt.requested = true;
  if (!state.ownsPreview) {
    receipt.reasonCode =
        "creative_editor_world_layout_terrain_region_preview_inactive";
    state.statusMessage = "No terrain region preview to apply";
    return receipt;
  }
  receipt = applyCreativeEditorTerrainGeneration(appState, terrainGeneration);
  if (receipt.accepted) {
    state.ownsPreview = false;
    state.editingOperationId = cr::kInvalidCreativeTerrainOperationId;
    clearCreativeEditorWorldLayoutTerrainRegionSelection(state);
  }
  state.statusMessage = receipt.accepted ? "Terrain region applied"
                                         : "Terrain region apply failed";
  return receipt;
}

bool cancelCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorTerrainGenerationState& terrainGeneration,
    std::string_view reason) {
  const bool changed = state.selecting || state.regionValid ||
                       state.manipulation.active || state.ownsPreview;
  if (state.ownsPreview) {
    static_cast<void>(cancelCreativeEditorTerrainGeneration(
        terrainGeneration, reason));
  }
  state.ownsPreview = false;
  state.editingOperationId = cr::kInvalidCreativeTerrainOperationId;
  clearCreativeEditorWorldLayoutTerrainRegionSelection(state);
  state.statusMessage = reason.empty() ? "Terrain region canceled"
                                       : std::string(reason);
  return changed;
}

bool synchronizeCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    const cr::CreativeDocument& document) {
  if (!state.ownsPreview ||
      creativeEditorTerrainGenerationPreviewMatches(terrainGeneration,
                                                     document)) {
    return false;
  }
  state.ownsPreview = false;
  state.editingOperationId = cr::kInvalidCreativeTerrainOperationId;
  clearCreativeEditorWorldLayoutTerrainRegionSelection(state);
  state.statusMessage = "Terrain region preview canceled: document changed";
  return true;
}

}  // namespace iggy3d_creative_app
