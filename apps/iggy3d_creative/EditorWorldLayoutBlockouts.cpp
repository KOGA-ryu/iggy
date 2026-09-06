#include "EditorWorldLayoutLifecycle.hpp"
#include "EditorWorldLayoutBuildings.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutBlockoutMaterialization.hpp"
#include "app/iggy3d/creative/world/WorldLayoutArchitecture.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {

using detail::noteWorldLayoutSourceChange;

namespace {

constexpr double kBlockoutGeometryEpsilon = 1.0e-9;

struct BlockoutFailurePresentation {
  std::string_view kernelReason;
  std::string_view editorReason;
  std::string_view message;
};

constexpr std::array<BlockoutFailurePresentation, 4U>
    kBlockoutFailurePresentations{{
        {"creative_world_layout_building_blockout_wall_too_short",
         "creative_editor_world_layout_wall_too_short",
         "Wall is too short for this opening"},
        {"creative_world_layout_building_blockout_opening_height_invalid",
         "creative_editor_world_layout_opening_height_invalid",
         "Opening exceeds the wall height"},
        {"creative_world_layout_building_blockout_opening_overlap",
         "creative_editor_world_layout_opening_overlap",
         "Opening overlaps an existing opening"},
        {"creative_world_layout_building_blockout_opening_host_invalid",
         "creative_editor_world_layout_building_blockout_opening_host",
         "building blockout could not resolve an opening host"},
    }};

const BlockoutFailurePresentation* blockoutFailurePresentation(
    std::string_view reasonCode) noexcept {
  const auto found = std::find_if(
      kBlockoutFailurePresentations.begin(),
      kBlockoutFailurePresentations.end(),
      [reasonCode](const BlockoutFailurePresentation& presentation) {
        return presentation.kernelReason == reasonCode;
      });
  return found == kBlockoutFailurePresentations.end() ? nullptr : &*found;
}

CreativeEditorWorldLayoutEditReceipt commitBuildingBlockoutCandidate(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayout candidate, std::uint64_t nextStableOrdinal,
    std::size_t buildingIndex, std::size_t levelIndex,
    std::string statusMessage =
        "building blockout staged; preview or confirm in 3D",
    std::string reasonCode =
        "creative_editor_world_layout_building_blockout_created") {
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(candidate);
  if (!expanded.accepted) {
    state.statusMessage =
        expanded.status ==
                cr::CreativeWorldLayoutRoomCompileStatus::OverlappingRooms
            ? "rooms may touch but cannot overlap"
            : expanded.reasonCode;
    return {false, false, expanded.reasonCode};
  }
  if (cr::creativeWorldLayoutHasInteriorRoomWindow(candidate)) {
    state.statusMessage =
        "blockout would turn an exterior window into an interior opening";
    return {false, false,
            "creative_editor_world_layout_blockout_interior_window"};
  }

  state.source = std::move(candidate);
  state.nextStableOrdinal = nextStableOrdinal;
  state.activeLevelIndex = levelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  noteWorldLayoutSourceChange(state, std::move(statusMessage));
  return {true, true, std::move(reasonCode)};
}
bool blockoutOverlapsExistingRoom(
    const cr::CreativeWorldLayout& layout,
    const CreativeEditorWorldLayoutRoomSettings& settings,
    double candidateTop,
    std::size_t ignoredBuildingIndex =
        cr::kInvalidCreativeWorldLayoutIndex) noexcept {
  const double candidateBottom = settings.floorTopLayer;
  for (const cr::CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.buildingIndex == ignoredBuildingIndex) {
      continue;
    }
    if (room.levelIndex >= layout.levels.size()) {
      continue;
    }
    const cr::CreativeWorldLayoutRect existing = room.footprint;
    const bool horizontalOverlap =
        std::max(settings.footprint.minimum.x, existing.minimum.x) <
            std::min(settings.footprint.maximum.x, existing.maximum.x) &&
        std::max(settings.footprint.minimum.z, existing.minimum.z) <
            std::min(settings.footprint.maximum.z, existing.maximum.z);
    const cr::CreativeWorldLayoutLevel& level = layout.levels[room.levelIndex];
    const double existingBottom = level.floorTopLayer;
    const double existingTop =
        existingBottom +
        cr::creativeWorldLayoutLevelFacadeHeightCells(layout,
                                                      room.levelIndex);
    const bool verticalOverlap =
        std::max(candidateBottom, existingBottom) <
        std::min(candidateTop, existingTop) - kBlockoutGeometryEpsilon;
    if (horizontalOverlap && verticalOverlap) {
      return true;
    }
  }
  return false;
}

bool blockoutTopLayer(
    double floorTopLayer, std::uint16_t floorToFloorCells,
    std::uint16_t storeyCount, double& topLayer) noexcept {
  const long double value =
      static_cast<long double>(floorTopLayer) +
      static_cast<long double>(floorToFloorCells) * storeyCount;
  if (!std::isfinite(value) ||
      value < -static_cast<long double>(std::numeric_limits<double>::max()) ||
      value > static_cast<long double>(std::numeric_limits<double>::max())) {
    return false;
  }
  topLayer = static_cast<double>(value);
  return std::isfinite(topLayer) && topLayer > floorTopLayer;
}

bool validBlockoutShellSettings(
    const CreativeEditorWorldLayoutBuildingBlockoutSettings& settings)
    noexcept {
  CreativeEditorWorldLayoutRoomSettings shell = settings.shell;
  shell.wallHeightCells = settings.floorToFloorCells;
  return detail::validWorldLayoutShellSettings(shell);
}

cr::CreativeWorldLayoutBuildingBlockoutRecipe blockoutRecipe(
    const CreativeEditorWorldLayoutBuildingBlockoutSettings& settings) {
  cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe;
  recipe.request.footprint = settings.shell.footprint;
  recipe.request.pattern = settings.pattern;
  recipe.request.wallThicknessCells = settings.shell.wallThicknessCells;
  recipe.request.connectRooms = settings.connectRooms;
  recipe.request.facade = settings.facade;
  recipe.request.floorToFloorCells = settings.floorToFloorCells;
  recipe.request.storeys = settings.storeys;
  recipe.floorTopLayer = settings.shell.floorTopLayer;
  recipe.floorThicknessLayers = settings.shell.floorThicknessLayers;
  recipe.ceilingThicknessLayers = settings.ceilingThicknessLayers;
  recipe.roofThicknessLayers = settings.shell.roofThicknessLayers;
  recipe.architecturalProfileKind = settings.architecturalProfileKind;
  recipe.exteriorWallMaterial = settings.exteriorWallMaterial;
  recipe.interiorWallMaterial = settings.interiorWallMaterial;
  recipe.roofStyle = settings.shell.roofStyle;
  recipe.roofRidgeAxis = settings.shell.roofRidgeAxis;
  recipe.roofSlopeDirection = settings.shell.roofSlopeDirection;
  recipe.roofPitchDegrees = settings.shell.roofPitchDegrees;
  recipe.roofOverhangCells = settings.shell.roofOverhangCells;
  recipe.roofMaterial = settings.shell.roofMaterial;
  return recipe;
}

CreativeEditorWorldLayoutBuildingBlockoutSettings blockoutSettings(
    const cr::CreativeWorldLayoutBuildingBlockoutRecipe& recipe) {
  CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
  settings.shell.footprint = recipe.request.footprint;
  settings.shell.floorTopLayer = recipe.floorTopLayer;
  settings.floorToFloorCells = recipe.request.floorToFloorCells;
  settings.shell.wallThicknessCells = recipe.request.wallThicknessCells;
  settings.shell.floorThicknessLayers = recipe.floorThicknessLayers;
  settings.ceilingThicknessLayers = recipe.ceilingThicknessLayers;
  settings.shell.roofThicknessLayers = recipe.roofThicknessLayers;
  settings.architecturalProfileKind = recipe.architecturalProfileKind;
  settings.exteriorWallMaterial = recipe.exteriorWallMaterial;
  settings.interiorWallMaterial = recipe.interiorWallMaterial;
  settings.shell.roofStyle = recipe.roofStyle;
  settings.shell.roofRidgeAxis = recipe.roofRidgeAxis;
  settings.shell.roofSlopeDirection = recipe.roofSlopeDirection;
  settings.shell.roofPitchDegrees = recipe.roofPitchDegrees;
  settings.shell.roofOverhangCells = recipe.roofOverhangCells;
  settings.shell.roofMaterial = recipe.roofMaterial;
  settings.pattern = recipe.request.pattern;
  settings.connectRooms = recipe.request.connectRooms;
  settings.facade = recipe.request.facade;
  settings.storeys = recipe.request.storeys;
  return settings;
}

bool sameBlockoutRecipe(
    const cr::CreativeWorldLayoutBuildingBlockoutRecipe& lhs,
    const cr::CreativeWorldLayoutBuildingBlockoutRecipe& rhs) noexcept {
  return lhs.version == rhs.version &&
         lhs.request.footprint.minimum == rhs.request.footprint.minimum &&
         lhs.request.footprint.maximum == rhs.request.footprint.maximum &&
         lhs.request.pattern == rhs.request.pattern &&
         lhs.request.wallThicknessCells == rhs.request.wallThicknessCells &&
         lhs.request.connectRooms == rhs.request.connectRooms &&
         lhs.request.facade.includeEntrance ==
             rhs.request.facade.includeEntrance &&
         lhs.request.facade.entranceEdge == rhs.request.facade.entranceEdge &&
         lhs.request.facade.entranceOffsetCells ==
             rhs.request.facade.entranceOffsetCells &&
         lhs.request.facade.includeExteriorWindows ==
             rhs.request.facade.includeExteriorWindows &&
         lhs.request.floorToFloorCells == rhs.request.floorToFloorCells &&
         lhs.request.storeys.count == rhs.request.storeys.count &&
         lhs.request.storeys.connectStoreys ==
             rhs.request.storeys.connectStoreys &&
         lhs.request.storeys.connectorKind ==
             rhs.request.storeys.connectorKind &&
         lhs.request.storeys.preferredDirection ==
             rhs.request.storeys.preferredDirection &&
         lhs.floorTopLayer == rhs.floorTopLayer &&
         lhs.floorThicknessLayers == rhs.floorThicknessLayers &&
         lhs.ceilingThicknessLayers == rhs.ceilingThicknessLayers &&
         lhs.roofThicknessLayers == rhs.roofThicknessLayers &&
         lhs.architecturalProfileKind == rhs.architecturalProfileKind &&
         lhs.exteriorWallMaterial == rhs.exteriorWallMaterial &&
         lhs.interiorWallMaterial == rhs.interiorWallMaterial &&
         lhs.roofStyle == rhs.roofStyle &&
         lhs.roofRidgeAxis == rhs.roofRidgeAxis &&
         lhs.roofSlopeDirection == rhs.roofSlopeDirection &&
         lhs.roofPitchDegrees == rhs.roofPitchDegrees &&
         lhs.roofOverhangCells == rhs.roofOverhangCells &&
         lhs.roofMaterial == rhs.roofMaterial;
}

std::size_t firstBuildingLevelIndex(const cr::CreativeWorldLayout& layout,
                                    std::size_t buildingIndex) noexcept {
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    if (layout.levels[index].buildingIndex == buildingIndex) {
      return index;
    }
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

}  // namespace

bool applyCreativeEditorWorldLayoutBlockoutArchitecturalProfile(
    CreativeEditorWorldLayoutBuildingBlockoutSettings& settings,
    cr::CreativeGridSettings grid,
    cr::CreativeWorldLayoutArchitecturalProfileKind kind) noexcept {
  if (kind >= cr::CreativeWorldLayoutArchitecturalProfileKind::Count) {
    return false;
  }
  if (kind == cr::CreativeWorldLayoutArchitecturalProfileKind::Custom) {
    settings.architecturalProfileKind = kind;
    return true;
  }
  const cr::CreativeWorldLayoutArchitecturalProfile profile =
      cr::defaultCreativeWorldLayoutArchitecturalProfile(kind);
  std::uint16_t floorToFloorCells = 0U;
  if (!cr::resolveCreativeWorldLayoutArchitecturalProfileFloorToFloorCells(
          grid, profile, floorToFloorCells)) {
    return false;
  }
  settings.architecturalProfileKind = kind;
  settings.floorToFloorCells = floorToFloorCells;
  settings.shell.floorThicknessLayers = profile.floorThicknessLayers;
  settings.ceilingThicknessLayers = profile.ceilingThicknessLayers;
  settings.shell.roofThicknessLayers = profile.roofThicknessLayers;
  return true;
}

CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutBuildingBlockout(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingBlockoutSettings settings) {
  if (!validBlockoutShellSettings(settings)) {
    state.statusMessage =
        "building blockout needs valid floor, wall, and roof dimensions";
    return {false, false,
            "creative_editor_world_layout_building_blockout_settings_invalid"};
  }

  const cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe =
      blockoutRecipe(settings);
  const cr::CreativeWorldLayoutBuildingBlockoutPlan blockout =
      cr::planCreativeWorldLayoutBuildingBlockout(recipe.request);
  if (!blockout.accepted) {
    state.statusMessage = blockout.reasonCode;
    return {false, false, std::string(blockout.reasonCode)};
  }
  double blockoutTop = 0.0;
  if (!blockoutTopLayer(settings.shell.floorTopLayer,
                        settings.floorToFloorCells, blockout.storeyCount,
                        blockoutTop)) {
    state.statusMessage = "building blockout elevation is outside supported limits";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_elevation_invalid"};
  }
  if (blockoutOverlapsExistingRoom(state.source, settings.shell,
                                   blockoutTop)) {
    state.statusMessage = "building blockout overlaps an existing building";
    return {false, false,
            "creative_editor_world_layout_building_blockout_overlap"};
  }

  const std::size_t firstLevelIndex = state.source.levels.size();
  cr::CreativeWorldLayoutBuildingEditResult materialized =
      cr::materializeCreativeWorldLayoutBuildingBlockout(
          state.source, recipe, state.nextStableOrdinal);
  if (!materialized.accepted) {
    if (const BlockoutFailurePresentation* presentation =
            blockoutFailurePresentation(materialized.reasonCode);
        presentation != nullptr) {
      state.statusMessage = presentation->message;
      return {false, false, std::string(presentation->editorReason)};
    }
    state.statusMessage = materialized.reasonCode;
    return {false, false, materialized.reasonCode};
  }
  return commitBuildingBlockoutCandidate(
      state, std::move(materialized.edited), materialized.nextStableOrdinal,
      materialized.resultBuildingIndex, firstLevelIndex);
}

bool readCreativeEditorWorldLayoutBuildingBlockoutSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBlockoutSettings& output) noexcept {
  const cr::CreativeWorldLayoutBuildingBlockoutProvenance provenance =
      cr::creativeWorldLayoutBuildingBlockoutProvenance(state.source,
                                                        buildingIndex);
  if (!provenance.valid) {
    return false;
  }
  output = blockoutSettings(provenance.recipe);
  return true;
}

CreativeEditorWorldLayoutEditReceipt
updateCreativeEditorWorldLayoutBuildingBlockout(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBlockoutSettings settings) {
  if (buildingIndex >= state.source.buildings.size() ||
      !detail::validWorldLayoutShellSettings(settings.shell)) {
    state.statusMessage =
        "building blockout update needs a valid linked building and settings";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_invalid"};
  }

  const cr::CreativeWorldLayoutBuildingBlockoutSyncReceipt sync =
      cr::inspectCreativeWorldLayoutBuildingBlockoutSync(state.source,
                                                         buildingIndex);
  if (!sync.accepted ||
      sync.state !=
          cr::CreativeWorldLayoutBuildingBlockoutSyncState::Current) {
    if (sync.state ==
        cr::CreativeWorldLayoutBuildingBlockoutSyncState::Unlinked) {
      state.statusMessage =
          "selected building was not created from an editable blockout";
      return {
          false, false,
          "creative_editor_world_layout_building_blockout_update_unlinked"};
    }
    if (sync.state ==
        cr::CreativeWorldLayoutBuildingBlockoutSyncState::LocallyModified) {
      state.statusMessage =
          "building blockout has local refinements; update cancelled";
      return {
          false, false,
          "creative_editor_world_layout_building_blockout_update_conflict"};
    }
    state.statusMessage = "building blockout provenance is invalid";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_provenance_"
        "invalid"};
  }

  const cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe =
      blockoutRecipe(settings);
  if (!cr::validCreativeWorldLayoutBuildingBlockoutRecipe(recipe)) {
    state.statusMessage = "building blockout update settings are invalid";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_invalid"};
  }
  if (sameBlockoutRecipe(sync.provenance.recipe, recipe)) {
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                       buildingIndex};
    return {
        true, false,
        "creative_editor_world_layout_building_blockout_update_no_change"};
  }

  double blockoutTop = 0.0;
  if (!blockoutTopLayer(settings.shell.floorTopLayer,
                        settings.floorToFloorCells, settings.storeys.count,
                        blockoutTop)) {
    state.statusMessage =
        "building blockout elevation is outside supported limits";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_elevation_invalid"};
  }
  if (blockoutOverlapsExistingRoom(state.source, settings.shell, blockoutTop,
                                   buildingIndex)) {
    state.statusMessage = "building blockout overlaps an existing building";
    return {false, false,
            "creative_editor_world_layout_building_blockout_overlap"};
  }

  CreativeEditorWorldLayoutState replacementState;
  resetCreativeEditorWorldLayout(replacementState,
                                 "building_blockout_replacement");
  const CreativeEditorWorldLayoutEditReceipt generated =
      createCreativeEditorWorldLayoutBuildingBlockout(replacementState,
                                                      settings);
  if (!generated.accepted || replacementState.source.buildings.size() != 1U) {
    state.statusMessage = replacementState.statusMessage;
    return {false, false, generated.reasonCode};
  }

  cr::CreativeWorldLayout candidate = state.source;
  std::uint64_t nextStableOrdinal = state.nextStableOrdinal;
  if (!cr::replaceCreativeWorldLayoutBuildingInCandidate(
          candidate, buildingIndex, replacementState.source,
          nextStableOrdinal, true)) {
    state.statusMessage = "building blockout replacement could not be remapped";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_remap_failed"};
  }

  cr::CreativeWorldLayoutBuildingBlockoutProvenance provenance;
  provenance.present = true;
  provenance.valid = true;
  provenance.recipe = recipe;
  const cr::CreativeWorldLayoutBuildingBlockoutFingerprint baseline =
      cr::fingerprintCreativeWorldLayoutBuildingBlockout(candidate,
                                                         buildingIndex);
  if (!baseline.valid) {
    state.statusMessage = "updated blockout provenance could not be measured";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_provenance_"
        "invalid"};
  }
  provenance.instanceBaselineFingerprint = baseline.value;
  if (!cr::setCreativeWorldLayoutBuildingBlockoutProvenance(
          candidate, buildingIndex, provenance)) {
    state.statusMessage = "updated blockout provenance could not be recorded";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_provenance_"
        "invalid"};
  }

  const std::size_t levelIndex =
      firstBuildingLevelIndex(candidate, buildingIndex);
  if (levelIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    state.statusMessage = "updated blockout has no building level";
    return {
        false, false,
        "creative_editor_world_layout_building_blockout_update_remap_failed"};
  }
  return commitBuildingBlockoutCandidate(
      state, std::move(candidate), nextStableOrdinal, buildingIndex,
      levelIndex, "building blockout updated; preview or confirm in 3D",
      "creative_editor_world_layout_building_blockout_updated");
}

}  // namespace iggy3d_creative_app
