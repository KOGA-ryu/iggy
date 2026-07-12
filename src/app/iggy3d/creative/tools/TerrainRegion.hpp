#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

enum class CreativeTerrainRegionOperation : std::uint8_t {
  Raise,
  Lower,
  Flatten,
  Smooth,
  Erase,
  Count,
};

enum class CreativeTerrainRegionAmount : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  EightCells,
  Count,
};

struct CreativeTerrainRegionRequest {
  std::span<const CreativeTerrainControlPoint> controls{};
  CreativeTerrainCoord2 minimumCoord{};
  CreativeTerrainCoord2 maximumCoord{};
  CreativeTerrainRegionOperation operation =
      CreativeTerrainRegionOperation::Raise;
  std::uint16_t amountCells = 1U;
  std::uint16_t targetHeightCells = 4U;
};

enum class CreativeTerrainRegionPlanStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  NoControlsInRegion,
  NoChange,
  CapacityExceeded,
  Ready,
};

struct CreativeTerrainRegionPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainRegionPlanStatus status =
      CreativeTerrainRegionPlanStatus::NotRequested;
  CreativeTerrainCoord2 minimumCoord{};
  CreativeTerrainCoord2 maximumCoord{};
  std::array<CreativeTerrainControlEdit, kCreativeTerrainControlCapacity>
      edits{};
  std::uint16_t affectedControlCount = 0U;
  std::uint16_t editCount = 0U;
  std::string_view reasonCode = "creative_terrain_region_not_requested";

  [[nodiscard]] std::span<const CreativeTerrainControlEdit> items()
      const noexcept {
    return {edits.data(), editCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeTerrainRegionPlan>);
static_assert(std::is_standard_layout_v<CreativeTerrainRegionPlan>);

[[nodiscard]] std::string_view toString(
    CreativeTerrainRegionOperation operation) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainRegionAmount amount) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainRegionPlanStatus status) noexcept;

[[nodiscard]] bool creativeTerrainRegionUsesAmount(
    CreativeTerrainRegionOperation operation) noexcept;
[[nodiscard]] bool creativeTerrainRegionUsesTargetHeight(
    CreativeTerrainRegionOperation operation) noexcept;
[[nodiscard]] std::uint16_t creativeTerrainRegionAmountCells(
    CreativeTerrainRegionAmount amount) noexcept;
[[nodiscard]] bool creativeTerrainCoordInsideRegion(
    CreativeTerrainCoord2 coord,
    CreativeTerrainCoord2 minimumCoord,
    CreativeTerrainCoord2 maximumCoord) noexcept;

// O(n^2), n <= 256 authored controls. Membership and output order follow the
// canonical input span. Smooth reads every neighbor from the unchanged input
// snapshot and uses each selected rod's authored influence radius.
[[nodiscard]] CreativeTerrainRegionPlan buildCreativeTerrainRegionPlan(
    const CreativeTerrainRegionRequest& request) noexcept;

}  // namespace iggy3d::creative
