#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/recipes/DoorRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;

template <typename Result, typename Status>
void reject(Result& result, Status status,
            std::string_view reasonCode) noexcept {
  result.accepted = false;
  result.status = status;
  result.reasonCode = reasonCode;
}

bool cardinalSegment(CreativeTerrainCoord2 start,
                     CreativeTerrainCoord2 end) noexcept {
  return (start.x == end.x) != (start.z == end.z);
}

bool boundsOverlap(const CreativeBounds& left,
                   const CreativeBounds& right) noexcept {
  return left.min.x < right.max.x - kGeometryEpsilon &&
         left.max.x > right.min.x + kGeometryEpsilon &&
         left.min.y < right.max.y - kGeometryEpsilon &&
         left.max.y > right.min.y + kGeometryEpsilon &&
         left.min.z < right.max.z - kGeometryEpsilon &&
         left.max.z > right.min.z + kGeometryEpsilon;
}

bool segmentBounds(CreativeTerrainCoord2 start, CreativeTerrainCoord2 end,
                   double base, double height, double thickness,
                   CreativeBounds& bounds) noexcept {
  if (!cardinalSegment(start, end) || !std::isfinite(base) ||
      !std::isfinite(height) || height <= 0.0 ||
      !std::isfinite(thickness) || thickness <= 0.0) {
    return false;
  }
  const double halfThickness = thickness * 0.5;
  if (start.z == end.z) {
    bounds = {{static_cast<double>(std::min(start.x, end.x)), base,
               start.z - halfThickness},
              {static_cast<double>(std::max(start.x, end.x)), base + height,
               start.z + halfThickness}};
  } else {
    bounds = {{start.x - halfThickness, base,
               static_cast<double>(std::min(start.z, end.z))},
              {start.x + halfThickness, base + height,
               static_cast<double>(std::max(start.z, end.z))}};
  }
  return measureCreativeBounds(bounds).valid;
}

bool planDoorSweep(const CreativeWorldLayout& layout,
                   const CreativeWorldLayoutOpening& opening,
                   CreativeBounds& sweepBounds) noexcept {
  const CreativeWorldLayoutOpeningHostFrame host =
      resolveCreativeWorldLayoutOpeningHost(layout, opening);
  if (!host.accepted || opening.kind != CreativeBuildingOpeningKind::Door ||
      !isValidCreativeDoorSettings(opening.door)) {
    return false;
  }
  CreativeStructuralWallOpeningRequest cutout;
  cutout.sortKey = opening.stableKey;
  cutout.centerOffsetMeters = opening.centerOffsetCells;
  cutout.widthMeters = opening.widthCells;
  cutout.cutoutBottomMeters = opening.cutoutBottomCells;
  cutout.cutoutHeightMeters = opening.cutoutHeightCells;
  cutout.includeInsert = true;
  const std::array cutouts{cutout};
  const CreativeStructuralWallRecipeResult wall = planCreativeStructuralWall(
      {{static_cast<double>(host.start.x), host.baseLayer,
        static_cast<double>(host.start.z)},
       {static_cast<double>(host.end.x), host.baseLayer,
        static_cast<double>(host.end.z)},
       host.wallHeightCells, host.wallThicknessCells, 0.0, 0.0, cutouts});
  if (!wall.accepted || wall.openings.size() != 1U) {
    return false;
  }
  const CreativeDoorRecipeResult door =
      planCreativeDoor({wall.frame, wall.openings[0].cutoutBounds, opening.door});
  if (!door.accepted) {
    return false;
  }
  sweepBounds = door.fullSweepBounds;
  return measureCreativeBounds(sweepBounds).valid;
}

double segmentLength(CreativeTerrainCoord2 start,
                     CreativeTerrainCoord2 end) noexcept {
  return std::hypot(static_cast<double>(end.x) - start.x,
                    static_cast<double>(end.z) - start.z);
}

std::pair<CreativeTerrainCoord2, CreativeTerrainCoord2> roomEdgeSegment(
    const CreativeWorldLayoutRoom& room,
    CreativeWorldLayoutRoomEdge edge) noexcept {
  switch (edge) {
    case CreativeWorldLayoutRoomEdge::North:
      return {{room.footprint.minimum.x, room.footprint.minimum.z},
              {room.footprint.maximum.x, room.footprint.minimum.z}};
    case CreativeWorldLayoutRoomEdge::East:
      return {{room.footprint.maximum.x, room.footprint.minimum.z},
              {room.footprint.maximum.x, room.footprint.maximum.z}};
    case CreativeWorldLayoutRoomEdge::South:
      return {{room.footprint.minimum.x, room.footprint.maximum.z},
              {room.footprint.maximum.x, room.footprint.maximum.z}};
    case CreativeWorldLayoutRoomEdge::West:
      return {{room.footprint.minimum.x, room.footprint.minimum.z},
              {room.footprint.minimum.x, room.footprint.maximum.z}};
    case CreativeWorldLayoutRoomEdge::Count:
      break;
  }
  return {};
}

CreativeWorldLayoutRoomEdge cardinalRoomEdge(
    const CreativeWorldLayoutRoom& room, CreativeTerrainCoord2 start,
    CreativeTerrainCoord2 end) noexcept {
  if (start.z == room.footprint.minimum.z &&
      end.z == room.footprint.minimum.z) {
    return CreativeWorldLayoutRoomEdge::North;
  }
  if (start.x == room.footprint.maximum.x &&
      end.x == room.footprint.maximum.x) {
    return CreativeWorldLayoutRoomEdge::East;
  }
  if (start.z == room.footprint.maximum.z &&
      end.z == room.footprint.maximum.z) {
    return CreativeWorldLayoutRoomEdge::South;
  }
  if (start.x == room.footprint.minimum.x &&
      end.x == room.footprint.minimum.x) {
    return CreativeWorldLayoutRoomEdge::West;
  }
  return CreativeWorldLayoutRoomEdge::Count;
}

bool edgeBelongsToRoom(const CreativeWorldLayout& layout,
                       std::size_t roomIndex,
                       std::size_t edgeIndex) noexcept {
  return std::any_of(
      layout.roomBoundaries.begin(), layout.roomBoundaries.end(),
      [roomIndex, edgeIndex](const CreativeWorldLayoutRoomBoundary& boundary) {
        return boundary.roomIndex == roomIndex &&
               boundary.topologyEdgeIndex == edgeIndex;
      });
}

std::size_t edgeOwnerCount(const CreativeWorldLayout& layout,
                           std::size_t edgeIndex) noexcept {
  return static_cast<std::size_t>(std::count_if(
      layout.roomBoundaries.begin(), layout.roomBoundaries.end(),
      [edgeIndex](const CreativeWorldLayoutRoomBoundary& boundary) {
        return boundary.topologyEdgeIndex == edgeIndex;
      }));
}

std::size_t firstEdgeOwner(const CreativeWorldLayout& layout,
                           std::size_t edgeIndex) noexcept {
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  for (const CreativeWorldLayoutRoomBoundary& boundary :
       layout.roomBoundaries) {
    if (boundary.topologyEdgeIndex == edgeIndex) {
      roomIndex = std::min(roomIndex, boundary.roomIndex);
    }
  }
  return roomIndex;
}

bool validOpeningKind(CreativeBuildingOpeningKind kind) noexcept {
  return kind == CreativeBuildingOpeningKind::Door ||
         kind == CreativeBuildingOpeningKind::Window;
}

bool validOpeningFacing(CreativeBuildingOpeningFacing facing) noexcept {
  return facing < CreativeBuildingOpeningFacing::Count;
}

bool validInsertAsset(const CreativeWorldLayoutOpening& opening) noexcept {
  const CreativeBoundsMetrics source =
      measureCreativeBounds(opening.insertAssetSourceBoundsMeters);
  return opening.hasInsertAssetSourceBounds
             ? !opening.insertAssetId.empty() && source.valid &&
                   isPositiveCreativeVec3(source.size)
             : opening.insertAssetId.empty();
}

bool intervalsOverlap(double firstMinimum, double firstMaximum,
                      double secondMinimum, double secondMaximum) noexcept {
  return firstMinimum <= secondMaximum + kGeometryEpsilon &&
         secondMinimum <= firstMaximum + kGeometryEpsilon;
}

bool openingsOverlap(
    const CreativeWorldLayoutOpening& candidate,
    const CreativeWorldLayoutOpeningHostFrame& candidateHost,
    const CreativeWorldLayoutOpening& existing,
    const CreativeWorldLayoutOpeningHostFrame& existingHost) noexcept {
  if (!candidateHost.accepted || !existingHost.accepted) {
    return false;
  }
  const double candidateDx =
      static_cast<double>(candidateHost.end.x) - candidateHost.start.x;
  const double candidateDz =
      static_cast<double>(candidateHost.end.z) - candidateHost.start.z;
  const double existingDx =
      static_cast<double>(existingHost.end.x) - existingHost.start.x;
  const double existingDz =
      static_cast<double>(existingHost.end.z) - existingHost.start.z;
  const double parallelCross = candidateDx * existingDz -
                               candidateDz * existingDx;
  const double lineCross =
      candidateDx *
          (static_cast<double>(existingHost.start.z) - candidateHost.start.z) -
      candidateDz *
          (static_cast<double>(existingHost.start.x) - candidateHost.start.x);
  if (std::fabs(parallelCross) > kGeometryEpsilon ||
      std::fabs(lineCross) > kGeometryEpsilon) {
    return false;
  }

  const CreativeWorldLayoutOpeningHostPoint existingCenter =
      creativeWorldLayoutOpeningHostPoint(existingHost,
                                          existing.centerOffsetCells);
  const double existingOffset = creativeWorldLayoutOpeningHostOffset(
      candidateHost, existingCenter);
  const bool horizontal =
      std::isfinite(existingOffset) &&
      intervalsOverlap(candidate.centerOffsetCells - candidate.widthCells * 0.5,
                       candidate.centerOffsetCells + candidate.widthCells * 0.5,
                       existingOffset - existing.widthCells * 0.5,
                       existingOffset + existing.widthCells * 0.5);
  const double candidateBottom =
      candidateHost.baseLayer + candidate.cutoutBottomCells;
  const double existingBottom =
      existingHost.baseLayer + existing.cutoutBottomCells;
  return horizontal &&
         intervalsOverlap(candidateBottom,
                          candidateBottom + candidate.cutoutHeightCells,
                          existingBottom,
                          existingBottom + existing.cutoutHeightCells);
}

void considerHost(CreativeWorldLayoutOpeningHostHit& best,
                  CreativeWorldLayoutOpeningHostPoint point,
                  double toleranceCells,
                  const CreativeWorldLayoutOpening& opening,
                  const CreativeWorldLayoutOpeningHostFrame& host) noexcept {
  if (!std::isfinite(host.lengthCells) || host.lengthCells <= 0.0) {
    return;
  }
  const double dx = static_cast<double>(host.end.x) - host.start.x;
  const double dz = static_cast<double>(host.end.z) - host.start.z;
  const double lengthSquared = dx * dx + dz * dz;
  const double t = std::clamp(
      ((point.x - host.start.x) * dx + (point.z - host.start.z) * dz) /
          lengthSquared,
      0.0, 1.0);
  const double projectedX = host.start.x + t * dx;
  const double projectedZ = host.start.z + t * dz;
  const double distance =
      std::hypot(point.x - projectedX, point.z - projectedZ);
  if (distance >= best.distanceCells) {
    return;
  }
  best.hit = distance <= toleranceCells;
  best.hostKind = opening.hostKind;
  best.wallIndex = opening.wallIndex;
  best.roomIndex = opening.roomIndex;
  best.topologyEdgeIndex = opening.roomTopologyEdgeIndex;
  best.roomEdge = host.roomEdge;
  best.centerOffsetCells = t * host.lengthCells;
  best.distanceCells = distance;
  best.host = host;
}

}  // namespace

CreativeWorldLayoutOpeningHostFrame resolveCreativeWorldLayoutOpeningHost(
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutOpening& opening) noexcept {
  CreativeWorldLayoutOpeningHostFrame result;
  result.requested = true;
  result.hostKind = opening.hostKind;
  result.wallIndex = opening.wallIndex;
  result.roomIndex = opening.roomIndex;
  result.topologyEdgeIndex = opening.roomTopologyEdgeIndex;
  result.roomEdge = opening.roomEdge;
  if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::Wall) {
    if (opening.wallIndex >= layout.walls.size()) {
      reject(result, CreativeWorldLayoutOpeningHostStatus::MissingHost,
             "creative_world_layout_opening_wall_host_missing");
      return result;
    }
    const CreativeWorldLayoutWall& wall = layout.walls[opening.wallIndex];
    result.buildingIndex = wall.buildingIndex;
    result.start = wall.start;
    result.end = wall.end;
    result.baseLayer = wall.baseLayer;
    result.wallHeightCells = static_cast<double>(wall.heightCells);
    result.wallThicknessCells = wall.thicknessCells;
    result.exterior = true;
  } else if (opening.hostKind ==
             CreativeWorldLayoutOpeningHostKind::RoomEdge) {
    if (opening.roomIndex >= layout.rooms.size()) {
      reject(result, CreativeWorldLayoutOpeningHostStatus::MissingHost,
             "creative_world_layout_opening_room_host_missing");
      return result;
    }
    const CreativeWorldLayoutRoom& room = layout.rooms[opening.roomIndex];
    result.buildingIndex = room.buildingIndex;
    result.levelIndex = room.levelIndex;
    if (room.levelIndex >= layout.levels.size()) {
      reject(result, CreativeWorldLayoutOpeningHostStatus::MissingHost,
             "creative_world_layout_opening_level_host_missing");
      return result;
    }
    const CreativeWorldLayoutLevel& level = layout.levels[room.levelIndex];
    if (level.buildingIndex != room.buildingIndex) {
      reject(result, CreativeWorldLayoutOpeningHostStatus::MissingHost,
             "creative_world_layout_opening_level_host_mismatch");
      return result;
    }
    result.baseLayer = level.floorTopLayer;
    if (opening.roomTopologyEdgeIndex != kInvalidCreativeWorldLayoutIndex) {
      if (opening.roomTopologyEdgeIndex >= layout.topologyEdges.size() ||
          !edgeBelongsToRoom(layout, opening.roomIndex,
                             opening.roomTopologyEdgeIndex)) {
        reject(result, CreativeWorldLayoutOpeningHostStatus::MissingHost,
               "creative_world_layout_opening_topology_host_missing");
        return result;
      }
      const CreativeWorldLayoutTopologyEdge& edge =
          layout.topologyEdges[opening.roomTopologyEdgeIndex];
      if (edge.levelIndex != room.levelIndex ||
          edge.startVertexIndex >= layout.topologyVertices.size() ||
          edge.endVertexIndex >= layout.topologyVertices.size()) {
        reject(result, CreativeWorldLayoutOpeningHostStatus::MissingHost,
               "creative_world_layout_opening_topology_host_invalid");
        return result;
      }
      const CreativeWorldLayoutTopologyVertex& start =
          layout.topologyVertices[edge.startVertexIndex];
      const CreativeWorldLayoutTopologyVertex& end =
          layout.topologyVertices[edge.endVertexIndex];
      if (start.levelIndex != edge.levelIndex ||
          end.levelIndex != edge.levelIndex) {
        reject(result, CreativeWorldLayoutOpeningHostStatus::MissingHost,
               "creative_world_layout_opening_topology_vertex_invalid");
        return result;
      }
      result.start = start.position;
      result.end = end.position;
      result.wallThicknessCells = edge.wallThicknessCells;
      result.owningRoomCount =
          edgeOwnerCount(layout, opening.roomTopologyEdgeIndex);
      result.exterior = result.owningRoomCount == 1U;
      result.wallHeightCells =
          edge.wallHeightCells != 0U
              ? static_cast<double>(edge.wallHeightCells)
              : result.exterior
                    ? creativeWorldLayoutLevelFacadeHeightCells(
                          layout, room.levelIndex)
                    : static_cast<double>(level.wallHeightCells);
      result.roomEdge = cardinalRoomEdge(room, result.start, result.end);
    } else {
      if (opening.roomEdge >= CreativeWorldLayoutRoomEdge::Count) {
        reject(result, CreativeWorldLayoutOpeningHostStatus::MissingHost,
               "creative_world_layout_opening_legacy_room_host_invalid");
        return result;
      }
      const auto segment = roomEdgeSegment(room, opening.roomEdge);
      result.start = segment.first;
      result.end = segment.second;
      result.wallThicknessCells = room.wallThicknessCells;
      result.owningRoomCount =
          creativeWorldLayoutRoomEdgeIntervalIsShared(
              layout, opening.roomIndex, opening.roomEdge,
              opening.centerOffsetCells, opening.widthCells)
              ? 2U
              : 1U;
      result.exterior = result.owningRoomCount == 1U;
      result.wallHeightCells =
          result.exterior
              ? creativeWorldLayoutLevelFacadeHeightCells(layout,
                                                          room.levelIndex)
              : static_cast<double>(level.wallHeightCells);
    }
  } else {
    reject(result, CreativeWorldLayoutOpeningHostStatus::InvalidOpening,
           "creative_world_layout_opening_host_kind_invalid");
    return result;
  }

  result.lengthCells = segmentLength(result.start, result.end);
  if (!cardinalSegment(result.start, result.end)) {
    reject(result,
           CreativeWorldLayoutOpeningHostStatus::UnsupportedOrientation,
           "creative_world_layout_opening_host_orientation_unsupported");
    return result;
  }
  if (!std::isfinite(result.lengthCells) || result.lengthCells <= 0.0 ||
      !std::isfinite(result.baseLayer) ||
      !std::isfinite(result.wallHeightCells) ||
      result.wallHeightCells <= 0.0 ||
      !std::isfinite(result.wallThicknessCells) ||
      result.wallThicknessCells <= 0.0 ||
      !std::isfinite(result.baseLayer + result.wallHeightCells)) {
    reject(result, CreativeWorldLayoutOpeningHostStatus::InvalidHostGeometry,
           "creative_world_layout_opening_host_geometry_invalid");
    return result;
  }
  result.accepted = true;
  result.status = CreativeWorldLayoutOpeningHostStatus::Ready;
  result.reasonCode = "creative_world_layout_opening_host_ready";
  return result;
}

double creativeWorldLayoutOpeningHostOffset(
    const CreativeWorldLayoutOpeningHostFrame& host,
    CreativeWorldLayoutOpeningHostPoint point) noexcept {
  if (!host.accepted || !std::isfinite(point.x) || !std::isfinite(point.z)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double dx = static_cast<double>(host.end.x) - host.start.x;
  const double dz = static_cast<double>(host.end.z) - host.start.z;
  return ((point.x - host.start.x) * dx + (point.z - host.start.z) * dz) /
         host.lengthCells;
}

CreativeWorldLayoutOpeningHostPoint creativeWorldLayoutOpeningHostPoint(
    const CreativeWorldLayoutOpeningHostFrame& host,
    double offsetCells) noexcept {
  if (!host.accepted || !std::isfinite(offsetCells)) {
    return {};
  }
  const double scale = offsetCells / host.lengthCells;
  return {host.start.x + (host.end.x - host.start.x) * scale,
          host.start.z + (host.end.z - host.start.z) * scale};
}

CreativeWorldLayoutOpeningHostHit findNearestCreativeWorldLayoutOpeningHost(
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutOpeningHostPoint point,
    double toleranceCells,
    std::size_t activeLevelIndex) noexcept {
  CreativeWorldLayoutOpeningHostHit best;
  best.requested = true;
  best.distanceCells = std::numeric_limits<double>::infinity();
  if (!std::isfinite(point.x) || !std::isfinite(point.z) ||
      !std::isfinite(toleranceCells) || toleranceCells <= 0.0) {
    return best;
  }
  const CreativeWorldLayoutLevel* activeLevel =
      activeLevelIndex < layout.levels.size()
          ? &layout.levels[activeLevelIndex]
          : nullptr;
  for (std::size_t wallIndex = 0U; wallIndex < layout.walls.size();
       ++wallIndex) {
    const CreativeWorldLayoutWall& wall = layout.walls[wallIndex];
    if (activeLevel != nullptr &&
        (wall.buildingIndex != activeLevel->buildingIndex ||
         std::fabs(wall.baseLayer - activeLevel->floorTopLayer) >
             kGeometryEpsilon)) {
      continue;
    }
    CreativeWorldLayoutOpening opening;
    opening.hostKind = CreativeWorldLayoutOpeningHostKind::Wall;
    opening.wallIndex = wallIndex;
    const CreativeWorldLayoutOpeningHostFrame host =
        resolveCreativeWorldLayoutOpeningHost(layout, opening);
    considerHost(best, point, toleranceCells, opening, host);
  }

  if (!layout.topologyEdges.empty() && !layout.roomBoundaries.empty()) {
    for (std::size_t edgeIndex = 0U; edgeIndex < layout.topologyEdges.size();
         ++edgeIndex) {
      const CreativeWorldLayoutTopologyEdge& edge =
          layout.topologyEdges[edgeIndex];
      if (activeLevel != nullptr && edge.levelIndex != activeLevelIndex) {
        continue;
      }
      const std::size_t roomIndex = firstEdgeOwner(layout, edgeIndex);
      if (roomIndex >= layout.rooms.size()) {
        continue;
      }
      CreativeWorldLayoutOpening opening;
      opening.hostKind = CreativeWorldLayoutOpeningHostKind::RoomEdge;
      opening.roomIndex = roomIndex;
      opening.roomTopologyEdgeIndex = edgeIndex;
      opening.roomEdge = CreativeWorldLayoutRoomEdge::Count;
      const CreativeWorldLayoutOpeningHostFrame host =
          resolveCreativeWorldLayoutOpeningHost(layout, opening);
      considerHost(best, point, toleranceCells, opening, host);
    }
    return best;
  }

  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
    if (activeLevel != nullptr &&
        layout.rooms[roomIndex].levelIndex != activeLevelIndex) {
      continue;
    }
    for (std::uint8_t value = 0U;
         value < static_cast<std::uint8_t>(CreativeWorldLayoutRoomEdge::Count);
         ++value) {
      CreativeWorldLayoutOpening opening;
      opening.hostKind = CreativeWorldLayoutOpeningHostKind::RoomEdge;
      opening.roomIndex = roomIndex;
      opening.roomEdge = static_cast<CreativeWorldLayoutRoomEdge>(value);
      const CreativeWorldLayoutOpeningHostFrame host =
          resolveCreativeWorldLayoutOpeningHost(layout, opening);
      considerHost(best, point, toleranceCells, opening, host);
    }
  }
  return best;
}

CreativeWorldLayoutDoorSwingClearanceResult
evaluateCreativeWorldLayoutDoorSwingClearance(
    const CreativeWorldLayoutDoorSwingClearanceRequest& request) noexcept {
  CreativeWorldLayoutDoorSwingClearanceResult result;
  result.requested = true;
  if (request.layout == nullptr || request.opening == nullptr) {
    reject(result, CreativeWorldLayoutDoorSwingClearanceStatus::InvalidRequest,
           "creative_world_layout_door_swing_request_invalid");
    return result;
  }
  const CreativeWorldLayout& layout = *request.layout;
  const CreativeWorldLayoutOpening& opening = *request.opening;
  if (opening.kind != CreativeBuildingOpeningKind::Door) {
    result.accepted = true;
    result.status = CreativeWorldLayoutDoorSwingClearanceStatus::NotDoor;
    result.reasonCode = "creative_world_layout_door_swing_not_door";
    return result;
  }
  if (!planDoorSweep(layout, opening, result.sweepBoundsCells)) {
    reject(result, CreativeWorldLayoutDoorSwingClearanceStatus::InvalidGeometry,
           "creative_world_layout_door_swing_geometry_invalid");
    return result;
  }
  result.accepted = true;

  for (std::size_t index = 0U; index < layout.walls.size(); ++index) {
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::Wall &&
        opening.wallIndex == index) {
      continue;
    }
    const CreativeWorldLayoutWall& wall = layout.walls[index];
    CreativeBounds obstacle;
    if (!segmentBounds(wall.start, wall.end, wall.baseLayer,
                       static_cast<double>(wall.heightCells),
                       wall.thicknessCells, obstacle)) {
      continue;
    }
    if (boundsOverlap(result.sweepBoundsCells, obstacle)) {
      result.conflictingWallIndex = index;
      result.status =
          CreativeWorldLayoutDoorSwingClearanceStatus::WallObstructed;
      result.reasonCode = "creative_world_layout_door_swing_wall_obstructed";
      return result;
    }
  }

  for (std::size_t index = 0U; index < layout.topologyEdges.size(); ++index) {
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        opening.roomTopologyEdgeIndex == index) {
      continue;
    }
    const CreativeWorldLayoutTopologyEdge& edge = layout.topologyEdges[index];
    if (edge.levelIndex >= layout.levels.size() ||
        edge.startVertexIndex >= layout.topologyVertices.size() ||
        edge.endVertexIndex >= layout.topologyVertices.size()) {
      continue;
    }
    const CreativeWorldLayoutLevel& level = layout.levels[edge.levelIndex];
    const double inheritedHeight =
        creativeWorldLayoutLevelFacadeHeightCells(layout, edge.levelIndex);
    const double height =
        edge.wallHeightCells != 0U
            ? static_cast<double>(edge.wallHeightCells)
            : (std::isfinite(inheritedHeight)
                   ? inheritedHeight
                   : static_cast<double>(level.wallHeightCells));
    CreativeBounds obstacle;
    if (!segmentBounds(layout.topologyVertices[edge.startVertexIndex].position,
                       layout.topologyVertices[edge.endVertexIndex].position,
                       level.floorTopLayer, height,
                       edge.wallThicknessCells, obstacle)) {
      continue;
    }
    if (boundsOverlap(result.sweepBoundsCells, obstacle)) {
      result.conflictingTopologyEdgeIndex = index;
      result.status =
          CreativeWorldLayoutDoorSwingClearanceStatus::WallObstructed;
      result.reasonCode = "creative_world_layout_door_swing_wall_obstructed";
      return result;
    }
  }

  for (std::size_t index = 0U; index < layout.openings.size(); ++index) {
    const CreativeWorldLayoutOpening& other = layout.openings[index];
    if (index == request.ignoredOpeningIndex || &other == request.opening ||
        other.kind != CreativeBuildingOpeningKind::Door) {
      continue;
    }
    CreativeBounds otherSweep;
    if (!planDoorSweep(layout, other, otherSweep)) {
      continue;
    }
    if (boundsOverlap(result.sweepBoundsCells, otherSweep)) {
      result.conflictingOpeningIndex = index;
      result.status =
          CreativeWorldLayoutDoorSwingClearanceStatus::DoorObstructed;
      result.reasonCode = "creative_world_layout_door_swing_door_obstructed";
      return result;
    }
  }

  for (std::size_t index = 0U; index < layout.verticalConnectors.size();
       ++index) {
    const CreativeWorldLayoutVerticalConnector& connector =
        layout.verticalConnectors[index];
    if (connector.lowerRoomIndex >= layout.rooms.size() ||
        connector.upperRoomIndex >= layout.rooms.size()) {
      continue;
    }
    const CreativeWorldLayoutRoom& lowerRoom =
        layout.rooms[connector.lowerRoomIndex];
    const CreativeWorldLayoutRoom& upperRoom =
        layout.rooms[connector.upperRoomIndex];
    if (lowerRoom.levelIndex >= layout.levels.size() ||
        upperRoom.levelIndex >= layout.levels.size() ||
        connector.footprint.maximum.x <= connector.footprint.minimum.x ||
        connector.footprint.maximum.z <= connector.footprint.minimum.z) {
      continue;
    }
    const double lower =
        std::min(layout.levels[lowerRoom.levelIndex].floorTopLayer,
                 layout.levels[upperRoom.levelIndex].floorTopLayer);
    const double upper =
        std::max(layout.levels[lowerRoom.levelIndex].floorTopLayer,
                 layout.levels[upperRoom.levelIndex].floorTopLayer);
    const CreativeBounds obstacle{
        {static_cast<double>(connector.footprint.minimum.x), lower,
         static_cast<double>(connector.footprint.minimum.z)},
        {static_cast<double>(connector.footprint.maximum.x), upper,
         static_cast<double>(connector.footprint.maximum.z)}};
    if (boundsOverlap(result.sweepBoundsCells, obstacle)) {
      result.conflictingVerticalConnectorIndex = index;
      result.status =
          CreativeWorldLayoutDoorSwingClearanceStatus::CriticalPathObstructed;
      result.reasonCode =
          "creative_world_layout_door_swing_critical_path_obstructed";
      return result;
    }
  }

  result.clear = true;
  result.status = CreativeWorldLayoutDoorSwingClearanceStatus::Ready;
  result.reasonCode = "creative_world_layout_door_swing_ready";
  return result;
}

CreativeWorldLayoutOpeningValidationResult validateCreativeWorldLayoutOpening(
    const CreativeWorldLayoutOpeningValidationRequest& request) noexcept {
  CreativeWorldLayoutOpeningValidationResult result;
  result.requested = true;
  if (request.layout == nullptr || request.opening == nullptr ||
      !std::isfinite(request.minimumEndClearanceCells) ||
      request.minimumEndClearanceCells < 0.0 ||
      !std::isfinite(request.minimumWidthCells) ||
      request.minimumWidthCells <= 0.0) {
    reject(result, CreativeWorldLayoutOpeningValidationStatus::InvalidRequest,
           "creative_world_layout_opening_validation_request_invalid");
    return result;
  }
  const CreativeWorldLayout& layout = *request.layout;
  const CreativeWorldLayoutOpening& opening = *request.opening;
  if (!validOpeningKind(opening.kind)) {
    reject(result, CreativeWorldLayoutOpeningValidationStatus::InvalidKind,
           "creative_world_layout_opening_kind_invalid");
    return result;
  }
  if (opening.kind == CreativeBuildingOpeningKind::Door &&
      !isValidCreativeDoorSettings(opening.door)) {
    reject(result,
           CreativeWorldLayoutOpeningValidationStatus::InvalidDoorSettings,
           "creative_world_layout_door_settings_invalid");
    return result;
  }
  if (opening.kind == CreativeBuildingOpeningKind::Window &&
      !isValidCreativeWindowSettings(opening.window)) {
    reject(result,
           CreativeWorldLayoutOpeningValidationStatus::InvalidWindowSettings,
           "creative_world_layout_window_settings_invalid");
    return result;
  }
  if (!validOpeningFacing(opening.facing)) {
    reject(result,
           CreativeWorldLayoutOpeningValidationStatus::InvalidFacing,
           "creative_world_layout_opening_facing_invalid");
    return result;
  }
  if (!std::isfinite(opening.centerOffsetCells) ||
      !std::isfinite(opening.widthCells) ||
      !std::isfinite(opening.cutoutBottomCells) ||
      !std::isfinite(opening.cutoutHeightCells) ||
      opening.widthCells < request.minimumWidthCells ||
      opening.cutoutBottomCells < 0.0 || opening.cutoutHeightCells <= 0.0) {
    reject(result, CreativeWorldLayoutOpeningValidationStatus::InvalidCutout,
           "creative_world_layout_opening_cutout_invalid");
    return result;
  }
  if (!validInsertAsset(opening) ||
      !std::isfinite(opening.insertBottomCells) ||
      !std::isfinite(opening.insertHeightCells) ||
      !std::isfinite(opening.insertWidthCells) ||
      !std::isfinite(opening.insertThicknessCells) ||
      opening.insertBottomCells < 0.0 || opening.insertHeightCells < 0.0 ||
      opening.insertWidthCells < 0.0 ||
      opening.insertThicknessCells < 0.0) {
    reject(result, CreativeWorldLayoutOpeningValidationStatus::InvalidInsert,
           "creative_world_layout_opening_insert_invalid");
    return result;
  }
  if (opening.kind == CreativeBuildingOpeningKind::Door &&
      std::fabs(opening.cutoutBottomCells) > kGeometryEpsilon) {
    reject(result,
           CreativeWorldLayoutOpeningValidationStatus::DoorSillInvalid,
           "creative_world_layout_door_sill_invalid");
    return result;
  }
  result.host = resolveCreativeWorldLayoutOpeningHost(layout, opening);
  if (!result.host.accepted) {
    const auto status =
        result.host.status ==
                CreativeWorldLayoutOpeningHostStatus::UnsupportedOrientation
            ? CreativeWorldLayoutOpeningValidationStatus::
                  UnsupportedHostOrientation
            : CreativeWorldLayoutOpeningValidationStatus::MissingHost;
    reject(result, status, result.host.reasonCode);
    return result;
  }
  const double halfWidth = opening.widthCells * 0.5;
  if (opening.centerOffsetCells - halfWidth <
          request.minimumEndClearanceCells - kGeometryEpsilon ||
      opening.centerOffsetCells + halfWidth >
          result.host.lengthCells - request.minimumEndClearanceCells +
              kGeometryEpsilon) {
    reject(result,
           CreativeWorldLayoutOpeningValidationStatus::EndClearanceInvalid,
           "creative_world_layout_opening_end_clearance_invalid");
    return result;
  }
  result.sillTopCells = opening.cutoutBottomCells;
  result.lintelBottomCells =
      opening.cutoutBottomCells + opening.cutoutHeightCells;
  result.lintelHeightCells =
      result.host.wallHeightCells - result.lintelBottomCells;
  if (result.lintelHeightCells < -kGeometryEpsilon) {
    reject(result,
           CreativeWorldLayoutOpeningValidationStatus::WallHeightExceeded,
           "creative_world_layout_opening_height_invalid");
    return result;
  }
  if (opening.includeInsert) {
    const double insertBottom =
        opening.kind == CreativeBuildingOpeningKind::Window &&
                opening.insertBottomCells == 0.0
            ? opening.cutoutBottomCells
            : opening.insertBottomCells;
    const double insertHeight = opening.insertHeightCells > 0.0
                                    ? opening.insertHeightCells
                                    : opening.cutoutHeightCells;
    const double insertWidth = opening.insertWidthCells > 0.0
                                   ? opening.insertWidthCells
                                   : opening.widthCells;
    if (insertWidth > opening.widthCells + kGeometryEpsilon ||
        insertBottom + kGeometryEpsilon < opening.cutoutBottomCells ||
        insertBottom + insertHeight > result.lintelBottomCells +
                                          kGeometryEpsilon) {
      reject(result,
             CreativeWorldLayoutOpeningValidationStatus::InsertDoesNotFit,
             "creative_world_layout_opening_insert_fit_invalid");
      return result;
    }
  }
  if (opening.kind == CreativeBuildingOpeningKind::Window &&
      !result.host.exterior) {
    reject(result,
           CreativeWorldLayoutOpeningValidationStatus::InteriorWindow,
           "creative_world_layout_window_requires_exterior");
    return result;
  }
  for (std::size_t index = 0U; index < layout.openings.size(); ++index) {
    if (index == request.ignoredOpeningIndex) {
      continue;
    }
    const CreativeWorldLayoutOpening& existing = layout.openings[index];
    if (!validOpeningKind(existing.kind) ||
        (existing.kind == CreativeBuildingOpeningKind::Door &&
         !isValidCreativeDoorSettings(existing.door)) ||
        (existing.kind == CreativeBuildingOpeningKind::Window &&
         !isValidCreativeWindowSettings(existing.window)) ||
        !validOpeningFacing(existing.facing) ||
        !std::isfinite(existing.centerOffsetCells) ||
        !std::isfinite(existing.widthCells) || existing.widthCells <= 0.0 ||
        !std::isfinite(existing.cutoutBottomCells) ||
        existing.cutoutBottomCells < 0.0 ||
        !std::isfinite(existing.cutoutHeightCells) ||
        existing.cutoutHeightCells <= 0.0) {
      reject(result, CreativeWorldLayoutOpeningValidationStatus::InvalidCutout,
             "creative_world_layout_existing_opening_invalid");
      result.conflictingOpeningIndex = index;
      return result;
    }
    const CreativeWorldLayoutOpeningHostFrame existingHost =
        resolveCreativeWorldLayoutOpeningHost(layout, existing);
    if (!existingHost.accepted) {
      reject(result, CreativeWorldLayoutOpeningValidationStatus::MissingHost,
             "creative_world_layout_existing_opening_host_invalid");
      result.conflictingOpeningIndex = index;
      return result;
    }
    if (openingsOverlap(opening, result.host, existing, existingHost)) {
      reject(result, CreativeWorldLayoutOpeningValidationStatus::Overlap,
             "creative_world_layout_opening_overlap");
      result.conflictingOpeningIndex = index;
      return result;
    }
  }
  if (opening.kind == CreativeBuildingOpeningKind::Door) {
    const CreativeWorldLayoutDoorSwingClearanceResult swing =
        evaluateCreativeWorldLayoutDoorSwingClearance(
            {&layout, &opening, request.ignoredOpeningIndex});
    result.doorSweepBoundsCells = swing.sweepBoundsCells;
    result.conflictingWallIndex = swing.conflictingWallIndex;
    result.conflictingTopologyEdgeIndex =
        swing.conflictingTopologyEdgeIndex;
    result.conflictingOpeningIndex = swing.conflictingOpeningIndex;
    result.conflictingVerticalConnectorIndex =
        swing.conflictingVerticalConnectorIndex;
    if (!swing.accepted) {
      reject(result,
             CreativeWorldLayoutOpeningValidationStatus::InvalidDoorSettings,
             swing.reasonCode);
      return result;
    }
    if (!swing.clear) {
      reject(result,
             CreativeWorldLayoutOpeningValidationStatus::DoorSwingObstructed,
             swing.reasonCode);
      return result;
    }
  }
  result.accepted = true;
  result.status = CreativeWorldLayoutOpeningValidationStatus::Ready;
  result.reasonCode = "creative_world_layout_opening_ready";
  return result;
}

bool validCreativeWorldLayoutOpenings(
    const CreativeWorldLayout& layout) noexcept {
  for (std::size_t openingIndex = 0U;
       openingIndex < layout.openings.size(); ++openingIndex) {
    if (!validateCreativeWorldLayoutOpening(
             {&layout, &layout.openings[openingIndex], openingIndex})
             .accepted) {
      return false;
    }
  }
  return true;
}

CreativeWorldLayoutOpeningClearanceResult
evaluateCreativeWorldLayoutOpeningClearance(
    const CreativeWorldLayoutOpeningClearanceRequest& request) noexcept {
  CreativeWorldLayoutOpeningClearanceResult result;
  result.requested = true;
  if (request.opening == nullptr ||
      !std::isfinite(request.gridCellSizeMeters) ||
      request.gridCellSizeMeters <= 0.0 ||
      !std::isfinite(request.actorRadiusMeters) ||
      request.actorRadiusMeters <= 0.0 ||
      !std::isfinite(request.actorHeightMeters) ||
      request.actorHeightMeters <= request.actorRadiusMeters * 2.0 ||
      !std::isfinite(request.maximumStepMeters) ||
      request.maximumStepMeters < 0.0 || !std::isfinite(request.skinMeters) ||
      request.skinMeters < 0.0) {
    reject(result, CreativeWorldLayoutOpeningClearanceStatus::InvalidRequest,
           "creative_world_layout_opening_clearance_request_invalid");
    return result;
  }
  const CreativeWorldLayoutOpening& opening = *request.opening;
  result.accepted = true;
  result.clearWidthMeters = opening.widthCells * request.gridCellSizeMeters;
  result.clearHeightMeters =
      opening.cutoutHeightCells * request.gridCellSizeMeters;
  result.thresholdMeters =
      opening.cutoutBottomCells * request.gridCellSizeMeters;
  result.minimumWidthMeters =
      request.actorRadiusMeters * 2.0 + request.skinMeters * 2.0;
  result.minimumHeightMeters = request.actorHeightMeters + request.skinMeters;
  if (opening.kind != CreativeBuildingOpeningKind::Door) {
    result.status = CreativeWorldLayoutOpeningClearanceStatus::NotPassage;
    result.reasonCode =
        "creative_world_layout_opening_clearance_not_passage";
    return result;
  }
  if (!std::isfinite(result.clearWidthMeters) ||
      result.clearWidthMeters + kGeometryEpsilon < result.minimumWidthMeters) {
    result.status =
        CreativeWorldLayoutOpeningClearanceStatus::WidthObstructed;
    result.reasonCode =
        "creative_world_layout_opening_clearance_width_obstructed";
    return result;
  }
  if (!std::isfinite(result.clearHeightMeters) ||
      result.clearHeightMeters + kGeometryEpsilon <
          result.minimumHeightMeters) {
    result.status =
        CreativeWorldLayoutOpeningClearanceStatus::HeightObstructed;
    result.reasonCode =
        "creative_world_layout_opening_clearance_height_obstructed";
    return result;
  }
  if (!std::isfinite(result.thresholdMeters) ||
      result.thresholdMeters > request.maximumStepMeters + kGeometryEpsilon) {
    result.status =
        CreativeWorldLayoutOpeningClearanceStatus::ThresholdObstructed;
    result.reasonCode =
        "creative_world_layout_opening_clearance_threshold_obstructed";
    return result;
  }
  result.traversable = true;
  result.status = CreativeWorldLayoutOpeningClearanceStatus::Ready;
  result.reasonCode = "creative_world_layout_opening_clearance_ready";
  return result;
}

}  // namespace iggy3d::creative
