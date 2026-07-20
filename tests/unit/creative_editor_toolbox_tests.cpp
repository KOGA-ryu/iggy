#include "EditorToolPresentation.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutPanel.hpp"
#include "EditorWorldLayoutPanelInternal.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {

namespace cr = iggy3d::creative;

using iggy3d_creative_app::CreativeEditorTerrainGenerationState;
using iggy3d_creative_app::CreativeEditorToolActionRule;
using iggy3d_creative_app::CreativeEditorToolActionSpec;
using iggy3d_creative_app::CreativeEditorToolActivation;
using iggy3d_creative_app::CreativeEditorToolGlyph;
using iggy3d_creative_app::CreativeEditorToolOptionBinding;
using iggy3d_creative_app::CreativeEditorToolOptionSpec;
using iggy3d_creative_app::CreativeEditorToolOptionWidget;
using iggy3d_creative_app::CreativeEditorToolPresentation;
using iggy3d_creative_app::CreativeEditorToolPresentationId;
using iggy3d_creative_app::CreativeEditorWorldLayoutPaletteActivation;
using iggy3d_creative_app::CreativeEditorWorldLayoutPaletteCategory;
using iggy3d_creative_app::CreativeEditorWorldLayoutState;
using iggy3d_creative_app::CreativeEditorWorldLayoutTerrainRegionOperation;
using iggy3d_creative_app::CreativeEditorWorldLayoutTool;
using iggy3d_creative_app::CreativeEditorWorldLayoutTopographyState;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const CreativeEditorToolPresentation* findPresentation(
    std::string_view name) {
  for (const CreativeEditorToolPresentation& presentation :
       iggy3d_creative_app::creativeEditorToolPresentations()) {
    if (presentation.name == name) {
      return &presentation;
    }
  }
  return nullptr;
}

const CreativeEditorToolOptionSpec* findOption(
    const CreativeEditorToolPresentation& presentation,
    CreativeEditorToolOptionBinding binding) {
  for (const CreativeEditorToolOptionSpec& option : presentation.options) {
    if (option.binding == binding) {
      return &option;
    }
  }
  return nullptr;
}

bool presentationsMirrorThePaletteExactly() {
  const auto presentations =
      iggy3d_creative_app::creativeEditorToolPresentations();
  const auto palette =
      iggy3d_creative_app::creativeEditorWorldLayoutPaletteEntries();
  bool idsUnique = true;
  bool categoriesOrdered = true;
  bool glyphsInked = true;
  std::uint8_t lastCategory = 0U;
  for (std::size_t index = 0U; index < presentations.size(); ++index) {
    const CreativeEditorToolPresentation& presentation = presentations[index];
    const auto categoryValue =
        static_cast<std::uint8_t>(presentation.category);
    if (categoryValue < lastCategory) {
      categoriesOrdered = false;
    }
    lastCategory = categoryValue;
    if (iggy3d_creative_app::creativeEditorToolGlyphOps(presentation.glyph)
            .empty()) {
      glyphsInked = false;
    }
    for (std::size_t other = index + 1U; other < presentations.size();
         ++other) {
      if (presentation.id == presentations[other].id) {
        idsUnique = false;
      }
    }
  }
  bool paletteCovered = true;
  for (const auto& paletteEntry : palette) {
    const CreativeEditorToolPresentation* presentation =
        findPresentation(paletteEntry.label);
    if (presentation == nullptr ||
        presentation->category != paletteEntry.category ||
        !presentation->toolbox) {
      paletteCovered = false;
      continue;
    }
    if (paletteEntry.activation ==
        CreativeEditorWorldLayoutPaletteActivation::Tool) {
      if (presentation->activation !=
              CreativeEditorToolActivation::WorldLayoutTool ||
          presentation->tool != paletteEntry.tool) {
        paletteCovered = false;
      }
    } else if (presentation->activation !=
                   CreativeEditorToolActivation::BuildingTemplate ||
               presentation->buildingTemplateId !=
                   paletteEntry.buildingTemplateId) {
      paletteCovered = false;
    }
  }
  const CreativeEditorToolPresentation* terrainRegion =
      findPresentation("Terrain region");
  const CreativeEditorToolPresentation* catalogAsset =
      findPresentation("Catalog asset");
  const bool regionShapeRight =
      terrainRegion != nullptr && terrainRegion->toolbox &&
      terrainRegion->category ==
          CreativeEditorWorldLayoutPaletteCategory::Terrain &&
      terrainRegion->glyph == CreativeEditorToolGlyph::MaskRectangle &&
      terrainRegion->options.size() == 7U &&
      terrainRegion->actions.size() == 3U &&
      terrainRegion->locks != iggy3d_creative_app::kCreativeEditorToolLockNone;
  return expect(presentations.size() == palette.size() + 2U,
                "the table is the palette plus terrain region and catalog") &&
         expect(idsUnique, "presentation ids never collide") &&
         expect(categoriesOrdered, "the table is ordered category-major") &&
         expect(glyphsInked, "every presentation carries a real glyph") &&
         expect(paletteCovered,
                "every palette entry has a faithful presentation") &&
         expect(regionShapeRight,
                "the terrain region session declares its options, actions, "
                "and locks") &&
         expect(catalogAsset != nullptr && !catalogAsset->toolbox,
                "the catalog asset presentation stays out of the toolbox");
}

bool evaluationTracksActiveToolAndTemplates() {
  CreativeEditorWorldLayoutState state;
  const CreativeEditorWorldLayoutTopographyState topography;
  const CreativeEditorTerrainGenerationState terrainGeneration;
  const CreativeEditorToolPresentation* selectPresentation =
      findPresentation("Select");
  const CreativeEditorToolPresentation* partitionPresentation =
      findPresentation("Partition");
  const CreativeEditorToolPresentation* estatePresentation =
      findPresentation("Estate House");
  if (!expect(selectPresentation != nullptr &&
                  partitionPresentation != nullptr &&
                  estatePresentation != nullptr,
              "select, partition, and estate house presentations exist")) {
    return false;
  }
  const auto evaluate = [&](const CreativeEditorToolPresentation& p) {
    return iggy3d_creative_app::evaluateCreativeEditorToolPresentation(
        p, state, topography, terrainGeneration);
  };
  const auto defaultSelect = evaluate(*selectPresentation);
  const auto defaultPartition = evaluate(*partitionPresentation);
  const auto estateWithoutLibrary = evaluate(*estatePresentation);
  state.tool = CreativeEditorWorldLayoutTool::Wall;
  const auto wallSelect = evaluate(*selectPresentation);
  const auto wallPartition = evaluate(*partitionPresentation);
  cr::CreativeWorldLayoutBuildingTemplate estateTemplate;
  estateTemplate.templateId =
      std::string(estatePresentation->buildingTemplateId);
  state.buildingTemplates.templates.push_back(estateTemplate);
  const auto estateIdle = evaluate(*estatePresentation);
  state.buildingTemplatePlacement.active = true;
  state.buildingTemplatePlacement.templateIndex = 0U;
  const auto estatePlacing = evaluate(*estatePresentation);
  const auto partitionDuringPlacement = evaluate(*partitionPresentation);
  return expect(defaultSelect.active && !defaultSelect.unavailable,
                "select is the default active tool") &&
         expect(!defaultPartition.active, "partition starts inactive") &&
         expect(estateWithoutLibrary.unavailable,
                "estate house is unavailable without its template") &&
         expect(!wallSelect.active && wallPartition.active,
                "activating wall moves the highlight to partition") &&
         expect(!estateIdle.unavailable && !estateIdle.active,
                "a loaded template is available but idle") &&
         expect(estatePlacing.active,
                "placing the template highlights its presentation") &&
         expect(!partitionDuringPlacement.active,
                "tool highlights drop during template placement");
}

bool terrainSessionLocksFollowTheWorkflow() {
  CreativeEditorWorldLayoutState state;
  CreativeEditorWorldLayoutTopographyState topography;
  CreativeEditorTerrainGenerationState terrainGeneration;
  const CreativeEditorToolPresentation* session =
      findPresentation("Terrain region");
  if (!expect(session != nullptr, "the terrain region session exists")) {
    return false;
  }
  const auto evaluate = [&]() {
    return iggy3d_creative_app::evaluateCreativeEditorToolPresentation(
        *session, state, topography, terrainGeneration);
  };
  const auto idle = evaluate();
  topography.region.editingEnabled = true;
  const auto editing = evaluate();
  terrainGeneration.previewActive = true;
  const auto foreignPreview = evaluate();
  topography.region.ownsPreview = true;
  const auto ownedPreview = evaluate();
  terrainGeneration.previewActive = false;
  topography.region.ownsPreview = false;
  state.buildingTransform.active = true;
  const auto duringTransform = evaluate();
  state.buildingTransform.active = false;
  state.buildingTemplatePlacement.active = true;
  const auto duringPlacement = evaluate();
  return expect(!idle.active && !idle.unavailable,
                "the session starts idle and available") &&
         expect(editing.active, "enabling region editing lights the session") &&
         expect(foreignPreview.unavailable,
                "a foreign terrain preview locks the session") &&
         expect(!ownedPreview.unavailable,
                "owning the preview keeps the session available") &&
         expect(duringTransform.unavailable,
                "building transforms lock the session") &&
         expect(duringPlacement.unavailable,
                "template placement locks the session");
}

bool activePresentationFollowsTheSession() {
  CreativeEditorWorldLayoutState state;
  CreativeEditorWorldLayoutTopographyState topography;
  const auto active = [&]() {
    return iggy3d_creative_app::activeCreativeEditorToolPresentation(
               state, topography)
        .id;
  };
  const auto defaultActive = active();
  state.tool = CreativeEditorWorldLayoutTool::Wall;
  const auto wallActive = active();
  state.tool = CreativeEditorWorldLayoutTool::CatalogAsset;
  const auto catalogActive = active();
  topography.region.editingEnabled = true;
  const auto regionActive = active();
  topography.region.editingEnabled = false;
  const CreativeEditorToolPresentation* estatePresentation =
      findPresentation("Estate House");
  cr::CreativeWorldLayoutBuildingTemplate estateTemplate;
  estateTemplate.templateId =
      std::string(estatePresentation->buildingTemplateId);
  state.buildingTemplates.templates.push_back(estateTemplate);
  state.buildingTemplatePlacement.active = true;
  state.buildingTemplatePlacement.templateIndex = 0U;
  const auto placingActive = active();
  return expect(defaultActive == CreativeEditorToolPresentationId::Select,
                "select is the default presentation") &&
         expect(wallActive == CreativeEditorToolPresentationId::Partition,
                "the wall tool activates the partition presentation") &&
         expect(catalogActive ==
                    CreativeEditorToolPresentationId::CatalogAsset,
                "the catalog tool activates the catalog presentation") &&
         expect(regionActive ==
                    CreativeEditorToolPresentationId::TerrainRegion,
                "region editing takes over the active presentation") &&
         expect(placingActive ==
                    CreativeEditorToolPresentationId::EstateHouse,
                "template placement activates the template presentation");
}

bool optionsExposeTheRightFieldsPerOperation() {
  CreativeEditorWorldLayoutTopographyState topography;
  const CreativeEditorToolPresentation* session =
      findPresentation("Terrain region");
  if (!expect(session != nullptr, "the terrain region session exists")) {
    return false;
  }
  const CreativeEditorToolOptionSpec* operationOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainOperation);
  const CreativeEditorToolOptionSpec* maskOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainMask);
  const CreativeEditorToolOptionSpec* heightOption = findOption(
      *session, CreativeEditorToolOptionBinding::TerrainTargetHeight);
  const CreativeEditorToolOptionSpec* reliefOption = findOption(
      *session, CreativeEditorToolOptionBinding::TerrainNoiseRelief);
  const CreativeEditorToolOptionSpec* scaleOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainNoiseScale);
  const CreativeEditorToolOptionSpec* seedOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainSeed);
  const CreativeEditorToolOptionSpec* featherOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainFeather);
  if (!expect(operationOption != nullptr && maskOption != nullptr &&
                  heightOption != nullptr && reliefOption != nullptr &&
                  scaleOption != nullptr && seedOption != nullptr &&
                  featherOption != nullptr,
              "all seven terrain options are declared")) {
    return false;
  }
  using Operation = CreativeEditorWorldLayoutTerrainRegionOperation;
  bool visibilityRight = true;
  for (const Operation operation :
       {Operation::Flatten, Operation::Raise, Operation::Lower,
        Operation::Smooth, Operation::Noise}) {
    topography.region.operation = operation;
    const bool noise = operation == Operation::Noise;
    const auto visible = [&](const CreativeEditorToolOptionSpec* option) {
      return iggy3d_creative_app::creativeEditorToolOptionVisible(*option,
                                                                  topography);
    };
    if (!visible(operationOption) || !visible(maskOption) ||
        !visible(featherOption) ||
        visible(heightOption) != (operation != Operation::Smooth) ||
        visible(reliefOption) != noise || visible(scaleOption) != noise ||
        visible(seedOption) != noise) {
      visibilityRight = false;
    }
  }
  topography.region.operation = Operation::Raise;
  const std::string_view raiseFull =
      iggy3d_creative_app::creativeEditorToolOptionFullLabel(*heightOption,
                                                             topography);
  const std::string_view raiseCompact =
      iggy3d_creative_app::creativeEditorToolOptionCompactLabel(*heightOption,
                                                                topography);
  topography.region.operation = Operation::Noise;
  const std::string_view noiseFull =
      iggy3d_creative_app::creativeEditorToolOptionFullLabel(*heightOption,
                                                             topography);
  const bool clampsRight =
      heightOption->minimum ==
          static_cast<double>(cr::kCreativeTerrainMinimumHeightCells) &&
      heightOption->maximum ==
          static_cast<double>(cr::kCreativeTerrainMaximumHeightCells) &&
      featherOption->maximum ==
          static_cast<double>(
              cr::kCreativeTerrainCompositionMaximumFeatherCells) &&
      scaleOption->minimum ==
          cr::kCreativeTerrainGeneratorMinimumHorizontalScaleCells &&
      scaleOption->maximum ==
          cr::kCreativeTerrainGeneratorMaximumHorizontalScaleCells;
  const bool choicesRight =
      operationOption->widget == CreativeEditorToolOptionWidget::GlyphChoice &&
      operationOption->choices.size() == 5U &&
      operationOption->choices[4].glyph ==
          CreativeEditorToolGlyph::RegionNoise &&
      operationOption->choices[4].value ==
          static_cast<std::uint8_t>(Operation::Noise) &&
      maskOption->choices.size() == 2U &&
      maskOption->choices[1].glyph == CreativeEditorToolGlyph::MaskEllipse;
  return expect(visibilityRight,
                "option visibility follows the operation exactly") &&
         expect(raiseFull == "Raise to at least" && raiseCompact == "Min" &&
                    noiseFull == "Base height",
                "the target height labels follow the operation") &&
         expect(clampsRight, "option clamps pin the engine constants") &&
         expect(choicesRight,
                "operation and mask choices carry the region glyphs");
}

bool optionBindingsReadWriteAndClamp() {
  CreativeEditorWorldLayoutTopographyState topography;
  const CreativeEditorToolPresentation* session =
      findPresentation("Terrain region");
  const CreativeEditorToolOptionSpec* heightOption = findOption(
      *session, CreativeEditorToolOptionBinding::TerrainTargetHeight);
  const CreativeEditorToolOptionSpec* reliefOption = findOption(
      *session, CreativeEditorToolOptionBinding::TerrainNoiseRelief);
  const CreativeEditorToolOptionSpec* scaleOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainNoiseScale);
  const CreativeEditorToolOptionSpec* operationOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainOperation);
  const bool heightChanged =
      iggy3d_creative_app::setCreativeEditorToolOptionScalarValue(
          *heightOption, topography, 12.0);
  const bool heightUnchanged =
      !iggy3d_creative_app::setCreativeEditorToolOptionScalarValue(
          *heightOption, topography, 12.0);
  const bool heightStored = topography.region.targetHeightCells == 12U;
  static_cast<void>(iggy3d_creative_app::setCreativeEditorToolOptionScalarValue(
      *reliefOption, topography, 99999.0));
  const bool reliefClamped =
      topography.region.noiseReliefCells ==
      cr::kCreativeTerrainMaximumHeightCells;
  static_cast<void>(iggy3d_creative_app::setCreativeEditorToolOptionScalarValue(
      *scaleOption, topography, 0.0));
  const bool scaleClamped =
      topography.region.noiseScaleCells ==
      cr::kCreativeTerrainGeneratorMinimumHorizontalScaleCells;
  const bool operationChanged =
      iggy3d_creative_app::setCreativeEditorToolOptionChoiceValue(
          *operationOption, topography,
          static_cast<std::uint8_t>(
              CreativeEditorWorldLayoutTerrainRegionOperation::Noise));
  const bool operationStored =
      topography.region.operation ==
      CreativeEditorWorldLayoutTerrainRegionOperation::Noise;
  const bool invalidRejected =
      !iggy3d_creative_app::setCreativeEditorToolOptionChoiceValue(
          *operationOption, topography, 99U);
  topography.region.seed = 5U;
  iggy3d_creative_app::advanceCreativeEditorToolOptionSeed(topography);
  const bool seedAdvanced = topography.region.seed == 6U;
  topography.region.seed = std::numeric_limits<std::uint64_t>::max();
  iggy3d_creative_app::advanceCreativeEditorToolOptionSeed(topography);
  const bool seedWrapped = topography.region.seed == 0U;
  return expect(heightChanged && heightStored,
                "setting the target height writes the region state") &&
         expect(heightUnchanged,
                "rewriting the same value reports no change") &&
         expect(reliefClamped, "relief writes clamp to the engine maximum") &&
         expect(scaleClamped, "scale writes clamp to the engine minimum") &&
         expect(operationChanged && operationStored,
                "choice writes switch the region operation") &&
         expect(invalidRejected, "out-of-range choices are rejected") &&
         expect(seedAdvanced && seedWrapped,
                "the seed advances and wraps at the numeric limit");
}

bool actionRulesMatchTheBuildWindow() {
  CreativeEditorWorldLayoutTopographyState topography;
  const CreativeEditorToolPresentation* session =
      findPresentation("Terrain region");
  if (!expect(session != nullptr && session->actions.size() == 3U,
              "the session declares three actions")) {
    return false;
  }
  const CreativeEditorToolActionSpec& preview = session->actions[0];
  const CreativeEditorToolActionSpec& apply = session->actions[1];
  const CreativeEditorToolActionSpec& cancel = session->actions[2];
  const bool commandsRight =
      preview.command ==
          iggy3d_creative_app::CreativeDesktopCommandId::
              WorldLayoutTerrainRegionPreview &&
      apply.command == iggy3d_creative_app::CreativeDesktopCommandId::
                           WorldLayoutTerrainRegionApply &&
      cancel.command == iggy3d_creative_app::CreativeDesktopCommandId::
                            WorldLayoutTerrainRegionCancel;
  const bool onChangeRight = preview.runOnOptionChange &&
                             !apply.runOnOptionChange &&
                             !cancel.runOnOptionChange;
  const auto enabled = [&](const CreativeEditorToolActionSpec& action) {
    return iggy3d_creative_app::creativeEditorToolActionEnabled(action,
                                                                topography);
  };
  const bool idleAllDisabled =
      !enabled(preview) && !enabled(apply) && !enabled(cancel);
  topography.region.regionValid = true;
  const bool validRegion =
      enabled(preview) && !enabled(apply) && enabled(cancel);
  topography.region.selecting = true;
  const bool whileSelecting = !enabled(preview) && enabled(cancel);
  topography.region.selecting = false;
  topography.region.ownsPreview = true;
  const bool owningPreview = enabled(apply);
  return expect(commandsRight, "the actions pin the region command ids") &&
         expect(onChangeRight,
                "only preview re-runs after an option change") &&
         expect(idleAllDisabled, "an empty region disables every action") &&
         expect(validRegion,
                "a valid idle region enables preview and cancel") &&
         expect(whileSelecting,
                "selecting suspends preview but keeps cancel") &&
         expect(owningPreview, "owning the preview enables apply");
}

bool statusLineComposesFromTheRightSources() {
  CreativeEditorWorldLayoutState state;
  CreativeEditorWorldLayoutTopographyState topography;
  CreativeEditorTerrainGenerationState terrainGeneration;
  iggy3d_creative_app::CreativeEditorWorldLayoutCanvasHoverStatus hover;
  const auto compose = [&]() {
    return iggy3d_creative_app::composeCreativeEditorWorldLayoutStatusLine(
        state, topography, terrainGeneration, hover);
  };
  const auto idle = compose();
  hover.present = true;
  hover.cellX = -0.2;
  hover.cellZ = 5.9;
  hover.semanticRole = "exterior wall";
  state.canvasPixelsPerCell = 33.6F;
  const auto hovered = compose();
  state.tool = CreativeEditorWorldLayoutTool::CatalogAsset;
  state.catalogPlacement.snapMode =
      iggy3d_creative_app::CreativeEditorWorldLayoutCatalogSnapMode::Wall;
  const auto snapping = compose();
  topography.region.editingEnabled = true;
  topography.region.regionValid = true;
  topography.region.bounds.minimum.x = -1;
  topography.region.bounds.minimum.z = -1;
  topography.region.bounds.widthCells = 2U;
  topography.region.bounds.depthCells = 2U;
  const auto validRegion = compose();
  topography.region.ownsPreview = true;
  terrainGeneration.operationPreview.receipt.replay.modifiedCellCount = 57U;
  terrainGeneration.operationPreview.receipt.replay.outputCellCount = 4096U;
  const auto owningPreview = compose();
  return expect(idle.cursor == "Cell --" && idle.zoom == "Zoom 100%",
                "the idle bar shows no cell and the baseline zoom") &&
         expect(idle.snap.empty() && idle.cells.empty(),
                "the idle bar omits snap and cell segments") &&
         expect(!idle.regionMessage && idle.message == state.statusMessage,
                "outside region editing the layout status is the message") &&
         expect(hovered.cursor == "Cell -1, 5 | exterior wall",
                "hover cursor names the semantic plan target") &&
         expect(hovered.zoom == "Zoom 120%",
                "zoom reads as a percentage of the default scale") &&
         expect(snapping.snap == "Snap wall",
                "the catalog tool reports its snap mode") &&
         expect(validRegion.regionMessage &&
                    validRegion.message == topography.region.statusMessage,
                "region editing promotes the region status message") &&
         expect(validRegion.cells == "4 candidate cells",
                "a valid region reports its candidate cells") &&
         expect(owningPreview.cells == "Changed 57 / 4096 cells",
                "an owned preview reports the replay cell counts") &&
         expect(owningPreview.phase ==
                    iggy3d_creative_app::
                        CreativeEditorWorldLayoutTerrainRegionPhase::Ready,
                "owning the preview classifies as ready");
}

bool blockoutDraftAndPatternChoicesArePinned() {
  const iggy3d_creative_app::CreativeEditorDesktopUiState desktopUi;
  const auto& draft = desktopUi.worldLayoutBlockoutDraft;
  const iggy3d_creative_app::CreativeEditorWorldLayoutRoomSettings defaults;
  const bool footprintRight = draft.shell.footprint.minimum.x == 0 &&
                              draft.shell.footprint.minimum.z == 0 &&
                              draft.shell.footprint.maximum.x == 8 &&
                              draft.shell.footprint.maximum.z == 8;
  const bool shellKeepsDefaults =
      draft.shell.floorTopLayer == defaults.floorTopLayer &&
      draft.shell.wallHeightCells == defaults.wallHeightCells &&
      draft.shell.wallThicknessCells == defaults.wallThicknessCells &&
      draft.shell.floorThicknessLayers == defaults.floorThicknessLayers &&
      draft.shell.roofThicknessLayers == defaults.roofThicknessLayers &&
      draft.shell.roofStyle == defaults.roofStyle &&
      draft.shell.roofRidgeAxis == defaults.roofRidgeAxis &&
      draft.shell.roofPitchDegrees == defaults.roofPitchDegrees &&
      draft.shell.roofOverhangCells == defaults.roofOverhangCells;
  const bool topologyKeepsDefaults =
      draft.connectRooms && draft.facade.includeEntrance &&
      draft.facade.entranceEdge == cr::CreativeWorldLayoutRoomEdge::South &&
      draft.facade.entranceOffsetCells == 0.0 &&
      draft.facade.includeExteriorWindows && draft.storeys.count == 1U &&
      draft.storeys.connectStoreys &&
      draft.storeys.connectorKind ==
          cr::CreativeWorldLayoutVerticalConnectorKind::Stair &&
      draft.storeys.preferredDirection ==
          cr::CreativeWorldLayoutVerticalDirection::PositiveZ;
  const auto choices =
      iggy3d_creative_app::creativeEditorWorldLayoutBlockoutPatternChoices();
  using Pattern = cr::CreativeWorldLayoutBuildingBlockoutPattern;
  const bool choicesRight =
      choices.size() == 4U &&
      std::string_view(choices[0].label) == "1 room" &&
      choices[0].pattern == Pattern::SingleRoom &&
      std::string_view(choices[1].label) == "Split X" &&
      choices[1].pattern == Pattern::SplitX &&
      std::string_view(choices[2].label) == "Split Z" &&
      choices[2].pattern == Pattern::SplitZ &&
      std::string_view(choices[3].label) == "2 x 2" &&
      choices[3].pattern == Pattern::Grid2x2;
  bool patternsUnique = true;
  for (std::size_t index = 0U; index < choices.size(); ++index) {
    for (std::size_t other = index + 1U; other < choices.size(); ++other) {
      if (choices[index].pattern == choices[other].pattern) {
        patternsUnique = false;
      }
    }
  }
  const cr::CreativeWorldLayoutBuildingBlockoutStoreySettings storeyDefaults;
  const bool storeysKeepDefaults =
      draft.storeys.count == storeyDefaults.count &&
      draft.storeys.connectStoreys == storeyDefaults.connectStoreys &&
      draft.storeys.connectorKind == storeyDefaults.connectorKind &&
      draft.storeys.preferredDirection == storeyDefaults.preferredDirection;
  return expect(footprintRight,
                "the blockout draft starts at the 0,0 to 8,8 footprint") &&
         expect(draft.pattern == Pattern::SingleRoom,
                "the blockout draft starts as a single room") &&
         expect(shellKeepsDefaults,
                "non-footprint shell values keep the room-settings defaults") &&
         expect(storeysKeepDefaults,
                "storey settings keep the backend defaults: one storey, "
                "stairs on, stair kind, positive-Z preference") &&
         expect(topologyKeepsDefaults,
                "blockout drafts default to one connected storey, stairs, "
                "and a south facade") &&
         expect(choicesRight,
                "the four pattern choices map one-to-one onto the planner "
                "enum") &&
         expect(patternsUnique, "pattern choices never repeat an enum value");
}

bool blockoutEditDraftSyncFollowsTheSource() {
  iggy3d_creative_app::CreativeEditorDesktopBlockoutEditDraft draft;
  CreativeEditorWorldLayoutState state;
  const auto inSync = [&]() {
    return iggy3d_creative_app::creativeEditorWorldLayoutBlockoutEditInSync(
        draft, state);
  };
  const bool defaultsInert =
      !draft.active &&
      draft.buildingIndex == cr::kInvalidCreativeWorldLayoutIndex &&
      draft.sourceRevision == 0U;
  const bool inactiveOut = !inSync();
  state.source.buildings.emplace_back();
  state.revision = 41U;
  draft.active = true;
  draft.buildingIndex = 0U;
  draft.sourceRevision = 41U;
  const bool freshIn = inSync();
  state.revision = 42U;
  const bool staleOut = !inSync();
  draft.sourceRevision = 42U;
  draft.buildingIndex = 5U;
  const bool missingBuildingOut = !inSync();
  return expect(defaultsInert, "the edit draft defaults are inert") &&
         expect(inactiveOut, "an inactive draft is never in sync") &&
         expect(freshIn,
                "a draft read at the current revision is in sync") &&
         expect(staleOut, "advancing the source revision marks it stale") &&
         expect(missingBuildingOut,
                "a vanished building index is never in sync");
}

}  // namespace

int main() {
  bool ok = true;
  ok = presentationsMirrorThePaletteExactly() && ok;
  ok = evaluationTracksActiveToolAndTemplates() && ok;
  ok = terrainSessionLocksFollowTheWorkflow() && ok;
  ok = activePresentationFollowsTheSession() && ok;
  ok = optionsExposeTheRightFieldsPerOperation() && ok;
  ok = optionBindingsReadWriteAndClamp() && ok;
  ok = actionRulesMatchTheBuildWindow() && ok;
  ok = statusLineComposesFromTheRightSources() && ok;
  ok = blockoutDraftAndPatternChoicesArePinned() && ok;
  ok = blockoutEditDraftSyncFollowsTheSource() && ok;
  return ok ? 0 : 1;
}
