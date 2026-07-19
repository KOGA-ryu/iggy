#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
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
    case CreativeWorldLayoutBuildingBlockoutStatus::InvalidEntranceEdge:
      return "InvalidEntranceEdge";
    case CreativeWorldLayoutBuildingBlockoutStatus::InvalidEntranceOffset:
      return "InvalidEntranceOffset";
    case CreativeWorldLayoutBuildingBlockoutStatus::RoomTooSmall:
      return "RoomTooSmall";
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
