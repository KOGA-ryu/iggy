#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>
#include <utility>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/input/Catalog.hpp"

namespace iggy3d_creative_app {
namespace {

struct AssetCategoryRule {
  std::string_view categoryId;
  CreativeEditorWorldLayoutAssetCategory category =
      CreativeEditorWorldLayoutAssetCategory::Props;
};

constexpr std::array kAssetCategoryRules{
    AssetCategoryRule{"bridge",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"ceiling",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"door",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"floor",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"foundation",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"roof",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"stairs",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"structure",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"walkway",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"wall",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"window",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"boulder",
                      CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"foliage",
                      CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"log", CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"rock", CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"shrub", CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"terrain",
                      CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"tree", CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"stealth_blockout",
                      CreativeEditorWorldLayoutAssetCategory::Cover},
    AssetCategoryRule{"gameplay",
                      CreativeEditorWorldLayoutAssetCategory::Gameplay},
    AssetCategoryRule{"npc",
                      CreativeEditorWorldLayoutAssetCategory::Gameplay},
    AssetCategoryRule{"objective",
                      CreativeEditorWorldLayoutAssetCategory::Gameplay},
    AssetCategoryRule{"spawn",
                      CreativeEditorWorldLayoutAssetCategory::Gameplay},
    AssetCategoryRule{"trigger",
                      CreativeEditorWorldLayoutAssetCategory::Gameplay},
};

constexpr std::array<std::string_view, 2U> kHostedOpeningCategoryIds{
    "door", "window"};

constexpr double kCatalogSnapTieEpsilon = 1.0e-9;

struct CatalogWallSnapHost {
  bool valid = false;
  CreativeEditorWorldLayoutCatalogSnapHostKind kind =
      CreativeEditorWorldLayoutCatalogSnapHostKind::None;
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutRoomEdge roomEdge =
      cr::CreativeWorldLayoutRoomEdge::Count;
  CreativeEditorWorldLayoutPoint start;
  CreativeEditorWorldLayoutPoint end;
  CreativeEditorWorldLayoutPoint projected;
  double baseLayer = 0.0;
  double thicknessCells = 0.0;
  double distanceCells = std::numeric_limits<double>::infinity();
};

struct CatalogWallFrame {
  bool valid = false;
  CreativeEditorWorldLayoutPoint tangent;
  CreativeEditorWorldLayoutPoint normal;
};

struct CatalogWallContact {
  bool valid = false;
  CreativeEditorWorldLayoutPoint pivot;
  CreativeEditorWorldLayoutPoint surfacePoint;
  CreativeEditorWorldLayoutPoint normal;
  double contactOffsetCells = 0.0;
};

struct CatalogSnapResolution {
  bool accepted = false;
  CreativeEditorWorldLayoutPoint point;
  double elevationCells = 0.0;
  double yawRadians = 0.0;
  CreativeEditorWorldLayoutCatalogSnapHostKind hostKind =
      CreativeEditorWorldLayoutCatalogSnapHostKind::None;
  std::size_t hostIndex = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutRoomEdge roomEdge =
      cr::CreativeWorldLayoutRoomEdge::Count;
  double distanceCells = 0.0;
  CreativeEditorWorldLayoutPoint surfacePoint;
  CreativeEditorWorldLayoutPoint normal;
  double wallThicknessCells = 0.0;
  double contactOffsetCells = 0.0;
  std::string_view message = "Choose a placement target";
  std::string_view reasonCode =
      "creative_editor_world_layout_catalog_snap_not_requested";
};

[[nodiscard]] CreativeEditorWorldLayoutAssetCategory classifyCategoryId(
    std::string_view categoryId) noexcept {
  const auto found = std::find_if(
      kAssetCategoryRules.begin(), kAssetCategoryRules.end(),
      [categoryId](const AssetCategoryRule& rule) {
        return rule.categoryId == categoryId;
      });
  return found == kAssetCategoryRules.end()
             ? CreativeEditorWorldLayoutAssetCategory::Props
             : found->category;
}

[[nodiscard]] bool hostedOpeningCategory(
    std::string_view categoryId) noexcept {
  return std::find(kHostedOpeningCategoryIds.begin(),
                   kHostedOpeningCategoryIds.end(),
                   categoryId) != kHostedOpeningCategoryIds.end();
}

[[nodiscard]] CreativeEditorWorldLayoutOpeningPlacementRequest
hostedOpeningPlacementRequest(
    const CreativeEditorWorldLayoutCatalogPlacementState& placement,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid) noexcept {
  CreativeEditorWorldLayoutOpeningPlacementRequest request;
  request.point = point;
  request.kind = placement.categoryId == "window"
                     ? cr::CreativeBuildingOpeningKind::Window
                     : cr::CreativeBuildingOpeningKind::Door;
  request.useExplicitDimensions = true;
  request.insertAssetId = placement.assetId;
  request.insertAssetSourceBoundsMeters = placement.sourceBoundsMeters;
  request.hasInsertAssetSourceBounds = true;
  request.label = placement.label;

  const CreativeEditorWorldLayoutOpeningAssetGeometryPlan geometry =
      planCreativeEditorWorldLayoutOpeningAssetGeometry(
          {request.kind, placement.sourceBoundsMeters, placement.scale,
           grid.cellSizeMeters});
  if (!geometry.accepted) {
    return request;
  }
  request.widthCells = geometry.widthCells;
  request.cutoutBottomCells = geometry.cutoutBottomCells;
  request.cutoutHeightCells = geometry.cutoutHeightCells;
  request.insertThicknessCells = geometry.insertThicknessCells;
  return request;
}

[[nodiscard]] bool validSnapMode(
    CreativeEditorWorldLayoutCatalogSnapMode mode) noexcept {
  return mode < CreativeEditorWorldLayoutCatalogSnapMode::Count;
}

[[nodiscard]] std::pair<CreativeEditorWorldLayoutPoint,
                        CreativeEditorWorldLayoutPoint>
roomEdgeSegment(const cr::CreativeWorldLayoutRoom& room,
                cr::CreativeWorldLayoutRoomEdge edge) noexcept {
  switch (edge) {
    case cr::CreativeWorldLayoutRoomEdge::North:
      return {CreativeEditorWorldLayoutPoint{
                  static_cast<double>(room.footprint.minimum.x),
                  static_cast<double>(room.footprint.minimum.z)},
              CreativeEditorWorldLayoutPoint{
                  static_cast<double>(room.footprint.maximum.x),
                  static_cast<double>(room.footprint.minimum.z)}};
    case cr::CreativeWorldLayoutRoomEdge::East:
      return {CreativeEditorWorldLayoutPoint{
                  static_cast<double>(room.footprint.maximum.x),
                  static_cast<double>(room.footprint.minimum.z)},
              CreativeEditorWorldLayoutPoint{
                  static_cast<double>(room.footprint.maximum.x),
                  static_cast<double>(room.footprint.maximum.z)}};
    case cr::CreativeWorldLayoutRoomEdge::South:
      return {CreativeEditorWorldLayoutPoint{
                  static_cast<double>(room.footprint.minimum.x),
                  static_cast<double>(room.footprint.maximum.z)},
              CreativeEditorWorldLayoutPoint{
                  static_cast<double>(room.footprint.maximum.x),
                  static_cast<double>(room.footprint.maximum.z)}};
    case cr::CreativeWorldLayoutRoomEdge::West:
      return {CreativeEditorWorldLayoutPoint{
                  static_cast<double>(room.footprint.minimum.x),
                  static_cast<double>(room.footprint.minimum.z)},
              CreativeEditorWorldLayoutPoint{
                  static_cast<double>(room.footprint.minimum.x),
                  static_cast<double>(room.footprint.maximum.z)}};
    case cr::CreativeWorldLayoutRoomEdge::Count:
      break;
  }
  return {};
}

void considerWallSnapHost(
    CatalogWallSnapHost& best, CreativeEditorWorldLayoutPoint pointer,
    CreativeEditorWorldLayoutPoint start,
    CreativeEditorWorldLayoutPoint end, double baseLayer,
    double thicknessCells,
    CreativeEditorWorldLayoutCatalogSnapHostKind kind, std::size_t index,
    cr::CreativeWorldLayoutRoomEdge roomEdge) noexcept {
  const double dx = end.x - start.x;
  const double dz = end.z - start.z;
  const double lengthSquared = dx * dx + dz * dz;
  if (!detail::finiteWorldLayoutPoint(pointer) ||
      !detail::finiteWorldLayoutPoint(start) ||
      !detail::finiteWorldLayoutPoint(end) || !std::isfinite(baseLayer) ||
      !std::isfinite(thicknessCells) || thicknessCells <= 0.0 ||
      !std::isfinite(lengthSquared) || lengthSquared <= 0.0) {
    return;
  }
  const double projection =
      ((pointer.x - start.x) * dx + (pointer.z - start.z) * dz) /
      lengthSquared;
  if (!std::isfinite(projection)) {
    return;
  }
  const double t = std::clamp(projection, 0.0, 1.0);
  const CreativeEditorWorldLayoutPoint projected{start.x + t * dx,
                                                  start.z + t * dz};
  const double distance =
      std::hypot(pointer.x - projected.x, pointer.z - projected.z);
  if (!std::isfinite(distance) ||
      distance >
          kCreativeEditorWorldLayoutCatalogWallSnapToleranceCells ||
      distance >= best.distanceCells - kCatalogSnapTieEpsilon) {
    return;
  }
  best.valid = true;
  best.kind = kind;
  best.index = index;
  best.roomEdge = roomEdge;
  best.start = start;
  best.end = end;
  best.projected = projected;
  best.baseLayer = baseLayer;
  best.thicknessCells = thicknessCells;
  best.distanceCells = distance;
}

// Hover-time scan is allocation-free; stable ties retain explicit and earlier
// room-edge hosts without constructing a second wall table.
[[nodiscard]] CatalogWallSnapHost nearestWallSnapHost(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint pointer) noexcept {
  CatalogWallSnapHost best;
  const cr::CreativeWorldLayoutLevel* activeLevel =
      state.activeLevelIndex < state.source.levels.size()
          ? &state.source.levels[state.activeLevelIndex]
          : nullptr;
  for (std::size_t wallIndex = 0U; wallIndex < state.source.walls.size();
       ++wallIndex) {
    const cr::CreativeWorldLayoutWall& wall = state.source.walls[wallIndex];
    if (activeLevel != nullptr &&
        (wall.buildingIndex != activeLevel->buildingIndex ||
         std::fabs(wall.baseLayer - activeLevel->floorTopLayer) >
             kCatalogSnapTieEpsilon)) {
      continue;
    }
    considerWallSnapHost(
        best, pointer,
        {static_cast<double>(wall.start.x),
         static_cast<double>(wall.start.z)},
        {static_cast<double>(wall.end.x), static_cast<double>(wall.end.z)},
        wall.baseLayer, wall.thicknessCells,
        CreativeEditorWorldLayoutCatalogSnapHostKind::ExplicitWall,
        wallIndex, cr::CreativeWorldLayoutRoomEdge::Count);
  }
  for (std::size_t roomIndex = 0U; roomIndex < state.source.rooms.size();
       ++roomIndex) {
    const cr::CreativeWorldLayoutRoom& room = state.source.rooms[roomIndex];
    if (room.levelIndex >= state.source.levels.size() ||
        (activeLevel != nullptr &&
         room.levelIndex != state.activeLevelIndex)) {
      continue;
    }
    const cr::CreativeWorldLayoutLevel& level =
        state.source.levels[room.levelIndex];
    if (level.buildingIndex != room.buildingIndex ||
        !std::isfinite(level.floorTopLayer)) {
      continue;
    }
    for (std::uint8_t edgeValue = 0U;
         edgeValue < static_cast<std::uint8_t>(
                         cr::CreativeWorldLayoutRoomEdge::Count);
         ++edgeValue) {
      const auto edge =
          static_cast<cr::CreativeWorldLayoutRoomEdge>(edgeValue);
      const auto [start, end] = roomEdgeSegment(room, edge);
      considerWallSnapHost(
          best, pointer, start, end, level.floorTopLayer,
          room.wallThicknessCells,
          CreativeEditorWorldLayoutCatalogSnapHostKind::RoomEdge, roomIndex,
          edge);
    }
  }
  return best;
}

[[nodiscard]] CatalogWallFrame wallFrame(
    const CatalogWallSnapHost& host) noexcept {
  CatalogWallFrame result;
  double dx = host.end.x - host.start.x;
  double dz = host.end.z - host.start.z;
  if (dx < -kCatalogSnapTieEpsilon ||
      (std::fabs(dx) <= kCatalogSnapTieEpsilon && dz < 0.0)) {
    dx = -dx;
    dz = -dz;
  }
  const double length = std::hypot(dx, dz);
  if (!std::isfinite(length) || length <= 0.0) {
    return result;
  }
  result.tangent = {dx / length, dz / length};
  result.normal = {-result.tangent.z, result.tangent.x};
  result.valid = detail::finiteWorldLayoutPoint(result.tangent) &&
                 detail::finiteWorldLayoutPoint(result.normal);
  return result;
}

[[nodiscard]] double floorAlignedElevationCells(
    const CreativeEditorWorldLayoutCatalogPlacementState& placement,
    double floorLayer, double cellSizeMeters) noexcept {
  if (!std::isfinite(floorLayer) || !std::isfinite(cellSizeMeters) ||
      cellSizeMeters <= 0.0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return floorLayer -
         placement.sourceBoundsMeters.min.y * placement.scale.y /
             cellSizeMeters;
}

[[nodiscard]] double wallAlignedYawRadians(
    const CreativeEditorWorldLayoutCatalogPlacementState& placement,
    const CatalogWallSnapHost& host) noexcept {
  const CatalogWallFrame frame = wallFrame(host);
  if (!frame.valid) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const cr::CreativeBoundsMetrics source =
      cr::measureCreativeBounds(placement.sourceBoundsMeters);
  const bool localXIsPrimary =
      source.size.x * placement.scale.x >= source.size.z * placement.scale.z;
  const double hostYaw =
      localXIsPrimary
          ? std::atan2(-frame.tangent.z, frame.tangent.x)
          : std::atan2(frame.tangent.x, frame.tangent.z);
  const double yawOffset =
      std::remainder(placement.yawDegrees, 360.0) * std::numbers::pi / 180.0;
  return std::remainder(hostYaw + yawOffset, std::numbers::pi * 2.0);
}

[[nodiscard]] CatalogWallContact resolveWallContact(
    const CreativeEditorWorldLayoutCatalogPlacementState& placement,
    const CatalogWallSnapHost& host, CreativeEditorWorldLayoutPoint pointer,
    double cellSizeMeters, double yawRadians) noexcept {
  CatalogWallContact result;
  const CatalogWallFrame frame = wallFrame(host);
  if (!frame.valid || !detail::finiteWorldLayoutPoint(pointer) ||
      !std::isfinite(cellSizeMeters) || cellSizeMeters <= 0.0 ||
      !std::isfinite(host.thicknessCells) || host.thicknessCells <= 0.0 ||
      !std::isfinite(yawRadians)) {
    return result;
  }

  const double pointerSide =
      (pointer.x - host.projected.x) * frame.normal.x +
      (pointer.z - host.projected.z) * frame.normal.z;
  const double sideSign =
      (pointerSide >= 0.0) != placement.wallSideFlipped ? 1.0 : -1.0;
  result.normal = {frame.normal.x * sideSign, frame.normal.z * sideSign};

  const double inverseCell = 1.0 / cellSizeMeters;
  const cr::CreativeBounds authoredCells{
      {placement.sourceBoundsMeters.min.x * inverseCell,
       placement.sourceBoundsMeters.min.y * inverseCell,
       placement.sourceBoundsMeters.min.z * inverseCell},
      {placement.sourceBoundsMeters.max.x * inverseCell,
       placement.sourceBoundsMeters.max.y * inverseCell,
       placement.sourceBoundsMeters.max.z * inverseCell},
  };
  cr::CreativeTransform transform;
  transform.rotationEulerRadians.y = yawRadians;
  transform.scale = placement.scale;
  const cr::CreativeTransformedBounds transformed =
      cr::resolveCreativeTransformedBounds(authoredCells, transform);
  if (!transformed.valid) {
    return {};
  }

  double minimumSupport = std::numeric_limits<double>::infinity();
  for (const cr::CreativeVec3 corner : transformed.corners) {
    minimumSupport =
        std::min(minimumSupport,
                 corner.x * result.normal.x + corner.z * result.normal.z);
  }
  const double halfThickness = host.thicknessCells * 0.5;
  result.contactOffsetCells = halfThickness - minimumSupport;
  result.surfacePoint = {
      host.projected.x + result.normal.x * halfThickness,
      host.projected.z + result.normal.z * halfThickness};
  result.pivot = {
      host.projected.x + result.normal.x * result.contactOffsetCells,
      host.projected.z + result.normal.z * result.contactOffsetCells};
  result.valid = std::isfinite(minimumSupport) &&
                 std::isfinite(result.contactOffsetCells) &&
                 detail::finiteWorldLayoutPoint(result.surfacePoint) &&
                 detail::finiteWorldLayoutPoint(result.pivot) &&
                 detail::finiteWorldLayoutPoint(result.normal);
  return result;
}

[[nodiscard]] std::string lowerAscii(std::string_view value) {
  std::string lowered(value);
  std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](char byte) {
    return static_cast<char>(
        std::tolower(static_cast<unsigned char>(byte)));
  });
  return lowered;
}

[[nodiscard]] bool finitePositiveScale(cr::CreativeVec3 scale) noexcept {
  return cr::isFiniteCreativeVec3(scale) && scale.x > 0.0 && scale.y > 0.0 &&
         scale.z > 0.0;
}

[[nodiscard]] bool coordinateInsideLayoutRange(double value) noexcept {
  return std::isfinite(value) &&
         value >=
             static_cast<double>(std::numeric_limits<std::int32_t>::min()) &&
         value <=
             static_cast<double>(std::numeric_limits<std::int32_t>::max());
}

[[nodiscard]] CatalogSnapResolution resolveCatalogSnap(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint pointer,
    cr::CreativeGridSettings grid) noexcept {
  CatalogSnapResolution result;
  const CreativeEditorWorldLayoutCatalogPlacementState& placement =
      state.catalogPlacement;
  if (!validSnapMode(placement.snapMode)) {
    result.message = "Choose a valid snap mode";
    result.reasonCode =
        "creative_editor_world_layout_catalog_snap_mode_invalid";
    return result;
  }

  const double manualYaw =
      std::remainder(placement.yawDegrees, 360.0) * std::numbers::pi / 180.0;
  switch (placement.snapMode) {
    case CreativeEditorWorldLayoutCatalogSnapMode::Grid: {
      result.point = {std::round(pointer.x), std::round(pointer.z)};
      result.elevationCells = placement.elevationCells;
      result.yawRadians = manualYaw;
      result.message = "Grid target ready";
      result.reasonCode = "creative_editor_world_layout_catalog_grid_ready";
      break;
    }
    case CreativeEditorWorldLayoutCatalogSnapMode::Floor: {
      if (state.activeLevelIndex >= state.source.levels.size()) {
        result.message = "Select a level for floor snap";
        result.reasonCode =
            "creative_editor_world_layout_catalog_floor_missing";
        return result;
      }
      const cr::CreativeWorldLayoutLevel& level =
          state.source.levels[state.activeLevelIndex];
      if (level.buildingIndex >= state.source.buildings.size() ||
          !std::isfinite(level.floorTopLayer)) {
        result.message = "Active level is invalid";
        result.reasonCode =
            "creative_editor_world_layout_catalog_floor_invalid";
        return result;
      }
      result.point = {std::round(pointer.x), std::round(pointer.z)};
      result.elevationCells = floorAlignedElevationCells(
          placement, level.floorTopLayer, grid.cellSizeMeters);
      result.yawRadians = manualYaw;
      result.hostKind =
          CreativeEditorWorldLayoutCatalogSnapHostKind::LevelFloor;
      result.hostIndex = state.activeLevelIndex;
      result.message = "Active-level floor target ready";
      result.reasonCode = "creative_editor_world_layout_catalog_floor_ready";
      break;
    }
    case CreativeEditorWorldLayoutCatalogSnapMode::Wall: {
      if (state.activeLevelIndex >= state.source.levels.size()) {
        result.message = "Select a level for wall snap";
        result.reasonCode =
            "creative_editor_world_layout_catalog_wall_level_missing";
        return result;
      }
      const cr::CreativeWorldLayoutLevel& level =
          state.source.levels[state.activeLevelIndex];
      if (level.buildingIndex >= state.source.buildings.size() ||
          !std::isfinite(level.floorTopLayer)) {
        result.message = "Active level is invalid";
        result.reasonCode =
            "creative_editor_world_layout_catalog_wall_level_invalid";
        return result;
      }
      if (!creativeEditorWorldLayoutCatalogAssetSupportsWallSnap(
              placement.categoryId)) {
        result.message = "Wall snap supports architecture assets";
        result.reasonCode =
            "creative_editor_world_layout_catalog_wall_asset_unsupported";
        return result;
      }
      const CatalogWallSnapHost host = nearestWallSnapHost(state, pointer);
      if (!host.valid) {
        result.message = "No active-level wall within 1.25 cells";
        result.reasonCode =
            "creative_editor_world_layout_catalog_wall_missing";
        return result;
      }
      result.yawRadians = wallAlignedYawRadians(placement, host);
      const CatalogWallContact contact = resolveWallContact(
          placement, host, pointer, grid.cellSizeMeters, result.yawRadians);
      if (!contact.valid) {
        result.message = "Wall contact could not be resolved";
        result.reasonCode =
            "creative_editor_world_layout_catalog_wall_contact_invalid";
        return result;
      }
      result.point = contact.pivot;
      result.elevationCells = floorAlignedElevationCells(
          placement, host.baseLayer, grid.cellSizeMeters);
      result.hostKind = host.kind;
      result.hostIndex = host.index;
      result.roomEdge = host.roomEdge;
      result.distanceCells = host.distanceCells;
      result.surfacePoint = contact.surfacePoint;
      result.normal = contact.normal;
      result.wallThicknessCells = host.thicknessCells;
      result.contactOffsetCells = contact.contactOffsetCells;
      result.message = host.kind ==
                               CreativeEditorWorldLayoutCatalogSnapHostKind::
                                   ExplicitWall
                           ? "Explicit wall target ready"
                           : "Room wall target ready";
      result.reasonCode = "creative_editor_world_layout_catalog_wall_ready";
      break;
    }
    case CreativeEditorWorldLayoutCatalogSnapMode::Count:
      return result;
  }

  if (!coordinateInsideLayoutRange(result.point.x) ||
      !coordinateInsideLayoutRange(result.point.z) ||
      !std::isfinite(result.elevationCells) ||
      !std::isfinite(result.yawRadians)) {
    result.message = "Snap result is outside the layout range";
    result.reasonCode =
        "creative_editor_world_layout_catalog_snap_out_of_range";
    return result;
  }
  result.accepted = true;
  return result;
}

[[nodiscard]] bool samePlacementAsset(
    const CreativeEditorWorldLayoutCatalogPlacementState& placement,
    const cr::CreativeCatalogEntry& entry) noexcept {
  return placement.active && placement.kind == entry.hotbarEntry.objectKind &&
         placement.assetId == cr::creativeHotbarAssetId(entry.hotbarEntry) &&
         placement.sourceBoundsMeters.min.x ==
             entry.hotbarEntry.assetSourceBounds.min.x &&
         placement.sourceBoundsMeters.min.y ==
             entry.hotbarEntry.assetSourceBounds.min.y &&
         placement.sourceBoundsMeters.min.z ==
             entry.hotbarEntry.assetSourceBounds.min.z &&
         placement.sourceBoundsMeters.max.x ==
             entry.hotbarEntry.assetSourceBounds.max.x &&
         placement.sourceBoundsMeters.max.y ==
             entry.hotbarEntry.assetSourceBounds.max.y &&
         placement.sourceBoundsMeters.max.z ==
             entry.hotbarEntry.assetSourceBounds.max.z;
}

[[nodiscard]] bool pointInsideFootprint(
    const CreativeEditorWorldLayoutObjectFootprint& footprint,
    CreativeEditorWorldLayoutPoint point) noexcept {
  bool positive = false;
  bool negative = false;
  for (std::size_t index = 0U; index < footprint.corners.size(); ++index) {
    const CreativeEditorWorldLayoutPoint start = footprint.corners[index];
    const CreativeEditorWorldLayoutPoint end =
        footprint.corners[(index + 1U) % footprint.corners.size()];
    const double cross = (end.x - start.x) * (point.z - start.z) -
                         (end.z - start.z) * (point.x - start.x);
    positive = positive || cross > 1.0e-9;
    negative = negative || cross < -1.0e-9;
    if (positive && negative) {
      return false;
    }
  }
  return true;
}

}  // namespace

const char* creativeEditorWorldLayoutAssetCategoryLabel(
    CreativeEditorWorldLayoutAssetCategory category) noexcept {
  switch (category) {
    case CreativeEditorWorldLayoutAssetCategory::Architecture:
      return "Architecture";
    case CreativeEditorWorldLayoutAssetCategory::Nature:
      return "Nature";
    case CreativeEditorWorldLayoutAssetCategory::Cover:
      return "Cover";
    case CreativeEditorWorldLayoutAssetCategory::Props:
      return "Props";
    case CreativeEditorWorldLayoutAssetCategory::Gameplay:
      return "Gameplay";
    case CreativeEditorWorldLayoutAssetCategory::Count:
      break;
  }
  return "Unknown";
}

const char* creativeEditorWorldLayoutCatalogSnapModeLabel(
    CreativeEditorWorldLayoutCatalogSnapMode mode) noexcept {
  switch (mode) {
    case CreativeEditorWorldLayoutCatalogSnapMode::Grid:
      return "Grid";
    case CreativeEditorWorldLayoutCatalogSnapMode::Floor:
      return "Floor";
    case CreativeEditorWorldLayoutCatalogSnapMode::Wall:
      return "Wall";
    case CreativeEditorWorldLayoutCatalogSnapMode::Count:
      break;
  }
  return "Unknown";
}

CreativeEditorWorldLayoutAssetCategory
classifyCreativeEditorWorldLayoutAsset(
    const cr::CreativeCatalogEntry& entry) noexcept {
  return classifyCategoryId(entry.assetAuthoringMetadata.categoryId);
}

bool creativeEditorWorldLayoutCatalogAssetSupportsWallSnap(
    std::string_view categoryId) noexcept {
  return classifyCategoryId(categoryId) ==
         CreativeEditorWorldLayoutAssetCategory::Architecture;
}

bool creativeEditorWorldLayoutCatalogAssetIsHostedOpening(
    std::string_view categoryId) noexcept {
  return hostedOpeningCategory(categoryId);
}

bool creativeEditorWorldLayoutCatalogAssetMatchesOpening(
    std::string_view categoryId,
    cr::CreativeBuildingOpeningKind kind) noexcept {
  return (categoryId == "door" &&
          kind == cr::CreativeBuildingOpeningKind::Door) ||
         (categoryId == "window" &&
          kind == cr::CreativeBuildingOpeningKind::Window);
}

bool creativeEditorWorldLayoutAssetMatchesQuery(
    const cr::CreativeCatalogEntry& entry, std::string_view query) {
  if (query.empty()) {
    return true;
  }
  const std::string lowered = lowerAscii(query);
  return entry.searchText.find(lowered) != std::string::npos;
}

CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutCatalogAsset(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogEntry& entry) {
  const std::string_view assetId = cr::creativeHotbarAssetId(entry.hotbarEntry);
  const cr::CreativeBoundsMetrics bounds =
      cr::measureCreativeBounds(entry.hotbarEntry.assetSourceBounds);
  if (entry.category != cr::CreativeCatalogEntryCategory::Asset ||
      entry.hotbarEntry.objectKind <= cr::CreativeObjectKind::Unknown ||
      entry.hotbarEntry.objectKind >= cr::CreativeObjectKind::Count ||
      assetId.empty() || !entry.hotbarEntry.hasAssetBounds || !bounds.valid ||
      !cr::isPositiveCreativeVec3(bounds.size)) {
    return {false, false,
            "creative_editor_world_layout_catalog_asset_invalid"};
  }

  const bool changed = state.tool != CreativeEditorWorldLayoutTool::CatalogAsset ||
                       !samePlacementAsset(state.catalogPlacement, entry);
  const CreativeEditorWorldLayoutCatalogSnapMode snapMode =
      state.catalogPlacement.snapMode;
  const double elevation = state.catalogPlacement.elevationCells;
  const double yaw = state.catalogPlacement.yawDegrees;
  const cr::CreativeVec3 scale = state.catalogPlacement.scale;
  const bool wallSideFlipped = state.catalogPlacement.wallSideFlipped;
  static_cast<void>(setCreativeEditorWorldLayoutTool(
      state, CreativeEditorWorldLayoutTool::CatalogAsset));
  state.catalogPlacement = {};
  state.catalogPlacement.active = true;
  state.catalogPlacement.kind = entry.hotbarEntry.objectKind;
  state.catalogPlacement.assetId = std::string(assetId);
  state.catalogPlacement.label =
      entry.label.empty() ? std::string(assetId) : entry.label;
  state.catalogPlacement.categoryId =
      entry.assetAuthoringMetadata.categoryId;
  state.catalogPlacement.sourceBoundsMeters =
      entry.hotbarEntry.assetSourceBounds;
  state.catalogPlacement.snapMode =
      hostedOpeningCategory(state.catalogPlacement.categoryId)
          ? CreativeEditorWorldLayoutCatalogSnapMode::Wall
          : snapMode;
  state.catalogPlacement.elevationCells = elevation;
  state.catalogPlacement.yawDegrees = yaw;
  state.catalogPlacement.scale = scale;
  state.catalogPlacement.wallSideFlipped = wallSideFlipped;
  state.assetCategory = classifyCreativeEditorWorldLayoutAsset(entry);
  state.statusMessage = state.catalogPlacement.label + " ready to place";
  return {true, changed,
          "creative_editor_world_layout_catalog_asset_selected"};
}

CreativeEditorWorldLayoutObjectFootprint
planCreativeEditorWorldLayoutObjectFootprint(
    const cr::CreativeWorldLayoutObject& object,
    cr::CreativeGridSettings grid) noexcept {
  CreativeEditorWorldLayoutObjectFootprint result;
  if (object.mode == cr::CreativeObjectLibraryPlacementMode::Bounds) {
    const cr::CreativeBoundsMetrics measured =
        cr::measureCreativeBounds(object.boundsCells);
    if (!measured.valid || !cr::isPositiveCreativeVec3(measured.size)) {
      return result;
    }
    result.axisAlignedBoundsCells = object.boundsCells;
    result.corners = {
        CreativeEditorWorldLayoutPoint{object.boundsCells.min.x,
                                       object.boundsCells.min.z},
        CreativeEditorWorldLayoutPoint{object.boundsCells.max.x,
                                       object.boundsCells.min.z},
        CreativeEditorWorldLayoutPoint{object.boundsCells.max.x,
                                       object.boundsCells.max.z},
        CreativeEditorWorldLayoutPoint{object.boundsCells.min.x,
                                       object.boundsCells.max.z},
    };
    result.valid = true;
    return result;
  }
  if (object.mode != cr::CreativeObjectLibraryPlacementMode::Point ||
      !object.hasAssetSourceBounds ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0 ||
      !cr::isFiniteCreativeVec3(object.pointCells) ||
      !std::isfinite(object.yawRadians) || !finitePositiveScale(object.scale)) {
    return result;
  }
  const cr::CreativeBoundsMetrics source =
      cr::measureCreativeBounds(object.assetSourceBoundsMeters);
  if (!source.valid || !cr::isPositiveCreativeVec3(source.size)) {
    return result;
  }

  const double inverseCell = 1.0 / grid.cellSizeMeters;
  const cr::CreativeBounds authoredCells{
      {object.pointCells.x + object.assetSourceBoundsMeters.min.x * inverseCell,
       object.pointCells.y + object.assetSourceBoundsMeters.min.y * inverseCell,
       object.pointCells.z + object.assetSourceBoundsMeters.min.z * inverseCell},
      {object.pointCells.x + object.assetSourceBoundsMeters.max.x * inverseCell,
       object.pointCells.y + object.assetSourceBoundsMeters.max.y * inverseCell,
       object.pointCells.z + object.assetSourceBoundsMeters.max.z * inverseCell},
  };
  cr::CreativeTransform transform;
  transform.position = object.pointCells;
  transform.rotationEulerRadians.y = object.yawRadians;
  transform.scale = object.scale;
  const cr::CreativeTransformedBounds transformed =
      cr::resolveCreativeTransformedBounds(authoredCells, transform);
  if (!transformed.valid) {
    return result;
  }
  constexpr std::array<std::size_t, 4U> kPlanCornerIndices{0U, 1U, 5U, 4U};
  for (std::size_t index = 0U; index < result.corners.size(); ++index) {
    const cr::CreativeVec3 corner = transformed.corners[kPlanCornerIndices[index]];
    result.corners[index] = {corner.x, corner.z};
  }
  result.axisAlignedBoundsCells = transformed.worldBounds;
  result.valid = true;
  return result;
}

CreativeEditorWorldLayoutCatalogPlacementPlan
planCreativeEditorWorldLayoutCatalogPlacement(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid) {
  CreativeEditorWorldLayoutCatalogPlacementPlan result;
  const CreativeEditorWorldLayoutCatalogPlacementState& selected =
      state.catalogPlacement;
  if (state.tool != CreativeEditorWorldLayoutTool::CatalogAsset ||
      !selected.active || !detail::finiteWorldLayoutPoint(point) ||
      selected.kind <= cr::CreativeObjectKind::Unknown ||
      selected.kind >= cr::CreativeObjectKind::Count ||
      selected.assetId.empty() || selected.label.empty() ||
      !std::isfinite(selected.elevationCells) ||
      !std::isfinite(selected.yawDegrees) ||
      !finitePositiveScale(selected.scale) ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    result.message = "Placement settings are invalid";
    result.reasonCode =
        "creative_editor_world_layout_catalog_placement_invalid";
    return result;
  }
  if (hostedOpeningCategory(selected.categoryId)) {
    result.hostedOpening = true;
    result.snapMode = CreativeEditorWorldLayoutCatalogSnapMode::Wall;
    result.openingPlacement = planCreativeEditorWorldLayoutOpeningPlacement(
        state, hostedOpeningPlacementRequest(selected, point, grid));
    result.accepted = result.openingPlacement.accepted;
    result.message = std::string(result.openingPlacement.message);
    result.reasonCode = std::string(result.openingPlacement.reasonCode);
    return result;
  }
  result.snapMode = selected.snapMode;
  const CatalogSnapResolution snap = resolveCatalogSnap(state, point, grid);
  result.snapHostKind = snap.hostKind;
  result.snapHostIndex = snap.hostIndex;
  result.snapRoomEdge = snap.roomEdge;
  result.snapDistanceCells = snap.distanceCells;
  result.snapSurfacePoint = snap.surfacePoint;
  result.snapNormal = snap.normal;
  result.snapWallThicknessCells = snap.wallThicknessCells;
  result.snapContactOffsetCells = snap.contactOffsetCells;
  result.message = snap.message;
  if (!snap.accepted) {
    result.reasonCode = snap.reasonCode;
    return result;
  }

  result.object.kind = selected.kind;
  result.object.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  result.object.name = selected.label;
  result.object.assetId = selected.assetId;
  result.object.pointCells = {snap.point.x, snap.elevationCells, snap.point.z};
  result.object.assetSourceBoundsMeters = selected.sourceBoundsMeters;
  result.object.hasAssetSourceBounds = true;
  result.object.yawRadians = snap.yawRadians;
  result.object.scale = selected.scale;
  result.object.tags = {"world_layout:object", "world_layout:catalog_asset"};
  result.footprint =
      planCreativeEditorWorldLayoutObjectFootprint(result.object, grid);
  if (!result.footprint.valid) {
    result.object = {};
    result.message = "Asset footprint is invalid";
    result.reasonCode =
        "creative_editor_world_layout_catalog_footprint_invalid";
    return result;
  }
  result.accepted = true;
  result.reasonCode = "creative_editor_world_layout_catalog_placement_ready";
  return result;
}

std::size_t findCreativeEditorWorldLayoutObjectAt(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid) noexcept {
  if (!detail::finiteWorldLayoutPoint(point)) {
    return cr::kInvalidCreativeWorldLayoutIndex;
  }
  for (std::size_t index = state.source.objects.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutObject& object =
        state.source.objects[index - 1U];
    const CreativeEditorWorldLayoutObjectFootprint footprint =
        planCreativeEditorWorldLayoutObjectFootprint(object, grid);
    const bool hit =
        footprint.valid
            ? pointInsideFootprint(footprint, point)
            : object.mode == cr::CreativeObjectLibraryPlacementMode::Point &&
                  std::hypot(point.x - object.pointCells.x,
                             point.z - object.pointCells.z) <= 0.6;
    if (hit) {
      return index - 1U;
    }
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

CreativeEditorWorldLayoutEditReceipt applyCreativeEditorWorldLayoutPoint(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid) {
  if (state.tool != CreativeEditorWorldLayoutTool::CatalogAsset) {
    return detail::applyWorldLayoutToolPoint(state, point, grid);
  }
  CreativeEditorWorldLayoutCatalogPlacementPlan plan =
      planCreativeEditorWorldLayoutCatalogPlacement(state, point, grid);
  if (!plan.accepted) {
    state.statusMessage = plan.message;
    return {false, false, std::move(plan.reasonCode)};
  }
  if (plan.hostedOpening) {
    return applyCreativeEditorWorldLayoutOpeningPlacement(
        state, hostedOpeningPlacementRequest(state.catalogPlacement, point,
                                             grid));
  }
  plan.object.stableKey = detail::mintWorldLayoutStableKey(state, "asset");
  state.source.objects.push_back(std::move(plan.object));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Object,
                     state.source.objects.size() - 1U};
  detail::noteWorldLayoutSourceChange(state, "catalog asset added");
  return {true, true,
          "creative_editor_world_layout_catalog_asset_added"};
}

}  // namespace iggy3d_creative_app
