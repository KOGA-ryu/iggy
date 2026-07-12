#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <cstdint>

namespace iggy3d::creative {

inline constexpr std::int32_t kCreativeTerrainQ15One = 32'767;
inline constexpr std::uint32_t kCreativeTerrainQ16One = 65'536U;

[[nodiscard]] constexpr std::uint64_t creativeTerrainIntegerSquareRoot(
    std::uint64_t value) noexcept {
  std::uint64_t result = 0U;
  std::uint64_t bit = std::uint64_t{1U} << 62U;
  while (bit > value) {
    bit >>= 2U;
  }
  while (bit != 0U) {
    if (value >= result + bit) {
      value -= result + bit;
      result = (result >> 1U) + bit;
    } else {
      result >>= 1U;
    }
    bit >>= 2U;
  }
  return result;
}

[[nodiscard]] std::int64_t creativeTerrainRoundDivideSymmetric(
    std::int64_t numerator,
    std::int64_t positiveDenominator) noexcept;
[[nodiscard]] std::int32_t creativeTerrainMultiplyQ15(
    std::int32_t lhs,
    std::int32_t rhs) noexcept;

[[nodiscard]] bool creativeTerrainInsideRadius(
    CreativeTerrainCoord2 center,
    CreativeTerrainCoord2 coord,
    std::uint16_t radiusCells) noexcept;

// These two functions preserve Terrain Sculpt's original Q8 distance and Q16
// weighting contract exactly.
[[nodiscard]] std::uint32_t creativeTerrainLinearFalloffWeightQ16(
    CreativeTerrainCoord2 center,
    CreativeTerrainCoord2 coord,
    std::uint16_t radiusCells) noexcept;
[[nodiscard]] std::uint32_t creativeTerrainSmoothstepWeightQ16(
    std::uint32_t linearWeightQ16) noexcept;

[[nodiscard]] std::uint32_t creativeTerrainNormalizedDistanceQ16(
    CreativeTerrainCoord2 center,
    CreativeTerrainCoord2 coord,
    std::uint16_t radiusCells) noexcept;

// Phase is measured in Q16 turns. The result is signed Q15.
[[nodiscard]] std::int32_t creativeTerrainSinTurnsQ15(
    std::int64_t phaseTurnsQ16) noexcept;
[[nodiscard]] std::int32_t creativeTerrainCosTurnsQ15(
    std::int64_t phaseTurnsQ16) noexcept;
[[nodiscard]] std::int32_t creativeTerrainCosineBellQ15(
    std::uint32_t normalizedQ16) noexcept;
[[nodiscard]] std::uint64_t creativeTerrainQuarterWaveChecksum() noexcept;

}  // namespace iggy3d::creative
