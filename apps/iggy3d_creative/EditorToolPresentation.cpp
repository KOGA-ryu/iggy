#include "EditorToolPresentation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

#include "EditorWorldLayout.hpp"

#include "app/iggy3d/creative/world/MapTemplate.hpp"

namespace iggy3d_creative_app {

namespace {

using Presentation = CreativeEditorToolPresentation;
using OptionSpec = CreativeEditorToolOptionSpec;
using ActionSpec = CreativeEditorToolActionSpec;
using Choice = CreativeEditorToolOptionChoice;

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

constexpr Choice kTerrainOperationChoices[] = {
    {CreativeEditorToolGlyph::RegionFlatten, "Flatten",
     static_cast<std::uint8_t>(
         CreativeEditorWorldLayoutTerrainRegionOperation::Flatten)},
    {CreativeEditorToolGlyph::RegionRaise, "Raise",
     static_cast<std::uint8_t>(
         CreativeEditorWorldLayoutTerrainRegionOperation::Raise)},
    {CreativeEditorToolGlyph::RegionLower, "Lower",
     static_cast<std::uint8_t>(
         CreativeEditorWorldLayoutTerrainRegionOperation::Lower)},
    {CreativeEditorToolGlyph::RegionSmooth, "Smooth",
     static_cast<std::uint8_t>(
         CreativeEditorWorldLayoutTerrainRegionOperation::Smooth)},
    {CreativeEditorToolGlyph::RegionNoise, "Noise",
     static_cast<std::uint8_t>(
         CreativeEditorWorldLayoutTerrainRegionOperation::Noise)},
};

constexpr Choice kTerrainMaskChoices[] = {
    {CreativeEditorToolGlyph::MaskRectangle, "Rectangle mask",
     static_cast<std::uint8_t>(cr::CreativeTerrainCompositionMask::Rectangle)},
    {CreativeEditorToolGlyph::MaskEllipse, "Ellipse mask",
     static_cast<std::uint8_t>(cr::CreativeTerrainCompositionMask::Ellipse)},
};

constexpr OptionSpec kTerrainRegionOptions[] = {
    {CreativeEditorToolOptionBinding::TerrainOperation,
     CreativeEditorToolOptionWidget::GlyphChoice, "Operation", "", 0.0, 0.0,
     kTerrainOperationChoices},
    {CreativeEditorToolOptionBinding::TerrainMask,
     CreativeEditorToolOptionWidget::GlyphChoice, "Shape", "", 0.0, 0.0,
     kTerrainMaskChoices},
    {CreativeEditorToolOptionBinding::TerrainTargetHeight,
     CreativeEditorToolOptionWidget::IntDrag, "", "",
     static_cast<double>(cr::kCreativeTerrainMinimumHeightCells),
     static_cast<double>(cr::kCreativeTerrainMaximumHeightCells),
     {}},
    {CreativeEditorToolOptionBinding::TerrainNoiseRelief,
     CreativeEditorToolOptionWidget::IntDrag, "Relief", "Relief", 0.0,
     static_cast<double>(cr::kCreativeTerrainMaximumHeightCells),
     {}},
    {CreativeEditorToolOptionBinding::TerrainNoiseScale,
     CreativeEditorToolOptionWidget::DoubleDrag, "Scale", "Scale",
     cr::kCreativeTerrainGeneratorMinimumHorizontalScaleCells,
     cr::kCreativeTerrainGeneratorMaximumHorizontalScaleCells,
     {}},
    {CreativeEditorToolOptionBinding::TerrainSeed,
     CreativeEditorToolOptionWidget::SeedButton, "New seed", "", 0.0, 0.0,
     {}},
    {CreativeEditorToolOptionBinding::TerrainFeather,
     CreativeEditorToolOptionWidget::IntDrag, "Feather", "Feather", 0.0,
     static_cast<double>(cr::kCreativeTerrainCompositionMaximumFeatherCells),
     {}},
};

constexpr ActionSpec kTerrainRegionActions[] = {
    {CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview,
     CreativeEditorToolGlyph::ActionPreview, "Preview",
     CreativeEditorToolActionRule::IdleValidRegion, true},
    {CreativeDesktopCommandId::WorldLayoutTerrainRegionApply,
     CreativeEditorToolGlyph::ActionApply, "Apply region",
     CreativeEditorToolActionRule::OwnedPreview, false},
    {CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel,
     CreativeEditorToolGlyph::ActionCancel, "Cancel",
     CreativeEditorToolActionRule::RegionActivity, false},
};

constexpr std::uint8_t kTerrainRegionLocks =
    kCreativeEditorToolLockTemplatePlacement |
    kCreativeEditorToolLockBuildingTransform |
    kCreativeEditorToolLockForeignTerrainPreview |
    kCreativeEditorToolLockExactLayoutPreview;

Presentation makeTool(CreativeEditorToolPresentationId id,
                      CreativeEditorWorldLayoutPaletteCategory category,
                      CreativeEditorWorldLayoutTool tool,
                      std::string_view name, bool toolbox = true) {
  Presentation presentation;
  presentation.id = id;
  presentation.activation = CreativeEditorToolActivation::WorldLayoutTool;
  presentation.category = category;
  presentation.glyph = creativeEditorToolGlyphForWorldLayoutTool(tool);
  presentation.name = name;
  presentation.toolbox = toolbox;
  presentation.tool = tool;
  return presentation;
}

Presentation makeEstateHouse() {
  Presentation presentation;
  presentation.id = CreativeEditorToolPresentationId::EstateHouse;
  presentation.activation = CreativeEditorToolActivation::BuildingTemplate;
  presentation.category = CreativeEditorWorldLayoutPaletteCategory::Structure;
  presentation.glyph = CreativeEditorToolGlyph::EstateHouse;
  presentation.name = "Estate House";
  presentation.buildingTemplateId = cr::kBuilderEstateHouseTemplateId;
  presentation.locks = kCreativeEditorToolLockMissingTemplate;
  return presentation;
}

Presentation makeTerrainRegion() {
  Presentation presentation;
  presentation.id = CreativeEditorToolPresentationId::TerrainRegion;
  presentation.activation = CreativeEditorToolActivation::TerrainRegionSession;
  presentation.category = CreativeEditorWorldLayoutPaletteCategory::Terrain;
  presentation.glyph = CreativeEditorToolGlyph::MaskRectangle;
  presentation.name = "Terrain region";
  presentation.hint = "Drag on the map to select a region.";
  presentation.locks = kTerrainRegionLocks;
  presentation.options = kTerrainRegionOptions;
  presentation.actions = kTerrainRegionActions;
  return presentation;
}

using Category = CreativeEditorWorldLayoutPaletteCategory;
using Id = CreativeEditorToolPresentationId;
using Tool = CreativeEditorWorldLayoutTool;

const std::array<Presentation, static_cast<std::size_t>(Id::Count)>&
toolPresentationTable() noexcept {
  static const std::array<Presentation, static_cast<std::size_t>(Id::Count)>
      table = {{
          makeTool(Id::Select, Category::Structure, Tool::Select, "Select"),
          makeEstateHouse(),
          makeTool(Id::BuildingShell, Category::Structure,
                   Tool::BuildingShell, "Building Shell"),
          makeTool(Id::Room, Category::Structure, Tool::Room, "Add Room"),
          makeTool(Id::Floor, Category::Structure, Tool::Floor, "Floor"),
          makeTool(Id::Partition, Category::Structure, Tool::Wall,
                   "Partition"),
          makeTool(Id::Door, Category::Structure, Tool::Door, "Door"),
          makeTool(Id::Window, Category::Structure, Tool::Window, "Window"),
          makeTool(Id::Stair, Category::Structure, Tool::Stair, "Stair"),
          makeTool(Id::Ramp, Category::Structure, Tool::Ramp, "Ramp"),
          makeTool(Id::Plateau, Category::Terrain, Tool::Plateau, "Plateau"),
          makeTool(Id::Road, Category::Terrain, Tool::Road, "Road"),
          makeTool(Id::Ditch, Category::Terrain, Tool::Ditch, "Ditch"),
          makeTerrainRegion(),
          makeTool(Id::Bridge, Category::Object, Tool::Bridge, "Bridge"),
          makeTool(Id::CatalogAsset, Category::Object, Tool::CatalogAsset,
                   "Catalog asset", false),
          makeTool(Id::PlayerSpawn, Category::Gameplay, Tool::PlayerSpawn,
                   "Player Spawn"),
          makeTool(Id::NpcSpawn, Category::Gameplay, Tool::NpcSpawn,
                   "NPC Spawn"),
      }};
  return table;
}

}  // namespace

std::span<const CreativeEditorToolPresentation>
creativeEditorToolPresentations() noexcept {
  return toolPresentationTable();
}

const CreativeEditorToolPresentation& findCreativeEditorToolPresentation(
    CreativeEditorToolPresentationId id) noexcept {
  const auto index = static_cast<std::size_t>(id);
  if (index >= toolPresentationTable().size()) {
    return toolPresentationTable().front();
  }
  return toolPresentationTable()[index];
}

const CreativeEditorToolPresentation& activeCreativeEditorToolPresentation(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  if (topography.region.editingEnabled) {
    return findCreativeEditorToolPresentation(
        CreativeEditorToolPresentationId::TerrainRegion);
  }
  if (state.buildingTemplatePlacement.active) {
    for (const CreativeEditorToolPresentation& presentation :
         toolPresentationTable()) {
      if (presentation.activation !=
          CreativeEditorToolActivation::BuildingTemplate) {
        continue;
      }
      const std::size_t templateIndex = findBuildingTemplate(
          state.buildingTemplates, presentation.buildingTemplateId);
      if (templateIndex != cr::kInvalidCreativeWorldLayoutIndex &&
          templateIndex == state.buildingTemplatePlacement.templateIndex) {
        return presentation;
      }
    }
    return findCreativeEditorToolPresentation(
        CreativeEditorToolPresentationId::EstateHouse);
  }
  for (const CreativeEditorToolPresentation& presentation :
       toolPresentationTable()) {
    if (presentation.activation ==
            CreativeEditorToolActivation::WorldLayoutTool &&
        presentation.tool == state.tool) {
      return presentation;
    }
  }
  return toolPresentationTable().front();
}

CreativeEditorToolPresentationStatus evaluateCreativeEditorToolPresentation(
    const CreativeEditorToolPresentation& presentation,
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration) noexcept {
  CreativeEditorToolPresentationStatus status;
  const std::uint8_t locks = presentation.locks;
  std::size_t templateIndex = cr::kInvalidCreativeWorldLayoutIndex;
  if (presentation.activation ==
      CreativeEditorToolActivation::BuildingTemplate) {
    templateIndex = findBuildingTemplate(state.buildingTemplates,
                                         presentation.buildingTemplateId);
  }
  if ((locks & kCreativeEditorToolLockTemplatePlacement) != 0U &&
      state.buildingTemplatePlacement.active) {
    status.unavailable = true;
  }
  if ((locks & kCreativeEditorToolLockBuildingTransform) != 0U &&
      state.buildingTransform.active) {
    status.unavailable = true;
  }
  if ((locks & kCreativeEditorToolLockForeignTerrainPreview) != 0U &&
      terrainGeneration.previewActive && !topography.region.ownsPreview) {
    status.unavailable = true;
  }
  if ((locks & kCreativeEditorToolLockExactLayoutPreview) != 0U &&
      creativeEditorWorldLayoutPreviewActive(state)) {
    status.unavailable = true;
  }
  if ((locks & kCreativeEditorToolLockMissingTemplate) != 0U &&
      templateIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    status.unavailable = true;
  }
  switch (presentation.activation) {
    case CreativeEditorToolActivation::WorldLayoutTool:
      status.active = state.tool == presentation.tool &&
                      !state.buildingTemplatePlacement.active;
      break;
    case CreativeEditorToolActivation::BuildingTemplate:
      status.active = !status.unavailable &&
                      state.buildingTemplatePlacement.active &&
                      state.buildingTemplatePlacement.templateIndex ==
                          templateIndex;
      break;
    case CreativeEditorToolActivation::TerrainRegionSession:
      status.active = topography.region.editingEnabled;
      break;
    case CreativeEditorToolActivation::Count:
      break;
  }
  return status;
}

bool creativeEditorToolOptionVisible(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  const CreativeEditorWorldLayoutTerrainRegionOperation operation =
      topography.region.operation;
  if (operation >= CreativeEditorWorldLayoutTerrainRegionOperation::Count) {
    return false;
  }
  const bool noise =
      operation == CreativeEditorWorldLayoutTerrainRegionOperation::Noise;
  switch (option.binding) {
    case CreativeEditorToolOptionBinding::TerrainOperation:
    case CreativeEditorToolOptionBinding::TerrainMask:
    case CreativeEditorToolOptionBinding::TerrainFeather:
      return true;
    case CreativeEditorToolOptionBinding::TerrainTargetHeight:
      return operation !=
             CreativeEditorWorldLayoutTerrainRegionOperation::Smooth;
    case CreativeEditorToolOptionBinding::TerrainNoiseRelief:
    case CreativeEditorToolOptionBinding::TerrainNoiseScale:
    case CreativeEditorToolOptionBinding::TerrainSeed:
      return noise;
    case CreativeEditorToolOptionBinding::Count:
      break;
  }
  return false;
}

std::string_view creativeEditorToolOptionFullLabel(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  if (!option.label.empty() ||
      option.binding != CreativeEditorToolOptionBinding::TerrainTargetHeight) {
    return option.label;
  }
  switch (topography.region.operation) {
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

std::string_view creativeEditorToolOptionCompactLabel(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  if (!option.compactLabel.empty() ||
      option.binding != CreativeEditorToolOptionBinding::TerrainTargetHeight) {
    return option.compactLabel;
  }
  switch (topography.region.operation) {
    case CreativeEditorWorldLayoutTerrainRegionOperation::Flatten:
      return "Height";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Raise:
      return "Min";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Lower:
      return "Max";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Noise:
      return "Base";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Smooth:
    case CreativeEditorWorldLayoutTerrainRegionOperation::Count:
      break;
  }
  return "";
}

double creativeEditorToolOptionScalarValue(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  const CreativeEditorWorldLayoutTerrainRegionState& region =
      topography.region;
  switch (option.binding) {
    case CreativeEditorToolOptionBinding::TerrainTargetHeight:
      return static_cast<double>(region.targetHeightCells);
    case CreativeEditorToolOptionBinding::TerrainNoiseRelief:
      return static_cast<double>(region.noiseReliefCells);
    case CreativeEditorToolOptionBinding::TerrainNoiseScale:
      return region.noiseScaleCells;
    case CreativeEditorToolOptionBinding::TerrainFeather:
      return static_cast<double>(region.featherCells);
    case CreativeEditorToolOptionBinding::TerrainOperation:
    case CreativeEditorToolOptionBinding::TerrainMask:
    case CreativeEditorToolOptionBinding::TerrainSeed:
    case CreativeEditorToolOptionBinding::Count:
      break;
  }
  return 0.0;
}

bool setCreativeEditorToolOptionScalarValue(
    const CreativeEditorToolOptionSpec& option,
    CreativeEditorWorldLayoutTopographyState& topography,
    double value) noexcept {
  CreativeEditorWorldLayoutTerrainRegionState& region = topography.region;
  const double clamped = std::clamp(value, option.minimum, option.maximum);
  switch (option.binding) {
    case CreativeEditorToolOptionBinding::TerrainTargetHeight: {
      const auto cells =
          static_cast<std::uint16_t>(std::llround(clamped));
      const bool changed = region.targetHeightCells != cells;
      region.targetHeightCells = cells;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainNoiseRelief: {
      const auto cells =
          static_cast<std::uint16_t>(std::llround(clamped));
      const bool changed = region.noiseReliefCells != cells;
      region.noiseReliefCells = cells;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainNoiseScale: {
      const bool changed = region.noiseScaleCells != clamped;
      region.noiseScaleCells = clamped;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainFeather: {
      const auto cells =
          static_cast<std::uint16_t>(std::llround(clamped));
      const bool changed = region.featherCells != cells;
      region.featherCells = cells;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainOperation:
    case CreativeEditorToolOptionBinding::TerrainMask:
    case CreativeEditorToolOptionBinding::TerrainSeed:
    case CreativeEditorToolOptionBinding::Count:
      break;
  }
  return false;
}

std::uint8_t creativeEditorToolOptionChoiceValue(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  switch (option.binding) {
    case CreativeEditorToolOptionBinding::TerrainOperation:
      return static_cast<std::uint8_t>(topography.region.operation);
    case CreativeEditorToolOptionBinding::TerrainMask:
      return static_cast<std::uint8_t>(topography.region.mask);
    case CreativeEditorToolOptionBinding::TerrainTargetHeight:
    case CreativeEditorToolOptionBinding::TerrainNoiseRelief:
    case CreativeEditorToolOptionBinding::TerrainNoiseScale:
    case CreativeEditorToolOptionBinding::TerrainSeed:
    case CreativeEditorToolOptionBinding::TerrainFeather:
    case CreativeEditorToolOptionBinding::Count:
      break;
  }
  return 0U;
}

bool setCreativeEditorToolOptionChoiceValue(
    const CreativeEditorToolOptionSpec& option,
    CreativeEditorWorldLayoutTopographyState& topography,
    std::uint8_t value) noexcept {
  switch (option.binding) {
    case CreativeEditorToolOptionBinding::TerrainOperation: {
      if (value >= static_cast<std::uint8_t>(
                       CreativeEditorWorldLayoutTerrainRegionOperation::
                           Count)) {
        return false;
      }
      const auto operation =
          static_cast<CreativeEditorWorldLayoutTerrainRegionOperation>(value);
      const bool changed = topography.region.operation != operation;
      topography.region.operation = operation;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainMask: {
      if (value >= static_cast<std::uint8_t>(
                       cr::CreativeTerrainCompositionMask::Count)) {
        return false;
      }
      const auto mask =
          static_cast<cr::CreativeTerrainCompositionMask>(value);
      const bool changed = topography.region.mask != mask;
      topography.region.mask = mask;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainTargetHeight:
    case CreativeEditorToolOptionBinding::TerrainNoiseRelief:
    case CreativeEditorToolOptionBinding::TerrainNoiseScale:
    case CreativeEditorToolOptionBinding::TerrainSeed:
    case CreativeEditorToolOptionBinding::TerrainFeather:
    case CreativeEditorToolOptionBinding::Count:
      break;
  }
  return false;
}

void advanceCreativeEditorToolOptionSeed(
    CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  CreativeEditorWorldLayoutTerrainRegionState& region = topography.region;
  region.seed = region.seed == std::numeric_limits<std::uint64_t>::max()
                    ? 0U
                    : region.seed + 1U;
}

bool creativeEditorToolActionEnabled(
    const CreativeEditorToolActionSpec& action,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  const CreativeEditorWorldLayoutTerrainRegionState& region =
      topography.region;
  switch (action.rule) {
    case CreativeEditorToolActionRule::IdleValidRegion:
      return region.regionValid && !region.selecting;
    case CreativeEditorToolActionRule::OwnedPreview:
      return region.ownsPreview;
    case CreativeEditorToolActionRule::RegionActivity:
      return region.selecting || region.regionValid || region.ownsPreview;
    case CreativeEditorToolActionRule::Count:
      break;
  }
  return false;
}

}  // namespace iggy3d_creative_app
