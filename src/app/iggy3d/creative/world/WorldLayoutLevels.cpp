#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"

#include <cmath>
#include <limits>

namespace iggy3d::creative {
namespace {

constexpr double kLevelElevationEpsilon = 1.0e-9;

bool near(double lhs, double rhs) noexcept {
  return std::isfinite(lhs) && std::isfinite(rhs) &&
         std::fabs(lhs - rhs) <= kLevelElevationEpsilon;
}

bool verticalBandsOverlap(double firstBottom, double firstTop,
                          double secondBottom, double secondTop) noexcept {
  return std::isfinite(firstBottom) && std::isfinite(firstTop) &&
         std::isfinite(secondBottom) && std::isfinite(secondTop) &&
         firstTop > firstBottom && secondTop > secondBottom &&
         firstBottom < secondTop - kLevelElevationEpsilon &&
         firstTop > secondBottom + kLevelElevationEpsilon;
}

bool validLevel(const CreativeWorldLayout& layout,
                const CreativeWorldLayoutLevel& level) noexcept {
  return level.buildingIndex < layout.buildings.size() &&
         validCreativeWorldLayoutStableKey(level.stableKey) &&
         !level.name.empty() && std::isfinite(level.floorTopLayer) &&
         level.wallHeightCells > 0U && level.floorThicknessLayers > 0U &&
         level.ceilingThicknessLayers > 0U && level.roofThicknessLayers > 0U &&
         validCreativeStructuralRoofSettings(
             level.roofStyle, level.roofRidgeAxis, level.roofPitchDegrees,
             level.roofOverhangCells) &&
         level.roofOverhangCells <=
             kMaximumCreativeWorldLayoutRoofOverhangCells;
}

bool buildingMatchesLevel(const CreativeWorldLayout& layout,
                          std::size_t buildingIndex,
                          std::size_t levelIndex) noexcept {
  return levelIndex < layout.levels.size() &&
         buildingIndex < layout.buildings.size() &&
         layout.levels[levelIndex].buildingIndex == buildingIndex;
}

bool roomTouchesLevel(const CreativeWorldLayout& layout,
                      std::size_t roomIndex,
                      std::size_t levelIndex) noexcept {
  return roomIndex < layout.rooms.size() &&
         layout.rooms[roomIndex].levelIndex == levelIndex &&
         buildingMatchesLevel(layout, layout.rooms[roomIndex].buildingIndex,
                              levelIndex);
}

bool connectorTouchesLevel(const CreativeWorldLayout& layout,
                           std::size_t connectorIndex,
                           std::size_t levelIndex) noexcept {
  if (connectorIndex >= layout.verticalConnectors.size()) {
    return false;
  }
  const CreativeWorldLayoutVerticalConnector& connector =
      layout.verticalConnectors[connectorIndex];
  return buildingMatchesLevel(layout, connector.buildingIndex, levelIndex) &&
         (roomTouchesLevel(layout, connector.lowerRoomIndex, levelIndex) ||
          roomTouchesLevel(layout, connector.upperRoomIndex, levelIndex));
}

bool wallTouchesLevel(const CreativeWorldLayout& layout,
                      std::size_t wallIndex,
                      std::size_t levelIndex) noexcept {
  if (wallIndex >= layout.walls.size() || levelIndex >= layout.levels.size()) {
    return false;
  }
  const CreativeWorldLayoutWall& wall = layout.walls[wallIndex];
  const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
  return buildingMatchesLevel(layout, wall.buildingIndex, levelIndex) &&
         verticalBandsOverlap(
             wall.baseLayer, wall.baseLayer + wall.heightCells,
             level.floorTopLayer,
             level.floorTopLayer + level.wallHeightCells);
}

bool openingTouchesLevel(const CreativeWorldLayout& layout,
                         std::size_t openingIndex,
                         std::size_t levelIndex) noexcept {
  if (openingIndex >= layout.openings.size() ||
      levelIndex >= layout.levels.size()) {
    return false;
  }
  const CreativeWorldLayoutOpening& opening = layout.openings[openingIndex];
  if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge) {
    return roomTouchesLevel(layout, opening.roomIndex, levelIndex);
  }
  if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::Wall ||
      !wallTouchesLevel(layout, opening.wallIndex, levelIndex)) {
    return false;
  }
  const CreativeWorldLayoutWall& wall = layout.walls[opening.wallIndex];
  const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
  const double openingBottom = wall.baseLayer + opening.cutoutBottomCells;
  return verticalBandsOverlap(
      openingBottom, openingBottom + opening.cutoutHeightCells,
      level.floorTopLayer, level.floorTopLayer + level.wallHeightCells);
}

}  // namespace

std::size_t firstInvalidCreativeWorldLayoutLevelIndex(
    const CreativeWorldLayout& layout) noexcept {
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    const CreativeWorldLayoutLevel& level = layout.levels[index];
    if (!validLevel(layout, level)) {
      return index;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      const CreativeWorldLayoutLevel& other = layout.levels[prior];
      if (other.buildingIndex == level.buildingIndex &&
          std::fabs(other.floorTopLayer - level.floorTopLayer) <=
              kLevelElevationEpsilon) {
        return index;
      }
    }
  }
  return kInvalidCreativeWorldLayoutIndex;
}

bool validCreativeWorldLayoutLevelOwnership(
    const CreativeWorldLayout& layout) noexcept {
  if (firstInvalidCreativeWorldLayoutLevelIndex(layout) !=
      kInvalidCreativeWorldLayoutIndex) {
    return false;
  }
  for (const CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.buildingIndex >= layout.buildings.size() ||
        room.levelIndex >= layout.levels.size() ||
        layout.levels[room.levelIndex].buildingIndex != room.buildingIndex) {
      return false;
    }
  }
  return true;
}

const CreativeWorldLayoutLevel* creativeWorldLayoutLevelForRoom(
    const CreativeWorldLayout& layout, std::size_t roomIndex) noexcept {
  if (roomIndex >= layout.rooms.size()) {
    return nullptr;
  }
  const CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
  if (room.levelIndex >= layout.levels.size()) {
    return nullptr;
  }
  const CreativeWorldLayoutLevel& level = layout.levels[room.levelIndex];
  return level.buildingIndex == room.buildingIndex ? &level : nullptr;
}

bool creativeWorldLayoutLevelHasRooms(const CreativeWorldLayout& layout,
                                      std::size_t levelIndex) noexcept {
  if (levelIndex >= layout.levels.size()) {
    return false;
  }
  for (const CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.levelIndex == levelIndex) {
      return true;
    }
  }
  return false;
}

bool creativeWorldLayoutLevelIsTopmostOccupied(
    const CreativeWorldLayout& layout, std::size_t levelIndex) noexcept {
  if (levelIndex >= layout.levels.size() ||
      !creativeWorldLayoutLevelHasRooms(layout, levelIndex)) {
    return false;
  }
  const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    if (index == levelIndex ||
        layout.levels[index].buildingIndex != level.buildingIndex ||
        !creativeWorldLayoutLevelHasRooms(layout, index)) {
      continue;
    }
    if (layout.levels[index].floorTopLayer >
        level.floorTopLayer + kLevelElevationEpsilon) {
      return false;
    }
  }
  return true;
}

CreativeWorldLayoutLevelNavigationResult navigateCreativeWorldLayoutLevel(
    const CreativeWorldLayout& layout, std::size_t activeLevelIndex,
    CreativeWorldLayoutLevelNavigationDirection direction) noexcept {
  CreativeWorldLayoutLevelNavigationResult result;
  if (direction != CreativeWorldLayoutLevelNavigationDirection::Lower &&
      direction != CreativeWorldLayoutLevelNavigationDirection::Higher) {
    result.status =
        CreativeWorldLayoutLevelNavigationStatus::InvalidDirection;
    return result;
  }
  if (activeLevelIndex >= layout.levels.size()) {
    return result;
  }

  const CreativeWorldLayoutLevel& active = layout.levels[activeLevelIndex];
  if (!validLevel(layout, active) ||
      !layout.buildings[active.buildingIndex].visible) {
    return result;
  }

  const bool moveHigher =
      direction == CreativeWorldLayoutLevelNavigationDirection::Higher;
  std::size_t bestIndex = kInvalidCreativeWorldLayoutIndex;
  double bestDatum = moveHigher ? std::numeric_limits<double>::infinity()
                                : -std::numeric_limits<double>::infinity();
  bool bestSharesBuilding = false;
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    const CreativeWorldLayoutLevel& candidate = layout.levels[index];
    if (!validLevel(layout, candidate)) {
      result.status = CreativeWorldLayoutLevelNavigationStatus::InvalidLayout;
      return result;
    }
    if (!layout.buildings[candidate.buildingIndex].visible) {
      continue;
    }

    const bool inDirection =
        moveHigher
            ? candidate.floorTopLayer >
                  active.floorTopLayer + kLevelElevationEpsilon
            : candidate.floorTopLayer <
                  active.floorTopLayer - kLevelElevationEpsilon;
    if (!inDirection) {
      continue;
    }

    const bool sharesBuilding =
        candidate.buildingIndex == active.buildingIndex;
    const bool nearer =
        bestIndex == kInvalidCreativeWorldLayoutIndex ||
        (moveHigher
             ? candidate.floorTopLayer < bestDatum - kLevelElevationEpsilon
             : candidate.floorTopLayer > bestDatum + kLevelElevationEpsilon);
    const bool sameDatum =
        bestIndex != kInvalidCreativeWorldLayoutIndex &&
        std::fabs(candidate.floorTopLayer - bestDatum) <=
            kLevelElevationEpsilon;
    const bool betterTie =
        sameDatum &&
        ((sharesBuilding && !bestSharesBuilding) ||
         (sharesBuilding == bestSharesBuilding && index < bestIndex));
    if (nearer || betterTie) {
      bestIndex = index;
      bestDatum = candidate.floorTopLayer;
      bestSharesBuilding = sharesBuilding;
    }
  }

  if (bestIndex == kInvalidCreativeWorldLayoutIndex) {
    result.status = CreativeWorldLayoutLevelNavigationStatus::Boundary;
    return result;
  }
  result.status = CreativeWorldLayoutLevelNavigationStatus::Ready;
  result.targetLevelIndex = bestIndex;
  result.targetFloorTopLayer = bestDatum;
  return result;
}

bool creativeWorldLayoutSourceTouchesLevel(
    const CreativeWorldLayout& layout, CreativeWorldLayoutTable table,
    std::size_t sourceIndex, std::size_t levelIndex) noexcept {
  if (levelIndex >= layout.levels.size() ||
      !validLevel(layout, layout.levels[levelIndex])) {
    return false;
  }
  const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
  switch (table) {
    case CreativeWorldLayoutTable::Building:
      return buildingMatchesLevel(layout, sourceIndex, levelIndex);
    case CreativeWorldLayoutTable::Level:
      return sourceIndex == levelIndex;
    case CreativeWorldLayoutTable::Room:
      return roomTouchesLevel(layout, sourceIndex, levelIndex);
    case CreativeWorldLayoutTable::VerticalConnector:
      return connectorTouchesLevel(layout, sourceIndex, levelIndex);
    case CreativeWorldLayoutTable::Box:
      return sourceIndex < layout.boxes.size() &&
             buildingMatchesLevel(layout,
                                  layout.boxes[sourceIndex].buildingIndex,
                                  levelIndex) &&
             near(layout.boxes[sourceIndex].anchorLayer,
                  level.floorTopLayer);
    case CreativeWorldLayoutTable::Wall:
      return wallTouchesLevel(layout, sourceIndex, levelIndex);
    case CreativeWorldLayoutTable::Opening:
      return openingTouchesLevel(layout, sourceIndex, levelIndex);
    case CreativeWorldLayoutTable::None:
    case CreativeWorldLayoutTable::Object:
    case CreativeWorldLayoutTable::TerrainProfile:
    case CreativeWorldLayoutTable::TerrainPath:
    case CreativeWorldLayoutTable::TerrainPathPoint:
      return false;
  }
  return false;
}

std::size_t creativeWorldLayoutSourceLevelAtDatum(
    const CreativeWorldLayout& layout, CreativeWorldLayoutTable table,
    std::size_t sourceIndex, double floorTopLayer) noexcept {
  if (!std::isfinite(floorTopLayer)) {
    return kInvalidCreativeWorldLayoutIndex;
  }
  for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
       ++levelIndex) {
    const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
    if (near(level.floorTopLayer, floorTopLayer) &&
        level.buildingIndex < layout.buildings.size() &&
        layout.buildings[level.buildingIndex].visible &&
        creativeWorldLayoutSourceTouchesLevel(layout, table, sourceIndex,
                                              levelIndex)) {
      return levelIndex;
    }
  }
  return kInvalidCreativeWorldLayoutIndex;
}

CreativeWorldLayoutResolvedRoomGeometry resolveCreativeWorldLayoutRoomGeometry(
    const CreativeWorldLayout& layout, std::size_t roomIndex) noexcept {
  CreativeWorldLayoutResolvedRoomGeometry result;
  const CreativeWorldLayoutLevel* level =
      creativeWorldLayoutLevelForRoom(layout, roomIndex);
  if (level == nullptr || !std::isfinite(level->floorTopLayer) ||
      level->wallHeightCells == 0U || level->floorThicknessLayers == 0U ||
      level->ceilingThicknessLayers == 0U ||
      level->roofThicknessLayers == 0U) {
    return result;
  }
  result.valid = true;
  result.levelIndex = layout.rooms[roomIndex].levelIndex;
  result.floorTopLayer = level->floorTopLayer;
  result.wallHeightCells = level->wallHeightCells;
  result.floorThicknessLayers = level->floorThicknessLayers;
  const bool top = creativeWorldLayoutLevelIsTopmostOccupied(
      layout, result.levelIndex);
  result.upperSurfaceKind =
      top ? CreativeObjectKind::Roof : CreativeObjectKind::Ceiling;
  result.upperSurfaceThicknessLayers =
      top ? level->roofThicknessLayers : level->ceilingThicknessLayers;
  return result;
}

}  // namespace iggy3d::creative
