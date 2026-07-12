#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

enum class CreativeTerrainRodStampMode : std::uint8_t {
  Single,
  Seed,
  Count,
};

enum class CreativeTerrainSeedRadius : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  EightCells,
  Count,
};

enum class CreativeTerrainSeedSpacing : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  Count,
};

enum class CreativeTerrainSeedOperation : std::uint8_t {
  SeedMissing,
  Clear,
  Count,
};

struct CreativeTerrainSeedRequest {
  const CreativeTerrainField* field = nullptr;
  CreativeTerrainCoord2 center{};
  CreativeTerrainSeedOperation operation =
      CreativeTerrainSeedOperation::SeedMissing;
  std::uint16_t seedRadiusCells = 4U;
  std::uint16_t spacingCells = 2U;
  std::uint16_t fallbackHeightCells = 4U;
  std::uint16_t controlRadiusCells = 4U;
};

enum class CreativeTerrainSeedPlanStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  NoChange,
  CapacityExceeded,
  Ready,
};

struct CreativeTerrainSeedPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainSeedPlanStatus status =
      CreativeTerrainSeedPlanStatus::NotRequested;
  std::array<CreativeTerrainControlEdit, kCreativeTerrainControlCapacity> edits{};
  std::uint16_t candidateCount = 0U;
  std::uint16_t editCount = 0U;
  std::string_view reasonCode = "creative_terrain_seed_not_requested";

  [[nodiscard]] std::span<const CreativeTerrainControlEdit> items()
      const noexcept {
    return {edits.data(), editCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeTerrainSeedPlan>);
static_assert(std::is_standard_layout_v<CreativeTerrainSeedPlan>);

[[nodiscard]] std::string_view toString(
    CreativeTerrainRodStampMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainSeedRadius radius) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainSeedSpacing spacing) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainSeedPlanStatus status) noexcept;

[[nodiscard]] std::uint16_t creativeTerrainSeedRadiusCells(
    CreativeTerrainSeedRadius radius) noexcept;
[[nodiscard]] std::uint16_t creativeTerrainSeedSpacingCells(
    CreativeTerrainSeedSpacing spacing) noexcept;

// O(a*n), where a <= 197 lattice points for the largest radius and n <= 256
// authored controls. Output is canonical Z/X order, existing rods are never
// overwritten by SeedMissing, and every rejection emits zero partial edits.
[[nodiscard]] CreativeTerrainSeedPlan buildCreativeTerrainSeedPlan(
    const CreativeTerrainSeedRequest& request) noexcept;

}  // namespace iggy3d::creative
