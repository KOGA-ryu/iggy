#include "EditorWorldLayoutPanel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <span>

namespace iggy3d_creative_app {
namespace {

std::size_t findBuildingTemplate(
    const CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    std::string_view templateId) noexcept {
  const auto found = std::find_if(
      library.templates.begin(), library.templates.end(),
      [templateId](const cr::CreativeWorldLayoutBuildingTemplate& value) {
        return value.templateId == templateId;
      });
  return found == library.templates.end()
             ? cr::kInvalidCreativeWorldLayoutIndex
             : static_cast<std::size_t>(found - library.templates.begin());
}

}  // namespace

std::vector<CreativeEditorWorldLayoutToolboxEntry>
buildCreativeEditorWorldLayoutToolboxEntries() {
  const std::span<const CreativeEditorWorldLayoutPaletteEntry> palette =
      creativeEditorWorldLayoutPaletteEntries();
  std::vector<CreativeEditorWorldLayoutToolboxEntry> entries;
  entries.reserve(palette.size() + 1U);
  for (std::uint8_t categoryValue = 0U;
       categoryValue < static_cast<std::uint8_t>(
                           CreativeEditorWorldLayoutPaletteCategory::Count);
       ++categoryValue) {
    const auto category =
        static_cast<CreativeEditorWorldLayoutPaletteCategory>(categoryValue);
    for (std::size_t index = 0U; index < palette.size(); ++index) {
      const CreativeEditorWorldLayoutPaletteEntry& paletteEntry =
          palette[index];
      if (paletteEntry.category != category) {
        continue;
      }
      CreativeEditorWorldLayoutToolboxEntry entry;
      const bool isTool =
          paletteEntry.activation ==
          CreativeEditorWorldLayoutPaletteActivation::Tool;
      entry.kind =
          isTool ? CreativeEditorWorldLayoutToolboxEntryKind::PaletteTool
                 : CreativeEditorWorldLayoutToolboxEntryKind::
                       PaletteBuildingTemplate;
      entry.category = category;
      entry.paletteIndex = index;
      entry.glyph =
          isTool ? creativeEditorToolGlyphForWorldLayoutTool(paletteEntry.tool)
                 : CreativeEditorToolGlyph::EstateHouse;
      entry.label = paletteEntry.label;
      entries.push_back(entry);
    }
    if (category == CreativeEditorWorldLayoutPaletteCategory::Terrain) {
      CreativeEditorWorldLayoutToolboxEntry toggle;
      toggle.kind =
          CreativeEditorWorldLayoutToolboxEntryKind::TerrainRegionToggle;
      toggle.category = category;
      toggle.paletteIndex = palette.size();
      toggle.glyph = CreativeEditorToolGlyph::MaskRectangle;
      toggle.label = "Terrain region";
      entries.push_back(toggle);
    }
  }
  return entries;
}

CreativeEditorWorldLayoutToolboxButtonState
classifyCreativeEditorWorldLayoutToolboxButton(
    const CreativeEditorWorldLayoutToolboxEntry& entry,
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration) noexcept {
  CreativeEditorWorldLayoutToolboxButtonState result;
  const std::span<const CreativeEditorWorldLayoutPaletteEntry> palette =
      creativeEditorWorldLayoutPaletteEntries();
  switch (entry.kind) {
    case CreativeEditorWorldLayoutToolboxEntryKind::PaletteTool: {
      if (entry.paletteIndex >= palette.size()) {
        return result;
      }
      result.active = state.tool == palette[entry.paletteIndex].tool &&
                      !state.buildingTemplatePlacement.active;
      return result;
    }
    case CreativeEditorWorldLayoutToolboxEntryKind::PaletteBuildingTemplate: {
      if (entry.paletteIndex >= palette.size()) {
        return result;
      }
      const std::size_t templateIndex = findBuildingTemplate(
          state.buildingTemplates,
          palette[entry.paletteIndex].buildingTemplateId);
      result.unavailable =
          templateIndex == cr::kInvalidCreativeWorldLayoutIndex;
      result.active = !result.unavailable &&
                      state.buildingTemplatePlacement.active &&
                      state.buildingTemplatePlacement.templateIndex ==
                          templateIndex;
      return result;
    }
    case CreativeEditorWorldLayoutToolboxEntryKind::TerrainRegionToggle: {
      result.active = topography.region.editingEnabled;
      result.unavailable =
          (terrainGeneration.previewActive && !topography.region.ownsPreview) ||
          creativeEditorWorldLayoutPreviewActive(state) ||
          state.buildingTransform.active ||
          state.buildingTemplatePlacement.active;
      return result;
    }
    case CreativeEditorWorldLayoutToolboxEntryKind::Count:
      break;
  }
  return result;
}

bool creativeEditorWorldLayoutTerrainRegionFieldVisible(
    CreativeEditorWorldLayoutTerrainRegionOperation operation,
    CreativeEditorWorldLayoutTerrainRegionField field) noexcept {
  if (operation >= CreativeEditorWorldLayoutTerrainRegionOperation::Count) {
    return false;
  }
  const bool noise =
      operation == CreativeEditorWorldLayoutTerrainRegionOperation::Noise;
  switch (field) {
    case CreativeEditorWorldLayoutTerrainRegionField::TargetHeight:
      return operation !=
             CreativeEditorWorldLayoutTerrainRegionOperation::Smooth;
    case CreativeEditorWorldLayoutTerrainRegionField::NoiseRelief:
    case CreativeEditorWorldLayoutTerrainRegionField::NoiseScale:
    case CreativeEditorWorldLayoutTerrainRegionField::Seed:
      return noise;
    case CreativeEditorWorldLayoutTerrainRegionField::Feather:
      return true;
    case CreativeEditorWorldLayoutTerrainRegionField::Count:
      break;
  }
  return false;
}

std::string_view creativeEditorWorldLayoutTerrainRegionTargetLabel(
    CreativeEditorWorldLayoutTerrainRegionOperation operation) noexcept {
  switch (operation) {
    case CreativeEditorWorldLayoutTerrainRegionOperation::Flatten:
      return "Height";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Raise:
      return "Raise to at least";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Lower:
      return "Lower to at most";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Noise:
      return "Base height";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Smooth:
    case CreativeEditorWorldLayoutTerrainRegionOperation::Count:
      break;
  }
  return "";
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
  const long long zoomPercent = std::llround(
      static_cast<double>(state.canvasPixelsPerCell) * 100.0 /
      static_cast<double>(kCreativeEditorWorldLayoutStatusZoomBaselinePixels));
  std::snprintf(buffer, sizeof buffer, "Zoom %lld%%", zoomPercent);
  line.zoom = buffer;
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
      !cr::isValidCreativeTerrainHeightFieldBounds(region.bounds)) {
    return metrics;
  }
  metrics.present = true;
  metrics.minimumX = region.bounds.minimum.x;
  metrics.minimumZ = region.bounds.minimum.z;
  metrics.widthCells = region.bounds.widthCells;
  metrics.depthCells = region.bounds.depthCells;
  metrics.candidateCellCount =
      static_cast<std::uint32_t>(region.bounds.widthCells) *
      static_cast<std::uint32_t>(region.bounds.depthCells);
  return metrics;
}

}  // namespace iggy3d_creative_app
