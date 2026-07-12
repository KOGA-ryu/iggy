#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

enum class CreativeTerrainProfileKind : std::uint8_t {
  Hill,
  Basin,
  Ring,
  Crater,
  Ridge,
  Wave,
  Ripple,
  Count,
};

enum class CreativeTerrainProfileBlend : std::uint8_t {
  Set,
  Add,
  Count,
};

enum class CreativeTerrainProfileRodPolicy : std::uint8_t {
  Fill,
  Existing,
  Count,
};

enum class CreativeTerrainProfileRadius : std::uint8_t {
  TwoCells,
  FourCells,
  EightCells,
  Count,
};

enum class CreativeTerrainProfileAmplitude : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  EightCells,
  SixteenCells,
  Count,
};

enum class CreativeTerrainProfileSpacing : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  Count,
};

enum class CreativeTerrainProfileDirection : std::uint8_t {
  PositiveX,
  PositiveXPositiveZ,
  PositiveZ,
  NegativeXPositiveZ,
  NegativeX,
  NegativeXNegativeZ,
  NegativeZ,
  PositiveXNegativeZ,
  Count,
};

enum class CreativeTerrainProfileFrequency : std::uint8_t {
  OneCycle,
  TwoCycles,
  Count,
};

struct CreativeTerrainProfileRequest {
  const CreativeTerrainField* field = nullptr;
  CreativeTerrainCoord2 center{};
  std::uint16_t baseHeightCells = 4U;
  CreativeTerrainProfileKind profile = CreativeTerrainProfileKind::Hill;
  CreativeTerrainProfileBlend blend = CreativeTerrainProfileBlend::Set;
  CreativeTerrainProfileRodPolicy rodPolicy =
      CreativeTerrainProfileRodPolicy::Fill;
  CreativeTerrainProfileDirection direction =
      CreativeTerrainProfileDirection::PositiveX;
  std::uint16_t radiusCells = 4U;
  std::uint16_t amplitudeCells = 4U;
  std::uint16_t spacingCells = 1U;
  std::uint8_t frequency = 1U;
};

enum class CreativeTerrainProfilePlanStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  CoordinateOverflow,
  CapacityExceeded,
  SamplingUnproven,
  UnderSampled,
  NoControlsInBrush,
  NoChange,
  Ready,
};

struct CreativeTerrainProfilePlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainProfilePlanStatus status =
      CreativeTerrainProfilePlanStatus::NotRequested;
  std::array<CreativeTerrainControlEdit, kCreativeTerrainControlCapacity> edits{};
  std::uint16_t candidateCount = 0U;
  std::uint16_t editCount = 0U;
  std::string_view reasonCode = "creative_terrain_profile_not_requested";

  [[nodiscard]] std::span<const CreativeTerrainControlEdit> items()
      const noexcept {
    return {edits.data(), editCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeTerrainProfilePlan>);
static_assert(std::is_standard_layout_v<CreativeTerrainProfilePlan>);

[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileKind value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileBlend value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileRodPolicy value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileRadius value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileAmplitude value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileSpacing value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileDirection value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileFrequency value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfilePlanStatus value) noexcept;

[[nodiscard]] std::uint16_t creativeTerrainProfileRadiusCells(
    CreativeTerrainProfileRadius value) noexcept;
[[nodiscard]] std::uint16_t creativeTerrainProfileAmplitudeCells(
    CreativeTerrainProfileAmplitude value) noexcept;
[[nodiscard]] std::uint16_t creativeTerrainProfileSpacingCells(
    CreativeTerrainProfileSpacing value) noexcept;
[[nodiscard]] std::uint8_t creativeTerrainProfileFrequencyCycles(
    CreativeTerrainProfileFrequency value) noexcept;
[[nodiscard]] bool creativeTerrainProfileUsesDirection(
    CreativeTerrainProfileKind value) noexcept;
[[nodiscard]] bool creativeTerrainProfileUsesFrequency(
    CreativeTerrainProfileKind value) noexcept;

// O(a*n) in the bounded field, where a is at most 256 candidates and n is at
// most 256 controls. Every rejected plan exposes zero edits.
[[nodiscard]] CreativeTerrainProfilePlan buildCreativeTerrainProfilePlan(
    const CreativeTerrainProfileRequest& request) noexcept;

}  // namespace iggy3d::creative
