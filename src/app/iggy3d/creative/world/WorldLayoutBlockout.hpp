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
};

inline constexpr std::size_t kCreativeWorldLayoutBuildingBlockoutRoomCapacity =
    4U;

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
  std::string_view reasonCode =
      "creative_world_layout_building_blockout_not_requested";
};

static_assert(
    std::is_trivially_copyable_v<CreativeWorldLayoutBuildingBlockoutRequest>);
static_assert(
    std::is_trivially_copyable_v<CreativeWorldLayoutBuildingBlockoutPlan>);

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingBlockoutPattern pattern) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingBlockoutStatus status) noexcept;

// Produces row-major room footprints on integer grid lines. Odd spans give the
// extra cell to the positive-X or positive-Z room. No source layout is mutated.
[[nodiscard]] CreativeWorldLayoutBuildingBlockoutPlan
planCreativeWorldLayoutBuildingBlockout(
    const CreativeWorldLayoutBuildingBlockoutRequest& request) noexcept;

}  // namespace iggy3d::creative
