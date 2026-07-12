#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeTerrainGradeEditCapacity =
    kCreativeTerrainControlCapacity;

struct CreativeTerrainGradeRequest {
  CreativeTerrainCoord2 start{};
  CreativeTerrainCoord2 end{};
  std::uint16_t startHeightCells = 4U;
  std::uint16_t endHeightCells = 4U;
  std::uint16_t radiusCells = 4U;
};

enum class CreativeTerrainGradePlanStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  CapacityExceeded,
  Ready,
};

struct CreativeTerrainGradePlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainGradePlanStatus status =
      CreativeTerrainGradePlanStatus::NotRequested;
  std::array<CreativeTerrainControlEdit, kCreativeTerrainGradeEditCapacity>
      edits{};
  std::uint16_t editCount = 0U;
  std::string_view reasonCode = "creative_terrain_grade_not_requested";

  [[nodiscard]] std::span<const CreativeTerrainControlEdit> items()
      const noexcept {
    return {edits.data(), editCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeTerrainGradePlan>);
static_assert(std::is_standard_layout_v<CreativeTerrainGradePlan>);

[[nodiscard]] std::string_view toString(
    CreativeTerrainGradePlanStatus status) noexcept;

// Emits one deterministic control point per Bresenham grid coordinate. Heights
// use integer nearest rounding, including symmetric behavior for descending
// grades, so preview and mutation cannot drift across platforms.
[[nodiscard]] CreativeTerrainGradePlan buildCreativeTerrainGradePlan(
    const CreativeTerrainGradeRequest& request) noexcept;

}  // namespace iggy3d::creative
