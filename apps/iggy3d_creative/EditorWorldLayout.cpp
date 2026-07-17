#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"
#include "EditorWorldLayoutHistory.hpp"

#include "app/iggy3d/creative/world/MapTemplate.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <tuple>
#include <utility>

namespace iggy3d_creative_app {
namespace {

constexpr double kOpeningHitToleranceCells = 0.75;
constexpr double kSelectionHitToleranceCells = 0.35;
constexpr double kOpeningSnapCells = 0.25;
constexpr double kOpeningEndClearanceCells = 0.25;
constexpr double kOpeningMinimumWidthCells = 0.25;
constexpr double kOpeningGeometryEpsilon = 1.0e-9;

constexpr std::array<CreativeEditorWorldLayoutPaletteEntry, 17U>
    kWorldLayoutPaletteEntries = {{
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Select",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Select, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Estate House",
         CreativeEditorWorldLayoutPaletteActivation::BuildingTemplate,
         CreativeEditorWorldLayoutTool::Select,
         cr::kBuilderEstateHouseTemplateId},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Building Shell",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::BuildingShell, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Add Room",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Room, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Floor",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Floor, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Partition",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Wall, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Door",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Door, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Window",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Window, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Stair",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Stair, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Structure, "Ramp",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Ramp, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Terrain, "Plateau",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Plateau, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Terrain, "Road",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Road, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Terrain, "Ditch",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Ditch, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Object, "Bridge",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Bridge, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Object, "Boulder",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::Boulder, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Gameplay, "Player Spawn",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::PlayerSpawn, {}},
        {CreativeEditorWorldLayoutPaletteCategory::Gameplay, "NPC Spawn",
         CreativeEditorWorldLayoutPaletteActivation::Tool,
         CreativeEditorWorldLayoutTool::NpcSpawn, {}},
    }};

struct VerticalConnectorToolSpec {
  CreativeEditorWorldLayoutTool tool = CreativeEditorWorldLayoutTool::Count;
  cr::CreativeWorldLayoutVerticalConnectorKind kind =
      cr::CreativeWorldLayoutVerticalConnectorKind::Count;
  const char* label = "Unknown";
  const char* keyPrefix = "vertical_connector";
};

constexpr std::array<VerticalConnectorToolSpec, 2U> kVerticalConnectorToolSpecs{
    {
        {CreativeEditorWorldLayoutTool::Stair,
         cr::CreativeWorldLayoutVerticalConnectorKind::Stair, "Stair", "stair"},
        {CreativeEditorWorldLayoutTool::Ramp,
         cr::CreativeWorldLayoutVerticalConnectorKind::Ramp, "Ramp", "ramp"},
    }};

const VerticalConnectorToolSpec*
verticalConnectorToolSpec(CreativeEditorWorldLayoutTool tool) noexcept {
  const auto found = std::find_if(
      kVerticalConnectorToolSpecs.begin(), kVerticalConnectorToolSpecs.end(),
      [tool](const VerticalConnectorToolSpec& candidate) {
        return candidate.tool == tool;
      });
  return found == kVerticalConnectorToolSpecs.end() ? nullptr : &*found;
}

std::string verticalConnectorReasonCode(const VerticalConnectorToolSpec& spec,
                                        std::string_view suffix) {
  return "creative_editor_world_layout_" + std::string(spec.keyPrefix) +
         std::string(suffix);
}

std::string verticalConnectorMessage(const VerticalConnectorToolSpec& spec,
                                     std::string_view suffix) {
  return std::string(spec.keyPrefix) + std::string(suffix);
}

using detail::clearWorldLayoutInteraction;
using detail::invalidateWorldLayoutPreview;
using detail::mintWorldLayoutStableKey;
using detail::noteWorldLayoutSourceChange;
using detail::worldLayoutManipulatedRect;
using detail::worldLayoutRectHandleAt;

bool finitePoint(CreativeEditorWorldLayoutPoint point) noexcept {
  return std::isfinite(point.x) && std::isfinite(point.z);
}

bool roomOnActiveLevel(const CreativeEditorWorldLayoutState& state,
                       std::size_t roomIndex) noexcept {
  return roomIndex < state.source.rooms.size() &&
         (state.activeLevelIndex >= state.source.levels.size() ||
          state.source.rooms[roomIndex].levelIndex == state.activeLevelIndex);
}

bool openingOnActiveLevel(const CreativeEditorWorldLayoutState& state,
                          std::size_t openingIndex) noexcept {
  if (openingIndex >= state.source.openings.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutOpening& opening =
      state.source.openings[openingIndex];
  return opening.hostKind !=
             cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
         roomOnActiveLevel(state, opening.roomIndex);
}

bool verticalConnectorOnLevel(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutVerticalConnector& connector,
    std::size_t levelIndex) noexcept {
  if (levelIndex >= layout.levels.size() ||
      connector.lowerRoomIndex >= layout.rooms.size() ||
      connector.upperRoomIndex >= layout.rooms.size()) {
    return levelIndex >= layout.levels.size();
  }
  return layout.rooms[connector.lowerRoomIndex].levelIndex == levelIndex ||
         layout.rooms[connector.upperRoomIndex].levelIndex == levelIndex;
}

bool toGridCoord(CreativeEditorWorldLayoutPoint point,
                 cr::CreativeTerrainCoord2& output) noexcept {
  if (!finitePoint(point)) {
    return false;
  }
  const double roundedX = std::round(point.x);
  const double roundedZ = std::round(point.z);
  if (roundedX < std::numeric_limits<std::int32_t>::min() ||
      roundedX > std::numeric_limits<std::int32_t>::max() ||
      roundedZ < std::numeric_limits<std::int32_t>::min() ||
      roundedZ > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output.x = static_cast<std::int32_t>(roundedX);
  output.z = static_cast<std::int32_t>(roundedZ);
  return true;
}

std::size_t ensurePrimaryBuilding(CreativeEditorWorldLayoutState& state) {
  if (!state.source.buildings.empty()) {
    return 0U;
  }
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = mintWorldLayoutStableKey(state, "building");
  building.name = "Building 1";
  building.rootMode = cr::CreativeBuildingRootMode::None;
  state.source.buildings.push_back(std::move(building));
  return 0U;
}

cr::CreativeWorldLayoutRect normalizedRect(cr::CreativeTerrainCoord2 first,
                                           cr::CreativeTerrainCoord2 second) {
  return {{std::min(first.x, second.x), std::min(first.z, second.z)},
          {std::max(first.x, second.x), std::max(first.z, second.z)}};
}

cr::CreativeTerrainCoord2 cardinalEnd(cr::CreativeTerrainCoord2 start,
                                      cr::CreativeTerrainCoord2 requested) {
  const std::int64_t deltaX = static_cast<std::int64_t>(requested.x) - start.x;
  const std::int64_t deltaZ = static_cast<std::int64_t>(requested.z) - start.z;
  if (std::llabs(deltaX) >= std::llabs(deltaZ)) {
    requested.z = start.z;
  } else {
    requested.x = start.x;
  }
  return requested;
}

struct OpeningHostProjection {
  bool hit = false;
  cr::CreativeWorldLayoutOpeningHostKind hostKind =
      cr::CreativeWorldLayoutOpeningHostKind::Wall;
  std::size_t wallIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t roomIndex = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutRoomEdge roomEdge =
      cr::CreativeWorldLayoutRoomEdge::North;
  double centerOffsetCells = 0.0;
  double lengthCells = 0.0;
  double distanceCells = std::numeric_limits<double>::infinity();
};

std::pair<cr::CreativeTerrainCoord2, cr::CreativeTerrainCoord2> roomEdgeSegment(
    const cr::CreativeWorldLayoutRoom& room,
    cr::CreativeWorldLayoutRoomEdge edge) {
  switch (edge) {
    case cr::CreativeWorldLayoutRoomEdge::North:
      return {{room.footprint.minimum.x, room.footprint.minimum.z},
              {room.footprint.maximum.x, room.footprint.minimum.z}};
    case cr::CreativeWorldLayoutRoomEdge::East:
      return {{room.footprint.maximum.x, room.footprint.minimum.z},
              {room.footprint.maximum.x, room.footprint.maximum.z}};
    case cr::CreativeWorldLayoutRoomEdge::South:
      return {{room.footprint.minimum.x, room.footprint.maximum.z},
              {room.footprint.maximum.x, room.footprint.maximum.z}};
    case cr::CreativeWorldLayoutRoomEdge::West:
      return {{room.footprint.minimum.x, room.footprint.minimum.z},
              {room.footprint.minimum.x, room.footprint.maximum.z}};
    case cr::CreativeWorldLayoutRoomEdge::Count:
      break;
  }
  return {};
}

CreativeEditorWorldLayoutOpeningHost openingHost(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutOpening& opening) noexcept {
  CreativeEditorWorldLayoutOpeningHost host;
  cr::CreativeTerrainCoord2 start{};
  cr::CreativeTerrainCoord2 end{};
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall) {
    if (opening.wallIndex >= layout.walls.size()) {
      return host;
    }
    const cr::CreativeWorldLayoutWall& wall = layout.walls[opening.wallIndex];
    start = wall.start;
    end = wall.end;
    host.wallHeightCells = wall.heightCells;
  } else if (opening.hostKind ==
             cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
    if (opening.roomIndex >= layout.rooms.size() ||
        opening.roomEdge >= cr::CreativeWorldLayoutRoomEdge::Count) {
      return host;
    }
    const cr::CreativeWorldLayoutRoom& room = layout.rooms[opening.roomIndex];
    std::tie(start, end) = roomEdgeSegment(room, opening.roomEdge);
    const cr::CreativeWorldLayoutLevel* level =
        cr::creativeWorldLayoutLevelForRoom(layout, opening.roomIndex);
    if (level == nullptr) {
      return host;
    }
    host.wallHeightCells = level->wallHeightCells;
  } else {
    return host;
  }
  const double dx = static_cast<double>(end.x) - start.x;
  const double dz = static_cast<double>(end.z) - start.z;
  host.lengthCells = std::hypot(dx, dz);
  if (!std::isfinite(host.lengthCells) || host.lengthCells <= 0.0 ||
      !std::isfinite(host.wallHeightCells) || host.wallHeightCells <= 0.0) {
    return {};
  }
  host.valid = true;
  host.start = {static_cast<double>(start.x), static_cast<double>(start.z)};
  host.end = {static_cast<double>(end.x), static_cast<double>(end.z)};
  return host;
}

double openingHostOffset(CreativeEditorWorldLayoutOpeningHost host,
                         CreativeEditorWorldLayoutPoint point) noexcept {
  if (!host.valid || !finitePoint(point)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double dx = host.end.x - host.start.x;
  const double dz = host.end.z - host.start.z;
  return ((point.x - host.start.x) * dx + (point.z - host.start.z) * dz) /
         host.lengthCells;
}

CreativeEditorWorldLayoutPoint openingHostPoint(
    CreativeEditorWorldLayoutOpeningHost host, double offsetCells) noexcept {
  if (!host.valid || !std::isfinite(offsetCells)) {
    return {};
  }
  const double inverseLength = 1.0 / host.lengthCells;
  return {host.start.x + (host.end.x - host.start.x) * inverseLength *
                             offsetCells,
          host.start.z + (host.end.z - host.start.z) * inverseLength *
                             offsetCells};
}

double pointDistance(CreativeEditorWorldLayoutPoint lhs,
                     CreativeEditorWorldLayoutPoint rhs) noexcept {
  return std::hypot(lhs.x - rhs.x, lhs.z - rhs.z);
}

void considerSegment(OpeningHostProjection& best,
                     CreativeEditorWorldLayoutPoint point,
                     cr::CreativeTerrainCoord2 start,
                     cr::CreativeTerrainCoord2 end, double tolerance,
                     cr::CreativeWorldLayoutOpeningHostKind hostKind,
                     std::size_t hostIndex,
                     cr::CreativeWorldLayoutRoomEdge roomEdge) {
  const double dx = static_cast<double>(end.x) - start.x;
  const double dz = static_cast<double>(end.z) - start.z;
  const double lengthSquared = dx * dx + dz * dz;
  if (lengthSquared <= 0.0) {
    return;
  }
  const double t = std::clamp(
      ((point.x - start.x) * dx + (point.z - start.z) * dz) /
          lengthSquared,
      0.0, 1.0);
  const double projectedX = start.x + t * dx;
  const double projectedZ = start.z + t * dz;
  const double distance =
      std::hypot(point.x - projectedX, point.z - projectedZ);
  if (distance >= best.distanceCells) {
    return;
  }
  const double length = std::sqrt(lengthSquared);
  best.hit = distance <= tolerance;
  best.hostKind = hostKind;
  best.wallIndex = hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall
                       ? hostIndex
                       : cr::kInvalidCreativeWorldLayoutIndex;
  best.roomIndex = hostKind == cr::CreativeWorldLayoutOpeningHostKind::RoomEdge
                       ? hostIndex
                       : cr::kInvalidCreativeWorldLayoutIndex;
  best.roomEdge = roomEdge;
  best.centerOffsetCells = t * length;
  best.lengthCells = length;
  best.distanceCells = distance;
}

OpeningHostProjection nearestOpeningHost(
    const cr::CreativeWorldLayout& layout,
    CreativeEditorWorldLayoutPoint point, double tolerance,
    std::size_t activeLevelIndex) {
  OpeningHostProjection best;
  for (std::size_t index = 0U; index < layout.walls.size(); ++index) {
    const cr::CreativeWorldLayoutWall& wall = layout.walls[index];
    considerSegment(best, point, wall.start, wall.end, tolerance,
                    cr::CreativeWorldLayoutOpeningHostKind::Wall, index,
                    cr::CreativeWorldLayoutRoomEdge::North);
  }
  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
    if (activeLevelIndex < layout.levels.size() &&
        layout.rooms[roomIndex].levelIndex != activeLevelIndex) {
      continue;
    }
    for (std::uint8_t edgeValue = 0U;
         edgeValue < static_cast<std::uint8_t>(
                         cr::CreativeWorldLayoutRoomEdge::Count);
         ++edgeValue) {
      const auto edge =
          static_cast<cr::CreativeWorldLayoutRoomEdge>(edgeValue);
      const auto [start, end] = roomEdgeSegment(layout.rooms[roomIndex], edge);
      considerSegment(best, point, start, end, tolerance,
                      cr::CreativeWorldLayoutOpeningHostKind::RoomEdge,
                      roomIndex, edge);
    }
  }
  return best;
}

bool sameHost(const cr::CreativeWorldLayoutOpening& opening,
              const OpeningHostProjection& projection) noexcept {
  if (opening.hostKind != projection.hostKind) {
    return false;
  }
  return opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall
             ? opening.wallIndex == projection.wallIndex
             : opening.roomIndex == projection.roomIndex &&
                   opening.roomEdge == projection.roomEdge;
}

double distanceToSegment(CreativeEditorWorldLayoutPoint point,
                         cr::CreativeTerrainCoord2 start,
                         cr::CreativeTerrainCoord2 end) noexcept {
  const double dx = static_cast<double>(end.x) - start.x;
  const double dz = static_cast<double>(end.z) - start.z;
  const double lengthSquared = dx * dx + dz * dz;
  if (lengthSquared <= 0.0) {
    return std::hypot(point.x - start.x, point.z - start.z);
  }
  const double t = std::clamp(
      ((point.x - start.x) * dx + (point.z - start.z) * dz) /
          lengthSquared,
      0.0, 1.0);
  return std::hypot(point.x - (start.x + t * dx),
                    point.z - (start.z + t * dz));
}

CreativeEditorWorldLayoutSelection hitTest(
    const cr::CreativeWorldLayout& layout,
    CreativeEditorWorldLayoutPoint point,
    std::size_t activeLevelIndex) {
  const OpeningHostProjection host =
      nearestOpeningHost(layout, point, kSelectionHitToleranceCells,
                         activeLevelIndex);
  if (host.hit) {
    for (std::size_t index = layout.openings.size(); index > 0U; --index) {
      const cr::CreativeWorldLayoutOpening& opening =
          layout.openings[index - 1U];
      if (!sameHost(opening, host)) {
        continue;
      }
      const double halfWidth = std::max(0.25, opening.widthCells * 0.5);
      if (std::fabs(opening.centerOffsetCells -
                    host.centerOffsetCells) <= halfWidth) {
        return {CreativeEditorWorldLayoutSelectionKind::Opening, index - 1U};
      }
    }
    if (host.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall) {
      return {CreativeEditorWorldLayoutSelectionKind::Wall,
              host.wallIndex};
    }
    return {CreativeEditorWorldLayoutSelectionKind::Room, host.roomIndex};
  }
  for (std::size_t index = layout.objects.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutObject& object = layout.objects[index - 1U];
    const bool hit =
        object.mode == cr::CreativeObjectLibraryPlacementMode::Bounds
            ? point.x >= object.boundsCells.min.x &&
                  point.x <= object.boundsCells.max.x &&
                  point.z >= object.boundsCells.min.z &&
                  point.z <= object.boundsCells.max.z
            : std::hypot(point.x - object.pointCells.x,
                         point.z - object.pointCells.z) <= 0.6;
    if (hit) {
      return {CreativeEditorWorldLayoutSelectionKind::Object, index - 1U};
    }
  }
  for (std::size_t index = layout.verticalConnectors.size(); index > 0U;
       --index) {
    const cr::CreativeWorldLayoutVerticalConnector& connector =
        layout.verticalConnectors[index - 1U];
    if (!verticalConnectorOnLevel(layout, connector, activeLevelIndex)) {
      continue;
    }
    const cr::CreativeWorldLayoutRect& rect = connector.footprint;
    if (point.x >= rect.minimum.x && point.x <= rect.maximum.x &&
        point.z >= rect.minimum.z && point.z <= rect.maximum.z) {
      return {CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
              index - 1U};
    }
  }
  for (std::size_t index = layout.rooms.size(); index > 0U; --index) {
    if (activeLevelIndex < layout.levels.size() &&
        layout.rooms[index - 1U].levelIndex != activeLevelIndex) {
      continue;
    }
    const cr::CreativeWorldLayoutRect& rect =
        layout.rooms[index - 1U].footprint;
    if (point.x >= rect.minimum.x && point.x <= rect.maximum.x &&
        point.z >= rect.minimum.z && point.z <= rect.maximum.z) {
      return {CreativeEditorWorldLayoutSelectionKind::Room, index - 1U};
    }
  }
  for (std::size_t index = layout.boxes.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutRect& rect =
        layout.boxes[index - 1U].footprint;
    if (point.x >= rect.minimum.x && point.x <= rect.maximum.x &&
        point.z >= rect.minimum.z && point.z <= rect.maximum.z) {
      return {CreativeEditorWorldLayoutSelectionKind::Box, index - 1U};
    }
  }
  for (std::size_t index = layout.terrainProfiles.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutTerrainProfile& profile =
        layout.terrainProfiles[index - 1U];
    if (std::hypot(point.x - profile.center.x, point.z - profile.center.z) <=
        0.65) {
      return {CreativeEditorWorldLayoutSelectionKind::TerrainProfile,
              index - 1U};
    }
  }
  for (std::size_t index = layout.terrainPaths.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutTerrainPath& path =
        layout.terrainPaths[index - 1U];
    if (path.pointCount < 2U ||
        path.firstPointIndex > layout.terrainPathPoints.size() ||
        path.pointCount >
            layout.terrainPathPoints.size() - path.firstPointIndex) {
      continue;
    }
    for (std::size_t pointIndex = path.firstPointIndex + 1U;
         pointIndex < path.firstPointIndex + path.pointCount; ++pointIndex) {
      if (distanceToSegment(
              point, layout.terrainPathPoints[pointIndex - 1U].coord,
              layout.terrainPathPoints[pointIndex].coord) <=
          std::max(0.65, static_cast<double>(path.halfWidthCells))) {
        return {CreativeEditorWorldLayoutSelectionKind::TerrainPath,
                index - 1U};
      }
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt selectAt(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point) {
  const CreativeEditorWorldLayoutSelection selected =
      hitTest(state.source, point, state.activeLevelIndex);
  const bool changed = selected.kind != state.selection.kind ||
                       selected.index != state.selection.index;
  state.selection = selected;
  if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      selected.index < state.source.rooms.size()) {
    state.activeLevelIndex = state.source.rooms[selected.index].levelIndex;
  } else if (selected.kind ==
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector &&
             selected.index < state.source.verticalConnectors.size()) {
    const cr::CreativeWorldLayoutVerticalConnector& connector =
        state.source.verticalConnectors[selected.index];
    if (state.activeLevelIndex >= state.source.levels.size() &&
        connector.lowerRoomIndex < state.source.rooms.size()) {
      state.activeLevelIndex =
          state.source.rooms[connector.lowerRoomIndex].levelIndex;
    }
  }
  state.anchorActive = false;
  clearWorldLayoutInteraction(state);
  state.statusMessage =
      selected.kind == CreativeEditorWorldLayoutSelectionKind::None
          ? "selection cleared"
          : "layout symbol selected";
  return {true, changed, "creative_editor_world_layout_selected"};
}

bool roomFootprintOverlaps(const cr::CreativeWorldLayout& layout,
                           cr::CreativeWorldLayoutRect footprint,
                           std::size_t buildingIndex,
                           std::size_t levelIndex,
                           std::size_t ignoredRoom =
                               cr::kInvalidCreativeWorldLayoutIndex) {
  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    if (index == ignoredRoom) {
      continue;
    }
    if (layout.rooms[index].buildingIndex != buildingIndex ||
        layout.rooms[index].levelIndex != levelIndex) {
      continue;
    }
    const cr::CreativeWorldLayoutRect existing = layout.rooms[index].footprint;
    if (std::max(footprint.minimum.x, existing.minimum.x) <
            std::min(footprint.maximum.x, existing.maximum.x) &&
        std::max(footprint.minimum.z, existing.minimum.z) <
            std::min(footprint.maximum.z, existing.maximum.z)) {
      return true;
    }
  }
  return false;
}

struct RoomSettingsValidation {
  bool accepted = false;
  std::string reasonCode =
      "creative_editor_world_layout_room_settings_invalid";
  std::string message = "room shell settings are invalid";
};

struct OpeningValidation {
  bool accepted = false;
  std::string reasonCode =
      "creative_editor_world_layout_opening_settings_invalid";
  std::string message = "opening settings are invalid";
};

bool nearlyEqual(double lhs, double rhs) noexcept {
  return std::fabs(lhs - rhs) <= kOpeningGeometryEpsilon;
}

bool validOpeningKind(cr::CreativeBuildingOpeningKind kind) noexcept {
  return kind == cr::CreativeBuildingOpeningKind::Door ||
         kind == cr::CreativeBuildingOpeningKind::Window;
}

bool validOpeningPose(cr::CreativeBuildingOpeningPose pose) noexcept {
  return pose >= cr::CreativeBuildingOpeningPose::Closed &&
         pose <=
             cr::CreativeBuildingOpeningPose::OpenFromEndPositiveNormal;
}

CreativeEditorWorldLayoutOpeningSettings openingSettings(
    const cr::CreativeWorldLayoutOpening& opening) noexcept {
  return {opening.centerOffsetCells,
          opening.widthCells,
          opening.cutoutBottomCells,
          opening.cutoutHeightCells,
          opening.pose,
          opening.includeInsert};
}

cr::CreativeWorldLayoutOpening openingWithSettings(
    const cr::CreativeWorldLayoutOpening& existing,
    CreativeEditorWorldLayoutOpeningSettings settings) {
  cr::CreativeWorldLayoutOpening candidate = existing;
  const bool insertTracksBottom =
      nearlyEqual(existing.insertBottomCells, existing.cutoutBottomCells);
  const bool insertTracksHeight =
      existing.insertHeightCells > 0.0 &&
      nearlyEqual(existing.insertHeightCells, existing.cutoutHeightCells);
  const bool insertTracksWidth =
      existing.insertWidthCells > 0.0 &&
      nearlyEqual(existing.insertWidthCells, existing.widthCells);
  candidate.centerOffsetCells = settings.centerOffsetCells;
  candidate.widthCells = settings.widthCells;
  candidate.cutoutBottomCells = settings.sillHeightCells;
  candidate.cutoutHeightCells = settings.heightCells;
  candidate.pose = settings.pose;
  candidate.includeInsert = settings.includeInsert;
  if (insertTracksBottom) {
    candidate.insertBottomCells = settings.sillHeightCells;
  }
  if (insertTracksHeight) {
    candidate.insertHeightCells = settings.heightCells;
  }
  if (insertTracksWidth) {
    candidate.insertWidthCells = settings.widthCells;
  }
  return candidate;
}

bool sameEditableOpening(const cr::CreativeWorldLayoutOpening& lhs,
                         const cr::CreativeWorldLayoutOpening& rhs) noexcept {
  return lhs.centerOffsetCells == rhs.centerOffsetCells &&
         lhs.widthCells == rhs.widthCells &&
         lhs.cutoutBottomCells == rhs.cutoutBottomCells &&
         lhs.cutoutHeightCells == rhs.cutoutHeightCells &&
         lhs.pose == rhs.pose && lhs.includeInsert == rhs.includeInsert &&
         lhs.insertBottomCells == rhs.insertBottomCells &&
         lhs.insertHeightCells == rhs.insertHeightCells &&
         lhs.insertWidthCells == rhs.insertWidthCells &&
         lhs.insertThicknessCells == rhs.insertThicknessCells;
}

OpeningValidation validateOpeningCandidate(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    const cr::CreativeWorldLayoutOpening& candidate) {
  if (openingIndex >= state.source.openings.size() ||
      !validOpeningKind(candidate.kind) || !validOpeningPose(candidate.pose) ||
      !std::isfinite(candidate.centerOffsetCells) ||
      !std::isfinite(candidate.widthCells) ||
      !std::isfinite(candidate.cutoutBottomCells) ||
      !std::isfinite(candidate.cutoutHeightCells) ||
      candidate.widthCells < kOpeningMinimumWidthCells ||
      candidate.cutoutBottomCells < 0.0 ||
      candidate.cutoutHeightCells <= 0.0) {
    return {};
  }
  if (candidate.kind == cr::CreativeBuildingOpeningKind::Door &&
      !nearlyEqual(candidate.cutoutBottomCells, 0.0)) {
    return {false, "creative_editor_world_layout_door_sill_invalid",
            "door openings must begin at floor height"};
  }
  if (candidate.kind == cr::CreativeBuildingOpeningKind::Window &&
      candidate.pose != cr::CreativeBuildingOpeningPose::Closed) {
    return {false, "creative_editor_world_layout_window_pose_invalid",
            "windows do not support door swing poses"};
  }

  const CreativeEditorWorldLayoutOpeningHost host =
      openingHost(state.source, candidate);
  if (!host.valid) {
    return {false, "creative_editor_world_layout_opening_host_invalid",
            "opening host is unavailable"};
  }
  const double halfWidth = candidate.widthCells * 0.5;
  if (candidate.centerOffsetCells - halfWidth <
          kOpeningEndClearanceCells - kOpeningGeometryEpsilon ||
      candidate.centerOffsetCells + halfWidth >
          host.lengthCells - kOpeningEndClearanceCells +
              kOpeningGeometryEpsilon) {
    return {false, "creative_editor_world_layout_opening_end_clearance_invalid",
            "opening needs a quarter-cell wall pier at each end"};
  }
  if (candidate.cutoutBottomCells + candidate.cutoutHeightCells >
      host.wallHeightCells + kOpeningGeometryEpsilon) {
    return {false, "creative_editor_world_layout_opening_height_invalid",
            "opening exceeds the host wall height"};
  }

  if (!std::isfinite(candidate.insertBottomCells) ||
      !std::isfinite(candidate.insertHeightCells) ||
      !std::isfinite(candidate.insertWidthCells) ||
      !std::isfinite(candidate.insertThicknessCells) ||
      candidate.insertBottomCells < 0.0 ||
      candidate.insertHeightCells < 0.0 ||
      candidate.insertWidthCells < 0.0 ||
      candidate.insertThicknessCells < 0.0) {
    return {false, "creative_editor_world_layout_opening_insert_invalid",
            "opening insert dimensions are invalid"};
  }
  if (candidate.includeInsert) {
    const double insertBottom =
        candidate.kind == cr::CreativeBuildingOpeningKind::Window &&
                candidate.insertBottomCells == 0.0
            ? candidate.cutoutBottomCells
            : candidate.insertBottomCells;
    const double insertHeight = candidate.insertHeightCells > 0.0
                                    ? candidate.insertHeightCells
                                    : candidate.cutoutHeightCells;
    const double insertWidth = candidate.insertWidthCells > 0.0
                                   ? candidate.insertWidthCells
                                   : candidate.widthCells;
    if (insertWidth > candidate.widthCells + kOpeningGeometryEpsilon ||
        insertBottom + kOpeningGeometryEpsilon <
            candidate.cutoutBottomCells ||
        insertBottom + insertHeight >
            candidate.cutoutBottomCells + candidate.cutoutHeightCells +
                kOpeningGeometryEpsilon) {
      return {false, "creative_editor_world_layout_opening_insert_fit_invalid",
              "opening insert no longer fits its cutout"};
    }
  }

  cr::CreativeWorldLayout staged = state.source;
  staged.openings[openingIndex] = candidate;
  if (cr::creativeWorldLayoutHasInteriorRoomWindow(staged)) {
    return {false, "creative_editor_world_layout_window_requires_exterior",
            "windows must remain on exterior room edges"};
  }
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(staged);
  if (!expanded.accepted ||
      openingIndex >= expanded.expanded.openings.size()) {
    return {false, expanded.reasonCode, expanded.reasonCode};
  }
  const cr::CreativeWorldLayoutOpening& resolved =
      expanded.expanded.openings[openingIndex];
  for (std::size_t index = 0U; index < expanded.expanded.openings.size();
       ++index) {
    if (index == openingIndex) {
      continue;
    }
    const cr::CreativeWorldLayoutOpening& existing =
        expanded.expanded.openings[index];
    if (existing.wallIndex == resolved.wallIndex &&
        std::fabs(existing.centerOffsetCells - resolved.centerOffsetCells) <=
            (existing.widthCells + resolved.widthCells) * 0.5 +
                kOpeningGeometryEpsilon) {
      return {false, "creative_editor_world_layout_opening_overlap",
              "opening overlaps another opening on this wall"};
    }
  }
  return {true, "creative_editor_world_layout_opening_settings_ready",
          "opening settings ready"};
}

CreativeEditorWorldLayoutEditReceipt commitOpeningCandidate(
    CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    const cr::CreativeWorldLayoutOpening& candidate,
    std::string statusMessage) {
  const OpeningValidation validation =
      validateOpeningCandidate(state, openingIndex, candidate);
  if (!validation.accepted) {
    state.statusMessage = validation.message;
    return {false, false, validation.reasonCode};
  }
  if (sameEditableOpening(state.source.openings[openingIndex], candidate)) {
    return {true, false,
            "creative_editor_world_layout_opening_settings_no_change"};
  }
  state.source.openings[openingIndex] = candidate;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                     openingIndex};
  noteWorldLayoutSourceChange(state, std::move(statusMessage));
  return {true, true,
          "creative_editor_world_layout_opening_settings_updated"};
}

CreativeEditorWorldLayoutRoomSettings roomSettings(
    const cr::CreativeWorldLayout& layout, std::size_t roomIndex,
    cr::CreativeWorldLayoutRect footprint) noexcept {
  if (roomIndex >= layout.rooms.size()) {
    return {};
  }
  const cr::CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
  const cr::CreativeWorldLayoutLevel* level =
      cr::creativeWorldLayoutLevelForRoom(layout, roomIndex);
  return level == nullptr
             ? CreativeEditorWorldLayoutRoomSettings{}
             : CreativeEditorWorldLayoutRoomSettings{
                   footprint, level->floorTopLayer, level->wallHeightCells,
                   room.wallThicknessCells, level->floorThicknessLayers};
}

RoomSettingsValidation validateRoomSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    const CreativeEditorWorldLayoutRoomSettings& settings) {
  const double width = static_cast<double>(settings.footprint.maximum.x) -
                       settings.footprint.minimum.x;
  const double depth = static_cast<double>(settings.footprint.maximum.z) -
                       settings.footprint.minimum.z;
  if (roomIndex >= state.source.rooms.size() || width <= 0.0 || depth <= 0.0 ||
      !std::isfinite(settings.floorTopLayer) ||
      settings.wallHeightCells == 0U || settings.floorThicknessLayers == 0U ||
      !std::isfinite(settings.wallThicknessCells) ||
      settings.wallThicknessCells <= 0.0 ||
      width <= settings.wallThicknessCells * 2.0 ||
      depth <= settings.wallThicknessCells * 2.0) {
    return {};
  }

  const cr::CreativeWorldLayoutRoom& existingRoom =
      state.source.rooms[roomIndex];
  if (cr::creativeWorldLayoutLevelForRoom(state.source, roomIndex) == nullptr) {
    return {};
  }
  if (roomFootprintOverlaps(state.source, settings.footprint,
                            existingRoom.buildingIndex,
                            existingRoom.levelIndex,
                            roomIndex)) {
    return {false, "creative_editor_world_layout_room_overlap",
            "rooms may touch but cannot overlap"};
  }

  for (const cr::CreativeWorldLayoutOpening& opening : state.source.openings) {
    if (opening.hostKind != cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        opening.roomIndex != roomIndex) {
      continue;
    }
    const bool horizontal =
        opening.roomEdge == cr::CreativeWorldLayoutRoomEdge::North ||
        opening.roomEdge == cr::CreativeWorldLayoutRoomEdge::South;
    const double edgeLength = horizontal ? width : depth;
    const double halfWidth = opening.widthCells * 0.5;
    if (opening.centerOffsetCells - halfWidth <
            kOpeningEndClearanceCells - kOpeningGeometryEpsilon ||
        opening.centerOffsetCells + halfWidth >
            edgeLength - kOpeningEndClearanceCells +
                kOpeningGeometryEpsilon) {
      return {
          false,
          "creative_editor_world_layout_room_resize_opening_invalid",
          "resize would move an opening outside its wall",
      };
    }
  }

  cr::CreativeWorldLayout candidate = state.source;
  cr::CreativeWorldLayoutRoom& candidateRoom = candidate.rooms[roomIndex];
  candidateRoom.footprint = settings.footprint;
  candidateRoom.wallThicknessCells = settings.wallThicknessCells;
  cr::CreativeWorldLayoutLevel& candidateLevel =
      candidate.levels[candidateRoom.levelIndex];
  candidateLevel.floorTopLayer = settings.floorTopLayer;
  candidateLevel.wallHeightCells = settings.wallHeightCells;
  candidateLevel.floorThicknessLayers = settings.floorThicknessLayers;
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(candidate);
  if (!expanded.accepted) {
    return {false, expanded.reasonCode, expanded.reasonCode};
  }
  if (cr::creativeWorldLayoutHasInteriorRoomWindow(candidate)) {
    return {false,
            "creative_editor_world_layout_room_resize_interior_window",
            "resize would turn a window into an interior opening"};
  }
  for (std::size_t index = 0U; index < expanded.expanded.openings.size();
       ++index) {
    const cr::CreativeWorldLayoutOpening& opening =
        expanded.expanded.openings[index];
    for (std::size_t prior = 0U; prior < index; ++prior) {
      const cr::CreativeWorldLayoutOpening& existing =
          expanded.expanded.openings[prior];
      if (opening.wallIndex == existing.wallIndex &&
          std::fabs(opening.centerOffsetCells -
                    existing.centerOffsetCells) <=
              (opening.widthCells + existing.widthCells) * 0.5 + 1.0e-9) {
        return {false, "creative_editor_world_layout_opening_overlap",
                "resize would overlap openings on a shared wall"};
      }
    }
  }
  return {true, "creative_editor_world_layout_room_settings_ready",
          "room shell settings ready"};
}

bool snappedOpeningDelta(double current, double start,
                         double& output) noexcept {
  const double delta = current - start;
  if (!std::isfinite(delta)) {
    return false;
  }
  output = std::round(delta / kOpeningSnapCells) * kOpeningSnapCells;
  return std::isfinite(output);
}

CreativeEditorWorldLayoutOpeningTarget openingTargetAt(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutPoint point, double toleranceCells,
    bool includeResizeHandles) noexcept {
  if (!openingOnActiveLevel(state, openingIndex) || !finitePoint(point) ||
      !std::isfinite(toleranceCells) || toleranceCells <= 0.0) {
    return {};
  }
  const cr::CreativeWorldLayoutOpening& opening =
      state.source.openings[openingIndex];
  const CreativeEditorWorldLayoutOpeningHost host =
      openingHost(state.source, opening);
  if (!host.valid) {
    return {};
  }
  const double halfWidth = opening.widthCells * 0.5;
  const CreativeEditorWorldLayoutPoint center =
      openingHostPoint(host, opening.centerOffsetCells);
  const CreativeEditorWorldLayoutPoint start =
      openingHostPoint(host, opening.centerOffsetCells - halfWidth);
  const CreativeEditorWorldLayoutPoint end =
      openingHostPoint(host, opening.centerOffsetCells + halfWidth);
  if (includeResizeHandles) {
    const double centerDistance = pointDistance(point, center);
    const double startDistance = pointDistance(point, start);
    const double endDistance = pointDistance(point, end);
    double nearestDistance = centerDistance;
    CreativeEditorWorldLayoutOpeningHandle nearestHandle =
        CreativeEditorWorldLayoutOpeningHandle::Move;
    if (startDistance < nearestDistance) {
      nearestDistance = startDistance;
      nearestHandle = CreativeEditorWorldLayoutOpeningHandle::Start;
    }
    if (endDistance < nearestDistance) {
      nearestDistance = endDistance;
      nearestHandle = CreativeEditorWorldLayoutOpeningHandle::End;
    }
    if (nearestDistance <= toleranceCells) {
      return {openingIndex, nearestHandle};
    }
  }
  const double pointOffset = openingHostOffset(host, point);
  if (!std::isfinite(pointOffset)) {
    return {};
  }
  const double boundedOffset =
      std::clamp(pointOffset, opening.centerOffsetCells - halfWidth,
                 opening.centerOffsetCells + halfWidth);
  if (pointDistance(point, openingHostPoint(host, boundedOffset)) <=
      toleranceCells) {
    return {openingIndex, CreativeEditorWorldLayoutOpeningHandle::Move};
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt addRoomPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = "drag room to its opposite corner";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  const cr::CreativeWorldLayoutRect rect = normalizedRect(state.anchor, point);
  state.anchorActive = false;
  const CreativeEditorWorldLayoutRoomSettings settings{
      rect, 0.0, cr::kDefaultCreativeWorldLayoutWallHeightCells,
      cr::kDefaultCreativeWorldLayoutWallThicknessCells, 1U};
  if (state.source.buildings.empty()) {
    return createCreativeEditorWorldLayoutBuildingShell(state, settings);
  }
  std::size_t buildingIndex = creativeEditorWorldLayoutSelectedBuilding(state);
  if (buildingIndex == cr::kInvalidCreativeWorldLayoutIndex &&
      state.source.buildings.size() == 1U) {
    buildingIndex = 0U;
  }
  std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  if (state.activeLevelIndex < state.source.levels.size() &&
      state.source.levels[state.activeLevelIndex].buildingIndex ==
          buildingIndex) {
    levelIndex = state.activeLevelIndex;
  } else {
    for (std::size_t index = 0U; index < state.source.levels.size(); ++index) {
      if (state.source.levels[index].buildingIndex != buildingIndex) {
        continue;
      }
      if (levelIndex != cr::kInvalidCreativeWorldLayoutIndex) {
        levelIndex = cr::kInvalidCreativeWorldLayoutIndex;
        break;
      }
      levelIndex = index;
    }
  }
  return createCreativeEditorWorldLayoutRoom(state, levelIndex, settings);
}

CreativeEditorWorldLayoutEditReceipt addBuildingShellPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = "drag building shell to its opposite corner";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  const cr::CreativeWorldLayoutRect rect = normalizedRect(state.anchor, point);
  state.anchorActive = false;
  return createCreativeEditorWorldLayoutBuildingShell(
      state,
      {rect, 0.0, cr::kDefaultCreativeWorldLayoutWallHeightCells,
       cr::kDefaultCreativeWorldLayoutWallThicknessCells, 1U});
}

CreativeEditorWorldLayoutEditReceipt addFloorPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = "floor start set; choose opposite corner";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  const cr::CreativeWorldLayoutRect rect = normalizedRect(state.anchor, point);
  state.anchorActive = false;
  if (rect.minimum.x == rect.maximum.x || rect.minimum.z == rect.maximum.z) {
    state.statusMessage = "floor needs width and depth";
    return {false, false, "creative_editor_world_layout_floor_degenerate"};
  }
  cr::CreativeWorldLayoutBox box;
  box.buildingIndex = ensurePrimaryBuilding(state);
  box.kind = cr::CreativeObjectKind::Floor;
  box.stableKey = mintWorldLayoutStableKey(state, "floor");
  box.name = "Floor " + std::to_string(state.source.boxes.size() + 1U);
  box.footprint = rect;
  box.anchorLayer = 0.0;
  box.layerCount = 1U;
  state.source.boxes.push_back(std::move(box));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Box,
                     state.source.boxes.size() - 1U};
  noteWorldLayoutSourceChange(state, "floor added");
  return {true, true, "creative_editor_world_layout_floor_added"};
}

CreativeEditorWorldLayoutEditReceipt addWallPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = "wall start set; choose end";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  point = cardinalEnd(state.anchor, point);
  state.anchorActive = false;
  if (point == state.anchor) {
    state.statusMessage = "wall needs length";
    return {false, false, "creative_editor_world_layout_wall_degenerate"};
  }
  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = ensurePrimaryBuilding(state);
  wall.stableKey = mintWorldLayoutStableKey(state, "wall");
  wall.name = "Wall " + std::to_string(state.source.walls.size() + 1U);
  wall.start = state.anchor;
  wall.end = point;
  wall.baseLayer = 0;
  state.source.walls.push_back(std::move(wall));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Wall,
                     state.source.walls.size() - 1U};
  noteWorldLayoutSourceChange(state, "wall added");
  return {true, true, "creative_editor_world_layout_wall_added"};
}

CreativeEditorWorldLayoutEditReceipt addOpening(
    CreativeEditorWorldLayoutState& state, CreativeEditorWorldLayoutPoint point,
    cr::CreativeBuildingOpeningKind kind) {
  const OpeningHostProjection projection =
      nearestOpeningHost(state.source, point, kOpeningHitToleranceCells,
                         state.activeLevelIndex);
  if (!projection.hit) {
    state.statusMessage = "place the opening on a room edge or partition";
    return {false, false, "creative_editor_world_layout_wall_not_found"};
  }
  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = projection.hostKind;
  opening.wallIndex = projection.wallIndex;
  opening.roomIndex = projection.roomIndex;
  opening.roomEdge = projection.roomEdge;
  opening.kind = kind;
  opening.pose = cr::CreativeBuildingOpeningPose::Closed;
  opening.includeInsert = true;
  if (kind == cr::CreativeBuildingOpeningKind::Door) {
    opening.widthCells = 1.0;
    opening.cutoutBottomCells = 0.0;
    opening.cutoutHeightCells = 2.1;
    opening.insertBottomCells = 0.0;
    opening.insertHeightCells = 2.1;
    opening.insertWidthCells = 1.0;
    opening.insertThicknessCells = 0.15;
  } else {
    opening.widthCells = 1.5;
    opening.cutoutBottomCells = 1.0;
    opening.cutoutHeightCells = 1.2;
    opening.insertBottomCells = 1.0;
    opening.insertHeightCells = 1.2;
    opening.insertWidthCells = 1.5;
    opening.insertThicknessCells = 0.10;
  }
  const double wallLength = projection.lengthCells;
  const double halfWidth = opening.widthCells * 0.5;
  const double minimumCenter = halfWidth + kOpeningEndClearanceCells;
  const double maximumCenter =
      wallLength - halfWidth - kOpeningEndClearanceCells;
  if (minimumCenter > maximumCenter + kOpeningGeometryEpsilon) {
    state.statusMessage = "wall is too short for this opening";
    return {false, false, "creative_editor_world_layout_wall_too_short"};
  }
  opening.centerOffsetCells =
      std::clamp(std::round(projection.centerOffsetCells * 4.0) / 4.0,
                 minimumCenter, maximumCenter);
  if (kind == cr::CreativeBuildingOpeningKind::Window &&
      projection.hostKind ==
          cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
      cr::creativeWorldLayoutRoomEdgeIntervalIsShared(
          state.source, projection.roomIndex, projection.roomEdge,
          opening.centerOffsetCells, opening.widthCells)) {
    state.statusMessage = "place windows on an exterior room edge";
    return {false, false,
            "creative_editor_world_layout_window_requires_exterior"};
  }
  cr::CreativeWorldLayout candidate = state.source;
  candidate.openings.push_back(opening);
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(candidate);
  if (!expanded.accepted) {
    state.statusMessage = expanded.reasonCode;
    return {false, false, expanded.reasonCode};
  }
  const cr::CreativeWorldLayoutOpening& resolved =
      expanded.expanded.openings.back();
  for (std::size_t index = 0U; index + 1U < expanded.expanded.openings.size();
       ++index) {
    const cr::CreativeWorldLayoutOpening& existing =
        expanded.expanded.openings[index];
    if (existing.wallIndex == resolved.wallIndex &&
        std::fabs(existing.centerOffsetCells - resolved.centerOffsetCells) <=
            (existing.widthCells + resolved.widthCells) * 0.5 +
                kOpeningGeometryEpsilon) {
      state.statusMessage = "opening overlaps an existing opening";
      return {false, false, "creative_editor_world_layout_opening_overlap"};
    }
  }
  opening.stableKey = mintWorldLayoutStableKey(
      state, kind == cr::CreativeBuildingOpeningKind::Door ? "door" : "window");
  opening.name =
      (kind == cr::CreativeBuildingOpeningKind::Door ? "Door " : "Window ") +
      std::to_string(state.source.openings.size() + 1U);
  state.source.openings.push_back(std::move(opening));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                     state.source.openings.size() - 1U};
  noteWorldLayoutSourceChange(
      state, kind == cr::CreativeBuildingOpeningKind::Door ? "door added"
                                                           : "window added");
  return {true, true, "creative_editor_world_layout_opening_added"};
}

struct ContainingRoomResult {
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
  bool ambiguous = false;
};

ContainingRoomResult findContainingRoom(
    const cr::CreativeWorldLayout& layout, std::size_t levelIndex,
    cr::CreativeWorldLayoutRect footprint,
    std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex) {
  ContainingRoomResult result;
  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    const cr::CreativeWorldLayoutRoom& room = layout.rooms[index];
    if (room.levelIndex != levelIndex ||
        (buildingIndex < layout.buildings.size() &&
         room.buildingIndex != buildingIndex) ||
        footprint.minimum.x < room.footprint.minimum.x ||
        footprint.maximum.x > room.footprint.maximum.x ||
        footprint.minimum.z < room.footprint.minimum.z ||
        footprint.maximum.z > room.footprint.maximum.z) {
      continue;
    }
    if (result.index != cr::kInvalidCreativeWorldLayoutIndex) {
      result.ambiguous = true;
      return result;
    }
    result.index = index;
  }
  return result;
}

std::size_t nextHigherLevel(const cr::CreativeWorldLayout& layout,
                            std::size_t lowerLevelIndex,
                            std::size_t buildingIndex) noexcept {
  if (lowerLevelIndex >= layout.levels.size()) {
    return cr::kInvalidCreativeWorldLayoutIndex;
  }
  const double lowerElevation = layout.levels[lowerLevelIndex].floorTopLayer;
  std::size_t result = cr::kInvalidCreativeWorldLayoutIndex;
  double resultElevation = std::numeric_limits<double>::infinity();
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    const cr::CreativeWorldLayoutLevel& level = layout.levels[index];
    if (level.buildingIndex != buildingIndex ||
        !std::isfinite(level.floorTopLayer) ||
        level.floorTopLayer <= lowerElevation ||
        level.floorTopLayer >= resultElevation) {
      continue;
    }
    result = index;
    resultElevation = level.floorTopLayer;
  }
  return result;
}

cr::CreativeWorldLayoutVerticalDirection
verticalDirection(cr::CreativeTerrainCoord2 start,
                  cr::CreativeTerrainCoord2 end) noexcept {
  const std::int64_t deltaX = static_cast<std::int64_t>(end.x) - start.x;
  const std::int64_t deltaZ = static_cast<std::int64_t>(end.z) - start.z;
  if (std::llabs(deltaX) >= std::llabs(deltaZ)) {
    return deltaX >= 0 ? cr::CreativeWorldLayoutVerticalDirection::PositiveX
                       : cr::CreativeWorldLayoutVerticalDirection::NegativeX;
  }
  return deltaZ >= 0 ? cr::CreativeWorldLayoutVerticalDirection::PositiveZ
                     : cr::CreativeWorldLayoutVerticalDirection::NegativeZ;
}

CreativeEditorWorldLayoutEditReceipt
addVerticalConnectorPoint(CreativeEditorWorldLayoutState& state,
                          cr::CreativeTerrainCoord2 point,
                          const VerticalConnectorToolSpec& spec) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage =
        verticalConnectorMessage(spec, " low end set; choose high end");
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }

  const cr::CreativeTerrainCoord2 start = state.anchor;
  const cr::CreativeWorldLayoutRect footprint = normalizedRect(start, point);
  state.anchorActive = false;
  if (footprint.minimum.x >= footprint.maximum.x ||
      footprint.minimum.z >= footprint.maximum.z) {
    state.statusMessage =
        verticalConnectorMessage(spec, " needs both run and width");
    return {false, false, verticalConnectorReasonCode(spec, "_degenerate")};
  }
  if (state.activeLevelIndex >= state.source.levels.size()) {
    state.statusMessage = verticalConnectorMessage(
        spec, " needs a selected lower building level");
    return {false, false,
            verticalConnectorReasonCode(spec, "_lower_level_missing")};
  }

  const ContainingRoomResult lower =
      findContainingRoom(state.source, state.activeLevelIndex, footprint);
  if (lower.ambiguous || lower.index == cr::kInvalidCreativeWorldLayoutIndex) {
    state.statusMessage =
        lower.ambiguous
            ? verticalConnectorMessage(spec,
                                       " footprint crosses room ownership")
            : verticalConnectorMessage(spec, " must fit inside one lower room");
    return {false, false,
            verticalConnectorReasonCode(spec, "_lower_room_invalid")};
  }
  const std::size_t buildingIndex =
      state.source.rooms[lower.index].buildingIndex;
  const std::size_t upperLevelIndex =
      nextHigherLevel(state.source, state.activeLevelIndex, buildingIndex);
  if (upperLevelIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    state.statusMessage = verticalConnectorMessage(
        spec, " needs an adjacent upper building level");
    return {false, false,
            verticalConnectorReasonCode(spec, "_upper_level_missing")};
  }
  const ContainingRoomResult upper = findContainingRoom(
      state.source, upperLevelIndex, footprint, buildingIndex);
  if (upper.ambiguous || upper.index == cr::kInvalidCreativeWorldLayoutIndex) {
    state.statusMessage =
        upper.ambiguous
            ? verticalConnectorMessage(
                  spec, " footprint crosses upper room ownership")
            : verticalConnectorMessage(spec, " must fit inside one upper room");
    return {false, false,
            verticalConnectorReasonCode(spec, "_upper_room_invalid")};
  }

  cr::CreativeWorldLayoutVerticalConnector connector;
  connector.buildingIndex = buildingIndex;
  connector.lowerRoomIndex = lower.index;
  connector.upperRoomIndex = upper.index;
  connector.kind = spec.kind;
  connector.direction = verticalDirection(start, point);
  connector.stableKey = "pending_" + std::string(spec.keyPrefix);
  connector.name = std::string(spec.label) + " " +
                   std::to_string(state.source.verticalConnectors.size() + 1U);
  connector.footprint = footprint;
  state.source.verticalConnectors.push_back(std::move(connector));
  const std::size_t connectorIndex =
      state.source.verticalConnectors.size() - 1U;
  const cr::CreativeWorldLayoutVerticalConnectorPlan plan =
      cr::planCreativeWorldLayoutVerticalConnector({}, state.source,
                                                   connectorIndex);
  if (!plan.accepted) {
    state.source.verticalConnectors.pop_back();
    state.statusMessage = std::string(plan.reasonCode);
    return {false, false, std::string(plan.reasonCode)};
  }
  state.source.verticalConnectors.back().stableKey =
      mintWorldLayoutStableKey(state, spec.keyPrefix);
  state.selection = {CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
                     connectorIndex};
  noteWorldLayoutSourceChange(state, verticalConnectorMessage(spec, " added"));
  return {true, true, verticalConnectorReasonCode(spec, "_added")};
}

CreativeEditorWorldLayoutEditReceipt addPlateau(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = mintWorldLayoutStableKey(state, "plateau");
  profile.kind = cr::CreativeTerrainRecipeKind::Plateau;
  profile.center = point;
  profile.baseHeightCells = 4U;
  profile.radiusCells = 8U;
  profile.amplitudeCells = 1U;
  profile.spacingCells = 4U;
  profile.blend = cr::CreativeTerrainProfileBlend::Set;
  profile.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Fill;
  state.source.terrainProfiles.push_back(std::move(profile));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::TerrainProfile,
                     state.source.terrainProfiles.size() - 1U};
  noteWorldLayoutSourceChange(state, "plateau added");
  return {true, true, "creative_editor_world_layout_plateau_added"};
}

CreativeEditorWorldLayoutEditReceipt addTerrainPathPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point,
    cr::CreativeTerrainRecipeKind kind) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = kind == cr::CreativeTerrainRecipeKind::Road
                              ? "road start set; choose end"
                              : "ditch start set; choose end";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  const cr::CreativeTerrainCoord2 start = state.anchor;
  state.anchorActive = false;
  if (start == point) {
    state.statusMessage = "terrain path needs length";
    return {false, false, "creative_editor_world_layout_path_degenerate"};
  }
  const std::uint16_t height =
      kind == cr::CreativeTerrainRecipeKind::Ditch ? 2U : 1U;
  cr::CreativeWorldLayoutTerrainPath path;
  path.stableKey = mintWorldLayoutStableKey(
      state, kind == cr::CreativeTerrainRecipeKind::Road ? "road" : "ditch");
  path.kind = kind;
  path.firstPointIndex = state.source.terrainPathPoints.size();
  path.pointCount = 2U;
  path.elevation = cr::CreativeTerrainPathElevation::Level;
  path.halfWidthCells = 1U;
  path.amplitudeCells = 1U;
  path.paintSurface = true;
  path.material = cr::CreativeTerrainMaterial::Count;
  state.source.terrainPathPoints.push_back({start, height});
  state.source.terrainPathPoints.push_back({point, height});
  state.source.terrainPaths.push_back(std::move(path));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::TerrainPath,
                     state.source.terrainPaths.size() - 1U};
  noteWorldLayoutSourceChange(
      state, kind == cr::CreativeTerrainRecipeKind::Road ? "road added"
                                                         : "ditch added");
  return {true, true, "creative_editor_world_layout_path_added"};
}

CreativeEditorWorldLayoutEditReceipt addBridgePoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = "bridge start set; choose opposite corner";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  const cr::CreativeWorldLayoutRect footprint =
      normalizedRect(state.anchor, point);
  state.anchorActive = false;
  if (footprint.minimum == footprint.maximum ||
      footprint.minimum.x == footprint.maximum.x ||
      footprint.minimum.z == footprint.maximum.z) {
    state.statusMessage = "bridge needs width and length";
    return {false, false, "creative_editor_world_layout_bridge_degenerate"};
  }
  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Bridge;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
  object.stableKey = mintWorldLayoutStableKey(state, "bridge");
  object.name = "Bridge " + std::to_string(state.source.objects.size() + 1U);
  object.boundsCells = {
      {static_cast<double>(footprint.minimum.x), 0.0,
       static_cast<double>(footprint.minimum.z)},
      {static_cast<double>(footprint.maximum.x), 0.35,
       static_cast<double>(footprint.maximum.z)},
  };
  object.tags = {"world_layout:object"};
  state.source.objects.push_back(std::move(object));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Object,
                     state.source.objects.size() - 1U};
  noteWorldLayoutSourceChange(state, "bridge added");
  return {true, true, "creative_editor_world_layout_bridge_added"};
}

CreativeEditorWorldLayoutEditReceipt addPointObject(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point,
    CreativeEditorWorldLayoutTool tool) {
  cr::CreativeWorldLayoutObject object;
  object.stableKey = mintWorldLayoutStableKey(
      state, tool == CreativeEditorWorldLayoutTool::Boulder
                 ? "boulder"
                 : tool == CreativeEditorWorldLayoutTool::PlayerSpawn
                       ? "player_spawn"
                       : "npc_spawn");
  object.tags = {"world_layout:object"};
  if (tool == CreativeEditorWorldLayoutTool::Boulder) {
    object.kind = cr::CreativeObjectKind::Rock;
    object.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
    object.name = "Boulder " +
                  std::to_string(state.source.objects.size() + 1U);
    object.assetId = "boulder_01";
    object.boundsCells = {
        {static_cast<double>(point.x) - 1.25, 0.0,
         static_cast<double>(point.z) - 1.25},
        {static_cast<double>(point.x) + 1.25, 2.0,
         static_cast<double>(point.z) + 1.25},
    };
  } else {
    object.kind = tool == CreativeEditorWorldLayoutTool::PlayerSpawn
                      ? cr::CreativeObjectKind::SpawnPoint
                      : cr::CreativeObjectKind::NpcSpawn;
    object.mode = cr::CreativeObjectLibraryPlacementMode::Point;
    object.name = tool == CreativeEditorWorldLayoutTool::PlayerSpawn
                      ? "Player Spawn"
                      : "NPC Spawn";
    object.pointCells = {static_cast<double>(point.x), 0.0,
                         static_cast<double>(point.z)};
  }
  state.source.objects.push_back(std::move(object));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Object,
                     state.source.objects.size() - 1U};
  noteWorldLayoutSourceChange(
      state, tool == CreativeEditorWorldLayoutTool::Boulder
                 ? "boulder added"
                 : tool == CreativeEditorWorldLayoutTool::PlayerSpawn
                       ? "player spawn added"
                       : "NPC spawn added");
  return {true, true, "creative_editor_world_layout_object_added"};
}

}  // namespace

const char* creativeEditorWorldLayoutToolLabel(
    CreativeEditorWorldLayoutTool tool) noexcept {
  switch (tool) {
    case CreativeEditorWorldLayoutTool::Select:
      return "Select";
    case CreativeEditorWorldLayoutTool::BuildingShell:
      return "Building Shell";
    case CreativeEditorWorldLayoutTool::Room:
      return "Room";
    case CreativeEditorWorldLayoutTool::Floor:
      return "Floor";
    case CreativeEditorWorldLayoutTool::Wall:
      return "Partition";
    case CreativeEditorWorldLayoutTool::Door:
      return "Door";
    case CreativeEditorWorldLayoutTool::Window:
      return "Window";
    case CreativeEditorWorldLayoutTool::Stair:
      return "Stair";
    case CreativeEditorWorldLayoutTool::Ramp:
      return "Ramp";
    case CreativeEditorWorldLayoutTool::Plateau:
      return "Plateau";
    case CreativeEditorWorldLayoutTool::Road:
      return "Road";
    case CreativeEditorWorldLayoutTool::Ditch:
      return "Ditch";
    case CreativeEditorWorldLayoutTool::Bridge:
      return "Bridge";
    case CreativeEditorWorldLayoutTool::Boulder:
      return "Boulder";
    case CreativeEditorWorldLayoutTool::PlayerSpawn:
      return "Player Spawn";
    case CreativeEditorWorldLayoutTool::NpcSpawn:
      return "NPC Spawn";
    case CreativeEditorWorldLayoutTool::Count:
      break;
  }
  return "Unknown";
}

bool creativeEditorWorldLayoutToolIsVerticalConnector(
    CreativeEditorWorldLayoutTool tool) noexcept {
  return verticalConnectorToolSpec(tool) != nullptr;
}

const char* creativeEditorWorldLayoutPaletteCategoryLabel(
    CreativeEditorWorldLayoutPaletteCategory category) noexcept {
  switch (category) {
    case CreativeEditorWorldLayoutPaletteCategory::Structure:
      return "Structures";
    case CreativeEditorWorldLayoutPaletteCategory::Terrain:
      return "Terrain";
    case CreativeEditorWorldLayoutPaletteCategory::Object:
      return "Objects";
    case CreativeEditorWorldLayoutPaletteCategory::Gameplay:
      return "Gameplay";
    case CreativeEditorWorldLayoutPaletteCategory::Count:
      break;
  }
  return "Unknown";
}

std::span<const CreativeEditorWorldLayoutPaletteEntry>
creativeEditorWorldLayoutPaletteEntries() noexcept {
  return kWorldLayoutPaletteEntries;
}

void resetCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                    std::string layoutKey) {
  CreativeEditorWorldLayoutBuildingTemplateLibrary buildingTemplates =
      std::move(state.buildingTemplates);
  state = {};
  state.buildingTemplates = std::move(buildingTemplates);
  state.source.stableKey =
      layoutKey.empty() ? "world_layout" : std::move(layoutKey);
  state.generatedBaseline = {state.source, state.revision, state.savedRevision,
                             state.generatedRevision,
                             state.nextStableOrdinal};
  state.statusMessage = "blank layout";
}

void installCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                      cr::CreativeWorldLayout layout) {
  CreativeEditorWorldLayoutBuildingTemplateLibrary buildingTemplates =
      std::move(state.buildingTemplates);
  state = {};
  state.buildingTemplates = std::move(buildingTemplates);
  state.source = std::move(layout);
  state.nextStableOrdinal =
      1U + state.source.buildings.size() + state.source.levels.size() +
      state.source.rooms.size() + state.source.verticalConnectors.size() +
      state.source.boxes.size() + state.source.walls.size() +
      state.source.openings.size() + state.source.objects.size() +
      state.source.terrainProfiles.size() + state.source.terrainPaths.size();
  state.generatedRevision = state.revision;
  repairCreativeEditorWorldLayoutActiveLevel(state);
  state.generatedBaseline = {state.source, state.revision, state.savedRevision,
                             state.generatedRevision,
                             state.nextStableOrdinal};
  state.statusMessage = "layout loaded";
}

void markCreativeEditorWorldLayoutSaved(
    CreativeEditorWorldLayoutState& state) noexcept {
  state.savedRevision = state.revision;
  if (state.generatedRevision == state.revision) {
    state.generatedBaseline.savedRevision = state.savedRevision;
  }
}

namespace {

[[nodiscard]] bool applyWorldLayoutSourceSelection(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutObjectProvenance provenance) {
  CreativeEditorWorldLayoutSelectionKind kind =
      CreativeEditorWorldLayoutSelectionKind::None;
  switch (provenance.table) {
    case cr::CreativeWorldLayoutTable::Building:
      kind = CreativeEditorWorldLayoutSelectionKind::Building;
      break;
    case cr::CreativeWorldLayoutTable::Level:
      return false;
    case cr::CreativeWorldLayoutTable::Room:
      kind = CreativeEditorWorldLayoutSelectionKind::Room;
      break;
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      kind = CreativeEditorWorldLayoutSelectionKind::VerticalConnector;
      break;
    case cr::CreativeWorldLayoutTable::Box:
      kind = CreativeEditorWorldLayoutSelectionKind::Box;
      break;
    case cr::CreativeWorldLayoutTable::Wall:
      kind = CreativeEditorWorldLayoutSelectionKind::Wall;
      break;
    case cr::CreativeWorldLayoutTable::Opening:
      kind = CreativeEditorWorldLayoutSelectionKind::Opening;
      break;
    case cr::CreativeWorldLayoutTable::Object:
      kind = CreativeEditorWorldLayoutSelectionKind::Object;
      break;
    case cr::CreativeWorldLayoutTable::None:
    case cr::CreativeWorldLayoutTable::TerrainProfile:
    case cr::CreativeWorldLayoutTable::TerrainPath:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
      return false;
  }
  if (!provenance.owned ||
      provenance.index == cr::kInvalidCreativeWorldLayoutIndex) {
    return false;
  }
  state.selection = {kind, provenance.index};
  if (kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      provenance.index < state.source.rooms.size()) {
    state.activeLevelIndex =
        state.source.rooms[provenance.index].levelIndex;
  } else if (kind == CreativeEditorWorldLayoutSelectionKind::Building) {
    repairCreativeEditorWorldLayoutActiveLevel(state, provenance.index);
  } else if (kind ==
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector &&
             provenance.index < state.source.verticalConnectors.size()) {
    const cr::CreativeWorldLayoutVerticalConnector& connector =
        state.source.verticalConnectors[provenance.index];
    if (connector.lowerRoomIndex < state.source.rooms.size()) {
      state.activeLevelIndex =
          state.source.rooms[connector.lowerRoomIndex].levelIndex;
    }
  } else if (kind == CreativeEditorWorldLayoutSelectionKind::Opening &&
             provenance.index < state.source.openings.size()) {
    const cr::CreativeWorldLayoutOpening& opening =
        state.source.openings[provenance.index];
    if (opening.hostKind ==
            cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        opening.roomIndex < state.source.rooms.size()) {
      state.activeLevelIndex =
          state.source.rooms[opening.roomIndex].levelIndex;
    }
  }
  state.statusMessage = provenance.contributorCount > 1U
                            ? "condensed generated wall selected"
                            : "generated layout source selected";
  return true;
}

}  // namespace

bool selectCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object) {
  if (state.generatedRevision != state.revision) {
    return false;
  }
  const cr::CreativeWorldLayoutObjectProvenance provenance =
      cr::resolveCreativeWorldLayoutObjectProvenance(state.source, object);
  return applyWorldLayoutSourceSelection(state, provenance);
}

bool selectCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object,
    cr::CreativeGridSettings grid,
    cr::CreativeVec3 worldPoint) {
  if (state.generatedRevision != state.revision ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0 ||
      !std::isfinite(grid.origin.x) || !std::isfinite(grid.origin.y) ||
      !std::isfinite(grid.origin.z) ||
      !std::isfinite(worldPoint.x) || !std::isfinite(worldPoint.y) ||
      !std::isfinite(worldPoint.z)) {
    return false;
  }
  const cr::CreativeVec3 sourcePointCells{
      (worldPoint.x - grid.origin.x) / grid.cellSizeMeters,
      (worldPoint.y - grid.origin.y) / grid.cellSizeMeters,
      (worldPoint.z - grid.origin.z) / grid.cellSizeMeters,
  };
  const cr::CreativeWorldLayoutObjectProvenance provenance =
      cr::resolveCreativeWorldLayoutObjectProvenance(
          state.source, object, sourcePointCells);
  return applyWorldLayoutSourceSelection(state, provenance);
}

bool creativeEditorWorldLayoutDirty(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.revision != state.savedRevision;
}

bool creativeEditorWorldLayoutPreviewActive(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.previewVisible && state.preview.accepted &&
         state.preview.document.isValid() &&
         state.previewLayoutRevision == state.revision;
}

const cr::CreativeDocument& creativeEditorWorldLayoutRenderDocument(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& liveDocument) noexcept {
  return creativeEditorWorldLayoutPreviewActive(state) ? state.preview.document
                                                       : liveDocument;
}

const cr::CreativeWorldLayout& creativeEditorWorldLayoutDisplaySource(
    const CreativeEditorWorldLayoutState& state) noexcept {
  if (state.buildingTemplatePlacement.active &&
      state.buildingTemplatePlacement.previewValid &&
      state.buildingTemplatePlacement.sourceRevision == state.revision &&
      state.buildingTemplatePlacement.resultBuildingIndex <
          state.buildingTemplatePlacement.candidate.buildings.size()) {
    return state.buildingTemplatePlacement.candidate;
  }
  return state.buildingTransform.active &&
                 state.buildingTransform.sourceRevision == state.revision &&
                 state.buildingTransform.buildingIndex <
                     state.buildingTransform.candidate.buildings.size()
             ? state.buildingTransform.candidate
             : state.source;
}

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutTool(
    CreativeEditorWorldLayoutState& state, CreativeEditorWorldLayoutTool tool) {
  if (tool >= CreativeEditorWorldLayoutTool::Count) {
    return {false, false, "creative_editor_world_layout_tool_invalid"};
  }
  const bool changed = state.tool != tool || state.anchorActive ||
                       state.roomManipulation.active ||
                       state.boxManipulation.active ||
                       state.wallManipulation.active ||
                       state.buildingManipulation.active ||
                       state.buildingTransform.active ||
                       state.buildingTemplatePlacement.active ||
                       state.openingManipulation.active ||
                       state.boxSettingsDraft.active ||
                       state.wallSettingsDraft.active ||
                       state.openingSettingsDraft.active;
  state.tool = tool;
  state.anchorActive = false;
  clearWorldLayoutInteraction(state);
  state.statusMessage =
      std::string(creativeEditorWorldLayoutToolLabel(tool)) + " tool";
  return {true, changed, "creative_editor_world_layout_tool_set"};
}

CreativeEditorWorldLayoutEditReceipt applyCreativeEditorWorldLayoutPoint(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point) {
  if (!finitePoint(point)) {
    return {false, false, "creative_editor_world_layout_point_non_finite"};
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Select) {
    return selectAt(state, point);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Door) {
    return addOpening(state, point, cr::CreativeBuildingOpeningKind::Door);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Window) {
    return addOpening(state, point, cr::CreativeBuildingOpeningKind::Window);
  }
  cr::CreativeTerrainCoord2 gridPoint;
  if (!toGridCoord(point, gridPoint)) {
    return {false, false, "creative_editor_world_layout_point_out_of_range"};
  }
  if (state.tool == CreativeEditorWorldLayoutTool::BuildingShell) {
    return addBuildingShellPoint(state, gridPoint);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Room) {
    return addRoomPoint(state, gridPoint);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Floor) {
    return addFloorPoint(state, gridPoint);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Wall) {
    return addWallPoint(state, gridPoint);
  }
  if (const VerticalConnectorToolSpec* connector =
          verticalConnectorToolSpec(state.tool)) {
    return addVerticalConnectorPoint(state, gridPoint, *connector);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Plateau) {
    return addPlateau(state, gridPoint);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Road) {
    return addTerrainPathPoint(state, gridPoint,
                               cr::CreativeTerrainRecipeKind::Road);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Ditch) {
    return addTerrainPathPoint(state, gridPoint,
                               cr::CreativeTerrainRecipeKind::Ditch);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Bridge) {
    return addBridgePoint(state, gridPoint);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Boulder ||
      state.tool == CreativeEditorWorldLayoutTool::PlayerSpawn ||
      state.tool == CreativeEditorWorldLayoutTool::NpcSpawn) {
    return addPointObject(state, gridPoint, state.tool);
  }
  return {false, false, "creative_editor_world_layout_tool_invalid"};
}

CreativeEditorWorldLayoutEditReceipt applyCreativeEditorWorldLayoutGesture(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutGesturePhase phase,
    CreativeEditorWorldLayoutPoint point) {
  if (phase >= CreativeEditorWorldLayoutGesturePhase::Count) {
    return {false, false, "creative_editor_world_layout_gesture_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutGesturePhase::Cancel) {
    state.anchorActive = false;
    state.statusMessage = "layout gesture cancelled";
    return {true, false, "creative_editor_world_layout_gesture_cancelled"};
  }
  const bool dragTool =
      state.tool == CreativeEditorWorldLayoutTool::BuildingShell ||
      state.tool == CreativeEditorWorldLayoutTool::Room ||
      state.tool == CreativeEditorWorldLayoutTool::Floor ||
      state.tool == CreativeEditorWorldLayoutTool::Wall ||
      creativeEditorWorldLayoutToolIsVerticalConnector(state.tool) ||
      state.tool == CreativeEditorWorldLayoutTool::Road ||
      state.tool == CreativeEditorWorldLayoutTool::Ditch ||
      state.tool == CreativeEditorWorldLayoutTool::Bridge;
  if (!dragTool) {
    return {false, false, "creative_editor_world_layout_gesture_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutGesturePhase::Begin) {
    cr::CreativeTerrainCoord2 gridPoint;
    if (!toGridCoord(point, gridPoint)) {
      return {false, false, "creative_editor_world_layout_point_out_of_range"};
    }
    state.anchorActive = true;
    state.anchor = gridPoint;
    clearWorldLayoutInteraction(state);
    if (state.tool == CreativeEditorWorldLayoutTool::BuildingShell) {
      state.statusMessage = "drag building shell to its opposite corner";
    } else if (state.tool == CreativeEditorWorldLayoutTool::Room) {
      state.statusMessage = "drag room to its opposite corner";
    } else if (state.tool == CreativeEditorWorldLayoutTool::Floor) {
      state.statusMessage = "drag floor to its opposite corner";
    } else if (state.tool == CreativeEditorWorldLayoutTool::Wall) {
      state.statusMessage = "drag partition to its end";
    } else if (const VerticalConnectorToolSpec* connector =
                   verticalConnectorToolSpec(state.tool)) {
      state.statusMessage = verticalConnectorMessage(
          *connector,
          " drag starts at the low end and finishes at the high end");
    } else if (state.tool == CreativeEditorWorldLayoutTool::Road) {
      state.statusMessage = "drag road to its end";
    } else if (state.tool == CreativeEditorWorldLayoutTool::Ditch) {
      state.statusMessage = "drag ditch to its end";
    } else {
      state.statusMessage = "drag bridge to its opposite corner";
    }
    return {true, false, "creative_editor_world_layout_gesture_started"};
  }
  if (!state.anchorActive) {
    return {false, false,
            "creative_editor_world_layout_gesture_not_active"};
  }
  return applyCreativeEditorWorldLayoutPoint(state, point);
}

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutRoomSettings(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings settings) {
  const RoomSettingsValidation validation =
      validateRoomSettings(state, roomIndex, settings);
  if (!validation.accepted) {
    state.statusMessage = validation.message;
    return {false, false, validation.reasonCode};
  }
  cr::CreativeWorldLayoutRoom& room = state.source.rooms[roomIndex];
  cr::CreativeWorldLayoutLevel& level = state.source.levels[room.levelIndex];
  if (room.footprint.minimum == settings.footprint.minimum &&
      room.footprint.maximum == settings.footprint.maximum &&
      level.floorTopLayer == settings.floorTopLayer &&
      level.wallHeightCells == settings.wallHeightCells &&
      room.wallThicknessCells == settings.wallThicknessCells &&
      level.floorThicknessLayers == settings.floorThicknessLayers) {
    return {true, false,
            "creative_editor_world_layout_room_settings_no_change"};
  }
  room.footprint = settings.footprint;
  room.wallThicknessCells = settings.wallThicknessCells;
  level.floorTopLayer = settings.floorTopLayer;
  level.wallHeightCells = settings.wallHeightCells;
  level.floorThicknessLayers = settings.floorThicknessLayers;
  state.activeLevelIndex = room.levelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Room, roomIndex};
  noteWorldLayoutSourceChange(state, "room shell settings updated");
  return {true, true, "creative_editor_world_layout_room_settings_updated"};
}

CreativeEditorWorldLayoutRoomTarget findCreativeEditorWorldLayoutRoomTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!finitePoint(point) || !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0) {
    return {};
  }
  const CreativeEditorWorldLayoutSelection hit =
      hitTest(state.source, point, state.activeLevelIndex);
  if (hit.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      hit.index < state.source.rooms.size()) {
    const CreativeEditorWorldLayoutRoomHandle handle = worldLayoutRectHandleAt(
        state.source.rooms[hit.index].footprint, point, toleranceCells);
    if (handle != CreativeEditorWorldLayoutRoomHandle::None) {
      return {hit.index, handle};
    }
  }
  if (hit.kind != CreativeEditorWorldLayoutSelectionKind::None) {
    return {};
  }
  if (state.selection.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      roomOnActiveLevel(state, state.selection.index)) {
    const CreativeEditorWorldLayoutRoomHandle handle = worldLayoutRectHandleAt(
        state.source.rooms[state.selection.index].footprint, point,
        toleranceCells);
    if (handle != CreativeEditorWorldLayoutRoomHandle::None) {
      return {state.selection.index, handle};
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoomManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (phase >= CreativeEditorWorldLayoutRoomManipulationPhase::Count) {
    return {false, false,
            "creative_editor_world_layout_room_manipulation_phase_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutRoomManipulationPhase::Cancel) {
    const bool changed = state.roomManipulation.active;
    state.roomManipulation = {};
    state.statusMessage = "room manipulation cancelled";
    return {true, changed,
            "creative_editor_world_layout_room_manipulation_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {false, false,
            "creative_editor_world_layout_room_manipulation_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutRoomManipulationPhase::Begin) {
    const CreativeEditorWorldLayoutRoomTarget target =
        findCreativeEditorWorldLayoutRoomTarget(state, point, toleranceCells);
    if (target.handle == CreativeEditorWorldLayoutRoomHandle::None ||
        target.roomIndex >= state.source.rooms.size()) {
      return selectAt(state, point);
    }
    const cr::CreativeWorldLayoutRect footprint =
        state.source.rooms[target.roomIndex].footprint;
    clearWorldLayoutInteraction(state);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Room,
                       target.roomIndex};
    state.anchorActive = false;
    state.roomManipulation = {
        true,
        state.revision,
        target,
        point,
        footprint,
        footprint,
        true,
        "creative_editor_world_layout_room_manipulation_ready",
    };
    state.statusMessage =
        target.handle == CreativeEditorWorldLayoutRoomHandle::Move
            ? "drag to move room"
            : "drag to resize room";
    return {true, true,
            "creative_editor_world_layout_room_manipulation_started"};
  }
  if (!state.roomManipulation.active ||
      state.roomManipulation.target.roomIndex >= state.source.rooms.size()) {
    return {false, false,
            "creative_editor_world_layout_room_manipulation_not_active"};
  }
  const std::size_t roomIndex = state.roomManipulation.target.roomIndex;
  const cr::CreativeWorldLayoutRoom& room = state.source.rooms[roomIndex];
  if (state.revision != state.roomManipulation.sourceRevision ||
      !(room.footprint.minimum ==
            state.roomManipulation.originalFootprint.minimum &&
        room.footprint.maximum ==
            state.roomManipulation.originalFootprint.maximum)) {
    state.roomManipulation = {};
    state.statusMessage = "room changed while drag was active";
    return {false, false,
            "creative_editor_world_layout_room_manipulation_stale"};
  }
  if (phase == CreativeEditorWorldLayoutRoomManipulationPhase::Update) {
    cr::CreativeWorldLayoutRect footprint;
    const bool coordinateValid = worldLayoutManipulatedRect(
        state.roomManipulation, point, footprint);
    if (coordinateValid &&
        footprint.minimum == state.roomManipulation.previewFootprint.minimum &&
        footprint.maximum == state.roomManipulation.previewFootprint.maximum) {
      return {true, false, state.roomManipulation.reasonCode};
    }
    RoomSettingsValidation validation;
    if (coordinateValid) {
      validation = validateRoomSettings(
          state, roomIndex,
          roomSettings(state.source, roomIndex, footprint));
    } else {
      validation.reasonCode =
          "creative_editor_world_layout_room_manipulation_out_of_range";
      validation.message = "room drag exceeds the layout coordinate range";
    }
    state.roomManipulation.previewFootprint = footprint;
    state.roomManipulation.previewValid = validation.accepted;
    state.roomManipulation.reasonCode = validation.reasonCode;
    state.statusMessage = validation.accepted ? "room drag preview"
                                              : validation.message;
    return {true, true, validation.reasonCode};
  }

  const CreativeEditorWorldLayoutEditReceipt updated =
      applyCreativeEditorWorldLayoutRoomManipulation(
          state, CreativeEditorWorldLayoutRoomManipulationPhase::Update, point,
          toleranceCells);
  if (!updated.accepted) {
    return updated;
  }
  if (!state.roomManipulation.previewValid) {
    const std::string reasonCode = state.roomManipulation.reasonCode;
    state.roomManipulation = {};
    return {false, false, reasonCode};
  }
  const cr::CreativeWorldLayoutRect footprint =
      state.roomManipulation.previewFootprint;
  state.roomManipulation = {};
  return setCreativeEditorWorldLayoutRoomSettings(
      state, roomIndex, roomSettings(state.source, roomIndex, footprint));
}

bool readCreativeEditorWorldLayoutOpeningSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings& output) noexcept {
  if (openingIndex >= state.source.openings.size()) {
    return false;
  }
  output = openingSettings(state.source.openings[openingIndex]);
  return true;
}

CreativeEditorWorldLayoutOpeningHost
resolveCreativeEditorWorldLayoutOpeningHost(
    const CreativeEditorWorldLayoutState& state,
    std::size_t openingIndex) noexcept {
  const cr::CreativeWorldLayout &source =
      creativeEditorWorldLayoutDisplaySource(state);
  if (openingIndex >= source.openings.size()) {
    return {};
  }
  const cr::CreativeWorldLayoutOpening& opening = source.openings[openingIndex];
  CreativeEditorWorldLayoutOpeningHost host = openingHost(source, opening);
  if (state.wallManipulation.active &&
      opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
      opening.wallIndex == state.wallManipulation.target.wallIndex) {
    host.start = {
        static_cast<double>(state.wallManipulation.previewStart.x),
        static_cast<double>(state.wallManipulation.previewStart.z),
    };
    host.end = {
        static_cast<double>(state.wallManipulation.previewEnd.x),
        static_cast<double>(state.wallManipulation.previewEnd.z),
    };
  }
  std::size_t hostBuildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
      opening.wallIndex < source.walls.size()) {
    hostBuildingIndex = source.walls[opening.wallIndex].buildingIndex;
  } else if (opening.hostKind ==
                 cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
             opening.roomIndex < source.rooms.size()) {
    hostBuildingIndex = source.rooms[opening.roomIndex].buildingIndex;
  }
  if (state.buildingManipulation.active &&
      hostBuildingIndex == state.buildingManipulation.buildingIndex) {
    const double deltaX = static_cast<double>(
        state.buildingManipulation.previewDeltaXCells);
    const double deltaZ = static_cast<double>(
        state.buildingManipulation.previewDeltaZCells);
    host.start.x += deltaX;
    host.start.z += deltaZ;
    host.end.x += deltaX;
    host.end.z += deltaZ;
  }
  host.lengthCells =
      std::hypot(host.end.x - host.start.x, host.end.z - host.start.z);
  host.valid = std::isfinite(host.lengthCells) && host.lengthCells > 0.0;
  return host;
}

CreativeEditorWorldLayoutOpeningTarget
findCreativeEditorWorldLayoutOpeningTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!finitePoint(point) || !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0) {
    return {};
  }
  if (state.selection.kind ==
          CreativeEditorWorldLayoutSelectionKind::Opening &&
      state.selection.index < state.source.openings.size()) {
    const CreativeEditorWorldLayoutOpeningTarget selectedTarget =
        openingTargetAt(state, state.selection.index, point, toleranceCells,
                        true);
    if (selectedTarget.handle !=
        CreativeEditorWorldLayoutOpeningHandle::None) {
      return selectedTarget;
    }
  }
  for (std::size_t index = state.source.openings.size(); index > 0U; --index) {
    const CreativeEditorWorldLayoutOpeningTarget target = openingTargetAt(
        state, index - 1U, point, toleranceCells, false);
    if (target.handle != CreativeEditorWorldLayoutOpeningHandle::None) {
      return target;
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutOpeningSettings(
    CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings settings) {
  if (openingIndex >= state.source.openings.size()) {
    state.statusMessage = "opening selection is stale";
    return {false, false,
            "creative_editor_world_layout_opening_index_invalid"};
  }
  const cr::CreativeWorldLayoutOpening candidate = openingWithSettings(
      state.source.openings[openingIndex], settings);
  return commitOpeningCandidate(state, openingIndex, candidate,
                                "opening settings updated");
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutOpeningManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutOpeningManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (phase >= CreativeEditorWorldLayoutOpeningManipulationPhase::Count) {
    return {
        false, false,
        "creative_editor_world_layout_opening_manipulation_phase_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutOpeningManipulationPhase::Cancel) {
    const bool changed = state.openingManipulation.active;
    state.openingManipulation = {};
    state.statusMessage = "opening manipulation cancelled";
    return {true, changed,
            "creative_editor_world_layout_opening_manipulation_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {
        false, false,
        "creative_editor_world_layout_opening_manipulation_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutOpeningManipulationPhase::Begin) {
    const CreativeEditorWorldLayoutOpeningTarget target =
        findCreativeEditorWorldLayoutOpeningTarget(state, point,
                                                   toleranceCells);
    if (target.handle == CreativeEditorWorldLayoutOpeningHandle::None ||
        target.openingIndex >= state.source.openings.size()) {
      return selectAt(state, point);
    }
    const cr::CreativeWorldLayoutOpening& opening =
        state.source.openings[target.openingIndex];
    const CreativeEditorWorldLayoutOpeningHost host =
        openingHost(state.source, opening);
    const double pointerOffset = openingHostOffset(host, point);
    if (!host.valid || !std::isfinite(pointerOffset)) {
      return {false, false,
              "creative_editor_world_layout_opening_host_invalid"};
    }
    clearWorldLayoutInteraction(state);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                       target.openingIndex};
    state.anchorActive = false;
    state.openingManipulation = {
        true,
        state.revision,
        target,
        pointerOffset,
        opening.centerOffsetCells,
        opening.widthCells,
        opening.centerOffsetCells,
        opening.widthCells,
        true,
        "creative_editor_world_layout_opening_manipulation_ready",
    };
    state.statusMessage =
        target.handle == CreativeEditorWorldLayoutOpeningHandle::Move
            ? "drag opening along its wall"
            : "drag to resize opening";
    return {true, true,
            "creative_editor_world_layout_opening_manipulation_started"};
  }

  if (!state.openingManipulation.active ||
      state.openingManipulation.target.openingIndex >=
          state.source.openings.size()) {
    return {
        false, false,
        "creative_editor_world_layout_opening_manipulation_not_active"};
  }
  const std::size_t openingIndex =
      state.openingManipulation.target.openingIndex;
  const cr::CreativeWorldLayoutOpening& opening =
      state.source.openings[openingIndex];
  if (state.revision != state.openingManipulation.sourceRevision ||
      opening.centerOffsetCells !=
          state.openingManipulation.originalCenterOffsetCells ||
      opening.widthCells != state.openingManipulation.originalWidthCells) {
    state.openingManipulation = {};
    state.statusMessage = "opening changed while drag was active";
    return {false, false,
            "creative_editor_world_layout_opening_manipulation_stale"};
  }

  if (phase == CreativeEditorWorldLayoutOpeningManipulationPhase::Update) {
    const CreativeEditorWorldLayoutOpeningHost host =
        openingHost(state.source, opening);
    const double pointerOffset = openingHostOffset(host, point);
    double delta = 0.0;
    if (!host.valid || !std::isfinite(pointerOffset) ||
        !snappedOpeningDelta(pointerOffset,
                             state.openingManipulation.startPointerOffsetCells,
                             delta)) {
      return {false, false,
              "creative_editor_world_layout_opening_pointer_invalid"};
    }

    double center = state.openingManipulation.originalCenterOffsetCells;
    double width = state.openingManipulation.originalWidthCells;
    const double originalMinimum = center - width * 0.5;
    const double originalMaximum = center + width * 0.5;
    switch (state.openingManipulation.target.handle) {
      case CreativeEditorWorldLayoutOpeningHandle::Move:
        center += delta;
        break;
      case CreativeEditorWorldLayoutOpeningHandle::Start: {
        const double minimum = originalMinimum + delta;
        center = (minimum + originalMaximum) * 0.5;
        width = originalMaximum - minimum;
        break;
      }
      case CreativeEditorWorldLayoutOpeningHandle::End: {
        const double maximum = originalMaximum + delta;
        center = (originalMinimum + maximum) * 0.5;
        width = maximum - originalMinimum;
        break;
      }
      case CreativeEditorWorldLayoutOpeningHandle::None:
      case CreativeEditorWorldLayoutOpeningHandle::Count:
        return {
            false, false,
            "creative_editor_world_layout_opening_manipulation_handle_invalid"};
    }
    if (center == state.openingManipulation.previewCenterOffsetCells &&
        width == state.openingManipulation.previewWidthCells) {
      return {true, false, state.openingManipulation.reasonCode};
    }

    CreativeEditorWorldLayoutOpeningSettings settings =
        openingSettings(opening);
    settings.centerOffsetCells = center;
    settings.widthCells = width;
    const cr::CreativeWorldLayoutOpening candidate =
        openingWithSettings(opening, settings);
    const OpeningValidation validation =
        validateOpeningCandidate(state, openingIndex, candidate);
    state.openingManipulation.previewCenterOffsetCells = center;
    state.openingManipulation.previewWidthCells = width;
    state.openingManipulation.previewValid = validation.accepted;
    state.openingManipulation.reasonCode = validation.reasonCode;
    state.statusMessage = validation.accepted ? "opening drag preview"
                                              : validation.message;
    return {true, true, validation.reasonCode};
  }

  const CreativeEditorWorldLayoutEditReceipt updated =
      applyCreativeEditorWorldLayoutOpeningManipulation(
          state, CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
          point, toleranceCells);
  if (!updated.accepted) {
    return updated;
  }
  if (!state.openingManipulation.previewValid) {
    const std::string reasonCode = state.openingManipulation.reasonCode;
    state.openingManipulation = {};
    return {false, false, reasonCode};
  }
  CreativeEditorWorldLayoutOpeningSettings settings = openingSettings(opening);
  settings.centerOffsetCells =
      state.openingManipulation.previewCenterOffsetCells;
  settings.widthCells = state.openingManipulation.previewWidthCells;
  const cr::CreativeWorldLayoutOpening candidate =
      openingWithSettings(opening, settings);
  state.openingManipulation = {};
  return commitOpeningCandidate(state, openingIndex, candidate,
                                "opening updated");
}

CreativeEditorWorldLayoutEditReceipt deleteCreativeEditorWorldLayoutSelection(
    CreativeEditorWorldLayoutState& state) {
  const CreativeEditorWorldLayoutSelection selected = state.selection;
  if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Building) {
    return deleteCreativeEditorWorldLayoutBuilding(state, selected.index);
  }
  if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      selected.index < state.source.rooms.size()) {
    const std::size_t removedRoom = selected.index;
    state.source.rooms.erase(state.source.rooms.begin() +
                             static_cast<std::ptrdiff_t>(removedRoom));
    std::erase_if(state.source.openings,
                  [&](cr::CreativeWorldLayoutOpening& opening) {
                    if (opening.hostKind !=
                        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
                      return false;
                    }
                    if (opening.roomIndex == removedRoom) {
                      return true;
                    }
                    if (opening.roomIndex > removedRoom) {
                      --opening.roomIndex;
                    }
                    return false;
                  });
    std::erase_if(state.source.verticalConnectors,
                  [&](cr::CreativeWorldLayoutVerticalConnector& connector) {
                    if (connector.lowerRoomIndex == removedRoom ||
                        connector.upperRoomIndex == removedRoom) {
                      return true;
                    }
                    if (connector.lowerRoomIndex > removedRoom) {
                      --connector.lowerRoomIndex;
                    }
                    if (connector.upperRoomIndex > removedRoom) {
                      --connector.upperRoomIndex;
                    }
                    return false;
                  });
  } else if (selected.kind ==
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector &&
             selected.index < state.source.verticalConnectors.size()) {
    state.source.verticalConnectors.erase(
        state.source.verticalConnectors.begin() +
        static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Box &&
             selected.index < state.source.boxes.size()) {
    state.source.boxes.erase(state.source.boxes.begin() +
                             static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Opening &&
             selected.index < state.source.openings.size()) {
    state.source.openings.erase(state.source.openings.begin() +
                                static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Wall &&
             selected.index < state.source.walls.size()) {
    const std::size_t removedWall = selected.index;
    state.source.walls.erase(state.source.walls.begin() +
                             static_cast<std::ptrdiff_t>(removedWall));
    std::erase_if(state.source.openings,
                  [&](cr::CreativeWorldLayoutOpening& opening) {
                    if (opening.hostKind !=
                        cr::CreativeWorldLayoutOpeningHostKind::Wall) {
                      return false;
                    }
                    if (opening.wallIndex == removedWall) {
                      return true;
                    }
                    if (opening.wallIndex > removedWall) {
                      --opening.wallIndex;
                    }
                    return false;
                  });
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Object &&
             selected.index < state.source.objects.size()) {
    state.source.objects.erase(state.source.objects.begin() +
                               static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind ==
                 CreativeEditorWorldLayoutSelectionKind::TerrainProfile &&
             selected.index < state.source.terrainProfiles.size()) {
    state.source.terrainProfiles.erase(
        state.source.terrainProfiles.begin() +
        static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind ==
                 CreativeEditorWorldLayoutSelectionKind::TerrainPath &&
             selected.index < state.source.terrainPaths.size()) {
    const cr::CreativeWorldLayoutTerrainPath removed =
        state.source.terrainPaths[selected.index];
    if (removed.firstPointIndex > state.source.terrainPathPoints.size() ||
        removed.pointCount >
            state.source.terrainPathPoints.size() - removed.firstPointIndex) {
      return {false, false,
              "creative_editor_world_layout_path_ownership_invalid"};
    }
    state.source.terrainPathPoints.erase(
        state.source.terrainPathPoints.begin() +
            static_cast<std::ptrdiff_t>(removed.firstPointIndex),
        state.source.terrainPathPoints.begin() +
            static_cast<std::ptrdiff_t>(removed.firstPointIndex +
                                        removed.pointCount));
    state.source.terrainPaths.erase(
        state.source.terrainPaths.begin() +
        static_cast<std::ptrdiff_t>(selected.index));
    for (cr::CreativeWorldLayoutTerrainPath& path :
         state.source.terrainPaths) {
      if (path.firstPointIndex > removed.firstPointIndex) {
        path.firstPointIndex -= removed.pointCount;
      }
    }
  } else {
    return {false, false, "creative_editor_world_layout_selection_missing"};
  }
  state.selection = {};
  noteWorldLayoutSourceChange(state, "layout symbol deleted");
  return {true, true, "creative_editor_world_layout_selection_deleted"};
}

CreativeEditorWorldLayoutEditReceipt cancelCreativeEditorWorldLayoutPreview(
    CreativeEditorWorldLayoutState& state) noexcept {
  const bool changed = state.previewVisible;
  invalidateWorldLayoutPreview(state);
  state.statusMessage = "3D preview closed";
  return {true, changed, "creative_editor_world_layout_preview_cancelled"};
}

CreativeEditorWorldLayoutPreviewReceipt previewCreativeEditorWorldLayout(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document) {
  CreativeEditorWorldLayoutPreviewReceipt receipt;
  state.anchorActive = false;
  clearWorldLayoutInteraction(state);
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, state.source);
  receipt.status = compiled.receipt.status;
  if (!compiled.receipt.accepted) {
    state.statusMessage = compiled.receipt.reasonCode;
    receipt.reasonCode = compiled.receipt.reasonCode;
    invalidateWorldLayoutPreview(state);
    return receipt;
  }
  cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  receipt.status = preview.status;
  receipt.accepted = preview.accepted;
  receipt.changed = preview.accepted;
  receipt.reasonCode = preview.reasonCode;
  if (!preview.accepted) {
    state.statusMessage = preview.reasonCode;
    invalidateWorldLayoutPreview(state);
    return receipt;
  }
  state.preview = std::move(preview);
  state.previewVisible = true;
  state.previewLayoutRevision = state.revision;
  state.statusMessage = "exact 3D preview ready";
  return receipt;
}

CreativeEditorWorldLayoutApplyReceipt confirmCreativeEditorWorldLayout(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState) {
  CreativeEditorWorldLayoutApplyReceipt result;
  state.anchorActive = false;
  clearWorldLayoutInteraction(state);
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       state.source);
  if (!compiled.receipt.accepted) {
    result.reasonCode = compiled.receipt.reasonCode;
    state.statusMessage = result.reasonCode;
    return result;
  }
  result.apply = applyCreativeEditorWorldLayoutPlanWithHistory(
      state, appState, compiled.plan,
      captureCreativeEditorWorldLayoutSnapshot(state),
      "desktop_world_layout_confirm");
  result.accepted = result.apply.accepted;
  result.changed = result.apply.changed;
  result.reasonCode = result.apply.reasonCode;
  if (result.accepted) {
    invalidateWorldLayoutPreview(state);
    state.statusMessage =
        result.changed ? "layout generated in 3D" : "3D output already current";
  } else {
    state.statusMessage = result.reasonCode;
  }
  return result;
}

}  // namespace iggy3d_creative_app
