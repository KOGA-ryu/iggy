#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"

#include "app/iggy3d/creative/recipes/RampRecipe.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <string_view>

namespace iggy3d::creative {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;

void fail(CreativeWorldLayoutBuildingBlockoutPlan& plan,
          CreativeWorldLayoutBuildingBlockoutStatus status,
          std::string_view reasonCode) noexcept {
  plan.status = status;
  plan.reasonCode = reasonCode;
}

std::int32_t midpoint(std::int32_t minimum, std::int32_t maximum) noexcept {
  const std::int64_t span = static_cast<std::int64_t>(maximum) - minimum;
  return static_cast<std::int32_t>(static_cast<std::int64_t>(minimum) +
                                   span / 2);
}

double center(std::int32_t minimum, std::int32_t maximum) noexcept {
  const std::int64_t span = static_cast<std::int64_t>(maximum) - minimum;
  return static_cast<double>(minimum) + static_cast<double>(span) * 0.5;
}

bool appendOpening(
    CreativeWorldLayoutBuildingBlockoutPlan& plan,
    CreativeWorldLayoutBuildingBlockoutOpeningRole role,
    CreativeBuildingOpeningKind kind, std::size_t roomIndex,
    CreativeWorldLayoutRoomEdge roomEdge, std::size_t adjacentRoomIndex,
    CreativeWorldLayoutRoomEdge adjacentRoomEdge, double xCells,
    double zCells) noexcept {
  if (plan.openingCount >= plan.openings.size()) {
    fail(plan,
         CreativeWorldLayoutBuildingBlockoutStatus::OpeningCapacityExceeded,
         "creative_world_layout_building_blockout_opening_capacity_exceeded");
    return false;
  }
  plan.openings[plan.openingCount++] = {role,
                                        kind,
                                        roomIndex,
                                        roomEdge,
                                        adjacentRoomIndex,
                                        adjacentRoomEdge,
                                        xCells,
                                        zCells};
  return true;
}

bool roomTouchesFacade(CreativeWorldLayoutRect room,
                       CreativeWorldLayoutRect footprint,
                       CreativeWorldLayoutRoomEdge edge) noexcept {
  switch (edge) {
    case CreativeWorldLayoutRoomEdge::North:
      return room.minimum.z == footprint.minimum.z;
    case CreativeWorldLayoutRoomEdge::East:
      return room.maximum.x == footprint.maximum.x;
    case CreativeWorldLayoutRoomEdge::South:
      return room.maximum.z == footprint.maximum.z;
    case CreativeWorldLayoutRoomEdge::West:
      return room.minimum.x == footprint.minimum.x;
    case CreativeWorldLayoutRoomEdge::Count:
      break;
  }
  return false;
}

bool horizontalEdge(CreativeWorldLayoutRoomEdge edge) noexcept {
  return edge == CreativeWorldLayoutRoomEdge::North ||
         edge == CreativeWorldLayoutRoomEdge::South;
}

double edgeMinimum(CreativeWorldLayoutRect room,
                   CreativeWorldLayoutRoomEdge edge) noexcept {
  return horizontalEdge(edge) ? static_cast<double>(room.minimum.x)
                              : static_cast<double>(room.minimum.z);
}

double edgeMaximum(CreativeWorldLayoutRect room,
                   CreativeWorldLayoutRoomEdge edge) noexcept {
  return horizontalEdge(edge) ? static_cast<double>(room.maximum.x)
                              : static_cast<double>(room.maximum.z);
}

void edgePoint(CreativeWorldLayoutRect room,
               CreativeWorldLayoutRoomEdge edge, double coordinate,
               double& xCells, double& zCells) noexcept {
  if (horizontalEdge(edge)) {
    xCells = coordinate;
    zCells = edge == CreativeWorldLayoutRoomEdge::North
                 ? static_cast<double>(room.minimum.z)
                 : static_cast<double>(room.maximum.z);
    return;
  }
  xCells = edge == CreativeWorldLayoutRoomEdge::West
               ? static_cast<double>(room.minimum.x)
               : static_cast<double>(room.maximum.x);
  zCells = coordinate;
}

bool appendFacadeOpenings(
    CreativeWorldLayoutBuildingBlockoutPlan& plan,
    const CreativeWorldLayoutBuildingBlockoutRequest& request) noexcept {
  std::size_t entranceRoomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge entranceRoomEdge =
      CreativeWorldLayoutRoomEdge::Count;

  if (request.facade.includeEntrance) {
    const double facadeCenter =
        horizontalEdge(request.facade.entranceEdge)
            ? center(request.footprint.minimum.x, request.footprint.maximum.x)
            : center(request.footprint.minimum.z, request.footprint.maximum.z);
    double bestDistance = std::numeric_limits<double>::infinity();
    for (std::size_t roomIndex = 0U; roomIndex < plan.roomCount; ++roomIndex) {
      const CreativeWorldLayoutRect room = plan.rooms[roomIndex];
      if (!roomTouchesFacade(room, request.footprint,
                             request.facade.entranceEdge)) {
        continue;
      }
      const double roomEdgeCenter =
          (edgeMinimum(room, request.facade.entranceEdge) +
           edgeMaximum(room, request.facade.entranceEdge)) *
          0.5;
      const double distance = std::fabs(roomEdgeCenter - facadeCenter);
      if (distance < bestDistance) {
        bestDistance = distance;
        entranceRoomIndex = roomIndex;
      }
    }
    if (entranceRoomIndex == kInvalidCreativeWorldLayoutIndex) {
      fail(plan, CreativeWorldLayoutBuildingBlockoutStatus::InvalidEntranceEdge,
           "creative_world_layout_building_blockout_entrance_edge_invalid");
      return false;
    }

    entranceRoomEdge = request.facade.entranceEdge;
    const CreativeWorldLayoutRect entranceRoom =
        plan.rooms[entranceRoomIndex];
    const double minimum = edgeMinimum(entranceRoom, entranceRoomEdge);
    const double maximum = edgeMaximum(entranceRoom, entranceRoomEdge);
    const double coordinate =
        (minimum + maximum) * 0.5 + request.facade.entranceOffsetCells;
    if (!(coordinate > minimum && coordinate < maximum)) {
      fail(plan,
           CreativeWorldLayoutBuildingBlockoutStatus::InvalidEntranceOffset,
           "creative_world_layout_building_blockout_entrance_offset_invalid");
      return false;
    }
    double xCells = 0.0;
    double zCells = 0.0;
    edgePoint(entranceRoom, entranceRoomEdge, coordinate, xCells, zCells);
    if (!appendOpening(
            plan, CreativeWorldLayoutBuildingBlockoutOpeningRole::Entrance,
            CreativeBuildingOpeningKind::Door, entranceRoomIndex,
            entranceRoomEdge, kInvalidCreativeWorldLayoutIndex,
            CreativeWorldLayoutRoomEdge::Count, xCells, zCells)) {
      return false;
    }
  }

  if (!request.facade.includeExteriorWindows) {
    return true;
  }
  for (std::size_t roomIndex = 0U; roomIndex < plan.roomCount; ++roomIndex) {
    const CreativeWorldLayoutRect room = plan.rooms[roomIndex];
    for (std::uint8_t edgeValue = 0U;
         edgeValue <
         static_cast<std::uint8_t>(CreativeWorldLayoutRoomEdge::Count);
         ++edgeValue) {
      const CreativeWorldLayoutRoomEdge edge =
          static_cast<CreativeWorldLayoutRoomEdge>(edgeValue);
      if (!roomTouchesFacade(room, request.footprint, edge) ||
          (roomIndex == entranceRoomIndex && edge == entranceRoomEdge)) {
        continue;
      }
      double xCells = 0.0;
      double zCells = 0.0;
      edgePoint(room, edge,
                (edgeMinimum(room, edge) + edgeMaximum(room, edge)) * 0.5,
                xCells, zCells);
      if (!appendOpening(
              plan,
              CreativeWorldLayoutBuildingBlockoutOpeningRole::ExteriorWindow,
              CreativeBuildingOpeningKind::Window, roomIndex, edge,
              kInvalidCreativeWorldLayoutIndex,
              CreativeWorldLayoutRoomEdge::Count, xCells, zCells)) {
        return false;
      }
    }
  }
  return true;
}

bool roomSupportsWalls(CreativeWorldLayoutRect room,
                       double wallThicknessCells) noexcept {
  const double width = static_cast<double>(
      static_cast<std::int64_t>(room.maximum.x) - room.minimum.x);
  const double depth = static_cast<double>(
      static_cast<std::int64_t>(room.maximum.z) - room.minimum.z);
  return width > wallThicknessCells * 2.0 + kGeometryEpsilon &&
         depth > wallThicknessCells * 2.0 + kGeometryEpsilon;
}

bool directionAlongX(
    CreativeWorldLayoutVerticalDirection direction) noexcept {
  return direction == CreativeWorldLayoutVerticalDirection::PositiveX ||
         direction == CreativeWorldLayoutVerticalDirection::NegativeX;
}

std::int64_t rectWidth(CreativeWorldLayoutRect rect) noexcept {
  return static_cast<std::int64_t>(rect.maximum.x) - rect.minimum.x;
}

std::int64_t rectDepth(CreativeWorldLayoutRect rect) noexcept {
  return static_cast<std::int64_t>(rect.maximum.z) - rect.minimum.z;
}

bool connectorFootprintForRoom(
    CreativeWorldLayoutRect room, std::uint16_t riseCells,
    double wallThicknessCells, CreativeWorldLayoutVerticalConnectorKind kind,
    CreativeWorldLayoutVerticalDirection direction,
    CreativeWorldLayoutRect& footprint) noexcept {
  const bool alongX = directionAlongX(direction);
  const std::int64_t runExtent = alongX ? rectWidth(room) : rectDepth(room);
  const std::int64_t widthExtent = alongX ? rectDepth(room) : rectWidth(room);
  std::int64_t runCells = riseCells;
  if (kind == CreativeWorldLayoutVerticalConnectorKind::Ramp) {
    const double maximumSlopeRadians =
        kCreativeRampMaximumWalkableSlopeDegrees * std::numbers::pi / 180.0;
    runCells = static_cast<std::int64_t>(std::ceil(
        static_cast<double>(riseCells) / std::tan(maximumSlopeRadians)));
  }
  constexpr std::int64_t kConnectorWidthCells = 1;
  const std::int64_t wallClearanceCells = std::max<std::int64_t>(
      1, static_cast<std::int64_t>(std::ceil(wallThicknessCells * 0.5)));
  if (runExtent < runCells + wallClearanceCells * 2 ||
      widthExtent < kConnectorWidthCells + wallClearanceCells * 2) {
    return false;
  }

  const std::int64_t runMinimum =
      (alongX ? static_cast<std::int64_t>(room.minimum.x)
              : static_cast<std::int64_t>(room.minimum.z)) +
      (runExtent - runCells) / 2;
  const std::int64_t widthMinimum =
      (alongX ? static_cast<std::int64_t>(room.minimum.z)
              : static_cast<std::int64_t>(room.minimum.x)) +
      (widthExtent - kConnectorWidthCells) / 2;
  if (alongX) {
    footprint = {{static_cast<std::int32_t>(runMinimum),
                  static_cast<std::int32_t>(widthMinimum)},
                 {static_cast<std::int32_t>(runMinimum + runCells),
                  static_cast<std::int32_t>(widthMinimum +
                                            kConnectorWidthCells)}};
  } else {
    footprint = {{static_cast<std::int32_t>(widthMinimum),
                  static_cast<std::int32_t>(runMinimum)},
                 {static_cast<std::int32_t>(widthMinimum +
                                            kConnectorWidthCells),
                  static_cast<std::int32_t>(runMinimum + runCells)}};
  }
  return true;
}

std::uint64_t roomArea(CreativeWorldLayoutRect room) noexcept {
  return static_cast<std::uint64_t>(rectWidth(room)) *
         static_cast<std::uint64_t>(rectDepth(room));
}

std::uint64_t absoluteDifference(std::int64_t lhs,
                                 std::int64_t rhs) noexcept {
  return lhs >= rhs ? static_cast<std::uint64_t>(lhs - rhs)
                    : static_cast<std::uint64_t>(rhs - lhs);
}

std::uint64_t distanceFromFootprintCenter(
    CreativeWorldLayoutRect room,
    CreativeWorldLayoutRect footprint) noexcept {
  const std::int64_t roomCenterX =
      static_cast<std::int64_t>(room.minimum.x) + room.maximum.x;
  const std::int64_t roomCenterZ =
      static_cast<std::int64_t>(room.minimum.z) + room.maximum.z;
  const std::int64_t footprintCenterX =
      static_cast<std::int64_t>(footprint.minimum.x) + footprint.maximum.x;
  const std::int64_t footprintCenterZ =
      static_cast<std::int64_t>(footprint.minimum.z) + footprint.maximum.z;
  return absoluteDifference(roomCenterX, footprintCenterX) +
         absoluteDifference(roomCenterZ, footprintCenterZ);
}

bool planVerticalConnector(
    CreativeWorldLayoutBuildingBlockoutPlan& plan,
    const CreativeWorldLayoutBuildingBlockoutRequest& request) noexcept {
  if (request.storeys.count <= 1U || !request.storeys.connectStoreys) {
    return true;
  }
  if (request.wallHeightCells < 2U ||
      request.storeys.connectorKind >=
          CreativeWorldLayoutVerticalConnectorKind::Count ||
      request.storeys.preferredDirection >=
          CreativeWorldLayoutVerticalDirection::Count) {
    fail(plan,
         CreativeWorldLayoutBuildingBlockoutStatus::InvalidVerticalConnector,
         "creative_world_layout_building_blockout_vertical_connector_invalid");
    return false;
  }

  std::array<CreativeWorldLayoutVerticalDirection, 4U> directions{};
  std::size_t directionCount = 0U;
  directions[directionCount++] = request.storeys.preferredDirection;
  for (std::uint8_t value = 0U;
       value <
       static_cast<std::uint8_t>(CreativeWorldLayoutVerticalDirection::Count);
       ++value) {
    const CreativeWorldLayoutVerticalDirection direction =
        static_cast<CreativeWorldLayoutVerticalDirection>(value);
    if (direction != request.storeys.preferredDirection) {
      directions[directionCount++] = direction;
    }
  }

  for (std::size_t directionIndex = 0U; directionIndex < directionCount;
       ++directionIndex) {
    const CreativeWorldLayoutVerticalDirection direction =
        directions[directionIndex];
    bool found = false;
    std::size_t bestRoomIndex = kInvalidCreativeWorldLayoutIndex;
    CreativeWorldLayoutRect bestFootprint;
    std::uint64_t bestArea = 0U;
    std::uint64_t bestCenterDistance = 0U;
    for (std::size_t roomIndex = 0U; roomIndex < plan.roomCount; ++roomIndex) {
      CreativeWorldLayoutRect candidateFootprint;
      if (!connectorFootprintForRoom(plan.rooms[roomIndex],
                                     request.wallHeightCells,
                                     request.wallThicknessCells,
                                     request.storeys.connectorKind, direction,
                                     candidateFootprint)) {
        continue;
      }
      const std::uint64_t candidateArea = roomArea(plan.rooms[roomIndex]);
      const std::uint64_t candidateCenterDistance =
          distanceFromFootprintCenter(plan.rooms[roomIndex], plan.footprint);
      if (!found || candidateArea > bestArea ||
          (candidateArea == bestArea &&
           candidateCenterDistance < bestCenterDistance)) {
        found = true;
        bestRoomIndex = roomIndex;
        bestFootprint = candidateFootprint;
        bestArea = candidateArea;
        bestCenterDistance = candidateCenterDistance;
      }
    }
    if (found) {
      plan.hasVerticalConnector = true;
      plan.verticalConnector = {bestRoomIndex,
                                request.storeys.connectorKind,
                                direction,
                                bestFootprint};
      return true;
    }
  }

  fail(plan,
       CreativeWorldLayoutBuildingBlockoutStatus::VerticalConnectorDoesNotFit,
       "creative_world_layout_building_blockout_vertical_connector_does_not_fit");
  return false;
}

}  // namespace

std::string_view toString(
    CreativeWorldLayoutBuildingBlockoutPattern pattern) noexcept {
  switch (pattern) {
    case CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom:
      return "SingleRoom";
    case CreativeWorldLayoutBuildingBlockoutPattern::SplitX:
      return "SplitX";
    case CreativeWorldLayoutBuildingBlockoutPattern::SplitZ:
      return "SplitZ";
    case CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2:
      return "Grid2x2";
    case CreativeWorldLayoutBuildingBlockoutPattern::Count:
      break;
  }
  return "Invalid";
}

std::string_view toString(
    CreativeWorldLayoutBuildingBlockoutStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutBuildingBlockoutStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutBuildingBlockoutStatus::InvalidPattern:
      return "InvalidPattern";
    case CreativeWorldLayoutBuildingBlockoutStatus::InvalidFootprint:
      return "InvalidFootprint";
    case CreativeWorldLayoutBuildingBlockoutStatus::InvalidWallThickness:
      return "InvalidWallThickness";
    case CreativeWorldLayoutBuildingBlockoutStatus::InvalidWallHeight:
      return "InvalidWallHeight";
    case CreativeWorldLayoutBuildingBlockoutStatus::InvalidStoreyCount:
      return "InvalidStoreyCount";
    case CreativeWorldLayoutBuildingBlockoutStatus::InvalidEntranceEdge:
      return "InvalidEntranceEdge";
    case CreativeWorldLayoutBuildingBlockoutStatus::InvalidEntranceOffset:
      return "InvalidEntranceOffset";
    case CreativeWorldLayoutBuildingBlockoutStatus::InvalidVerticalConnector:
      return "InvalidVerticalConnector";
    case CreativeWorldLayoutBuildingBlockoutStatus::RoomTooSmall:
      return "RoomTooSmall";
    case CreativeWorldLayoutBuildingBlockoutStatus::
        VerticalConnectorDoesNotFit:
      return "VerticalConnectorDoesNotFit";
    case CreativeWorldLayoutBuildingBlockoutStatus::OpeningCapacityExceeded:
      return "OpeningCapacityExceeded";
    case CreativeWorldLayoutBuildingBlockoutStatus::Ready:
      return "Ready";
  }
  return "Invalid";
}

CreativeWorldLayoutBuildingBlockoutPlan
planCreativeWorldLayoutBuildingBlockout(
    const CreativeWorldLayoutBuildingBlockoutRequest& request) noexcept {
  CreativeWorldLayoutBuildingBlockoutPlan plan;
  plan.requested = true;
  plan.pattern = request.pattern;
  plan.footprint = request.footprint;
  plan.storeyCount = request.storeys.count;

  if (request.pattern >= CreativeWorldLayoutBuildingBlockoutPattern::Count) {
    fail(plan, CreativeWorldLayoutBuildingBlockoutStatus::InvalidPattern,
         "creative_world_layout_building_blockout_pattern_invalid");
    return plan;
  }
  if (request.footprint.minimum.x >= request.footprint.maximum.x ||
      request.footprint.minimum.z >= request.footprint.maximum.z) {
    fail(plan, CreativeWorldLayoutBuildingBlockoutStatus::InvalidFootprint,
         "creative_world_layout_building_blockout_footprint_invalid");
    return plan;
  }
  if (!std::isfinite(request.wallThicknessCells) ||
      request.wallThicknessCells <= 0.0) {
    fail(plan, CreativeWorldLayoutBuildingBlockoutStatus::InvalidWallThickness,
         "creative_world_layout_building_blockout_wall_thickness_invalid");
    return plan;
  }
  if (request.wallHeightCells == 0U) {
    fail(plan, CreativeWorldLayoutBuildingBlockoutStatus::InvalidWallHeight,
         "creative_world_layout_building_blockout_wall_height_invalid");
    return plan;
  }
  if (request.storeys.count == 0U ||
      request.storeys.count >
          kCreativeWorldLayoutBuildingBlockoutStoreyCapacity) {
    fail(plan, CreativeWorldLayoutBuildingBlockoutStatus::InvalidStoreyCount,
         "creative_world_layout_building_blockout_storey_count_invalid");
    return plan;
  }
  if (request.facade.includeEntrance &&
      request.facade.entranceEdge >= CreativeWorldLayoutRoomEdge::Count) {
    fail(plan, CreativeWorldLayoutBuildingBlockoutStatus::InvalidEntranceEdge,
         "creative_world_layout_building_blockout_entrance_edge_invalid");
    return plan;
  }
  if (request.facade.includeEntrance &&
      !std::isfinite(request.facade.entranceOffsetCells)) {
    fail(plan, CreativeWorldLayoutBuildingBlockoutStatus::InvalidEntranceOffset,
         "creative_world_layout_building_blockout_entrance_offset_invalid");
    return plan;
  }

  const std::int32_t splitX =
      midpoint(request.footprint.minimum.x, request.footprint.maximum.x);
  const std::int32_t splitZ =
      midpoint(request.footprint.minimum.z, request.footprint.maximum.z);
  switch (request.pattern) {
    case CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom:
      plan.rooms[0] = request.footprint;
      plan.roomCount = 1U;
      break;
    case CreativeWorldLayoutBuildingBlockoutPattern::SplitX:
      plan.rooms[0] = {request.footprint.minimum,
                       {splitX, request.footprint.maximum.z}};
      plan.rooms[1] = {{splitX, request.footprint.minimum.z},
                       request.footprint.maximum};
      plan.roomCount = 2U;
      break;
    case CreativeWorldLayoutBuildingBlockoutPattern::SplitZ:
      plan.rooms[0] = {request.footprint.minimum,
                       {request.footprint.maximum.x, splitZ}};
      plan.rooms[1] = {{request.footprint.minimum.x, splitZ},
                       request.footprint.maximum};
      plan.roomCount = 2U;
      break;
    case CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2:
      plan.rooms[0] = {request.footprint.minimum, {splitX, splitZ}};
      plan.rooms[1] = {{splitX, request.footprint.minimum.z},
                       {request.footprint.maximum.x, splitZ}};
      plan.rooms[2] = {{request.footprint.minimum.x, splitZ},
                       {splitX, request.footprint.maximum.z}};
      plan.rooms[3] = {{splitX, splitZ}, request.footprint.maximum};
      plan.roomCount = 4U;
      break;
    case CreativeWorldLayoutBuildingBlockoutPattern::Count:
      break;
  }

  for (std::size_t index = 0U; index < plan.roomCount; ++index) {
    if (!roomSupportsWalls(plan.rooms[index], request.wallThicknessCells)) {
      plan.roomCount = 0U;
      plan.rooms = {};
      fail(plan, CreativeWorldLayoutBuildingBlockoutStatus::RoomTooSmall,
           "creative_world_layout_building_blockout_room_too_small");
      return plan;
    }
  }

  if (!planVerticalConnector(plan, request)) {
    return plan;
  }

  if (request.connectRooms) {
    const double footprintCenterX =
        center(request.footprint.minimum.x, request.footprint.maximum.x);
    const double footprintCenterZ =
        center(request.footprint.minimum.z, request.footprint.maximum.z);
    switch (request.pattern) {
      case CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom:
        break;
      case CreativeWorldLayoutBuildingBlockoutPattern::SplitX:
        if (!appendOpening(
                plan,
                CreativeWorldLayoutBuildingBlockoutOpeningRole::
                    InteriorConnection,
                CreativeBuildingOpeningKind::Door, 0U,
                CreativeWorldLayoutRoomEdge::East, 1U,
                CreativeWorldLayoutRoomEdge::West,
                static_cast<double>(splitX), footprintCenterZ)) {
          return plan;
        }
        break;
      case CreativeWorldLayoutBuildingBlockoutPattern::SplitZ:
        if (!appendOpening(
                plan,
                CreativeWorldLayoutBuildingBlockoutOpeningRole::
                    InteriorConnection,
                CreativeBuildingOpeningKind::Door, 0U,
                CreativeWorldLayoutRoomEdge::South, 1U,
                CreativeWorldLayoutRoomEdge::North, footprintCenterX,
                static_cast<double>(splitZ))) {
          return plan;
        }
        break;
      case CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2:
        // Row-major spanning tree: 1 -> 0, 2 -> 0, 3 -> 2. Three doors connect
        // all four rooms without imposing an extra circulation loop.
        if (!appendOpening(
                plan,
                CreativeWorldLayoutBuildingBlockoutOpeningRole::
                    InteriorConnection,
                CreativeBuildingOpeningKind::Door, 0U,
                CreativeWorldLayoutRoomEdge::East, 1U,
                CreativeWorldLayoutRoomEdge::West,
                static_cast<double>(splitX),
                center(request.footprint.minimum.z, splitZ)) ||
            !appendOpening(
                plan,
                CreativeWorldLayoutBuildingBlockoutOpeningRole::
                    InteriorConnection,
                CreativeBuildingOpeningKind::Door, 0U,
                CreativeWorldLayoutRoomEdge::South, 2U,
                CreativeWorldLayoutRoomEdge::North,
                center(request.footprint.minimum.x, splitX),
                static_cast<double>(splitZ)) ||
            !appendOpening(
                plan,
                CreativeWorldLayoutBuildingBlockoutOpeningRole::
                    InteriorConnection,
                CreativeBuildingOpeningKind::Door, 2U,
                CreativeWorldLayoutRoomEdge::East, 3U,
                CreativeWorldLayoutRoomEdge::West,
                static_cast<double>(splitX),
                center(splitZ, request.footprint.maximum.z))) {
          return plan;
        }
        break;
      case CreativeWorldLayoutBuildingBlockoutPattern::Count:
        break;
    }
  }
  if (!appendFacadeOpenings(plan, request)) {
    return plan;
  }

  plan.accepted = true;
  plan.status = CreativeWorldLayoutBuildingBlockoutStatus::Ready;
  plan.reasonCode = "creative_world_layout_building_blockout_ready";
  return plan;
}

}  // namespace iggy3d::creative
