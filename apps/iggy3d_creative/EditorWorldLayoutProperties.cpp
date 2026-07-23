#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/recipes/ObjectLibraryRecipe.hpp"
#include "app/iggy3d/creative/recipes/TerrainRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] const cr::CreativeDocument& validationDocument() {
  static const cr::CreativeDocument document = [] {
    cr::CreativeDocument value =
        cr::CreativeDocument::create("World Layout Property Validation");
    static_cast<void>(value.assignId(1U));
    return value;
  }();
  return document;
}

[[nodiscard]] bool validProfileSettings(
    const CreativeEditorWorldLayoutTerrainProfileSettings& settings) {
  const bool validRetainingData =
      settings.retainingEdge.version ==
          cr::kCreativeRetainingEdgeRecipeVersion &&
      cr::isValidCreativeRetainingEdgeSettings(
          settings.retainingEdge.settings);
  if (!validRetainingData ||
      (settings.usesRetainingEdgeRecipe &&
       (!settings.usesLandformRecipe ||
        settings.landform.edge !=
            cr::CreativeTerrainLandformEdge::Retaining ||
        !cr::isValidCreativeRetainingEdgeSourceRecipe(
            settings.retainingEdge)))) {
    return false;
  }
  if (settings.usesLandformRecipe) {
    cr::CreativeTerrainLandformKind expectedKind =
        cr::CreativeTerrainLandformKind::Count;
    return cr::creativeTerrainRecipeLandformKind(settings.kind,
                                                 expectedKind) &&
           expectedKind == settings.landform.kind &&
           cr::isValidCreativeTerrainLandformRecipe(settings.landform);
  }
  if (settings.kind == cr::CreativeTerrainRecipeKind::Terrace ||
      settings.kind == cr::CreativeTerrainRecipeKind::Cliff) {
    return false;
  }
  cr::CreativeTerrainProfileRecipeRequest request;
  request.document = &validationDocument();
  request.kind = settings.kind;
  request.center = settings.center;
  request.baseHeightCells = settings.baseHeightCells;
  request.radiusCells = settings.radiusCells;
  request.amplitudeCells = settings.amplitudeCells;
  request.spacingCells = settings.spacingCells;
  request.blend = cr::CreativeTerrainProfileBlend::Set;
  request.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Fill;
  request.direction = settings.direction;
  request.frequency = settings.frequency;
  return cr::buildCreativeTerrainProfileRecipe(request).receipt.accepted;
}

[[nodiscard]] bool validPathSettings(
    const CreativeEditorWorldLayoutTerrainPathSettings& settings) {
  return cr::isValidCreativeTerrainPathSourceRecipe(settings.recipe);
}

[[nodiscard]] bool sameBounds(const cr::CreativeBounds& lhs,
                              const cr::CreativeBounds& rhs) noexcept {
  return lhs.min.x == rhs.min.x && lhs.min.y == rhs.min.y &&
         lhs.min.z == rhs.min.z && lhs.max.x == rhs.max.x &&
         lhs.max.y == rhs.max.y && lhs.max.z == rhs.max.z;
}

[[nodiscard]] bool validBridgeAttachment(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeBridgeSourceRecipe& source) {
  const auto path = std::find_if(
      layout.terrainPaths.begin(), layout.terrainPaths.end(),
      [&](const cr::CreativeWorldLayoutTerrainPath& candidate) {
        return candidate.stableKey == source.watercoursePathKey;
      });
  if (path == layout.terrainPaths.end() ||
      (path->recipe.kind != cr::CreativeTerrainPathKind::River &&
       path->recipe.kind != cr::CreativeTerrainPathKind::Trench)) {
    return false;
  }
  return std::any_of(
      path->recipe.watercourse.crossings.begin(),
      path->recipe.watercourse.crossings.end(),
      [&](const cr::CreativeTerrainWatercourseCrossing& crossing) {
        return crossing.id == source.crossingId;
      });
}

[[nodiscard]] bool bridgeAttachmentAvailable(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutObject& current,
    const cr::CreativeBridgeSourceRecipe& source) {
  return std::none_of(
      layout.objects.begin(), layout.objects.end(),
      [&](const cr::CreativeWorldLayoutObject& candidate) {
        return candidate.stableKey != current.stableKey &&
               candidate.usesBridgeRecipe &&
               candidate.bridge.watercoursePathKey ==
                   source.watercoursePathKey &&
               candidate.bridge.crossingId == source.crossingId;
      });
}

[[nodiscard]] bool validObjectSettings(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutObject& current,
    const CreativeEditorWorldLayoutObjectSettings& settings) {
  if (settings.kind != current.kind || settings.mode != current.mode ||
      settings.usesBridgeRecipe != current.usesBridgeRecipe ||
      !detail::hasVisibleWorldLayoutName(settings.name) ||
      !cr::isFiniteCreativeVec3(settings.boundsCells.min) ||
      !cr::isFiniteCreativeVec3(settings.boundsCells.max) ||
      !cr::isFiniteCreativeVec3(settings.pointCells) ||
      !cr::isFiniteCreativeVec3(settings.assetSourceBoundsMeters.min) ||
      !cr::isFiniteCreativeVec3(settings.assetSourceBoundsMeters.max) ||
      !std::isfinite(settings.yawRadians) ||
      !cr::isFiniteCreativeVec3(settings.scale)) {
    return false;
  }
  if ((settings.kind == cr::CreativeObjectKind::SpawnPoint &&
       !cr::isValidCreativePlayerSpawnSettings(settings.playerSpawn)) ||
      (settings.kind != cr::CreativeObjectKind::SpawnPoint &&
       !(settings.playerSpawn == cr::CreativePlayerSpawnSettings{}))) {
    return false;
  }
  if (settings.usesBridgeRecipe) {
    return settings.kind == cr::CreativeObjectKind::Bridge &&
           settings.mode == cr::CreativeObjectLibraryPlacementMode::Bounds &&
           settings.assetId.empty() && !settings.hasAssetSourceBounds &&
           settings.yawRadians == 0.0 && settings.scale.x == 1.0 &&
           settings.scale.y == 1.0 && settings.scale.z == 1.0 &&
           settings.boundsCells.max.x > settings.boundsCells.min.x &&
           settings.boundsCells.max.y > settings.boundsCells.min.y &&
           settings.boundsCells.max.z > settings.boundsCells.min.z &&
           sameBounds(settings.boundsCells, current.boundsCells) &&
           cr::isValidCreativeBridgeSourceRecipe(settings.bridge) &&
           validBridgeAttachment(layout, settings.bridge) &&
           bridgeAttachmentAvailable(layout, current, settings.bridge);
  }
  if (!(settings.bridge == current.bridge)) {
    return false;
  }
  cr::CreativeObjectLibraryPlacementSpec placement;
  placement.kind = settings.kind;
  placement.mode = settings.mode;
  placement.stableKey = current.stableKey;
  placement.name = settings.name;
  placement.assetId = settings.assetId;
  placement.bounds = settings.boundsCells;
  placement.point = settings.pointCells;
  placement.assetSourceBounds = settings.assetSourceBoundsMeters;
  placement.hasAssetSourceBounds = settings.hasAssetSourceBounds;
  placement.yawRadians = settings.yawRadians;
  placement.scale = settings.scale;
  placement.visible = settings.visible;
  placement.playerSpawn = settings.playerSpawn;
  placement.tags = current.tags;
  cr::CreativeObjectLibraryRecipeRequest request;
  request.placements.push_back(std::move(placement));
  return cr::buildCreativeObjectLibraryRecipe(request).receipt.accepted;
}

[[nodiscard]] bool offsetFiniteCoordinate(double value, std::int64_t delta,
                                          double& output) noexcept {
  output = value + static_cast<double>(delta);
  return std::isfinite(output);
}

[[nodiscard]] bool translatedObjectSettings(
    const CreativeEditorWorldLayoutObjectManipulationState& manipulation,
    CreativeEditorWorldLayoutPoint point,
    CreativeEditorWorldLayoutObjectSettings& output) noexcept {
  std::int64_t deltaX = 0;
  std::int64_t deltaZ = 0;
  if (!detail::snappedWorldLayoutPointerDelta(
          point.x, manipulation.startPoint.x, deltaX) ||
      !detail::snappedWorldLayoutPointerDelta(
          point.z, manipulation.startPoint.z, deltaZ)) {
    return false;
  }
  output = manipulation.originalSettings;
  if (output.usesBridgeRecipe) {
    return false;
  }
  if (output.mode == cr::CreativeObjectLibraryPlacementMode::Bounds) {
    return offsetFiniteCoordinate(output.boundsCells.min.x, deltaX,
                                  output.boundsCells.min.x) &&
           offsetFiniteCoordinate(output.boundsCells.max.x, deltaX,
                                  output.boundsCells.max.x) &&
           offsetFiniteCoordinate(output.boundsCells.min.z, deltaZ,
                                  output.boundsCells.min.z) &&
           offsetFiniteCoordinate(output.boundsCells.max.z, deltaZ,
                                  output.boundsCells.max.z);
  }
  if (output.mode == cr::CreativeObjectLibraryPlacementMode::Point) {
    return offsetFiniteCoordinate(output.pointCells.x, deltaX,
                                  output.pointCells.x) &&
           offsetFiniteCoordinate(output.pointCells.z, deltaZ,
                                  output.pointCells.z);
  }
  return false;
}

}  // namespace

bool readCreativeEditorWorldLayoutLevelSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    CreativeEditorWorldLayoutLevelSettings& output) {
  if (levelIndex >= state.source.levels.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutLevel& level = state.source.levels[levelIndex];
  output = {level.name,
            level.floorTopLayer,
            level.wallHeightCells,
            level.floorThicknessLayers,
            level.ceilingThicknessLayers,
            level.roofThicknessLayers,
            level.roofStyle,
            level.roofRidgeAxis,
            level.roofPitchDegrees,
            level.roofOverhangCells,
            level.roofSlopeDirection,
            level.roofMaterial};
  return true;
}

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutLevelSettings(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    CreativeEditorWorldLayoutLevelSettings settings) {
  CreativeEditorWorldLayoutLevelSettings current;
  if (!readCreativeEditorWorldLayoutLevelSettings(state, levelIndex, current) ||
      !detail::hasVisibleWorldLayoutName(settings.name)) {
    state.statusMessage = "level settings are invalid";
    return {false, false,
            "creative_editor_world_layout_level_settings_invalid"};
  }
  cr::CreativeWorldLayout candidate = state.source;
  cr::CreativeWorldLayoutLevel& level = candidate.levels[levelIndex];
  level.name = settings.name;
  level.floorTopLayer = settings.floorTopLayer;
  level.wallHeightCells = settings.wallHeightCells;
  level.floorThicknessLayers = settings.floorThicknessLayers;
  level.ceilingThicknessLayers = settings.ceilingThicknessLayers;
  level.roofThicknessLayers = settings.roofThicknessLayers;
  level.roofStyle = settings.roofStyle;
  level.roofRidgeAxis = settings.roofRidgeAxis;
  level.roofSlopeDirection = settings.roofSlopeDirection;
  level.roofPitchDegrees = settings.roofPitchDegrees;
  level.roofOverhangCells = settings.roofOverhangCells;
  level.roofMaterial = settings.roofMaterial;
  if (cr::firstInvalidCreativeWorldLayoutLevelIndex(candidate) !=
      cr::kInvalidCreativeWorldLayoutIndex) {
    state.statusMessage = "level settings conflict with building geometry";
    return {false, false,
            "creative_editor_world_layout_level_settings_rejected"};
  }
  if (current == settings) {
    state.statusMessage = "level settings unchanged";
    return {true, false,
            "creative_editor_world_layout_level_settings_no_change"};
  }
  state.source.levels[levelIndex] = std::move(level);
  state.activeLevelIndex = levelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Level,
                     levelIndex};
  detail::noteWorldLayoutSourceChange(state, "level settings updated");
  return {true, true,
          "creative_editor_world_layout_level_settings_updated"};
}

CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutRoofAperture(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    cr::CreativeStructuralRoofApertureKind kind) {
  if (levelIndex >= state.source.levels.size() ||
      kind >= cr::CreativeStructuralRoofApertureKind::Count ||
      !cr::creativeWorldLayoutLevelIsTopmostOccupied(state.source,
                                                      levelIndex)) {
    state.statusMessage = "roof aperture target is invalid";
    return {false, false,
            "creative_editor_world_layout_roof_aperture_target_invalid"};
  }
  const std::size_t levelApertureCount = static_cast<std::size_t>(std::count_if(
      state.source.roofApertures.begin(), state.source.roofApertures.end(),
      [levelIndex](const cr::CreativeWorldLayoutRoofAperture& aperture) {
        return aperture.levelIndex == levelIndex;
      }));
  if (levelApertureCount >= cr::kCreativeStructuralRoofApertureCapacity) {
    state.statusMessage = "roof aperture capacity reached";
    return {false, false,
            "creative_editor_world_layout_roof_aperture_capacity_reached"};
  }
  const cr::CreativeWorldLayoutLevel& level = state.source.levels[levelIndex];
  if (level.roofStyle == cr::CreativeStructuralRoofStyle::Hip) {
    state.statusMessage = "hip roof apertures need polygon panel support";
    return {false, false,
            "creative_editor_world_layout_roof_aperture_hip_unsupported"};
  }
  cr::CreativeWorldLayoutRect footprint;
  if (!cr::creativeWorldLayoutLevelRoofFootprint(state.source, levelIndex,
                                                 footprint)) {
    state.statusMessage = "roof aperture needs one rectangular roof";
    return {false, false,
            "creative_editor_world_layout_roof_aperture_footprint_invalid"};
  }

  constexpr std::array<double, 5U> kFractions{0.5, 0.25, 0.75, 0.125,
                                               0.875};
  constexpr std::array<double, 4U> kSizes{1.0, 0.75, 0.5, 0.25};
  constexpr double kPerimeterInsetCells = 0.2;
  const double minimumX = static_cast<double>(footprint.minimum.x) +
                          kPerimeterInsetCells;
  const double maximumX = static_cast<double>(footprint.maximum.x) -
                          kPerimeterInsetCells;
  const double minimumZ = static_cast<double>(footprint.minimum.z) +
                          kPerimeterInsetCells;
  const double maximumZ = static_cast<double>(footprint.maximum.z) -
                          kPerimeterInsetCells;
  const double availableX = maximumX - minimumX;
  const double availableZ = maximumZ - minimumZ;
  cr::CreativeWorldLayout accepted;
  bool found = false;
  std::uint64_t acceptedOrdinal = state.nextStableOrdinal;
  for (const double requestedSize : kSizes) {
    const double sizeX = std::min(requestedSize, availableX);
    const double sizeZ = std::min(requestedSize, availableZ);
    if (sizeX <= 0.0 || sizeZ <= 0.0) {
      continue;
    }
    for (const double fractionZ : kFractions) {
      for (const double fractionX : kFractions) {
        const double apertureMinimumX =
            minimumX + (availableX - sizeX) * fractionX;
        const double apertureMinimumZ =
            minimumZ + (availableZ - sizeZ) * fractionZ;
        cr::CreativeWorldLayout candidate = state.source;
        std::uint64_t nextOrdinal = state.nextStableOrdinal;
        cr::CreativeWorldLayoutRoofAperture aperture;
        aperture.levelIndex = levelIndex;
        aperture.kind = kind;
        aperture.stableKey = cr::mintCreativeWorldLayoutStableKey(
            candidate, nextOrdinal,
            kind == cr::CreativeStructuralRoofApertureKind::Skylight
                ? "skylight"
                : "chimney_clearance");
        aperture.name =
            kind == cr::CreativeStructuralRoofApertureKind::Skylight
                ? "Skylight"
                : "Chimney clearance";
        aperture.minimumXCells = apertureMinimumX;
        aperture.maximumXCells = apertureMinimumX + sizeX;
        aperture.minimumZCells = apertureMinimumZ;
        aperture.maximumZCells = apertureMinimumZ + sizeZ;
        candidate.roofApertures.push_back(std::move(aperture));
        if (!cr::planCreativeWorldLayoutRoof(
                 validationDocument().gridSettings(), candidate, levelIndex)
                 .accepted) {
          continue;
        }
        accepted = std::move(candidate);
        acceptedOrdinal = nextOrdinal;
        found = true;
        break;
      }
      if (found) {
        break;
      }
    }
    if (found) {
      break;
    }
  }
  if (!found) {
    state.statusMessage = "no valid roof aperture position is available";
    return {false, false,
            "creative_editor_world_layout_roof_aperture_no_position"};
  }

  state.source = std::move(accepted);
  state.nextStableOrdinal = acceptedOrdinal;
  state.activeLevelIndex = levelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::RoofAperture,
                     state.source.roofApertures.size() - 1U};
  detail::noteWorldLayoutSourceChange(
      state, kind == cr::CreativeStructuralRoofApertureKind::Skylight
                 ? "skylight added"
                 : "chimney clearance added");
  return {true, true,
          "creative_editor_world_layout_roof_aperture_created"};
}

bool readCreativeEditorWorldLayoutRoofApertureSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t apertureIndex,
    CreativeEditorWorldLayoutRoofApertureSettings& output) noexcept {
  if (apertureIndex >= state.source.roofApertures.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutRoofAperture& aperture =
      state.source.roofApertures[apertureIndex];
  output = {aperture.kind, aperture.minimumXCells, aperture.maximumXCells,
            aperture.minimumZCells, aperture.maximumZCells};
  return true;
}

CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutRoofApertureSettings(
    CreativeEditorWorldLayoutState& state, std::size_t apertureIndex,
    CreativeEditorWorldLayoutRoofApertureSettings settings,
    cr::CreativeGridSettings grid) {
  CreativeEditorWorldLayoutRoofApertureSettings current;
  if (!readCreativeEditorWorldLayoutRoofApertureSettings(
          state, apertureIndex, current) ||
      settings.kind >= cr::CreativeStructuralRoofApertureKind::Count ||
      !std::isfinite(settings.minimumXCells) ||
      !std::isfinite(settings.maximumXCells) ||
      !std::isfinite(settings.minimumZCells) ||
      !std::isfinite(settings.maximumZCells) ||
      settings.minimumXCells >= settings.maximumXCells ||
      settings.minimumZCells >= settings.maximumZCells) {
    state.statusMessage = "roof aperture settings are invalid";
    return {false, false,
            "creative_editor_world_layout_roof_aperture_settings_invalid"};
  }
  if (current == settings) {
    state.statusMessage = "roof aperture settings unchanged";
    return {true, false,
            "creative_editor_world_layout_roof_aperture_settings_no_change"};
  }
  cr::CreativeWorldLayout candidate = state.source;
  cr::CreativeWorldLayoutRoofAperture& aperture =
      candidate.roofApertures[apertureIndex];
  aperture.kind = settings.kind;
  aperture.minimumXCells = settings.minimumXCells;
  aperture.maximumXCells = settings.maximumXCells;
  aperture.minimumZCells = settings.minimumZCells;
  aperture.maximumZCells = settings.maximumZCells;
  const std::size_t levelIndex = aperture.levelIndex;
  const cr::CreativeWorldLayoutRoofPlan roof =
      cr::planCreativeWorldLayoutRoof(grid, candidate, levelIndex);
  if (!roof.accepted) {
    state.statusMessage = "roof aperture conflicts with roof geometry";
    return {false, false, std::string(roof.reasonCode)};
  }
  state.source = std::move(candidate);
  state.activeLevelIndex = levelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::RoofAperture,
                     apertureIndex};
  detail::noteWorldLayoutSourceChange(state, "roof aperture updated");
  return {true, true,
          "creative_editor_world_layout_roof_aperture_settings_updated"};
}

bool readCreativeEditorWorldLayoutBuildingGroundingSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingGroundingSettings& output) noexcept {
  if (buildingIndex >= state.source.buildings.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutBuilding& building =
      state.source.buildings[buildingIndex];
  output = {building.groundingMode, building.maximumGroundReliefCells};
  return true;
}

CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutBuildingGroundingSettings(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingGroundingSettings settings) {
  CreativeEditorWorldLayoutBuildingGroundingSettings current;
  if (!readCreativeEditorWorldLayoutBuildingGroundingSettings(
          state, buildingIndex, current) ||
      settings.mode >= cr::CreativeWorldLayoutGroundingMode::Count) {
    state.statusMessage = "building grounding settings are invalid";
    return {false, false,
            "creative_editor_world_layout_building_grounding_invalid"};
  }
  const cr::CreativeWorldLayoutRect footprint =
      state.source.buildings[buildingIndex].rootFootprint;
  if (settings.mode == cr::CreativeWorldLayoutGroundingMode::Foundation &&
      (footprint.minimum.x >= footprint.maximum.x ||
       footprint.minimum.z >= footprint.maximum.z)) {
    state.statusMessage = "grounded buildings need a valid footprint";
    return {false, false,
            "creative_editor_world_layout_building_grounding_footprint_invalid"};
  }
  if (current == settings) {
    state.statusMessage = "building grounding unchanged";
    return {true, false,
            "creative_editor_world_layout_building_grounding_no_change"};
  }

  cr::CreativeWorldLayoutBuilding& building =
      state.source.buildings[buildingIndex];
  building.groundingMode = settings.mode;
  building.maximumGroundReliefCells = settings.maximumReliefCells;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  detail::noteWorldLayoutSourceChange(state,
                                      "building grounding updated");
  return {true, true,
          "creative_editor_world_layout_building_grounding_updated"};
}

bool readCreativeEditorWorldLayoutTerrainProfileSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t profileIndex,
    CreativeEditorWorldLayoutTerrainProfileSettings& output) noexcept {
  if (profileIndex >= state.source.terrainProfiles.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutTerrainProfile& profile =
      state.source.terrainProfiles[profileIndex];
  output = {};
  output.kind = profile.kind;
  output.center = profile.center;
  output.baseHeightCells = profile.baseHeightCells;
  output.radiusCells = profile.radiusCells;
  output.amplitudeCells = profile.amplitudeCells;
  output.spacingCells = profile.spacingCells;
  output.direction = profile.direction;
  output.frequency = profile.frequency;
  output.usesLandformRecipe = profile.usesLandformRecipe;
  output.landform = profile.landform;
  output.usesRetainingEdgeRecipe = profile.usesRetainingEdgeRecipe;
  output.retainingEdge = profile.retainingEdge;
  return true;
}

CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutTerrainProfileSettings(
    CreativeEditorWorldLayoutState& state, std::size_t profileIndex,
    CreativeEditorWorldLayoutTerrainProfileSettings settings) {
  CreativeEditorWorldLayoutTerrainProfileSettings current;
  if (!readCreativeEditorWorldLayoutTerrainProfileSettings(
          state, profileIndex, current) ||
      !validProfileSettings(settings) ||
      (settings.usesRetainingEdgeRecipe &&
       settings.retainingEdge.terrainProfileKey !=
           state.source.terrainProfiles[profileIndex].stableKey)) {
    state.statusMessage = "terrain profile settings are invalid";
    return {false, false,
            "creative_editor_world_layout_profile_settings_invalid"};
  }
  if (current == settings) {
    state.statusMessage = "terrain profile settings unchanged";
    return {true, false,
            "creative_editor_world_layout_profile_settings_no_change"};
  }
  cr::CreativeWorldLayoutTerrainProfile& profile =
      state.source.terrainProfiles[profileIndex];
  profile.kind = settings.kind;
  profile.center = settings.center;
  profile.baseHeightCells = settings.baseHeightCells;
  profile.radiusCells = settings.radiusCells;
  profile.amplitudeCells = settings.amplitudeCells;
  profile.spacingCells = settings.spacingCells;
  profile.blend = cr::CreativeTerrainProfileBlend::Set;
  profile.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Fill;
  profile.direction = settings.direction;
  profile.frequency = settings.frequency;
  profile.usesLandformRecipe = settings.usesLandformRecipe;
  profile.landform = settings.landform;
  profile.usesRetainingEdgeRecipe = settings.usesRetainingEdgeRecipe;
  profile.retainingEdge = std::move(settings.retainingEdge);
  state.selection = {CreativeEditorWorldLayoutSelectionKind::TerrainProfile,
                     profileIndex};
  detail::noteWorldLayoutSourceChange(state,
                                      "terrain profile settings updated");
  return {true, true,
          "creative_editor_world_layout_profile_settings_updated"};
}

bool readCreativeEditorWorldLayoutTerrainPathSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t pathIndex,
    CreativeEditorWorldLayoutTerrainPathSettings& output) {
  if (pathIndex >= state.source.terrainPaths.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutTerrainPath& path =
      state.source.terrainPaths[pathIndex];
  if (!cr::isValidCreativeTerrainPathSourceRecipe(path.recipe)) {
    return false;
  }
  output.recipe = path.recipe;
  return true;
}

CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutTerrainPathSettings(
    CreativeEditorWorldLayoutState& state, std::size_t pathIndex,
    CreativeEditorWorldLayoutTerrainPathSettings settings) {
  CreativeEditorWorldLayoutTerrainPathSettings current;
  if (!readCreativeEditorWorldLayoutTerrainPathSettings(state, pathIndex,
                                                        current) ||
      !validPathSettings(settings)) {
    state.statusMessage = "terrain path settings are invalid";
    return {false, false,
            "creative_editor_world_layout_path_settings_invalid"};
  }
  if (current == settings) {
    state.statusMessage = "terrain path settings unchanged";
    return {true, false,
            "creative_editor_world_layout_path_settings_no_change"};
  }
  cr::CreativeWorldLayoutTerrainPath& path =
      state.source.terrainPaths[pathIndex];
  path.recipe = std::move(settings.recipe);
  state.selection = {CreativeEditorWorldLayoutSelectionKind::TerrainPath,
                     pathIndex};
  detail::noteWorldLayoutSourceChange(state,
                                      "terrain path settings updated");
  return {true, true,
          "creative_editor_world_layout_path_settings_updated"};
}

bool readCreativeEditorWorldLayoutObjectSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t objectIndex,
    CreativeEditorWorldLayoutObjectSettings& output) {
  if (objectIndex >= state.source.objects.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutObject& object = state.source.objects[objectIndex];
  output = {};
  output.kind = object.kind;
  output.mode = object.mode;
  output.name = object.name;
  output.assetId = object.assetId;
  output.boundsCells = object.boundsCells;
  output.pointCells = object.pointCells;
  output.assetSourceBoundsMeters = object.assetSourceBoundsMeters;
  output.hasAssetSourceBounds = object.hasAssetSourceBounds;
  output.yawRadians = object.yawRadians;
  output.scale = object.scale;
  output.visible = object.visible;
  output.usesBridgeRecipe = object.usesBridgeRecipe;
  output.bridge = object.bridge;
  output.playerSpawn = object.playerSpawn;
  return true;
}

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutObjectSettings(
    CreativeEditorWorldLayoutState& state, std::size_t objectIndex,
    CreativeEditorWorldLayoutObjectSettings settings) {
  CreativeEditorWorldLayoutObjectSettings current;
  if (!readCreativeEditorWorldLayoutObjectSettings(state, objectIndex,
                                                   current) ||
      !validObjectSettings(state.source, state.source.objects[objectIndex],
                           settings)) {
    state.statusMessage = "object settings are invalid";
    return {false, false,
            "creative_editor_world_layout_object_settings_invalid"};
  }
  if (current == settings) {
    state.statusMessage = "object settings unchanged";
    return {true, false,
            "creative_editor_world_layout_object_settings_no_change"};
  }
  cr::CreativeWorldLayoutObject& object = state.source.objects[objectIndex];
  object.name = std::move(settings.name);
  object.assetId = std::move(settings.assetId);
  object.boundsCells = settings.boundsCells;
  object.pointCells = settings.pointCells;
  object.assetSourceBoundsMeters = settings.assetSourceBoundsMeters;
  object.hasAssetSourceBounds = settings.hasAssetSourceBounds;
  object.yawRadians = settings.yawRadians;
  object.scale = settings.scale;
  object.visible = settings.visible;
  object.usesBridgeRecipe = settings.usesBridgeRecipe;
  object.bridge = std::move(settings.bridge);
  object.playerSpawn = std::move(settings.playerSpawn);
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Object,
                     objectIndex};
  detail::noteWorldLayoutSourceChange(state, "object settings updated");
  return {true, true,
          "creative_editor_world_layout_object_settings_updated"};
}

std::size_t findCreativeEditorWorldLayoutObjectAt(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point) noexcept {
  return findCreativeEditorWorldLayoutObjectAt(state, point, {});
}

CreativeEditorWorldLayoutEditReceipt
beginCreativeEditorWorldLayoutObjectManipulation(
    CreativeEditorWorldLayoutState& state, std::size_t objectIndex,
    CreativeEditorWorldLayoutPoint point) {
  CreativeEditorWorldLayoutObjectSettings settings;
  if (state.tool != CreativeEditorWorldLayoutTool::Select ||
      !detail::finiteWorldLayoutPoint(point) ||
      !readCreativeEditorWorldLayoutObjectSettings(state, objectIndex,
                                                   settings)) {
    return {false, false,
            "creative_editor_world_layout_object_manipulation_target_invalid"};
  }
  if (settings.usesBridgeRecipe) {
    state.statusMessage = "attached bridges move with their crossing";
    return {false, false,
            "creative_editor_world_layout_bridge_manipulation_attached"};
  }
  detail::clearWorldLayoutInteraction(state);
  const std::string stableKey = state.source.objects[objectIndex].stableKey;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Object,
                     objectIndex};
  state.anchorActive = false;
  state.objectManipulation = {
      true,
      state.revision,
      objectIndex,
      stableKey,
      point,
      settings,
      settings,
      true,
      "creative_editor_world_layout_object_manipulation_ready",
  };
  state.statusMessage = "drag to move object";
  return {true, true,
          "creative_editor_world_layout_object_manipulation_started"};
}

CreativeEditorWorldLayoutEditReceipt
updateCreativeEditorWorldLayoutObjectManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point) {
  CreativeEditorWorldLayoutObjectManipulationState& manipulation =
      state.objectManipulation;
  if (!manipulation.active ||
      manipulation.objectIndex >= state.source.objects.size()) {
    return {false, false,
            "creative_editor_world_layout_object_manipulation_not_active"};
  }
  const cr::CreativeWorldLayoutObject& object =
      state.source.objects[manipulation.objectIndex];
  CreativeEditorWorldLayoutObjectSettings current;
  if (state.revision != manipulation.sourceRevision ||
      object.stableKey != manipulation.stableKey ||
      !readCreativeEditorWorldLayoutObjectSettings(
          state, manipulation.objectIndex, current) ||
      !(current == manipulation.originalSettings)) {
    state.objectManipulation = {};
    state.statusMessage = "object changed while drag was active";
    return {false, false,
            "creative_editor_world_layout_object_manipulation_stale"};
  }
  CreativeEditorWorldLayoutObjectSettings preview;
  const bool coordinatesValid =
      translatedObjectSettings(manipulation, point, preview);
  const bool previewValid =
      coordinatesValid &&
      validObjectSettings(state.source, object, preview);
  const char* reasonCode =
      previewValid
          ? "creative_editor_world_layout_object_manipulation_preview"
          : "creative_editor_world_layout_object_manipulation_out_of_range";
  if (preview == manipulation.previewSettings &&
      previewValid == manipulation.previewValid) {
    return {true, false, reasonCode};
  }
  manipulation.previewSettings = std::move(preview);
  manipulation.previewValid = previewValid;
  manipulation.reasonCode = reasonCode;
  state.statusMessage = previewValid ? "object drag preview"
                                     : "object drag exceeds the layout range";
  return {true, true, reasonCode};
}

CreativeEditorWorldLayoutEditReceipt
cancelCreativeEditorWorldLayoutObjectManipulation(
    CreativeEditorWorldLayoutState& state) noexcept {
  const bool changed = state.objectManipulation.active;
  state.objectManipulation = {};
  state.statusMessage = "object manipulation cancelled";
  return {true, changed,
          "creative_editor_world_layout_object_manipulation_cancelled"};
}

}  // namespace iggy3d_creative_app
