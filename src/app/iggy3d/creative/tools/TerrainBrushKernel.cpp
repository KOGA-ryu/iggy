#include "app/iggy3d/creative/tools/TerrainBrushKernel.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace iggy3d::creative {
namespace {

constexpr std::array<std::int32_t, 65U> kQuarterWaveQ15{{
    0,     804,   1608,  2410,  3212,  4011,  4808,  5602,  6393,  7179,
    7962,  8739,  9512,  10278, 11039, 11793, 12539, 13279, 14010, 14732,
    15446, 16151, 16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403,
    22005, 22594, 23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
    27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956, 30273, 30571,
    30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521,
    32609, 32678, 32728, 32757, 32767,
}};

constexpr std::int64_t kTurnQ16 = 65'536;
constexpr std::int64_t kQuarterTurnQ16 = kTurnQ16 / 4;
constexpr std::int64_t kTableStepQ16 = 256;

[[nodiscard]] std::int64_t normalizedPhase(std::int64_t phase) noexcept {
  const std::int64_t remainder = phase % kTurnQ16;
  return remainder < 0 ? remainder + kTurnQ16 : remainder;
}

[[nodiscard]] std::int32_t quarterWaveSample(
    std::int64_t quarterPhaseQ16) noexcept {
  const std::int64_t clamped =
      std::clamp(quarterPhaseQ16, std::int64_t{0}, kQuarterTurnQ16);
  const std::size_t index =
      static_cast<std::size_t>(clamped / kTableStepQ16);
  if (index >= kQuarterWaveQ15.size() - 1U) {
    return kQuarterWaveQ15.back();
  }
  const std::int64_t remainder = clamped % kTableStepQ16;
  const std::int64_t first = kQuarterWaveQ15[index];
  const std::int64_t second = kQuarterWaveQ15[index + 1U];
  return static_cast<std::int32_t>(
      first + creativeTerrainRoundDivideSymmetric(
                  (second - first) * remainder, kTableStepQ16));
}

}  // namespace

std::int64_t creativeTerrainRoundDivideSymmetric(
    std::int64_t numerator,
    std::int64_t positiveDenominator) noexcept {
  if (positiveDenominator <= 0) {
    return 0;
  }
  if (numerator >= 0) {
    return (numerator + positiveDenominator / 2) / positiveDenominator;
  }
  return -((-numerator + positiveDenominator / 2) / positiveDenominator);
}

std::int32_t creativeTerrainMultiplyQ15(std::int32_t lhs,
                                       std::int32_t rhs) noexcept {
  return static_cast<std::int32_t>(creativeTerrainRoundDivideSymmetric(
      static_cast<std::int64_t>(lhs) * rhs, kCreativeTerrainQ15One));
}

bool creativeTerrainInsideRadius(CreativeTerrainCoord2 center,
                                 CreativeTerrainCoord2 coord,
                                 std::uint16_t radiusCells) noexcept {
  const std::int64_t dx = static_cast<std::int64_t>(coord.x) - center.x;
  const std::int64_t dz = static_cast<std::int64_t>(coord.z) - center.z;
  const std::int64_t radius = radiusCells;
  if (radius <= 0 || dx < -radius || dx > radius || dz < -radius ||
      dz > radius) {
    return false;
  }
  return dx * dx + dz * dz <= radius * radius;
}

std::uint32_t creativeTerrainLinearFalloffWeightQ16(
    CreativeTerrainCoord2 center,
    CreativeTerrainCoord2 coord,
    std::uint16_t radiusCells) noexcept {
  constexpr std::uint32_t kDistanceScale = 256U;
  const std::int64_t dx = static_cast<std::int64_t>(center.x) - coord.x;
  const std::int64_t dz = static_cast<std::int64_t>(center.z) - coord.z;
  const std::int64_t radius = radiusCells;
  if (radius <= 0 || dx < -radius || dx > radius || dz < -radius ||
      dz > radius) {
    return 0U;
  }
  const std::uint64_t distanceSquared =
      static_cast<std::uint64_t>(dx * dx + dz * dz);
  const std::uint64_t radiusSquared =
      static_cast<std::uint64_t>(radiusCells) * radiusCells;
  if (distanceSquared >= radiusSquared) {
    return 0U;
  }
  const std::uint32_t distance = static_cast<std::uint32_t>(
      creativeTerrainIntegerSquareRoot(distanceSquared * kDistanceScale *
                                       kDistanceScale));
  const std::uint32_t radiusQ8 = radiusCells * kDistanceScale;
  return static_cast<std::uint32_t>(
      (static_cast<std::uint64_t>(radiusQ8 - distance) *
           kCreativeTerrainQ16One +
       radiusQ8 / 2U) /
      radiusQ8);
}

std::uint32_t creativeTerrainSmoothstepWeightQ16(
    std::uint32_t linearWeightQ16) noexcept {
  const std::uint64_t linear =
      std::min(linearWeightQ16, kCreativeTerrainQ16One);
  const std::uint64_t scaleSquared =
      static_cast<std::uint64_t>(kCreativeTerrainQ16One) *
      kCreativeTerrainQ16One;
  const std::uint64_t smooth = linear * linear *
                               (3U * kCreativeTerrainQ16One - 2U * linear);
  return static_cast<std::uint32_t>((smooth + scaleSquared / 2U) /
                                    scaleSquared);
}

std::uint32_t creativeTerrainNormalizedDistanceQ16(
    CreativeTerrainCoord2 center,
    CreativeTerrainCoord2 coord,
    std::uint16_t radiusCells) noexcept {
  if (!creativeTerrainInsideRadius(center, coord, radiusCells)) {
    return kCreativeTerrainQ16One;
  }
  const std::int64_t dx = static_cast<std::int64_t>(coord.x) - center.x;
  const std::int64_t dz = static_cast<std::int64_t>(coord.z) - center.z;
  const std::uint64_t distanceSquared =
      static_cast<std::uint64_t>(dx * dx + dz * dz);
  const std::uint64_t distanceQ16 = creativeTerrainIntegerSquareRoot(
      distanceSquared * kCreativeTerrainQ16One * kCreativeTerrainQ16One);
  return static_cast<std::uint32_t>(std::min<std::uint64_t>(
      kCreativeTerrainQ16One,
      (distanceQ16 + radiusCells / 2U) / radiusCells));
}

std::int32_t creativeTerrainSinTurnsQ15(
    std::int64_t phaseTurnsQ16) noexcept {
  const std::int64_t phase = normalizedPhase(phaseTurnsQ16);
  const std::int64_t quadrant = phase / kQuarterTurnQ16;
  const std::int64_t offset = phase % kQuarterTurnQ16;
  const bool mirrored = quadrant == 1 || quadrant == 3;
  const bool negative = quadrant >= 2;
  const std::int64_t quarterPhase =
      mirrored ? kQuarterTurnQ16 - offset : offset;
  const std::int32_t magnitude = quarterWaveSample(quarterPhase);
  return negative ? -magnitude : magnitude;
}

std::int32_t creativeTerrainCosTurnsQ15(
    std::int64_t phaseTurnsQ16) noexcept {
  return creativeTerrainSinTurnsQ15(phaseTurnsQ16 + kQuarterTurnQ16);
}

std::int32_t creativeTerrainCosineBellQ15(
    std::uint32_t normalizedQ16) noexcept {
  if (normalizedQ16 >= kCreativeTerrainQ16One) {
    return 0;
  }
  const std::int32_t cosine = creativeTerrainCosTurnsQ15(
      static_cast<std::int64_t>(normalizedQ16) / 2);
  return static_cast<std::int32_t>((kCreativeTerrainQ15One + cosine + 1) / 2);
}

std::uint64_t creativeTerrainQuarterWaveChecksum() noexcept {
  std::uint64_t checksum = 0U;
  for (std::size_t index = 0U; index < kQuarterWaveQ15.size(); ++index) {
    checksum += (index + 1U) *
                static_cast<std::uint64_t>(kQuarterWaveQ15[index]);
  }
  return checksum;
}

}  // namespace iggy3d::creative
