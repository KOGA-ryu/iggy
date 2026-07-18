#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/recipes/ObjectLibraryRecipe.hpp"
#include "app/iggy3d/creative/recipes/TerrainRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"

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
  if (settings.elevation == cr::CreativeTerrainPathElevation::Follow) {
    return false;
  }
  cr::CreativeTerrainPathRecipeRequest request;
  request.document = &validationDocument();
  request.kind = settings.kind;
  request.points = settings.points;
  request.elevation = settings.elevation;
  request.halfWidthCells = settings.halfWidthCells;
  request.amplitudeCells = settings.amplitudeCells;
  request.paintSurface = settings.paintSurface;
  request.material = settings.material;
  return cr::buildCreativeTerrainPathRecipe(request).receipt.accepted;
}

[[nodiscard]] bool validObjectSettings(
    const cr::CreativeWorldLayoutObject& current,
    const CreativeEditorWorldLayoutObjectSettings& settings) {
  if (settings.kind != current.kind || settings.mode != current.mode ||
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
            level.roofOverhangCells};
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
  level.roofPitchDegrees = settings.roofPitchDegrees;
  level.roofOverhangCells = settings.roofOverhangCells;
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

bool readCreativeEditorWorldLayoutTerrainProfileSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t profileIndex,
    CreativeEditorWorldLayoutTerrainProfileSettings& output) noexcept {
  if (profileIndex >= state.source.terrainProfiles.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutTerrainProfile& profile =
      state.source.terrainProfiles[profileIndex];
  output = {profile.kind,          profile.center,
            profile.baseHeightCells, profile.radiusCells,
            profile.amplitudeCells,  profile.spacingCells,
            profile.direction,       profile.frequency};
  return true;
}

CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutTerrainProfileSettings(
    CreativeEditorWorldLayoutState& state, std::size_t profileIndex,
    CreativeEditorWorldLayoutTerrainProfileSettings settings) {
  CreativeEditorWorldLayoutTerrainProfileSettings current;
  if (!readCreativeEditorWorldLayoutTerrainProfileSettings(
          state, profileIndex, current) ||
      !validProfileSettings(settings)) {
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
  if (path.firstPointIndex > state.source.terrainPathPoints.size() ||
      path.pointCount >
          state.source.terrainPathPoints.size() - path.firstPointIndex) {
    return false;
  }
  output.kind = path.kind;
  output.elevation = path.elevation;
  output.halfWidthCells = path.halfWidthCells;
  output.amplitudeCells = path.amplitudeCells;
  output.paintSurface = path.paintSurface;
  output.material = path.material;
  output.points.assign(
      state.source.terrainPathPoints.begin() +
          static_cast<std::ptrdiff_t>(path.firstPointIndex),
      state.source.terrainPathPoints.begin() +
          static_cast<std::ptrdiff_t>(path.firstPointIndex + path.pointCount));
  return true;
}

CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutTerrainPathSettings(
    CreativeEditorWorldLayoutState& state, std::size_t pathIndex,
    CreativeEditorWorldLayoutTerrainPathSettings settings) {
  CreativeEditorWorldLayoutTerrainPathSettings current;
  if (!readCreativeEditorWorldLayoutTerrainPathSettings(state, pathIndex,
                                                        current) ||
      settings.points.size() != current.points.size() ||
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
  path.kind = settings.kind;
  path.elevation = settings.elevation;
  path.halfWidthCells = settings.halfWidthCells;
  path.amplitudeCells = settings.amplitudeCells;
  path.paintSurface = settings.paintSurface;
  path.material = settings.material;
  std::copy(settings.points.begin(), settings.points.end(),
            state.source.terrainPathPoints.begin() +
                static_cast<std::ptrdiff_t>(path.firstPointIndex));
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
  return true;
}

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutObjectSettings(
    CreativeEditorWorldLayoutState& state, std::size_t objectIndex,
    CreativeEditorWorldLayoutObjectSettings settings) {
  CreativeEditorWorldLayoutObjectSettings current;
  if (!readCreativeEditorWorldLayoutObjectSettings(state, objectIndex,
                                                   current) ||
      !validObjectSettings(state.source.objects[objectIndex], settings)) {
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
      coordinatesValid && validObjectSettings(object, preview);
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
