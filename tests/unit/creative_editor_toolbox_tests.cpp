#include "EditorWorldLayoutPanel.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace cr = iggy3d::creative;

using iggy3d_creative_app::CreativeEditorTerrainGenerationState;
using iggy3d_creative_app::CreativeEditorToolGlyph;
using iggy3d_creative_app::CreativeEditorWorldLayoutPaletteActivation;
using iggy3d_creative_app::CreativeEditorWorldLayoutPaletteCategory;
using iggy3d_creative_app::CreativeEditorWorldLayoutState;
using iggy3d_creative_app::CreativeEditorWorldLayoutTool;
using iggy3d_creative_app::CreativeEditorWorldLayoutToolboxEntry;
using iggy3d_creative_app::CreativeEditorWorldLayoutToolboxEntryKind;
using iggy3d_creative_app::CreativeEditorWorldLayoutTopographyState;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const CreativeEditorWorldLayoutToolboxEntry* findEntry(
    const std::vector<CreativeEditorWorldLayoutToolboxEntry>& entries,
    std::string_view label) {
  for (const CreativeEditorWorldLayoutToolboxEntry& entry : entries) {
    if (entry.label == label) {
      return &entry;
    }
  }
  return nullptr;
}

bool rosterMirrorsThePaletteAndAddsTheRegionToggle() {
  const auto entries =
      iggy3d_creative_app::buildCreativeEditorWorldLayoutToolboxEntries();
  const auto palette =
      iggy3d_creative_app::creativeEditorWorldLayoutPaletteEntries();
  std::vector<int> paletteCoverage(palette.size(), 0);
  std::size_t toggleCount = 0U;
  bool categoriesOrdered = true;
  bool labelsMatch = true;
  bool glyphsFaithful = true;
  bool toggleShapeRight = true;
  std::uint8_t lastCategory = 0U;
  for (const CreativeEditorWorldLayoutToolboxEntry& entry : entries) {
    const auto categoryValue = static_cast<std::uint8_t>(entry.category);
    if (categoryValue < lastCategory) {
      categoriesOrdered = false;
    }
    lastCategory = categoryValue;
    if (entry.kind ==
        CreativeEditorWorldLayoutToolboxEntryKind::TerrainRegionToggle) {
      ++toggleCount;
      const bool shapeRight =
          entry.category == CreativeEditorWorldLayoutPaletteCategory::Terrain &&
          entry.glyph == CreativeEditorToolGlyph::MaskRectangle &&
          entry.label == "Terrain region" &&
          entry.paletteIndex == palette.size();
      if (!shapeRight) {
        toggleShapeRight = false;
      }
      continue;
    }
    if (entry.paletteIndex >= palette.size()) {
      labelsMatch = false;
      continue;
    }
    ++paletteCoverage[entry.paletteIndex];
    const auto& paletteEntry = palette[entry.paletteIndex];
    if (entry.label != paletteEntry.label ||
        entry.category != paletteEntry.category) {
      labelsMatch = false;
    }
    const bool isTool = paletteEntry.activation ==
                        CreativeEditorWorldLayoutPaletteActivation::Tool;
    const CreativeEditorToolGlyph expectedGlyph =
        isTool ? iggy3d_creative_app::creativeEditorToolGlyphForWorldLayoutTool(
                     paletteEntry.tool)
               : CreativeEditorToolGlyph::EstateHouse;
    const bool kindRight =
        entry.kind ==
        (isTool ? CreativeEditorWorldLayoutToolboxEntryKind::PaletteTool
                : CreativeEditorWorldLayoutToolboxEntryKind::
                      PaletteBuildingTemplate);
    if (entry.glyph != expectedGlyph || !kindRight ||
        iggy3d_creative_app::creativeEditorToolGlyphOps(entry.glyph).empty()) {
      glyphsFaithful = false;
    }
  }
  bool everyPaletteEntryOnce = true;
  for (const int count : paletteCoverage) {
    if (count != 1) {
      everyPaletteEntryOnce = false;
    }
  }
  return expect(entries.size() == palette.size() + 1U,
                "roster is the palette plus one region toggle") &&
         expect(everyPaletteEntryOnce,
                "every palette entry appears exactly once") &&
         expect(toggleCount == 1U, "exactly one terrain region toggle") &&
         expect(toggleShapeRight,
                "the toggle sits in terrain with the dashed-rect glyph") &&
         expect(categoriesOrdered, "entries group by category in enum order") &&
         expect(labelsMatch, "labels and categories mirror the palette") &&
         expect(glyphsFaithful,
                "every entry carries its mapped glyph with real ink");
}

bool classificationTracksActiveToolAndTemplates() {
  CreativeEditorWorldLayoutState state;
  const CreativeEditorWorldLayoutTopographyState topography;
  const CreativeEditorTerrainGenerationState terrainGeneration;
  const auto entries =
      iggy3d_creative_app::buildCreativeEditorWorldLayoutToolboxEntries();
  const auto palette =
      iggy3d_creative_app::creativeEditorWorldLayoutPaletteEntries();
  const CreativeEditorWorldLayoutToolboxEntry* selectEntry =
      findEntry(entries, "Select");
  const CreativeEditorWorldLayoutToolboxEntry* partitionEntry =
      findEntry(entries, "Partition");
  const CreativeEditorWorldLayoutToolboxEntry* estateEntry =
      findEntry(entries, "Estate House");
  if (!expect(selectEntry != nullptr && partitionEntry != nullptr &&
                  estateEntry != nullptr,
              "select, partition, and estate house entries exist")) {
    return false;
  }
  const auto classify =
      [&](const CreativeEditorWorldLayoutToolboxEntry& entry) {
        return iggy3d_creative_app::
            classifyCreativeEditorWorldLayoutToolboxButton(
                entry, state, topography, terrainGeneration);
      };
  const auto defaultSelect = classify(*selectEntry);
  const auto defaultPartition = classify(*partitionEntry);
  const auto estateWithoutLibrary = classify(*estateEntry);
  state.tool = CreativeEditorWorldLayoutTool::Wall;
  const auto wallSelect = classify(*selectEntry);
  const auto wallPartition = classify(*partitionEntry);
  cr::CreativeWorldLayoutBuildingTemplate estateTemplate;
  estateTemplate.templateId =
      std::string(palette[estateEntry->paletteIndex].buildingTemplateId);
  state.buildingTemplates.templates.push_back(estateTemplate);
  const auto estateIdle = classify(*estateEntry);
  state.buildingTemplatePlacement.active = true;
  state.buildingTemplatePlacement.templateIndex = 0U;
  const auto estatePlacing = classify(*estateEntry);
  const auto partitionDuringPlacement = classify(*partitionEntry);
  return expect(defaultSelect.active && !defaultSelect.unavailable,
                "select is the default active tool") &&
         expect(!defaultPartition.active,
                "partition starts inactive") &&
         expect(estateWithoutLibrary.unavailable,
                "estate house is unavailable without its template") &&
         expect(!wallSelect.active && wallPartition.active,
                "activating wall moves the highlight to partition") &&
         expect(!estateIdle.unavailable && !estateIdle.active,
                "a loaded template is available but idle") &&
         expect(estatePlacing.active,
                "placing the template highlights its entry") &&
         expect(!partitionDuringPlacement.active,
                "tool highlights drop during template placement");
}

bool regionToggleFollowsTheTerrainWorkflow() {
  CreativeEditorWorldLayoutState state;
  CreativeEditorWorldLayoutTopographyState topography;
  CreativeEditorTerrainGenerationState terrainGeneration;
  const auto entries =
      iggy3d_creative_app::buildCreativeEditorWorldLayoutToolboxEntries();
  const CreativeEditorWorldLayoutToolboxEntry* toggle =
      findEntry(entries, "Terrain region");
  if (!expect(toggle != nullptr, "the region toggle exists")) {
    return false;
  }
  const auto classify = [&]() {
    return iggy3d_creative_app::classifyCreativeEditorWorldLayoutToolboxButton(
        *toggle, state, topography, terrainGeneration);
  };
  const auto idle = classify();
  topography.region.editingEnabled = true;
  const auto editing = classify();
  terrainGeneration.previewActive = true;
  const auto foreignPreview = classify();
  topography.region.ownsPreview = true;
  const auto ownedPreview = classify();
  terrainGeneration.previewActive = false;
  topography.region.ownsPreview = false;
  state.buildingTransform.active = true;
  const auto duringTransform = classify();
  state.buildingTransform.active = false;
  state.buildingTemplatePlacement.active = true;
  const auto duringPlacement = classify();
  return expect(!idle.active && !idle.unavailable,
                "the toggle starts idle and available") &&
         expect(editing.active, "enabling region editing lights the toggle") &&
         expect(foreignPreview.unavailable,
                "a foreign terrain preview locks the toggle") &&
         expect(!ownedPreview.unavailable,
                "owning the preview keeps the toggle available") &&
         expect(duringTransform.unavailable,
                "building transforms lock the toggle") &&
         expect(duringPlacement.unavailable,
                "template placement locks the toggle");
}

}  // namespace

int main() {
  bool ok = true;
  ok = rosterMirrorsThePaletteAndAddsTheRegionToggle() && ok;
  ok = classificationTracksActiveToolAndTemplates() && ok;
  ok = regionToggleFollowsTheTerrainWorkflow() && ok;
  return ok ? 0 : 1;
}
