#include "EditorWorldLayoutPanel.hpp"

#include "EditorWorldLayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <span>

namespace iggy3d_creative_app {
CreativeEditorWorldLayoutInspectionStatus
composeCreativeEditorWorldLayoutInspectionStatus(
    const CreativeEditorWorldLayoutState& state) {
  const CreativeEditorWorldLayoutInspection inspection =
      inspectCreativeEditorWorldLayout(state);
  CreativeEditorWorldLayoutInspectionStatus status;
  status.validity = inspection.previewValidity;
  switch (inspection.previewValidity) {
    case CreativeEditorWorldLayoutPreviewValidity::None:
      status.text = "Authored";
      break;
    case CreativeEditorWorldLayoutPreviewValidity::Valid:
      status.text = "Preview valid";
      break;
    case CreativeEditorWorldLayoutPreviewValidity::Invalid:
      status.text = "Preview invalid";
      break;
    case CreativeEditorWorldLayoutPreviewValidity::Count:
      status.text = "Inspection invalid";
      break;
  }

  std::string_view stableKey;
  if (inspection.authoredSource.table != cr::CreativeWorldLayoutTable::None) {
    stableKey = creativeEditorWorldLayoutSourceStableKey(
        state, inspection.authoredSource.table,
        inspection.authoredSource.index);
  }
  if (stableKey.empty() && inspection.source != nullptr) {
    stableKey = inspection.source->stableKey;
  }
  if (!stableKey.empty()) {
    status.text += " | ";
    status.text.append(stableKey);
  }
  return status;
}

CreativeEditorWorldLayoutStatusLine composeCreativeEditorWorldLayoutStatusLine(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    const CreativeEditorWorldLayoutCanvasHoverStatus& hover) {
  CreativeEditorWorldLayoutStatusLine line;
  char buffer[64];
  if (hover.present) {
    std::snprintf(buffer, sizeof buffer, "Cell %lld, %lld",
                  static_cast<long long>(std::floor(hover.cellX)),
                  static_cast<long long>(std::floor(hover.cellZ)));
  } else {
    std::snprintf(buffer, sizeof buffer, "Cell --");
  }
  line.cursor = buffer;
  if (!hover.semanticRole.empty()) {
    line.cursor += " | ";
    line.cursor.append(hover.semanticRole);
  }
  const float pixelsPerCell =
      state.viewMode == CreativeEditorWorldLayoutViewMode::Elevation
          ? state.elevationPixelsPerCell
          : state.canvasPixelsPerCell;
  const long long zoomPercent = std::llround(
      static_cast<double>(pixelsPerCell) * 100.0 /
      static_cast<double>(kCreativeEditorWorldLayoutStatusZoomBaselinePixels));
  std::snprintf(buffer, sizeof buffer, "Zoom %lld%%", zoomPercent);
  line.zoom = buffer;
  line.inspection = composeCreativeEditorWorldLayoutInspectionStatus(state);
  if (state.tool == CreativeEditorWorldLayoutTool::CatalogAsset) {
    const char* snapName = "grid";
    switch (state.catalogPlacement.snapMode) {
      case CreativeEditorWorldLayoutCatalogSnapMode::Floor:
        snapName = "floor";
        break;
      case CreativeEditorWorldLayoutCatalogSnapMode::Wall:
        snapName = "wall";
        break;
      case CreativeEditorWorldLayoutCatalogSnapMode::Grid:
      case CreativeEditorWorldLayoutCatalogSnapMode::Count:
        break;
    }
    std::snprintf(buffer, sizeof buffer, "Snap %s", snapName);
    line.snap = buffer;
  }
  const CreativeEditorWorldLayoutTerrainRegionState& region = topography.region;
  if (region.editingEnabled) {
    line.regionMessage = true;
    line.phase = classifyCreativeEditorWorldLayoutTerrainRegionPhase(region);
    line.message = region.statusMessage;
    if (region.ownsPreview) {
      const cr::CreativeTerrainOperationReplayReceipt& replay =
          terrainGeneration.operationPreview.receipt.replay;
      std::snprintf(buffer, sizeof buffer, "Changed %llu / %llu cells",
                    static_cast<unsigned long long>(replay.modifiedCellCount),
                    static_cast<unsigned long long>(replay.outputCellCount));
      line.cells = buffer;
    } else {
      const CreativeEditorWorldLayoutTerrainRegionMetrics metrics =
          measureCreativeEditorWorldLayoutTerrainRegion(region);
      if (metrics.present) {
        std::snprintf(buffer, sizeof buffer, "%u candidate cells",
                      metrics.candidateCellCount);
        line.cells = buffer;
      }
    }
  } else {
    line.message = state.statusMessage;
  }
  return line;
}

CreativeEditorWorldLayoutTerrainRegionPhase
classifyCreativeEditorWorldLayoutTerrainRegionPhase(
    const CreativeEditorWorldLayoutTerrainRegionState& region) noexcept {
  using Phase = CreativeEditorWorldLayoutTerrainRegionPhase;
  if (!region.editingEnabled) {
    return Phase::Idle;
  }
  if (region.selecting) {
    return Phase::Selecting;
  }
  if (region.ownsPreview) {
    return Phase::Ready;
  }
  // The terrain state carries its lifecycle in exact status messages; the
  // classifier keys on the strings the kernels emit rather than inventing a
  // parallel flag the state could contradict.
  if (region.statusMessage ==
      "Terrain region preview canceled: document changed") {
    return Phase::Stale;
  }
  if (region.statusMessage == "Terrain region preview rejected" ||
      region.statusMessage == "Another terrain preview is active" ||
      region.statusMessage == "No terrain region preview to apply" ||
      region.statusMessage == "Terrain region apply failed") {
    return Phase::Rejected;
  }
  if (region.regionValid) {
    return Phase::AwaitingPreview;
  }
  return Phase::Idle;
}

std::string_view toString(
    CreativeEditorWorldLayoutTerrainRegionPhase phase) noexcept {
  switch (phase) {
    case CreativeEditorWorldLayoutTerrainRegionPhase::Idle:
      return "idle";
    case CreativeEditorWorldLayoutTerrainRegionPhase::Selecting:
      return "selecting";
    case CreativeEditorWorldLayoutTerrainRegionPhase::AwaitingPreview:
      return "awaiting preview";
    case CreativeEditorWorldLayoutTerrainRegionPhase::Ready:
      return "ready";
    case CreativeEditorWorldLayoutTerrainRegionPhase::Rejected:
      return "rejected";
    case CreativeEditorWorldLayoutTerrainRegionPhase::Stale:
      return "stale";
  }
  return "idle";
}

CreativeEditorWorldLayoutTerrainRegionMetrics
measureCreativeEditorWorldLayoutTerrainRegion(
    const CreativeEditorWorldLayoutTerrainRegionState& region) noexcept {
  CreativeEditorWorldLayoutTerrainRegionMetrics metrics;
  if (!region.regionValid ||
      !cr::isValidCreativeTerrainHeightFieldBounds(region.recipe.bounds)) {
    return metrics;
  }
  metrics.present = true;
  metrics.minimumX = region.recipe.bounds.minimum.x;
  metrics.minimumZ = region.recipe.bounds.minimum.z;
  metrics.widthCells = region.recipe.bounds.widthCells;
  metrics.depthCells = region.recipe.bounds.depthCells;
  metrics.candidateCellCount =
      static_cast<std::uint32_t>(region.recipe.bounds.widthCells) *
      static_cast<std::uint32_t>(region.recipe.bounds.depthCells);
  return metrics;
}

}  // namespace iggy3d_creative_app
