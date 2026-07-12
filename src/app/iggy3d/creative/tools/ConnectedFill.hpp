#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"

namespace iggy3d::creative {

class CreativeVoxelField;

inline constexpr std::size_t kCreativeConnectedFillCapacity = 512U;

enum class CreativeConnectedFillLimit : std::uint8_t {
  Cells64,
  Cells128,
  Cells256,
  Cells512,
  Count,
};

enum class CreativeConnectedFillStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  InvalidLimit,
  EmptySeed,
  CapacityExceeded,
  Planned,
};

struct CreativeConnectedFillRequest {
  const CreativeVoxelField* field = nullptr;
  CreativeGridCoord3 seedCell{};
  CreativeConnectedFillLimit limit = CreativeConnectedFillLimit::Cells256;
};

struct CreativeConnectedFillPlan {
  bool requested = false;
  bool accepted = false;
  CreativeConnectedFillLimit limit = CreativeConnectedFillLimit::Cells256;
  CreativeConnectedFillStatus status =
      CreativeConnectedFillStatus::NotRequested;
  CreativeObjectKind sourceMaterial = CreativeObjectKind::Unknown;
  CreativeGridCoord3 seedCell{};
  CreativeGridCoord3 minCell{};
  CreativeGridCoord3 maxCell{};
  std::array<CreativeGridCoord3, kCreativeConnectedFillCapacity> cells{};
  std::uint16_t cellCount = 0U;
  std::string_view reasonCode = "creative_connected_fill_not_requested";

  [[nodiscard]] std::span<const CreativeGridCoord3> generatedCells()
      const noexcept {
    return {cells.data(), cellCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeConnectedFillPlan>);
static_assert(std::is_standard_layout_v<CreativeConnectedFillPlan>);

[[nodiscard]] std::uint16_t connectedFillCellLimit(
    CreativeConnectedFillLimit limit) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeConnectedFillLimit limit) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeConnectedFillStatus status) noexcept;

// Deterministic six-neighbor BFS in -X,+X,-Y,+Y,-Z,+Z order. The planner is
// allocation-free and fails closed if the connected component exceeds limit.
[[nodiscard]] CreativeConnectedFillPlan planCreativeConnectedFill(
    const CreativeConnectedFillRequest& request) noexcept;

}  // namespace iggy3d::creative
