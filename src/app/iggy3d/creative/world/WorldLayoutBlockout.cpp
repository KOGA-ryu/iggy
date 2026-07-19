#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"

#include <cmath>
#include <cstdint>
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
    case CreativeWorldLayoutBuildingBlockoutStatus::RoomTooSmall:
      return "RoomTooSmall";
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

  plan.accepted = true;
  plan.status = CreativeWorldLayoutBuildingBlockoutStatus::Ready;
  plan.reasonCode = "creative_world_layout_building_blockout_ready";
  return plan;
}

}  // namespace iggy3d::creative
