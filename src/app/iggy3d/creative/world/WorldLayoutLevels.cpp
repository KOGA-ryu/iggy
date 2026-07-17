#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"

#include <cmath>

namespace iggy3d::creative {
namespace {

constexpr double kLevelElevationEpsilon = 1.0e-9;

bool validLevel(const CreativeWorldLayout& layout,
                const CreativeWorldLayoutLevel& level) noexcept {
  return level.buildingIndex < layout.buildings.size() &&
         validCreativeWorldLayoutStableKey(level.stableKey) &&
         !level.name.empty() && std::isfinite(level.floorTopLayer) &&
         level.wallHeightCells > 0U && level.floorThicknessLayers > 0U &&
         level.ceilingThicknessLayers > 0U && level.roofThicknessLayers > 0U;
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
