#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void invalidateRegionPreview(CreativeTerrainRegionState& region) noexcept {
  region.preview.valid = false;
  region.preview.renderAccepted = false;
  region.preview.operationPreview = {};
  region.preview.patches.clear();
}

void setRegionFeedback(CreativeEditorState& editor, bool accepted) noexcept {
  setCreativeEditorPlacementFeedback(
      editor.interaction,
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex);
}

[[nodiscard]] bool regionBounds(
    const cr::CreativeVolumeSelection& selection,
    cr::CreativeTerrainHeightFieldBounds& output) noexcept {
  if (!cr::creativeVolumeSelectionValid(selection)) {
    return false;
  }
  const std::int64_t minimumX =
      std::min(selection.firstCell.x, selection.secondCell.x);
  const std::int64_t minimumZ =
      std::min(selection.firstCell.z, selection.secondCell.z);
  const std::uint64_t width = static_cast<std::uint64_t>(
      std::max(selection.firstCell.x, selection.secondCell.x) - minimumX) +
                              1U;
  const std::uint64_t depth = static_cast<std::uint64_t>(
      std::max(selection.firstCell.z, selection.secondCell.z) - minimumZ) +
                              1U;
  if (width > std::numeric_limits<std::uint16_t>::max() ||
      depth > std::numeric_limits<std::uint16_t>::max() ||
      width * depth > cr::kCreativeTerrainHeightFieldCellCapacity) {
    return false;
  }
  output = {{static_cast<std::int32_t>(minimumX),
             static_cast<std::int32_t>(minimumZ)},
            static_cast<std::uint16_t>(width),
            static_cast<std::uint16_t>(depth)};
  return cr::isValidCreativeTerrainHeightFieldBounds(output);
}

[[nodiscard]] cr::CreativeTerrainOperationMutationRequest regionRequest(
    const cr::CreativeDocument& document,
    const CreativeTerrainRegionState& state,
    const cr::CreativeTerrainRegionRecipe& recipe) {
  cr::CreativeTerrainOperationMutationRequest request;
  const cr::CreativeTerrainOperation* existing =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       state.editingOperationId);
  request.kind = existing == nullptr
                     ? cr::CreativeTerrainOperationMutationKind::Add
                     : cr::CreativeTerrainOperationMutationKind::Update;
  request.operationId = existing == nullptr
                            ? cr::kInvalidCreativeTerrainOperationId
                            : state.editingOperationId;
  request.operationKind = cr::CreativeTerrainOperationKind::Region;
  request.region = recipe;
  request.enabled = existing == nullptr ? true : existing->enabled;
  return request;
}

[[nodiscard]] bool previewKeyMatches(
    const CreativeTerrainRegionPreviewCache& cache,
    const cr::CreativeDocument& document,
    cr::CreativeTerrainOperationId editingOperationId,
    const cr::CreativeTerrainRegionRecipe& recipe) noexcept {
  return cache.valid && cache.documentId == document.id() &&
         cache.documentRevision == document.revision() &&
         cache.editingOperationId == editingOperationId &&
         cache.recipe == recipe;
}

[[nodiscard]] std::uint16_t terrainHeightAt(
    const cr::CreativeDocument& document,
    cr::CreativeTerrainCoord2 coord) noexcept {
  const std::optional<std::uint16_t> authored =
      document.terrainHeightField().heightAt(coord);
  if (authored.has_value()) {
    return *authored;
  }
  const cr::CreativeTerrainHeightSample legacy =
      cr::sampleCreativeTerrainHeight(document.terrainField(), coord);
  return legacy.present ? legacy.heightCells : 0U;
}

[[nodiscard]] bool stepRegionValue(std::uint16_t& value,
                                   std::uint16_t minimum,
                                   std::uint16_t maximum,
                                   int direction) noexcept {
  const std::uint16_t before = value;
  if (direction > 0 && value < maximum) {
    ++value;
  } else if (direction < 0 && value > minimum) {
    --value;
  }
  return value != before;
}

[[nodiscard]] bool cycleRegionMode(
    cr::CreativeTerrainRegionMode& mode,
    int direction) noexcept {
  const int count = static_cast<int>(cr::CreativeTerrainRegionMode::Count);
  const int before = static_cast<int>(mode);
  const int after = (before + (direction > 0 ? 1 : count - 1)) % count;
  mode = static_cast<cr::CreativeTerrainRegionMode>(after);
  return after != before;
}

[[nodiscard]] bool regionContainsCoord(
    const cr::CreativeTerrainRegionRecipe& recipe,
    cr::CreativeTerrainCoord2 coord) noexcept {
  const std::int64_t localX =
      static_cast<std::int64_t>(coord.x) - recipe.bounds.minimum.x;
  const std::int64_t localZ =
      static_cast<std::int64_t>(coord.z) - recipe.bounds.minimum.z;
  if (localX < 0 || localZ < 0 || localX >= recipe.bounds.widthCells ||
      localZ >= recipe.bounds.depthCells) {
    return false;
  }
  return cr::creativeTerrainCompositionMaskWeight(
             recipe.mask, static_cast<std::uint16_t>(localX),
             static_cast<std::uint16_t>(localZ), recipe.bounds,
             recipe.featherCells) > 0U;
}

[[nodiscard]] bool selectionForRegion(
    const cr::CreativeDocument& document,
    const cr::CreativeTerrainRegionRecipe& recipe,
    cr::CreativeVolumeSelection& output) noexcept {
  const std::int64_t maximumX =
      static_cast<std::int64_t>(recipe.bounds.minimum.x) +
      recipe.bounds.widthCells;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(recipe.bounds.minimum.z) +
      recipe.bounds.depthCells;
  if (maximumX > std::numeric_limits<std::int32_t>::max() ||
      maximumZ > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  const cr::CreativeGridSettings grid = document.gridSettings();
  if (!std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0 ||
      !cr::isFiniteCreativeVec3(grid.origin)) {
    return false;
  }
  cr::CreativeVolumeSelection candidate;
  candidate.origin = grid.origin;
  candidate.cellSize = grid.cellSizeMeters;
  if (!cr::setCreativeVolumeSelectionGridBounds(
          candidate,
          {{recipe.bounds.minimum.x, 0, recipe.bounds.minimum.z},
           {static_cast<std::int32_t>(maximumX), 1,
            static_cast<std::int32_t>(maximumZ)}})) {
    return false;
  }
  output = candidate;
  return true;
}

}  // namespace

bool buildCreativeEditorTerrainRegionRecipe(
    const CreativeEditorState& editor,
    const cr::CreativeVolumeSelection& selection,
    cr::CreativeTerrainRegionRecipe& output) noexcept {
  cr::CreativeTerrainHeightFieldBounds bounds;
  if (!regionBounds(selection, bounds)) {
    return false;
  }
  output = editor.toolSettings.terrainRegionRecipe;
  output.bounds = bounds;
  return cr::isValidCreativeTerrainRegionRecipe(output);
}

cr::CreativeTerrainOperationMutationPlan planCreativeEditorTerrainRegion(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    const cr::CreativeVolumeSelection& selection) noexcept {
  cr::CreativeTerrainRegionRecipe recipe;
  if (!buildCreativeEditorTerrainRegionRecipe(editor, selection, recipe)) {
    return {};
  }
  return cr::planCreativeTerrainOperationMutation(
      document.terrainField(), document.terrainHeightField(),
      document.terrainMaterialField(), document.terrainOperationStack(),
      regionRequest(document, editor.terrain.region, recipe));
}

CreativeEditorTerrainRegionReceipt
applyCreativeEditorTerrainRegionWithHistory(cr::CreativeAppState& appState,
                                            CreativeEditorState& editor,
                                            std::string_view source) {
  CreativeEditorTerrainRegionReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainRegionAction::Apply;
  const cr::CreativeDocument& document = appState.facade.document();
  cr::CreativeTerrainRegionRecipe recipe;
  if (!buildCreativeEditorTerrainRegionRecipe(
          editor, editor.volume.selection, recipe)) {
    receipt.reasonCode = "creative_editor_terrain_region_selection_invalid";
    setRegionFeedback(editor, false);
    return receipt;
  }
  receipt.plan = planCreativeEditorTerrainRegion(
      document, editor, editor.volume.selection);
  if (!receipt.plan.receipt.accepted) {
    receipt.reasonCode = receipt.plan.receipt.reasonCode;
    setRegionFeedback(editor, false);
    return receipt;
  }

  const cr::CreativeTerrainOperationMutationRequest request =
      regionRequest(document, editor.terrain.region, recipe);
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  receipt.operation = appState.facade.applyTerrainOperationMutation(request);
  receipt.history = completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.operation.accepted && receipt.operation.changed,
      receipt.operation.reasonCode);
  receipt.changed = receipt.operation.changed;
  receipt.accepted = receipt.operation.accepted &&
                     (!receipt.changed || receipt.history.accepted);
  receipt.reasonCode = !receipt.operation.accepted
                           ? receipt.operation.reasonCode
                       : receipt.changed && !receipt.history.accepted
                           ? receipt.history.reasonCode
                           : "creative_editor_terrain_region_applied";
  if (receipt.operation.accepted) {
    editor.terrain.region.editingOperationId = receipt.operation.operationId;
    invalidateRegionPreview(editor.terrain.region);
  }
  setRegionFeedback(editor, receipt.accepted);
  return receipt;
}

CreativeEditorTerrainRegionReceipt
selectCreativeEditorTerrainRegionOperationAtPointer(
    const cr::CreativeDocument& document,
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainRegionReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainRegionAction::SelectOperation;
  cr::CreativeTerrainCoord2 target{};
  if (!resolveCreativeEditorTerrainPointerCoord(editor, target)) {
    receipt.reasonCode =
        "creative_editor_terrain_region_select_target_invalid";
    return receipt;
  }

  const cr::CreativeTerrainOperation* selected = nullptr;
  for (auto operation = document.terrainOperationStack().operations.rbegin();
       operation != document.terrainOperationStack().operations.rend();
       ++operation) {
    if (!operation->enabled ||
        operation->owner != cr::CreativeTerrainOperationOwner::Manual ||
        operation->kind != cr::CreativeTerrainOperationKind::Region ||
        !regionContainsCoord(operation->region, target)) {
      continue;
    }
    selected = &*operation;
    break;
  }
  if (selected == nullptr) {
    receipt.reasonCode = "creative_editor_terrain_region_select_not_found";
    return receipt;
  }

  cr::CreativeVolumeSelection selection;
  if (!selectionForRegion(document, selected->region, selection)) {
    receipt.reasonCode =
        "creative_editor_terrain_region_select_bounds_invalid";
    return receipt;
  }
  receipt.accepted = true;
  const cr::CreativeGridBounds3 currentBounds =
      cr::creativeVolumeGridBounds(editor.volume.selection);
  const cr::CreativeGridBounds3 selectedBounds =
      cr::creativeVolumeGridBounds(selection);
  const bool sameBounds =
      cr::creativeVolumeSelectionComplete(editor.volume.selection) &&
      currentBounds.min.x == selectedBounds.min.x &&
      currentBounds.min.y == selectedBounds.min.y &&
      currentBounds.min.z == selectedBounds.min.z &&
      currentBounds.max.x == selectedBounds.max.x &&
      currentBounds.max.y == selectedBounds.max.y &&
      currentBounds.max.z == selectedBounds.max.z;
  receipt.changed =
      editor.terrain.region.editingOperationId != selected->id ||
      editor.toolSettings.terrainRegionRecipe != selected->region ||
      !sameBounds;
  receipt.operationId = selected->id;
  editor.terrain.region.editingOperationId = selected->id;
  editor.toolSettings.terrainRegionRecipe = selected->region;
  editor.volume.selection = selection;
  editor.volume.lastReceipt = {};
  invalidateRegionPreview(editor.terrain.region);
  setRegionFeedback(editor, true);
  receipt.reasonCode = "creative_editor_terrain_region_selected";
  return receipt;
}

CreativeEditorTerrainRegionReceipt sampleCreativeEditorTerrainRegionHeight(
    const cr::CreativeDocument& document,
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainRegionReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainRegionAction::SampleHeight;
  if (!cr::creativeTerrainRegionModeUsesTargetHeight(
          editor.toolSettings.terrainRegionRecipe.mode)) {
    receipt.reasonCode = "creative_editor_terrain_region_sample_unused";
    return receipt;
  }
  cr::CreativeTerrainCoord2 target{};
  if (!resolveCreativeEditorTerrainPointerCoord(editor, target)) {
    receipt.reasonCode = "creative_editor_terrain_region_sample_target_invalid";
    setRegionFeedback(editor, false);
    return receipt;
  }
  const std::uint16_t height = terrainHeightAt(document, target);
  if (height < cr::kCreativeTerrainMinimumHeightCells) {
    receipt.reasonCode = "creative_editor_terrain_region_sample_missing";
    setRegionFeedback(editor, false);
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed =
      editor.toolSettings.terrainRegionRecipe.targetHeightCells != height;
  editor.toolSettings.terrainRegionRecipe.targetHeightCells = height;
  receipt.reasonCode = "creative_editor_terrain_region_height_sampled";
  invalidateRegionPreview(editor.terrain.region);
  setRegionFeedback(editor, true);
  return receipt;
}

CreativeEditorTerrainRegionReceipt cancelCreativeEditorTerrainRegion(
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainRegionReceipt receipt;
  receipt.requested = true;
  receipt.accepted = true;
  receipt.action = CreativeEditorTerrainRegionAction::Cancel;
  receipt.changed = editor.volume.selection.phase !=
                        cr::CreativeVolumeSelectionPhase::Empty ||
                    editor.terrain.region.editingOperationId !=
                        cr::kInvalidCreativeTerrainOperationId;
  cr::clearCreativeVolumeSelection(editor.volume.selection);
  editor.terrain.region.editingOperationId =
      cr::kInvalidCreativeTerrainOperationId;
  invalidateRegionPreview(editor.terrain.region);
  clearCreativeEditorPlacementFeedback(editor.interaction);
  receipt.reasonCode = receipt.changed
                           ? "creative_editor_terrain_region_cancelled"
                           : "creative_editor_terrain_region_already_empty";
  return receipt;
}

bool processCreativeEditorTerrainRegionQuickEdit(
    CreativeEditorState& editor,
    cr::CreativeInputActionId action) noexcept {
  if (editor.terrain.region.stamp.active) {
    return processCreativeEditorTerrainStampQuickEdit(editor, action);
  }
  bool changed = false;
  cr::CreativeTerrainRegionRecipe& recipe =
      editor.toolSettings.terrainRegionRecipe;
  switch (action) {
    case cr::CreativeInputActionId::QuickEditPrevious:
    case cr::CreativeInputActionId::QuickEditNext: {
      const int direction =
          action == cr::CreativeInputActionId::QuickEditPrevious ? 1 : -1;
      if (cr::creativeTerrainRegionModeUsesTargetHeight(recipe.mode)) {
        changed = stepRegionValue(
            recipe.targetHeightCells, cr::kCreativeTerrainMinimumHeightCells,
            cr::kCreativeTerrainMaximumHeightCells, direction);
      } else if (cr::creativeTerrainRegionModeUsesAmount(recipe.mode)) {
        changed = stepRegionValue(
            recipe.amountCells, 1U,
            cr::kCreativeTerrainRegionMaximumAmountCells, direction);
      }
      break;
    }
    case cr::CreativeInputActionId::QuickEditDecrease:
      changed = cycleRegionMode(recipe.mode, -1);
      break;
    case cr::CreativeInputActionId::QuickEditIncrease:
      changed = cycleRegionMode(recipe.mode, 1);
      break;
    default:
      return false;
  }
  if (changed) {
    invalidateRegionPreview(editor.terrain.region);
  }
  return changed;
}

std::string creativeEditorTerrainRegionQuickEditLabel(
    const CreativeEditorState& editor) {
  if (editor.terrain.region.stamp.active) {
    return creativeEditorTerrainStampQuickEditLabel(editor);
  }
  const cr::CreativeTerrainRegionRecipe& recipe =
      editor.toolSettings.terrainRegionRecipe;
  std::string label(cr::toString(recipe.mode));
  if (cr::creativeTerrainRegionModeUsesTargetHeight(recipe.mode)) {
    label.append(" | TARGET ");
    label.append(std::to_string(recipe.targetHeightCells));
  } else if (cr::creativeTerrainRegionModeUsesAmount(recipe.mode)) {
    label.append(" | AMOUNT ");
    label.append(std::to_string(recipe.amountCells));
  }
  return label;
}

bool refreshCreativeEditorTerrainRegionPreview(
    CreativeEditorTerrainState& state,
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor) {
  const cr::CreativeVolumeSelection selection =
      creativeEditorVolumePreviewSelection(editor.volume);
  cr::CreativeTerrainRegionRecipe recipe;
  if (!buildCreativeEditorTerrainRegionRecipe(editor, selection, recipe)) {
    invalidateRegionPreview(state.region);
    return false;
  }
  CreativeTerrainRegionPreviewCache& cache = state.region.preview;
  if (previewKeyMatches(cache, document, state.region.editingOperationId,
                        recipe)) {
    return false;
  }

  const std::uint64_t nextBuildCount = cache.buildCount + 1U;
  cache = {};
  cache.valid = true;
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.buildCount = nextBuildCount;
  cache.editingOperationId = state.region.editingOperationId;
  cache.recipe = recipe;
  cache.operationPreview = cr::planCreativeTerrainOperationMutation(
      document.terrainField(), document.terrainHeightField(),
      document.terrainMaterialField(), document.terrainOperationStack(),
      regionRequest(document, state.region, recipe));
  if (!cache.operationPreview.receipt.accepted) {
    return true;
  }

  const cr::CreativeGridSettings grid = document.gridSettings();
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeComposedTerrainSurfacePlan(
          document.terrainField(), cache.operationPreview.heightField,
          cache.operationPreview.hardEdges);
  const cr::CreativeTerrainRenderPlan render =
      cr::buildCreativeTerrainRenderPlan(
          surface, cache.operationPreview.materialField, grid.origin,
          grid.cellSizeMeters);
  if (!render.accepted) {
    return true;
  }

  const std::int64_t expansion =
      static_cast<std::int64_t>(recipe.featherCells) + 1;
  const std::int64_t minimumX =
      static_cast<std::int64_t>(recipe.bounds.minimum.x) - expansion;
  const std::int64_t minimumZ =
      static_cast<std::int64_t>(recipe.bounds.minimum.z) - expansion;
  const std::int64_t maximumX =
      static_cast<std::int64_t>(recipe.bounds.minimum.x) +
      recipe.bounds.widthCells - 1 + expansion;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(recipe.bounds.minimum.z) +
      recipe.bounds.depthCells - 1 + expansion;
  for (const cr::CreativeTerrainSurfacePatch& patch : render.patches) {
    if (patch.coord.x >= minimumX && patch.coord.x <= maximumX &&
        patch.coord.z >= minimumZ && patch.coord.z <= maximumZ) {
      cache.patches.push_back(patch);
    }
  }
  cache.renderAccepted = true;
  return true;
}

void appendCreativeEditorTerrainRegionOverlay(
    const cr::CreativeDocument&,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const CreativeTerrainRegionPreviewCache& preview =
      editor.terrain.region.preview;
  if (!preview.valid) {
    return;
  }
  const bool accepted =
      preview.operationPreview.receipt.accepted && preview.renderAccepted;
  if (!accepted) {
    return;
  }
  for (const cr::CreativeTerrainSurfacePatch& patch : preview.patches) {
    appendCreativeEditorTerrainPatchSlopeTriangles(
        wireLines, patch, wireThickness * 0.7F);
  }
}

}  // namespace iggy3d_creative_app
