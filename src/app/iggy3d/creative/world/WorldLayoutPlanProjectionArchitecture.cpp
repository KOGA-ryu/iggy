#include "app/iggy3d/creative/world/WorldLayoutPlanProjectionInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace iggy3d::creative::plan_projection_internal {
namespace {

struct OpeningCut {
  std::size_t openingIndex = kInvalidCreativeWorldLayoutIndex;
  double beginCells = 0.0;
  double endCells = 0.0;
};

Point add(Point lhs, Point rhs) noexcept {
  return {lhs.x + rhs.x, lhs.z + rhs.z};
}

Point subtract(Point lhs, Point rhs) noexcept {
  return {lhs.x - rhs.x, lhs.z - rhs.z};
}

Point multiply(Point value, double scalar) noexcept {
  return {value.x * scalar, value.z * scalar};
}

double dot(Point lhs, Point rhs) noexcept {
  return lhs.x * rhs.x + lhs.z * rhs.z;
}

double cross(Point lhs, Point rhs) noexcept {
  return lhs.x * rhs.z - lhs.z * rhs.x;
}

bool oppositeEdges(CreativeWorldLayoutRoomEdge lhs,
                   CreativeWorldLayoutRoomEdge rhs) noexcept {
  return (lhs == CreativeWorldLayoutRoomEdge::North &&
          rhs == CreativeWorldLayoutRoomEdge::South) ||
         (lhs == CreativeWorldLayoutRoomEdge::East &&
          rhs == CreativeWorldLayoutRoomEdge::West) ||
         (lhs == CreativeWorldLayoutRoomEdge::South &&
          rhs == CreativeWorldLayoutRoomEdge::North) ||
         (lhs == CreativeWorldLayoutRoomEdge::West &&
          rhs == CreativeWorldLayoutRoomEdge::East);
}

Role wallRole(
    const CreativeWorldLayoutRoomCompileResult::WallProvenance& provenance)
    noexcept {
  if (provenance.contributors.empty()) {
    return Role::InteriorPartition;
  }
  for (std::size_t first = 0U; first < provenance.contributors.size(); ++first) {
    for (std::size_t second = first + 1U;
         second < provenance.contributors.size(); ++second) {
      if (oppositeEdges(provenance.contributors[first].roomEdge,
                        provenance.contributors[second].roomEdge)) {
        return Role::SharedBoundary;
      }
    }
  }
  return Role::ExteriorWall;
}

SourceRef wallSourceRef(
    const CreativeWorldLayout& authored, std::size_t wallIndex,
    const CreativeWorldLayoutRoomCompileResult::WallProvenance& provenance,
    std::span<const std::uint8_t> levelMask) noexcept {
  if (provenance.contributors.empty()) {
    return source(CreativeWorldLayoutTable::Wall, wallIndex);
  }
  SourceRef result;
  for (const auto& contributor : provenance.contributors) {
    if (contributor.roomIndex >= authored.rooms.size()) {
      continue;
    }
    const CreativeWorldLayoutRoom& room = authored.rooms[contributor.roomIndex];
    if (room.levelIndex >= levelMask.size() || !levelMask[room.levelIndex]) {
      continue;
    }
    if (result.primaryTable == CreativeWorldLayoutTable::None) {
      result.primaryTable = CreativeWorldLayoutTable::Room;
      result.primaryIndex = contributor.roomIndex;
    } else if (result.primaryIndex != contributor.roomIndex &&
               result.secondaryTable == CreativeWorldLayoutTable::None) {
      result.secondaryTable = CreativeWorldLayoutTable::Room;
      result.secondaryIndex = contributor.roomIndex;
      break;
    }
  }
  if (result.primaryTable == CreativeWorldLayoutTable::None) {
    result.primaryTable = CreativeWorldLayoutTable::Room;
    result.primaryIndex = provenance.contributors.front().roomIndex;
  }
  return result;
}

bool buildingSelected(std::span<const std::uint8_t> buildingMask,
                      std::size_t buildingIndex) noexcept {
  return buildingIndex < buildingMask.size() && buildingMask[buildingIndex];
}

bool projectFloors(Projection& projection, const CreativeWorldLayout& layout,
                   std::span<const std::uint8_t> levelMask,
                   std::span<const std::uint8_t> buildingMask, Layer layer) {
  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
    const CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
    if (room.levelIndex >= levelMask.size() || !levelMask[room.levelIndex]) {
      continue;
    }
    if (!validRect(room.footprint) ||
        !appendPrimitive(
            projection,
            rectPrimitive(Role::RoomFloor, layer,
                          source(CreativeWorldLayoutTable::Room, roomIndex),
                          room.footprint))) {
      return false;
    }
    ++projection.receipt.roomPrimitiveCount;
  }

  for (std::size_t boxIndex = 0U; boxIndex < layout.boxes.size(); ++boxIndex) {
    const CreativeWorldLayoutBox& box = layout.boxes[boxIndex];
    if (box.kind != CreativeObjectKind::Floor ||
        !buildingSelected(buildingMask, box.buildingIndex)) {
      continue;
    }
    bool onSelectedLevel = false;
    for (std::size_t levelIndex = 0U; levelIndex < levelMask.size();
         ++levelIndex) {
      if (levelMask[levelIndex] &&
          layout.levels[levelIndex].buildingIndex == box.buildingIndex &&
          near(layout.levels[levelIndex].floorTopLayer, box.anchorLayer)) {
        onSelectedLevel = true;
        break;
      }
    }
    if (!onSelectedLevel) {
      continue;
    }
    if (!validRect(box.footprint) ||
        !appendPrimitive(
            projection,
            rectPrimitive(Role::RoomFloor, layer,
                          source(CreativeWorldLayoutTable::Box, boxIndex),
                          box.footprint))) {
      return false;
    }
    ++projection.receipt.roomPrimitiveCount;
  }
  return true;
}

bool appendDoor(Projection& projection,
                const CreativeWorldLayoutOpening& opening,
                std::size_t openingIndex, Layer layer, Point openingStart,
                Point openingEnd, Point wallDirection, Point wallNormal) {
  const SourceRef openingSource =
      source(CreativeWorldLayoutTable::Opening, openingIndex);
  if (opening.pose == CreativeBuildingOpeningPose::Closed) {
    if (!appendPrimitive(projection,
                         segment(Role::Door, layer, openingSource,
                                 openingStart, openingEnd))) {
      return false;
    }
    ++projection.receipt.openingPrimitiveCount;
    return true;
  }

  bool hingeAtStart = false;
  double normalSign = 0.0;
  switch (opening.pose) {
    case CreativeBuildingOpeningPose::OpenFromStartNegativeNormal:
      hingeAtStart = true;
      normalSign = -1.0;
      break;
    case CreativeBuildingOpeningPose::OpenFromStartPositiveNormal:
      hingeAtStart = true;
      normalSign = 1.0;
      break;
    case CreativeBuildingOpeningPose::OpenFromEndNegativeNormal:
      normalSign = -1.0;
      break;
    case CreativeBuildingOpeningPose::OpenFromEndPositiveNormal:
      normalSign = 1.0;
      break;
    case CreativeBuildingOpeningPose::Closed:
      break;
  }
  if (normalSign == 0.0) {
    return false;
  }

  const Point hinge = hingeAtStart ? openingStart : openingEnd;
  const Point closedDirection =
      hingeAtStart ? wallDirection : multiply(wallDirection, -1.0);
  const Point openDirection = multiply(wallNormal, normalSign);
  const Point leafEnd =
      add(hinge, multiply(openDirection, opening.widthCells));
  if (!appendPrimitive(
          projection,
          segment(Role::Door, layer, openingSource, hinge, leafEnd))) {
    return false;
  }
  ++projection.receipt.openingPrimitiveCount;

  Primitive swing;
  swing.role = Role::DoorSwing;
  swing.kind = PrimitiveKind::Arc;
  swing.layer = layer;
  swing.source = openingSource;
  swing.points[0] = hinge;
  swing.pointCount = 1U;
  swing.radiusCells = opening.widthCells;
  swing.startRadians = std::atan2(closedDirection.z, closedDirection.x);
  swing.sweepRadians =
      std::atan2(cross(closedDirection, openDirection),
                 dot(closedDirection, openDirection));
  if (!near(std::abs(swing.sweepRadians), kHalfPi) ||
      !appendPrimitive(projection, swing)) {
    return false;
  }
  ++projection.receipt.openingPrimitiveCount;
  return true;
}

bool appendWindow(Projection& projection,
                  const CreativeWorldLayoutWall& wall,
                  std::size_t openingIndex, Layer layer, Point openingStart,
                  Point openingEnd, Point wallNormal) {
  const double separation = wall.thicknessCells * 0.22;
  if (!std::isfinite(separation) || separation <= 0.0) {
    return false;
  }
  const SourceRef openingSource =
      source(CreativeWorldLayoutTable::Opening, openingIndex);
  for (const double sign : {-1.0, 1.0}) {
    const Point offset = multiply(wallNormal, separation * sign);
    if (!appendPrimitive(
            projection,
            segment(Role::Window, layer, openingSource,
                    add(openingStart, offset), add(openingEnd, offset)))) {
      return false;
    }
    ++projection.receipt.openingPrimitiveCount;
  }
  return true;
}

bool projectWallsAndOpenings(
    Projection& projection, const CreativeWorldLayout& authored,
    const CreativeWorldLayoutRoomCompileResult& compiled,
    std::span<const std::uint8_t> levelMask,
    std::span<const std::uint8_t> buildingMask, double cutPlaneLayer,
    Layer layer) {
  const CreativeWorldLayout& expanded = compiled.expanded;
  std::vector<std::vector<OpeningCut>> cuts(expanded.walls.size());
  for (std::size_t openingIndex = 0U;
       openingIndex < expanded.openings.size(); ++openingIndex) {
    const CreativeWorldLayoutOpening& opening = expanded.openings[openingIndex];
    if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::Wall ||
        opening.wallIndex >= expanded.walls.size() ||
        opening.kind > CreativeBuildingOpeningKind::Window ||
        !std::isfinite(opening.centerOffsetCells) ||
        !std::isfinite(opening.widthCells) || opening.widthCells <= 0.0 ||
        !std::isfinite(opening.cutoutBottomCells) ||
        !std::isfinite(opening.cutoutHeightCells) ||
        opening.cutoutHeightCells <= 0.0) {
      return false;
    }
    const CreativeWorldLayoutWall& wall = expanded.walls[opening.wallIndex];
    if (!buildingSelected(buildingMask, wall.buildingIndex)) {
      continue;
    }
    const double openingBottom = wall.baseLayer + opening.cutoutBottomCells;
    const double openingTop = openingBottom + opening.cutoutHeightCells;
    if (cutPlaneLayer < openingBottom - kGeometryEpsilon ||
        cutPlaneLayer >= openingTop - kGeometryEpsilon) {
      continue;
    }
    cuts[opening.wallIndex].push_back(
        {openingIndex, opening.centerOffsetCells - opening.widthCells * 0.5,
         opening.centerOffsetCells + opening.widthCells * 0.5});
  }

  for (std::size_t wallIndex = 0U; wallIndex < expanded.walls.size();
       ++wallIndex) {
    const CreativeWorldLayoutWall& wall = expanded.walls[wallIndex];
    const double wallTop = wall.baseLayer + wall.heightCells;
    if (!buildingSelected(buildingMask, wall.buildingIndex) ||
        cutPlaneLayer < wall.baseLayer - kGeometryEpsilon ||
        cutPlaneLayer >= wallTop - kGeometryEpsilon) {
      continue;
    }
    const Point start = point(wall.start);
    const Point end = point(wall.end);
    const Point delta = subtract(end, start);
    const double length = std::hypot(delta.x, delta.z);
    if (!std::isfinite(length) || length <= kGeometryEpsilon ||
        !std::isfinite(wall.thicknessCells) || wall.thicknessCells <= 0.0 ||
        wallIndex >= compiled.wallProvenance.size()) {
      return false;
    }
    const Point direction{delta.x / length, delta.z / length};
    const Point normal{-direction.z, direction.x};
    auto& wallCuts = cuts[wallIndex];
    std::sort(wallCuts.begin(), wallCuts.end(),
              [](const OpeningCut& lhs, const OpeningCut& rhs) {
                if (lhs.beginCells != rhs.beginCells) {
                  return lhs.beginCells < rhs.beginCells;
                }
                return lhs.openingIndex < rhs.openingIndex;
              });
    double cursor = 0.0;
    for (const OpeningCut& cut : wallCuts) {
      if (!std::isfinite(cut.beginCells) || !std::isfinite(cut.endCells) ||
          cut.beginCells < cursor - kGeometryEpsilon ||
          cut.beginCells < -kGeometryEpsilon ||
          cut.endCells > length + kGeometryEpsilon ||
          cut.endCells - cut.beginCells <= kGeometryEpsilon) {
        return false;
      }
      cursor = std::max(cursor, cut.endCells);
    }

    const auto& provenance = compiled.wallProvenance[wallIndex];
    const Role role = wallRole(provenance);
    const SourceRef wallSource =
        wallSourceRef(authored, wallIndex, provenance, levelMask);
    cursor = 0.0;
    for (const OpeningCut& cut : wallCuts) {
      if (cut.beginCells > cursor + kGeometryEpsilon) {
        if (!appendPrimitive(
                projection,
                segment(role, layer, wallSource,
                        add(start, multiply(direction, cursor)),
                        add(start, multiply(direction, cut.beginCells))))) {
          return false;
        }
        ++projection.receipt.wallPrimitiveCount;
      }
      const CreativeWorldLayoutOpening& opening =
          expanded.openings[cut.openingIndex];
      const Point openingStart =
          add(start, multiply(direction, cut.beginCells));
      const Point openingEnd = add(start, multiply(direction, cut.endCells));
      const bool emitted = opening.kind == CreativeBuildingOpeningKind::Door
                               ? appendDoor(projection, opening,
                                            cut.openingIndex, layer,
                                            openingStart, openingEnd,
                                            direction, normal)
                               : appendWindow(projection, wall,
                                              cut.openingIndex, layer,
                                              openingStart, openingEnd, normal);
      if (!emitted) {
        return false;
      }
      cursor = cut.endCells;
    }
    if (cursor < length - kGeometryEpsilon) {
      if (!appendPrimitive(
              projection,
              segment(role, layer, wallSource,
                      add(start, multiply(direction, cursor)), end))) {
        return false;
      }
      ++projection.receipt.wallPrimitiveCount;
    }
  }
  return true;
}

bool connectorTouchesLevels(
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutVerticalConnector& connector,
    std::span<const std::uint8_t> levelMask) noexcept {
  for (const std::size_t roomIndex :
       {connector.lowerRoomIndex, connector.upperRoomIndex}) {
    if (roomIndex < layout.rooms.size()) {
      const std::size_t levelIndex = layout.rooms[roomIndex].levelIndex;
      if (levelIndex < levelMask.size() && levelMask[levelIndex]) {
        return true;
      }
    }
  }
  return false;
}

bool connectorAxis(CreativeWorldLayoutRect rect,
                   CreativeWorldLayoutVerticalDirection direction,
                   Point& low, Point& high) noexcept {
  if (!validRect(rect) ||
      direction >= CreativeWorldLayoutVerticalDirection::Count) {
    return false;
  }
  const double centerX =
      (static_cast<double>(rect.minimum.x) + rect.maximum.x) * 0.5;
  const double centerZ =
      (static_cast<double>(rect.minimum.z) + rect.maximum.z) * 0.5;
  switch (direction) {
    case CreativeWorldLayoutVerticalDirection::PositiveX:
      low = {static_cast<double>(rect.minimum.x), centerZ};
      high = {static_cast<double>(rect.maximum.x), centerZ};
      return true;
    case CreativeWorldLayoutVerticalDirection::NegativeX:
      low = {static_cast<double>(rect.maximum.x), centerZ};
      high = {static_cast<double>(rect.minimum.x), centerZ};
      return true;
    case CreativeWorldLayoutVerticalDirection::PositiveZ:
      low = {centerX, static_cast<double>(rect.minimum.z)};
      high = {centerX, static_cast<double>(rect.maximum.z)};
      return true;
    case CreativeWorldLayoutVerticalDirection::NegativeZ:
      low = {centerX, static_cast<double>(rect.maximum.z)};
      high = {centerX, static_cast<double>(rect.minimum.z)};
      return true;
    case CreativeWorldLayoutVerticalDirection::Count:
      return false;
  }
  return false;
}

bool projectConnectors(Projection& projection,
                       const CreativeWorldLayout& layout,
                       std::span<const std::uint8_t> levelMask, Layer layer) {
  for (std::size_t index = 0U; index < layout.verticalConnectors.size();
       ++index) {
    const CreativeWorldLayoutVerticalConnector& connector =
        layout.verticalConnectors[index];
    if (!connectorTouchesLevels(layout, connector, levelMask)) {
      continue;
    }
    if (connector.kind >= CreativeWorldLayoutVerticalConnectorKind::Count ||
        !validRect(connector.footprint)) {
      return false;
    }
    const Role role =
        connector.kind == CreativeWorldLayoutVerticalConnectorKind::Stair
            ? Role::Stair
            : Role::Ramp;
    const SourceRef connectorSource =
        source(CreativeWorldLayoutTable::VerticalConnector, index);
    if (!appendPrimitive(
            projection,
            rectPrimitive(role, layer, connectorSource, connector.footprint))) {
      return false;
    }
    ++projection.receipt.connectorPrimitiveCount;

    Point low;
    Point high;
    if (!connectorAxis(connector.footprint, connector.direction, low, high) ||
        !appendPrimitive(
            projection,
            segment(role, layer, connectorSource, low, high))) {
      return false;
    }
    ++projection.receipt.connectorPrimitiveCount;

    const Point axis = subtract(high, low);
    const double axisLength = std::hypot(axis.x, axis.z);
    const Point direction{axis.x / axisLength, axis.z / axisLength};
    const Point normal{-direction.z, direction.x};
    const double arrowLength = std::min(0.45, axisLength * 0.18);
    const Point arrowBase = subtract(high, multiply(direction, arrowLength));
    for (const double sign : {-1.0, 1.0}) {
      const Point arrowEnd =
          add(arrowBase, multiply(normal, arrowLength * 0.55 * sign));
      if (!appendPrimitive(
              projection,
              segment(role, layer, connectorSource, high, arrowEnd))) {
        return false;
      }
      ++projection.receipt.connectorPrimitiveCount;
    }

    if (connector.kind == CreativeWorldLayoutVerticalConnectorKind::Stair) {
      const bool axisX = std::abs(axis.x) >= std::abs(axis.z);
      for (std::uint8_t tread = 1U; tread < 6U; ++tread) {
        const double t = static_cast<double>(tread) / 6.0;
        Point first;
        Point second;
        if (axisX) {
          const double x = low.x + axis.x * t;
          first = {x, static_cast<double>(connector.footprint.minimum.z)};
          second = {x, static_cast<double>(connector.footprint.maximum.z)};
        } else {
          const double z = low.z + axis.z * t;
          first = {static_cast<double>(connector.footprint.minimum.x), z};
          second = {static_cast<double>(connector.footprint.maximum.x), z};
        }
        if (!appendPrimitive(
                projection,
                segment(role, layer, connectorSource, first, second))) {
          return false;
        }
        ++projection.receipt.connectorPrimitiveCount;
      }
    }
  }
  return true;
}

}  // namespace

bool projectLevelArchitecture(
    Projection& projection, const CreativeWorldLayout& authored,
    const CreativeWorldLayoutRoomCompileResult& compiled,
    std::span<const std::uint8_t> levelMask,
    std::span<const std::uint8_t> buildingMask, double floorTopLayer,
    double cutPlaneHeightCells, Layer layer) {
  return projectFloors(projection, authored, levelMask, buildingMask, layer) &&
         projectWallsAndOpenings(projection, authored, compiled, levelMask,
                                 buildingMask,
                                 floorTopLayer + cutPlaneHeightCells, layer) &&
         projectConnectors(projection, authored, levelMask, layer);
}

bool projectRoofs(Projection& projection, const CreativeWorldLayout& layout,
                  std::span<const std::uint8_t> activeLevelMask) {
  for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
       ++levelIndex) {
    if (!activeLevelMask[levelIndex] ||
        !creativeWorldLayoutLevelIsTopmostOccupied(layout, levelIndex)) {
      continue;
    }
    CreativeWorldLayoutRect footprint;
    if (!creativeWorldLayoutLevelRoofFootprint(layout, levelIndex, footprint) ||
        !validRect(footprint)) {
      return false;
    }
    const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
    if (!std::isfinite(level.roofOverhangCells) ||
        level.roofOverhangCells < 0.0 ||
        level.roofRidgeAxis >= CreativeStructuralRoofRidgeAxis::Count) {
      return false;
    }
    const double minimumX = footprint.minimum.x - level.roofOverhangCells;
    const double minimumZ = footprint.minimum.z - level.roofOverhangCells;
    const double maximumX = footprint.maximum.x + level.roofOverhangCells;
    const double maximumZ = footprint.maximum.z + level.roofOverhangCells;
    const SourceRef levelSource =
        source(CreativeWorldLayoutTable::Level, levelIndex);
    if (!appendPrimitive(
            projection,
            polygon(Role::RoofOutline, Layer::Overhead, levelSource,
                    {{{minimumX, minimumZ},
                      {maximumX, minimumZ},
                      {maximumX, maximumZ},
                      {minimumX, maximumZ}}}))) {
      return false;
    }
    ++projection.receipt.roofPrimitiveCount;
    if (level.roofStyle == CreativeStructuralRoofStyle::Gable) {
      const Primitive ridge =
          level.roofRidgeAxis == CreativeStructuralRoofRidgeAxis::X
              ? segment(Role::RoofRidge, Layer::Overhead, levelSource,
                        {minimumX, (minimumZ + maximumZ) * 0.5},
                        {maximumX, (minimumZ + maximumZ) * 0.5})
              : segment(Role::RoofRidge, Layer::Overhead, levelSource,
                        {(minimumX + maximumX) * 0.5, minimumZ},
                        {(minimumX + maximumX) * 0.5, maximumZ});
      if (!appendPrimitive(projection, ridge)) {
        return false;
      }
      ++projection.receipt.roofPrimitiveCount;
    }
  }
  return true;
}

}  // namespace iggy3d::creative::plan_projection_internal
