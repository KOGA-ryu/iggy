#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

enum class CreativeWorldLayoutBuildingBlockoutPattern : std::uint8_t {
  SingleRoom,
  SplitX,
  SplitZ,
  Grid2x2,
  Count,
};

enum class CreativeWorldLayoutBuildingBlockoutStatus : std::uint8_t {
  NotRequested,
  InvalidPattern,
  InvalidFootprint,
  InvalidWallThickness,
  RoomTooSmall,
  Ready,
};

struct CreativeWorldLayoutBuildingBlockoutRequest {
  CreativeWorldLayoutRect footprint;
  CreativeWorldLayoutBuildingBlockoutPattern pattern =
      CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom;
  double wallThicknessCells =
      kDefaultCreativeWorldLayoutWallThicknessCells;
  bool connectRooms = true;
};

inline constexpr std::size_t kCreativeWorldLayoutBuildingBlockoutRoomCapacity =
    4U;
inline constexpr std::size_t
    kCreativeWorldLayoutBuildingBlockoutConnectionCapacity = 3U;
static_assert(kCreativeWorldLayoutBuildingBlockoutConnectionCapacity >=
              kCreativeWorldLayoutBuildingBlockoutRoomCapacity - 1U);

// One deterministic edge in the minimal room-circulation tree. The world-space
// point is consumed by the existing opening-placement kernel, which remains the
// sole owner of door dimensions, wall clearance, and host validation.
struct CreativeWorldLayoutBuildingBlockoutConnection {
  std::size_t firstRoomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge firstRoomEdge =
      CreativeWorldLayoutRoomEdge::North;
  std::size_t secondRoomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge secondRoomEdge =
      CreativeWorldLayoutRoomEdge::North;
  double xCells = 0.0;
  double zCells = 0.0;
};

struct CreativeWorldLayoutBuildingBlockoutPlan {
  bool requested = false;
  bool accepted = false;
  CreativeWorldLayoutBuildingBlockoutStatus status =
      CreativeWorldLayoutBuildingBlockoutStatus::NotRequested;
  CreativeWorldLayoutBuildingBlockoutPattern pattern =
      CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom;
  CreativeWorldLayoutRect footprint;
  std::array<CreativeWorldLayoutRect,
             kCreativeWorldLayoutBuildingBlockoutRoomCapacity>
      rooms{};
  std::size_t roomCount = 0U;
  std::array<CreativeWorldLayoutBuildingBlockoutConnection,
             kCreativeWorldLayoutBuildingBlockoutConnectionCapacity>
      connections{};
  std::size_t connectionCount = 0U;
  std::string_view reasonCode =
      "creative_world_layout_building_blockout_not_requested";
};

static_assert(
    std::is_trivially_copyable_v<CreativeWorldLayoutBuildingBlockoutRequest>);
static_assert(std::is_trivially_copyable_v<
              CreativeWorldLayoutBuildingBlockoutConnection>);
static_assert(
    std::is_trivially_copyable_v<CreativeWorldLayoutBuildingBlockoutPlan>);

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingBlockoutPattern pattern) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingBlockoutStatus status) noexcept;

// Produces row-major room footprints on integer grid lines. Odd spans give the
// extra cell to the positive-X or positive-Z room. When requested, connections
// form a deterministic minimal spanning tree over the preset's adjacent rooms.
// No source layout is mutated.
[[nodiscard]] CreativeWorldLayoutBuildingBlockoutPlan
planCreativeWorldLayoutBuildingBlockout(
    const CreativeWorldLayoutBuildingBlockoutRequest& request) noexcept;

}  // namespace iggy3d::creative
