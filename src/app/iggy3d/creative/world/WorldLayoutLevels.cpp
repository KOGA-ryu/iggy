#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"

#include <algorithm>
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
             level.roofStyle, level.roofRidgeAxis,
             level.roofSlopeDirection, level.roofPitchDegrees,
             level.roofOverhangCells, level.roofMaterial) &&
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

bool buildingRootTouchesLevel(const CreativeWorldLayout& layout,
                              std::size_t buildingIndex,
                              std::size_t levelIndex) noexcept {
  if (buildingIndex >= layout.buildings.size() ||
      levelIndex >= layout.levels.size()) {
    return false;
  }
  const CreativeWorldLayoutBuilding& building =
      layout.buildings[buildingIndex];
  const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
  return building.rootMode == CreativeBuildingRootMode::CreateRoom &&
         level.buildingIndex == buildingIndex &&
         verticalBandsOverlap(
             building.rootBaseLayer,
             static_cast<double>(building.rootBaseLayer) +
                 building.rootHeightCells,
             level.floorTopLayer,
             level.floorTopLayer + level.wallHeightCells);
}

bool topologyEdgeTouchesLevel(const CreativeWorldLayout& layout,
                              std::size_t edgeIndex,
                              std::size_t levelIndex) noexcept {
  return edgeIndex < layout.topologyEdges.size() &&
         layout.topologyEdges[edgeIndex].levelIndex == levelIndex &&
         levelIndex < layout.levels.size() &&
         layout.levels[levelIndex].buildingIndex < layout.buildings.size();
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

bool scopeIncludesLevel(
    CreativeWorldLayoutLevelDatumEditScope scope,
    std::size_t candidateIndex,
    std::size_t selectedIndex,
    double candidateDatum,
    double selectedDatum) noexcept {
  switch (scope) {
    case CreativeWorldLayoutLevelDatumEditScope::Selected:
      return candidateIndex == selectedIndex;
    case CreativeWorldLayoutLevelDatumEditScope::SelectedAndAbove:
      return candidateDatum >= selectedDatum - kLevelElevationEpsilon;
    case CreativeWorldLayoutLevelDatumEditScope::SelectedAndBelow:
      return candidateDatum <= selectedDatum + kLevelElevationEpsilon;
    case CreativeWorldLayoutLevelDatumEditScope::All:
      return true;
    case CreativeWorldLayoutLevelDatumEditScope::Count:
      return false;
  }
  return false;
}

double snapLevelDatum(const CreativeWorldLayout& layout,
                      std::size_t levelIndex,
                      double requested) noexcept {
  const CreativeWorldLayoutLevel& selected = layout.levels[levelIndex];
  double anchor = selected.floorTopLayer;
  for (const CreativeWorldLayoutLevel& candidate : layout.levels) {
    if (candidate.buildingIndex == selected.buildingIndex) {
      anchor = std::min(anchor, candidate.floorTopLayer);
    }
  }
  return anchor + std::round(requested - anchor);
}

bool plannedLevelDatum(
    const CreativeWorldLayout& layout,
    std::size_t selectedLevelIndex,
    CreativeWorldLayoutLevelDatumEditScope scope,
    double delta,
    std::size_t levelIndex,
    double& output) noexcept {
  if (selectedLevelIndex >= layout.levels.size() ||
      levelIndex >= layout.levels.size()) {
    return false;
  }
  const CreativeWorldLayoutLevel& selected =
      layout.levels[selectedLevelIndex];
  const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
  output = level.floorTopLayer;
  if (level.buildingIndex != selected.buildingIndex ||
      !scopeIncludesLevel(scope, levelIndex, selectedLevelIndex,
                          level.floorTopLayer, selected.floorTopLayer)) {
    return true;
  }
  const long double moved =
      static_cast<long double>(level.floorTopLayer) +
      static_cast<long double>(delta);
  if (!std::isfinite(moved) ||
      moved < -static_cast<long double>(
                  std::numeric_limits<double>::max()) ||
      moved >
          static_cast<long double>(std::numeric_limits<double>::max())) {
    return false;
  }
  output = static_cast<double>(moved);
  return std::isfinite(output);
}

bool levelOrderPreserved(
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    std::size_t selectedLevelIndex,
    CreativeWorldLayoutLevelDatumEditScope scope,
    double delta) noexcept {
  for (std::size_t first = 0U; first < layout.levels.size(); ++first) {
    if (layout.levels[first].buildingIndex != buildingIndex) {
      continue;
    }
    double firstDatum = 0.0;
    if (!plannedLevelDatum(layout, selectedLevelIndex, scope, delta, first,
                           firstDatum)) {
      return false;
    }
    for (std::size_t second = first + 1U; second < layout.levels.size();
         ++second) {
      if (layout.levels[second].buildingIndex != buildingIndex) {
        continue;
      }
      double secondDatum = 0.0;
      if (!plannedLevelDatum(layout, selectedLevelIndex, scope, delta, second,
                             secondDatum)) {
        return false;
      }
      const double originalDelta =
          layout.levels[first].floorTopLayer -
          layout.levels[second].floorTopLayer;
      const double candidateDelta = firstDatum - secondDatum;
      if ((originalDelta < -kLevelElevationEpsilon &&
           candidateDelta >= -kLevelElevationEpsilon) ||
          (originalDelta > kLevelElevationEpsilon &&
           candidateDelta <= kLevelElevationEpsilon)) {
        return false;
      }
    }
  }
  return true;
}

struct VerticalSpanAdjustment {
  bool touched = false;
  bool changed = false;
  double baseLayer = 0.0;
  std::uint16_t heightCells = 0U;
};

template <typename TouchesLevel>
bool planVerticalSpanAdjustment(
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    std::size_t selectedLevelIndex,
    CreativeWorldLayoutLevelDatumEditScope scope,
    double delta,
    double baseLayer,
    std::uint16_t heightCells,
    TouchesLevel&& touchesLevel,
    VerticalSpanAdjustment& output) noexcept {
  output.baseLayer = baseLayer;
  output.heightCells = heightCells;
  if (!std::isfinite(baseLayer) || heightCells == 0U) {
    return false;
  }

  bool found = false;
  double lowestDatum = 0.0;
  double highestDatum = 0.0;
  bool lowestMoves = false;
  bool highestMoves = false;
  const double selectedDatum =
      layout.levels[selectedLevelIndex].floorTopLayer;
  for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
       ++levelIndex) {
    const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
    if (level.buildingIndex != buildingIndex || !touchesLevel(levelIndex)) {
      continue;
    }
    const bool moves =
        scopeIncludesLevel(scope, levelIndex, selectedLevelIndex,
                           level.floorTopLayer, selectedDatum);
    if (!found || level.floorTopLayer < lowestDatum) {
      lowestDatum = level.floorTopLayer;
      lowestMoves = moves;
    }
    if (!found || level.floorTopLayer > highestDatum) {
      highestDatum = level.floorTopLayer;
      highestMoves = moves;
    }
    found = true;
  }
  output.touched = found;
  if (!found) {
    return true;
  }

  const long double movedBottom =
      static_cast<long double>(baseLayer) +
      (lowestMoves ? static_cast<long double>(delta) : 0.0L);
  const long double movedTop =
      static_cast<long double>(baseLayer) +
      static_cast<long double>(heightCells) +
      (highestMoves ? static_cast<long double>(delta) : 0.0L);
  const long double movedHeight = movedTop - movedBottom;
  const long double roundedHeight = std::round(movedHeight);
  if (!std::isfinite(movedBottom) || !std::isfinite(movedHeight) ||
      std::fabs(movedHeight - roundedHeight) >
          static_cast<long double>(kLevelElevationEpsilon) ||
      movedBottom <
          -static_cast<long double>(std::numeric_limits<double>::max()) ||
      movedBottom >
          static_cast<long double>(std::numeric_limits<double>::max()) ||
      roundedHeight < 1.0L ||
      roundedHeight >
          static_cast<long double>(
              std::numeric_limits<std::uint16_t>::max())) {
    return false;
  }
  output.baseLayer = static_cast<double>(movedBottom);
  output.heightCells = static_cast<std::uint16_t>(roundedHeight);
  output.changed =
      !near(output.baseLayer, baseLayer) || output.heightCells != heightCells;
  return true;
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
  return std::all_of(
      layout.roofApertures.begin(), layout.roofApertures.end(),
      [&](const CreativeWorldLayoutRoofAperture& aperture) {
        return aperture.levelIndex < layout.levels.size();
      });
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

double creativeWorldLayoutLevelFacadeHeightCells(
    const CreativeWorldLayout& layout, std::size_t levelIndex) noexcept {
  if (levelIndex >= layout.levels.size() ||
      !creativeWorldLayoutLevelHasRooms(layout, levelIndex)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
  double nextFloorTopLayer = std::numeric_limits<double>::infinity();
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    const CreativeWorldLayoutLevel& candidate = layout.levels[index];
    if (candidate.buildingIndex != level.buildingIndex ||
        !creativeWorldLayoutLevelHasRooms(layout, index) ||
        candidate.floorTopLayer <=
            level.floorTopLayer + kLevelElevationEpsilon) {
      continue;
    }
    nextFloorTopLayer =
        std::min(nextFloorTopLayer, candidate.floorTopLayer);
  }
  if (!std::isfinite(nextFloorTopLayer)) {
    return static_cast<double>(level.wallHeightCells);
  }
  const double heightCells = nextFloorTopLayer - level.floorTopLayer;
  return std::isfinite(heightCells) && heightCells > kLevelElevationEpsilon
             ? heightCells
             : std::numeric_limits<double>::quiet_NaN();
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
    case CreativeWorldLayoutTable::TopologyEdge:
      return topologyEdgeTouchesLevel(layout, sourceIndex, levelIndex);
    case CreativeWorldLayoutTable::RoofAperture:
      return sourceIndex < layout.roofApertures.size() &&
             layout.roofApertures[sourceIndex].levelIndex == levelIndex;
    case CreativeWorldLayoutTable::None:
    case CreativeWorldLayoutTable::Object:
    case CreativeWorldLayoutTable::TerrainProfile:
    case CreativeWorldLayoutTable::TerrainPath:
    case CreativeWorldLayoutTable::TerrainPathPoint:
      return false;
  }
  return false;
}

CreativeWorldLayoutLevelDatumEditPlan
planCreativeWorldLayoutLevelDatumEdit(
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutLevelDatumEditRequest request) noexcept {
  CreativeWorldLayoutLevelDatumEditPlan result;
  result.selectedLevelIndex = request.levelIndex;
  result.scope = request.scope;
  if (request.levelIndex >= layout.levels.size() ||
      request.scope >= CreativeWorldLayoutLevelDatumEditScope::Count ||
      !std::isfinite(request.requestedFloorTopLayer) ||
      firstInvalidCreativeWorldLayoutLevelIndex(layout) !=
          kInvalidCreativeWorldLayoutIndex) {
    result.reasonCode =
        "creative_world_layout_level_datum_edit_request_invalid";
    return result;
  }

  const CreativeWorldLayoutLevel& selected =
      layout.levels[request.levelIndex];
  if (selected.buildingIndex >= layout.buildings.size()) {
    result.reasonCode =
        "creative_world_layout_level_datum_edit_request_invalid";
    return result;
  }
  result.buildingIndex = selected.buildingIndex;
  result.selectedFloorTopLayerBefore = selected.floorTopLayer;
  result.snappedFloorTopLayer =
      snapLevelDatum(layout, request.levelIndex,
                     request.requestedFloorTopLayer);
  if (!std::isfinite(result.snappedFloorTopLayer)) {
    result.reasonCode =
        "creative_world_layout_level_datum_edit_unrepresentable";
    return result;
  }

  result.deltaCells =
      result.snappedFloorTopLayer - selected.floorTopLayer;
  result.accepted = true;
  if (std::abs(result.deltaCells) <= kLevelElevationEpsilon) {
    result.reasonCode =
        "creative_world_layout_level_datum_edit_no_change";
    return result;
  }

  for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
       ++levelIndex) {
    const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
    if (level.buildingIndex != selected.buildingIndex ||
        !scopeIncludesLevel(request.scope, levelIndex, request.levelIndex,
                            level.floorTopLayer,
                            selected.floorTopLayer)) {
      continue;
    }
    double moved = 0.0;
    if (!plannedLevelDatum(layout, request.levelIndex, request.scope,
                           result.deltaCells, levelIndex, moved)) {
      result.accepted = false;
      result.reasonCode =
          "creative_world_layout_level_datum_edit_unrepresentable";
      return result;
    }
    ++result.affectedLevelCount;
  }
  if (result.affectedLevelCount == 0U ||
      !levelOrderPreserved(layout, selected.buildingIndex,
                           request.levelIndex, request.scope,
                           result.deltaCells)) {
    result.accepted = false;
    result.affectedLevelCount = 0U;
    result.reasonCode =
        "creative_world_layout_level_datum_edit_crosses_level";
    return result;
  }

  for (std::size_t boxIndex = 0U; boxIndex < layout.boxes.size();
       ++boxIndex) {
    const CreativeWorldLayoutBox& box = layout.boxes[boxIndex];
    if (box.buildingIndex != selected.buildingIndex) {
      continue;
    }
    bool moves = false;
    for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
         ++levelIndex) {
      const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
      if (level.buildingIndex == selected.buildingIndex &&
          scopeIncludesLevel(request.scope, levelIndex, request.levelIndex,
                             level.floorTopLayer,
                             selected.floorTopLayer) &&
          creativeWorldLayoutSourceTouchesLevel(
              layout, CreativeWorldLayoutTable::Box, boxIndex, levelIndex)) {
        moves = true;
        break;
      }
    }
    if (!moves) {
      continue;
    }
    const long double moved =
        static_cast<long double>(box.anchorLayer) +
        static_cast<long double>(result.deltaCells);
    if (!std::isfinite(moved) ||
        moved <
            -static_cast<long double>(
                std::numeric_limits<double>::max()) ||
        moved >
            static_cast<long double>(
                std::numeric_limits<double>::max())) {
      result.accepted = false;
      result.reasonCode =
          "creative_world_layout_level_datum_attached_source_unrepresentable";
      return result;
    }
    ++result.affectedBoxCount;
  }

  for (std::size_t wallIndex = 0U; wallIndex < layout.walls.size();
       ++wallIndex) {
    const CreativeWorldLayoutWall& wall = layout.walls[wallIndex];
    if (wall.buildingIndex != selected.buildingIndex) {
      continue;
    }
    VerticalSpanAdjustment adjustment;
    if (!planVerticalSpanAdjustment(
            layout, selected.buildingIndex, request.levelIndex,
            request.scope, result.deltaCells, wall.baseLayer,
            wall.heightCells,
            [&layout, wallIndex](std::size_t levelIndex) {
              return wallTouchesLevel(layout, wallIndex, levelIndex);
            },
            adjustment)) {
      result.accepted = false;
      result.reasonCode =
          "creative_world_layout_level_datum_attached_source_unrepresentable";
      return result;
    }
    if (adjustment.changed) {
      ++result.adjustedWallCount;
    }
  }

  const CreativeWorldLayoutBuilding& building =
      layout.buildings[selected.buildingIndex];
  if (building.rootMode == CreativeBuildingRootMode::CreateRoom) {
    VerticalSpanAdjustment rootAdjustment;
    if (!planVerticalSpanAdjustment(
            layout, selected.buildingIndex, request.levelIndex,
            request.scope, result.deltaCells,
            static_cast<double>(building.rootBaseLayer),
            building.rootHeightCells,
            [&layout, buildingIndex = selected.buildingIndex](
                std::size_t levelIndex) {
              return buildingRootTouchesLevel(layout, buildingIndex,
                                              levelIndex);
            },
            rootAdjustment)) {
      result.accepted = false;
      result.reasonCode =
          "creative_world_layout_level_datum_attached_source_unrepresentable";
      return result;
    }
    if (rootAdjustment.changed) {
      const double roundedBase = std::round(rootAdjustment.baseLayer);
      if (std::fabs(rootAdjustment.baseLayer - roundedBase) >
              kLevelElevationEpsilon ||
          roundedBase <
              static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
          roundedBase >
              static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
        result.accepted = false;
        result.reasonCode =
            "creative_world_layout_level_datum_attached_source_unrepresentable";
        return result;
      }
      result.buildingRootAdjusted = true;
    }
  }

  result.changed = true;
  result.reasonCode = "creative_world_layout_level_datum_edit_ready";
  return result;
}

bool applyCreativeWorldLayoutLevelDatumEditPlan(
    CreativeWorldLayout& layout,
    const CreativeWorldLayoutLevelDatumEditPlan& plan) noexcept {
  if (!plan.accepted || !plan.changed ||
      plan.selectedLevelIndex >= layout.levels.size() ||
      plan.scope >= CreativeWorldLayoutLevelDatumEditScope::Count) {
    return false;
  }
  const CreativeWorldLayoutLevelDatumEditPlan current =
      planCreativeWorldLayoutLevelDatumEdit(
          layout,
          {plan.selectedLevelIndex, plan.scope, plan.snappedFloorTopLayer});
  if (!current.accepted || !current.changed ||
      current.buildingIndex != plan.buildingIndex ||
      !near(current.selectedFloorTopLayerBefore,
            plan.selectedFloorTopLayerBefore) ||
      !near(current.snappedFloorTopLayer, plan.snappedFloorTopLayer) ||
      !near(current.deltaCells, plan.deltaCells) ||
      current.affectedLevelCount != plan.affectedLevelCount ||
      current.affectedBoxCount != plan.affectedBoxCount ||
      current.adjustedWallCount != plan.adjustedWallCount ||
      current.buildingRootAdjusted != plan.buildingRootAdjusted) {
    return false;
  }

  const CreativeWorldLayoutLevel& selected =
      layout.levels[plan.selectedLevelIndex];
  for (std::size_t boxIndex = 0U; boxIndex < layout.boxes.size();
       ++boxIndex) {
    CreativeWorldLayoutBox& box = layout.boxes[boxIndex];
    if (box.buildingIndex != plan.buildingIndex) {
      continue;
    }
    bool moves = false;
    for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
         ++levelIndex) {
      const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
      if (level.buildingIndex == plan.buildingIndex &&
          scopeIncludesLevel(plan.scope, levelIndex,
                             plan.selectedLevelIndex,
                             level.floorTopLayer,
                             selected.floorTopLayer) &&
          creativeWorldLayoutSourceTouchesLevel(
              layout, CreativeWorldLayoutTable::Box, boxIndex, levelIndex)) {
        moves = true;
        break;
      }
    }
    if (moves) {
      box.anchorLayer += plan.deltaCells;
    }
  }

  for (std::size_t wallIndex = 0U; wallIndex < layout.walls.size();
       ++wallIndex) {
    CreativeWorldLayoutWall& wall = layout.walls[wallIndex];
    if (wall.buildingIndex != plan.buildingIndex) {
      continue;
    }
    VerticalSpanAdjustment adjustment;
    const bool valid = planVerticalSpanAdjustment(
        layout, plan.buildingIndex, plan.selectedLevelIndex, plan.scope,
        plan.deltaCells, wall.baseLayer, wall.heightCells,
        [&layout, wallIndex](std::size_t levelIndex) {
          return wallTouchesLevel(layout, wallIndex, levelIndex);
        },
        adjustment);
    if (valid && adjustment.changed) {
      wall.baseLayer = adjustment.baseLayer;
      wall.heightCells = adjustment.heightCells;
    }
  }

  CreativeWorldLayoutBuilding& building =
      layout.buildings[plan.buildingIndex];
  if (building.rootMode == CreativeBuildingRootMode::CreateRoom) {
    VerticalSpanAdjustment rootAdjustment;
    const bool rootValid = planVerticalSpanAdjustment(
        layout, plan.buildingIndex, plan.selectedLevelIndex, plan.scope,
        plan.deltaCells, static_cast<double>(building.rootBaseLayer),
        building.rootHeightCells,
        [&layout, buildingIndex = plan.buildingIndex](
            std::size_t levelIndex) {
          return buildingRootTouchesLevel(layout, buildingIndex, levelIndex);
        },
        rootAdjustment);
    if (!rootValid) {
      return false;
    }
    if (rootAdjustment.changed) {
      building.rootBaseLayer =
          static_cast<std::int32_t>(std::llround(rootAdjustment.baseLayer));
      building.rootHeightCells = rootAdjustment.heightCells;
    }
  }

  for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
       ++levelIndex) {
    CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
    if (level.buildingIndex == plan.buildingIndex &&
        scopeIncludesLevel(plan.scope, levelIndex,
                           plan.selectedLevelIndex,
                           level.floorTopLayer,
                           plan.selectedFloorTopLayerBefore)) {
      level.floorTopLayer += plan.deltaCells;
    }
  }
  return true;
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
