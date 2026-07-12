#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void invalidateRegionPreview(CreativeTerrainRegionState& region) noexcept {
  region.preview.valid = false;
  region.preview.renderAccepted = false;
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
    cr::CreativeTerrainCoord2& minimumCoord,
    cr::CreativeTerrainCoord2& maximumCoord) noexcept {
  if (!cr::creativeVolumeSelectionValid(selection)) {
    return false;
  }
  minimumCoord = {
      std::min(selection.firstCell.x, selection.secondCell.x),
      std::min(selection.firstCell.z, selection.secondCell.z)};
  maximumCoord = {
      std::max(selection.firstCell.x, selection.secondCell.x),
      std::max(selection.firstCell.z, selection.secondCell.z)};
  return true;
}

[[nodiscard]] cr::CreativeTerrainRegionRequest regionRequest(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 minimumCoord,
    cr::CreativeTerrainCoord2 maximumCoord) noexcept {
  cr::CreativeTerrainRegionRequest request;
  request.controls = document.terrainField().controls();
  request.minimumCoord = minimumCoord;
  request.maximumCoord = maximumCoord;
  request.operation = editor.toolSettings.terrainRegionOperation;
  request.amountCells = cr::creativeTerrainRegionAmountCells(
      editor.toolSettings.terrainRegionAmount);
  request.targetHeightCells = editor.terrain.region.targetHeightCells;
  return request;
}

[[nodiscard]] bool previewKeyMatches(
    const CreativeTerrainRegionPreviewCache& cache,
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 minimumCoord,
    cr::CreativeTerrainCoord2 maximumCoord) noexcept {
  return cache.valid && cache.documentId == document.id() &&
         cache.terrainRevision == document.terrainField().revision() &&
         cache.minimumCoord == minimumCoord &&
         cache.maximumCoord == maximumCoord &&
         cache.operation == editor.toolSettings.terrainRegionOperation &&
         cache.amountCells == cr::creativeTerrainRegionAmountCells(
                                  editor.toolSettings.terrainRegionAmount) &&
         (!cr::creativeTerrainRegionUsesTargetHeight(cache.operation) ||
          cache.targetHeightCells == editor.terrain.region.targetHeightCells);
}

template <typename Enum>
[[nodiscard]] bool stepClampedEnum(Enum& value,
                                   Enum count,
                                   int direction) noexcept {
  const int before = static_cast<int>(value);
  const int last = static_cast<int>(count) - 1;
  const int after = std::clamp(before + direction, 0, last);
  value = static_cast<Enum>(after);
  return after != before;
}

[[nodiscard]] bool cycleRegionOperation(
    cr::CreativeTerrainRegionOperation& operation,
    int direction) noexcept {
  const int count = static_cast<int>(cr::CreativeTerrainRegionOperation::Count);
  const int before = static_cast<int>(operation);
  const int after = (before + (direction > 0 ? 1 : count - 1)) % count;
  operation = static_cast<cr::CreativeTerrainRegionOperation>(after);
  return after != before;
}

}  // namespace

cr::CreativeTerrainRegionPlan planCreativeEditorTerrainRegion(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    const cr::CreativeVolumeSelection& selection) noexcept {
  cr::CreativeTerrainCoord2 minimumCoord{};
  cr::CreativeTerrainCoord2 maximumCoord{};
  if (!regionBounds(selection, minimumCoord, maximumCoord)) {
    return {};
  }
  return cr::buildCreativeTerrainRegionPlan(
      regionRequest(document, editor, minimumCoord, maximumCoord));
}

CreativeEditorTerrainRegionReceipt
applyCreativeEditorTerrainRegionWithHistory(cr::CreativeAppState& appState,
                                            CreativeEditorState& editor,
                                            std::string_view source) {
  CreativeEditorTerrainRegionReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainRegionAction::Apply;
  receipt.plan = planCreativeEditorTerrainRegion(
      appState.facade.document(), editor, editor.volume.selection);
  if (!receipt.plan.requested) {
    receipt.reasonCode = "creative_editor_terrain_region_selection_invalid";
    setRegionFeedback(editor, false);
    return receipt;
  }
  receipt.accepted = receipt.plan.accepted;
  receipt.reasonCode = receipt.plan.reasonCode;
  if (!receipt.plan.accepted || receipt.plan.items().empty()) {
    setRegionFeedback(editor, receipt.plan.accepted);
    return receipt;
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  receipt.mutation =
      appState.facade.applyTerrainControlEdits(receipt.plan.items());
  editor.terrain.lastMutation = receipt.mutation;
  receipt.accepted = receipt.mutation.accepted;
  receipt.changed = receipt.mutation.changed;
  receipt.reasonCode = receipt.mutation.reasonCode;
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.mutation.accepted && receipt.mutation.changed,
      receipt.mutation.reasonCode));
  if (receipt.changed) {
    invalidateRegionPreview(editor.terrain.region);
  }
  setRegionFeedback(editor, receipt.accepted);
  return receipt;
}

CreativeEditorTerrainRegionReceipt sampleCreativeEditorTerrainRegionHeight(
    const cr::CreativeDocument& document,
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainRegionReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainRegionAction::SampleHeight;
  if (!cr::creativeTerrainRegionUsesTargetHeight(
          editor.toolSettings.terrainRegionOperation)) {
    receipt.reasonCode = "creative_editor_terrain_region_sample_unused";
    return receipt;
  }
  cr::CreativeTerrainCoord2 target{};
  if (!resolveCreativeEditorTerrainPointerCoord(editor, target)) {
    receipt.reasonCode = "creative_editor_terrain_region_sample_target_invalid";
    setRegionFeedback(editor, false);
    return receipt;
  }
  const cr::CreativeTerrainHeightSample sample =
      cr::sampleCreativeTerrainHeight(document.terrainField(), target);
  if (!sample.present) {
    receipt.reasonCode = "creative_editor_terrain_region_sample_missing";
    setRegionFeedback(editor, false);
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed =
      editor.terrain.region.targetHeightCells != sample.heightCells;
  editor.terrain.region.targetHeightCells = sample.heightCells;
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
                    cr::CreativeVolumeSelectionPhase::Empty;
  cr::clearCreativeVolumeSelection(editor.volume.selection);
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
  const cr::CreativeTerrainRegionOperation operation =
      editor.toolSettings.terrainRegionOperation;
  switch (action) {
    case cr::CreativeInputActionId::QuickEditPrevious:
    case cr::CreativeInputActionId::QuickEditNext: {
      const int direction =
          action == cr::CreativeInputActionId::QuickEditPrevious ? 1 : -1;
      if (cr::creativeTerrainRegionUsesTargetHeight(operation)) {
        const std::uint16_t before = editor.terrain.region.targetHeightCells;
        editor.terrain.region.targetHeightCells =
            static_cast<std::uint16_t>(std::clamp(
                static_cast<int>(before) + direction,
                static_cast<int>(cr::kCreativeTerrainMinimumHeightCells),
                static_cast<int>(cr::kCreativeTerrainMaximumHeightCells)));
        changed = editor.terrain.region.targetHeightCells != before;
      } else if (cr::creativeTerrainRegionUsesAmount(operation)) {
        changed = stepClampedEnum(
            editor.toolSettings.terrainRegionAmount,
            cr::CreativeTerrainRegionAmount::Count, direction);
      }
      break;
    }
    case cr::CreativeInputActionId::QuickEditDecrease:
      changed = cycleRegionOperation(
          editor.toolSettings.terrainRegionOperation, -1);
      break;
    case cr::CreativeInputActionId::QuickEditIncrease:
      changed = cycleRegionOperation(
          editor.toolSettings.terrainRegionOperation, 1);
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
  const cr::CreativeTerrainRegionOperation operation =
      editor.toolSettings.terrainRegionOperation;
  std::string label(cr::toString(operation));
  if (cr::creativeTerrainRegionUsesTargetHeight(operation)) {
    label.append(" | TARGET ");
    label.append(std::to_string(editor.terrain.region.targetHeightCells));
  } else if (cr::creativeTerrainRegionUsesAmount(operation)) {
    label.append(" | AMOUNT ");
    label.append(std::to_string(cr::creativeTerrainRegionAmountCells(
        editor.toolSettings.terrainRegionAmount)));
  }
  return label;
}

bool refreshCreativeEditorTerrainRegionPreview(
    CreativeEditorTerrainState& state,
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor) {
  const cr::CreativeVolumeSelection selection =
      creativeEditorVolumePreviewSelection(editor.volume);
  cr::CreativeTerrainCoord2 minimumCoord{};
  cr::CreativeTerrainCoord2 maximumCoord{};
  if (!regionBounds(selection, minimumCoord, maximumCoord)) {
    invalidateRegionPreview(state.region);
    return false;
  }
  CreativeTerrainRegionPreviewCache& cache = state.region.preview;
  if (previewKeyMatches(cache, document, editor, minimumCoord, maximumCoord)) {
    return false;
  }

  const std::uint64_t nextBuildCount = cache.buildCount + 1U;
  cache = {};
  cache.valid = true;
  cache.documentId = document.id();
  cache.terrainRevision = document.terrainField().revision();
  cache.buildCount = nextBuildCount;
  cache.minimumCoord = minimumCoord;
  cache.maximumCoord = maximumCoord;
  cache.operation = editor.toolSettings.terrainRegionOperation;
  cache.amountCells = cr::creativeTerrainRegionAmountCells(
      editor.toolSettings.terrainRegionAmount);
  cache.targetHeightCells = state.region.targetHeightCells;
  cache.plan = cr::buildCreativeTerrainRegionPlan(
      regionRequest(document, editor, minimumCoord, maximumCoord));
  if (!cache.plan.accepted) {
    return true;
  }

  const cr::CreativeGridSettings grid = document.gridSettings();
  const cr::CreativeTerrainMutationPreviewReceipt preview =
      cr::buildCreativeTerrainMutationPreview(
          document.terrainField(), cache.plan.items(), grid.origin,
          grid.cellSizeMeters);
  if (!preview.accepted) {
    return true;
  }

  std::uint16_t expansion = 1U;
  for (const cr::CreativeTerrainControlPoint& control :
       document.terrainField().controls()) {
    if (cr::creativeTerrainCoordInsideRegion(
            control.coord, minimumCoord, maximumCoord)) {
      expansion = std::max(expansion, control.radiusCells);
    }
  }
  const std::int64_t minimumX =
      static_cast<std::int64_t>(minimumCoord.x) - expansion;
  const std::int64_t minimumZ =
      static_cast<std::int64_t>(minimumCoord.z) - expansion;
  const std::int64_t maximumX =
      static_cast<std::int64_t>(maximumCoord.x) + expansion;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(maximumCoord.z) + expansion;
  for (const cr::CreativeTerrainSurfacePatch& patch : preview.render.patches) {
    if (patch.coord.x >= minimumX && patch.coord.x <= maximumX &&
        patch.coord.z >= minimumZ && patch.coord.z <= maximumZ) {
      cache.patches.push_back(patch);
    }
  }
  cache.renderAccepted = true;
  return true;
}

void appendCreativeEditorTerrainRegionOverlay(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const CreativeTerrainRegionPreviewCache& preview =
      editor.terrain.region.preview;
  if (!preview.valid) {
    return;
  }
  constexpr iggy3d::RenderLineColor admitted{0.20F, 1.0F, 0.35F, 1.0F};
  constexpr iggy3d::RenderLineColor removing{1.0F, 0.46F, 0.12F, 1.0F};
  constexpr iggy3d::RenderLineColor rejected{1.0F, 0.20F, 0.18F, 1.0F};
  const bool accepted = preview.plan.accepted && preview.renderAccepted;
  const iggy3d::RenderLineColor color =
      !accepted ? rejected
                : preview.operation == cr::CreativeTerrainRegionOperation::Erase
                      ? removing
                      : admitted;
  const cr::CreativeGridSettings grid = document.gridSettings();
  const auto plannedEdits = preview.plan.items();
  for (const cr::CreativeTerrainControlPoint& control :
       document.terrainField().controls()) {
    if (!cr::creativeTerrainCoordInsideRegion(
            control.coord, preview.minimumCoord, preview.maximumCoord)) {
      continue;
    }
    const cr::CreativeTerrainControlPoint* displayedControl = &control;
    if (accepted) {
      const auto planned = std::find_if(
          plannedEdits.begin(), plannedEdits.end(),
          [&control](const cr::CreativeTerrainControlEdit& edit) {
            return edit.control.coord == control.coord;
          });
      if (planned != plannedEdits.end()) {
        displayedControl = &planned->control;
      }
    }
    appendCreativeEditorTerrainControlGuide(
        wireLines, grid, *displayedControl, color, wireThickness * 1.5F);
  }
  if (!accepted) {
    return;
  }
  for (const cr::CreativeTerrainSurfacePatch& patch : preview.patches) {
    appendCreativeEditorTerrainPatchSlopeTriangles(
        wireLines, patch, wireThickness * 0.7F);
  }
}

}  // namespace iggy3d_creative_app
