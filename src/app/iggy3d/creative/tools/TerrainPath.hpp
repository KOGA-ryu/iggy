#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeTerrainPathPointCapacity = 32U;
inline constexpr std::uint16_t kCreativeTerrainPathMaximumSegmentCells = 255U;

enum class CreativeTerrainPathKind : std::uint8_t {
  Road,
  River,
  Ridge,
  Trench,
  Count,
};

enum class CreativeTerrainPathElevation : std::uint8_t {
  Follow,
  Level,
  Grade,
  Count,
};

enum class CreativeTerrainPathWidth : std::uint8_t {
  OneCell,
  ThreeCells,
  FiveCells,
  SevenCells,
  Count,
};

enum class CreativeTerrainPathAmplitude : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  EightCells,
  Count,
};

struct CreativeTerrainPathPoint {
  CreativeTerrainCoord2 coord{};
  std::uint16_t heightCells = 4U;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainPathPoint,
      CreativeTerrainPathPoint) noexcept = default;
};

struct CreativeTerrainPathRequest {
  const CreativeTerrainField* field = nullptr;
  std::span<const CreativeTerrainPathPoint> points;
  CreativeTerrainPathKind kind = CreativeTerrainPathKind::Road;
  CreativeTerrainPathElevation elevation =
      CreativeTerrainPathElevation::Follow;
  std::uint16_t halfWidthCells = 1U;
  std::uint16_t amplitudeCells = 1U;
};

enum class CreativeTerrainPathPlanStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  CoordinateOverflow,
  PathTooLong,
  CapacityExceeded,
  NoChange,
  Ready,
};

struct CreativeTerrainPathPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainPathPlanStatus status =
      CreativeTerrainPathPlanStatus::NotRequested;
  CreativeTerrainCoord2 minimumCoord{};
  CreativeTerrainCoord2 maximumCoord{};
  std::array<CreativeTerrainControlPoint, kCreativeTerrainControlCapacity>
      controls{};
  std::array<CreativeTerrainControlEdit, kCreativeTerrainControlCapacity> edits{};
  std::uint16_t centerlineCount = 0U;
  std::uint16_t controlCount = 0U;
  std::uint16_t editCount = 0U;
  std::string_view reasonCode = "creative_terrain_path_not_requested";

  [[nodiscard]] std::span<const CreativeTerrainControlPoint> finalControls()
      const noexcept {
    return {controls.data(), controlCount};
  }

  [[nodiscard]] std::span<const CreativeTerrainControlEdit> items()
      const noexcept {
    return {edits.data(), editCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeTerrainPathPlan>);
static_assert(std::is_standard_layout_v<CreativeTerrainPathPlan>);

[[nodiscard]] std::string_view toString(CreativeTerrainPathKind value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPathElevation value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPathWidth value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPathAmplitude value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPathPlanStatus value) noexcept;

[[nodiscard]] std::uint16_t creativeTerrainPathHalfWidthCells(
    CreativeTerrainPathWidth value) noexcept;
[[nodiscard]] std::uint16_t creativeTerrainPathWidthCells(
    CreativeTerrainPathWidth value) noexcept;
[[nodiscard]] std::uint16_t creativeTerrainPathAmplitudeCells(
    CreativeTerrainPathAmplitude value) noexcept;
[[nodiscard]] bool creativeTerrainPathUsesDepth(
    CreativeTerrainPathKind value) noexcept;

// O(c*p + c log c), where c is at most 256 generated controls and p is at
// most 256 centerline cells. Ordering is canonical Z-then-X. Any validation,
// arithmetic, traversal, or capacity failure returns zero edits atomically.
[[nodiscard]] CreativeTerrainPathPlan buildCreativeTerrainPathPlan(
    const CreativeTerrainPathRequest& request) noexcept;

}  // namespace iggy3d::creative
