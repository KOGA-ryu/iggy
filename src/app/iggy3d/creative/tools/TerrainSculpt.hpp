#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

enum class CreativeTerrainSculptMode : std::uint8_t {
  Flatten,
  Smooth,
  Raise,
  Lower,
  Count,
};

enum class CreativeTerrainSculptRadius : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  EightCells,
  Count,
};

enum class CreativeTerrainSculptStrength : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  EightCells,
  Count,
};

enum class CreativeTerrainSculptFalloff : std::uint8_t {
  Uniform,
  Linear,
  Smooth,
  Count,
};

enum class CreativeTerrainSculptMask : std::uint8_t {
  Circle,
  Square,
  Count,
};

inline constexpr std::size_t kCreativeTerrainSculptEditCapacity =
    kCreativeTerrainControlCapacity;

struct CreativeTerrainSculptRequest {
  std::span<const CreativeTerrainControlPoint> controls{};
  CreativeTerrainCoord2 center{};
  CreativeTerrainSculptMode mode = CreativeTerrainSculptMode::Flatten;
  std::uint16_t radiusCells = 4U;
  std::uint16_t strengthCells = 1U;
  std::uint16_t targetHeightCells = 4U;
  CreativeTerrainSculptFalloff falloff = CreativeTerrainSculptFalloff::Uniform;
  CreativeTerrainSculptMask mask = CreativeTerrainSculptMask::Circle;
};

enum class CreativeTerrainSculptPlanStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  NoControlsInBrush,
  NoChange,
  CapacityExceeded,
  ArithmeticOverflow,
  Ready,
};

// Inclusive terrain-patch coordinates whose rendered geometry can change when
// this plan is applied. The extra one-cell border accounts for corner-height
// sharing between adjacent patches.
struct CreativeTerrainSculptDirtyRegion {
  bool valid = false;
  CreativeTerrainCoord2 minimum{};
  CreativeTerrainCoord2 maximum{};
  std::uint64_t candidatePatchCount = 0U;
};

struct CreativeTerrainSculptPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainSculptPlanStatus status =
      CreativeTerrainSculptPlanStatus::NotRequested;
  std::array<CreativeTerrainControlEdit, kCreativeTerrainSculptEditCapacity>
      edits{};
  std::uint16_t inspectedControlCount = 0U;
  std::uint16_t affectedControlCount = 0U;
  std::uint16_t editCount = 0U;
  CreativeTerrainSculptDirtyRegion dirtyRegion{};
  std::string_view reasonCode = "creative_terrain_sculpt_not_requested";

  [[nodiscard]] std::span<const CreativeTerrainControlEdit> items()
      const noexcept {
    return {edits.data(), editCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeTerrainSculptPlan>);
static_assert(std::is_standard_layout_v<CreativeTerrainSculptPlan>);

[[nodiscard]] std::string_view toString(
    CreativeTerrainSculptMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainSculptRadius radius) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainSculptStrength strength) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainSculptFalloff falloff) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainSculptMask mask) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainSculptPlanStatus status) noexcept;
[[nodiscard]] bool creativeTerrainSculptUsesTargetHeight(
    CreativeTerrainSculptMode mode) noexcept;

[[nodiscard]] std::uint16_t creativeTerrainSculptRadiusCells(
    CreativeTerrainSculptRadius radius) noexcept;
[[nodiscard]] std::uint16_t creativeTerrainSculptStrengthCells(
    CreativeTerrainSculptStrength strength) noexcept;

// O(n^2), n <= 256 authored controls. Every output height is derived from the
// same input snapshot, output order follows canonical Z/X control order, and a
// rejected plan emits no partial edits.
[[nodiscard]] CreativeTerrainSculptPlan buildCreativeTerrainSculptPlan(
    const CreativeTerrainSculptRequest& request) noexcept;

}  // namespace iggy3d::creative
