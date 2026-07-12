#include "app/iggy3d/creative/tools/TerrainProfile.hpp"

#include "app/iggy3d/creative/tools/TerrainBrushKernel.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>

namespace iggy3d::creative {
namespace {

template <typename Enum>
[[nodiscard]] constexpr bool validEnum(Enum value, Enum count) noexcept {
  return static_cast<std::size_t>(value) < static_cast<std::size_t>(count);
}

[[nodiscard]] constexpr bool coordLess(CreativeTerrainCoord2 lhs,
                                       CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z < rhs.z || (lhs.z == rhs.z && lhs.x < rhs.x);
}

[[nodiscard]] bool offsetCoord(CreativeTerrainCoord2 center,
                               std::int32_t dx,
                               std::int32_t dz,
                               CreativeTerrainCoord2& output) noexcept {
  const std::int64_t x = static_cast<std::int64_t>(center.x) + dx;
  const std::int64_t z = static_cast<std::int64_t>(center.z) + dz;
  if (x < std::numeric_limits<std::int32_t>::min() ||
      x > std::numeric_limits<std::int32_t>::max() ||
      z < std::numeric_limits<std::int32_t>::min() ||
      z > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)};
  return true;
}

[[nodiscard]] bool allowedDiscreteValue(std::uint16_t value,
                                        std::span<const std::uint16_t> values) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

[[nodiscard]] bool validRequest(
    const CreativeTerrainProfileRequest& request) noexcept {
  constexpr std::array<std::uint16_t, 3U> radii{{2U, 4U, 8U}};
  constexpr std::array<std::uint16_t, 5U> amplitudes{{1U, 2U, 4U, 8U, 16U}};
  constexpr std::array<std::uint16_t, 3U> spacings{{1U, 2U, 4U}};
  return request.field != nullptr && request.field->validateInvariants() &&
         validEnum(request.profile, CreativeTerrainProfileKind::Count) &&
         validEnum(request.blend, CreativeTerrainProfileBlend::Count) &&
         validEnum(request.rodPolicy,
                   CreativeTerrainProfileRodPolicy::Count) &&
         validEnum(request.direction,
                   CreativeTerrainProfileDirection::Count) &&
         request.baseHeightCells >= kCreativeTerrainMinimumHeightCells &&
         request.baseHeightCells <= kCreativeTerrainMaximumHeightCells &&
         allowedDiscreteValue(request.radiusCells, radii) &&
         allowedDiscreteValue(request.amplitudeCells, amplitudes) &&
         allowedDiscreteValue(request.spacingCells, spacings) &&
         request.frequency >= 1U && request.frequency <= 2U;
}

[[nodiscard]] bool oscillatory(CreativeTerrainProfileKind profile) noexcept {
  return profile == CreativeTerrainProfileKind::Wave ||
         profile == CreativeTerrainProfileKind::Ripple;
}

struct DirectionRow {
  std::int32_t xQ15 = 0;
  std::int32_t zQ15 = 0;
};

[[nodiscard]] DirectionRow directionRow(
    CreativeTerrainProfileDirection direction) noexcept {
  constexpr std::int32_t diagonal = 23'170;
  constexpr std::array<DirectionRow, 8U> rows{{
      {kCreativeTerrainQ15One, 0},
      {diagonal, diagonal},
      {0, kCreativeTerrainQ15One},
      {-diagonal, diagonal},
      {-kCreativeTerrainQ15One, 0},
      {-diagonal, -diagonal},
      {0, -kCreativeTerrainQ15One},
      {diagonal, -diagonal},
  }};
  const std::size_t index = static_cast<std::size_t>(direction);
  return index < rows.size() ? rows[index] : DirectionRow{};
}

[[nodiscard]] std::int32_t normalizedDirectionalQ16(
    std::int64_t dotCellsQ15,
    std::uint16_t radiusCells) noexcept {
  return static_cast<std::int32_t>(creativeTerrainRoundDivideSymmetric(
      dotCellsQ15 * kCreativeTerrainQ16One,
      static_cast<std::int64_t>(radiusCells) * kCreativeTerrainQ15One));
}

[[nodiscard]] std::int32_t profileWeightQ15(
    const CreativeTerrainProfileRequest& request,
    CreativeTerrainCoord2 coord) noexcept {
  constexpr std::uint32_t kCraterInnerRadiusQ16 = 45'875U;
  constexpr std::uint32_t kCraterRimCenterQ16 = 51'118U;
  constexpr std::uint32_t kCraterRimWidthQ16 = 14'418U;
  constexpr std::int32_t kCraterRimScaleQ15 = 11'468;

  const std::uint32_t radialQ16 = creativeTerrainNormalizedDistanceQ16(
      request.center, coord, request.radiusCells);
  const std::int32_t bell = creativeTerrainCosineBellQ15(radialQ16);
  switch (request.profile) {
    case CreativeTerrainProfileKind::Hill:
      return bell;
    case CreativeTerrainProfileKind::Basin:
      return -bell;
    case CreativeTerrainProfileKind::Ring:
      return creativeTerrainSinTurnsQ15(
          static_cast<std::int64_t>(radialQ16) / 2);
    case CreativeTerrainProfileKind::Crater: {
      const std::uint32_t innerQ16 = static_cast<std::uint32_t>(
          (static_cast<std::uint64_t>(radialQ16) *
               kCreativeTerrainQ16One +
           kCraterInnerRadiusQ16 / 2U) /
          kCraterInnerRadiusQ16);
      const std::uint32_t rimDistance =
          radialQ16 > kCraterRimCenterQ16
              ? radialQ16 - kCraterRimCenterQ16
              : kCraterRimCenterQ16 - radialQ16;
      const std::uint32_t rimQ16 = static_cast<std::uint32_t>(
          (static_cast<std::uint64_t>(rimDistance) *
               kCreativeTerrainQ16One +
           kCraterRimWidthQ16 / 2U) /
          kCraterRimWidthQ16);
      const std::int32_t inner = creativeTerrainCosineBellQ15(innerQ16);
      const std::int32_t rim = creativeTerrainMultiplyQ15(
          kCraterRimScaleQ15, creativeTerrainCosineBellQ15(rimQ16));
      return std::clamp(-inner + rim, -kCreativeTerrainQ15One,
                        kCreativeTerrainQ15One);
    }
    case CreativeTerrainProfileKind::Ridge:
    case CreativeTerrainProfileKind::Wave: {
      const DirectionRow direction = directionRow(request.direction);
      const std::int64_t dx = static_cast<std::int64_t>(coord.x) -
                              request.center.x;
      const std::int64_t dz = static_cast<std::int64_t>(coord.z) -
                              request.center.z;
      const std::int32_t alongQ16 = normalizedDirectionalQ16(
          dx * direction.xQ15 + dz * direction.zQ15,
          request.radiusCells);
      if (request.profile == CreativeTerrainProfileKind::Wave) {
        const std::int64_t phaseQ16 = creativeTerrainRoundDivideSymmetric(
            static_cast<std::int64_t>(alongQ16 +
                                      static_cast<std::int32_t>(
                                          kCreativeTerrainQ16One)) *
                request.frequency,
            2);
        return creativeTerrainMultiplyQ15(
            creativeTerrainSinTurnsQ15(phaseQ16), bell);
      }
      const std::int32_t acrossQ16 = normalizedDirectionalQ16(
          -dx * direction.zQ15 + dz * direction.xQ15,
          request.radiusCells);
      const std::uint32_t acrossProfileQ16 = static_cast<std::uint32_t>(
          std::min<std::int64_t>(kCreativeTerrainQ16One,
                                 std::abs(static_cast<std::int64_t>(
                                     acrossQ16)) *
                                     3));
      const std::uint32_t alongProfileQ16 = static_cast<std::uint32_t>(
          std::min<std::int64_t>(kCreativeTerrainQ16One,
                                 std::abs(static_cast<std::int64_t>(
                                     alongQ16))));
      return creativeTerrainMultiplyQ15(
          creativeTerrainCosineBellQ15(acrossProfileQ16),
          creativeTerrainCosineBellQ15(alongProfileQ16));
    }
    case CreativeTerrainProfileKind::Ripple:
      return creativeTerrainMultiplyQ15(
          creativeTerrainCosTurnsQ15(
              static_cast<std::int64_t>(radialQ16) * request.frequency),
          bell);
    case CreativeTerrainProfileKind::Count:
      break;
  }
  return 0;
}

[[nodiscard]] CreativeTerrainProfilePlan rejectedPlan(
    CreativeTerrainProfilePlan plan,
    CreativeTerrainProfilePlanStatus status,
    std::string_view reasonCode) noexcept {
  plan.accepted = false;
  plan.status = status;
  plan.editCount = 0U;
  plan.reasonCode = reasonCode;
  return plan;
}

}  // namespace

std::string_view toString(CreativeTerrainProfileKind value) noexcept {
  constexpr std::array names{"HILL", "BASIN", "RING", "CRATER", "RIDGE",
                             "WAVE", "RIPPLE"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainProfileBlend value) noexcept {
  constexpr std::array names{"SET", "ADD"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainProfileRodPolicy value) noexcept {
  constexpr std::array names{"FILL", "EXISTING"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainProfileRadius value) noexcept {
  constexpr std::array names{"2 CELLS", "4 CELLS", "8 CELLS"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainProfileAmplitude value) noexcept {
  constexpr std::array names{"1 CELL", "2 CELLS", "4 CELLS", "8 CELLS",
                             "16 CELLS"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainProfileSpacing value) noexcept {
  constexpr std::array names{"1 CELL", "2 CELLS", "4 CELLS"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainProfileDirection value) noexcept {
  constexpr std::array names{"+X", "+X +Z", "+Z", "-X +Z", "-X",
                             "-X -Z", "-Z", "+X -Z"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainProfileFrequency value) noexcept {
  constexpr std::array names{"1 CYCLE", "2 CYCLES"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainProfilePlanStatus value) noexcept {
  constexpr std::array names{
      "NotRequested",      "InvalidRequest",   "CoordinateOverflow",
      "CapacityExceeded",  "SamplingUnproven", "UnderSampled",
      "NoControlsInBrush", "NoChange",         "Ready"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "Unknown";
}

std::uint16_t creativeTerrainProfileRadiusCells(
    CreativeTerrainProfileRadius value) noexcept {
  constexpr std::array<std::uint16_t, 3U> values{{2U, 4U, 8U}};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < values.size() ? values[index] : 0U;
}

std::uint16_t creativeTerrainProfileAmplitudeCells(
    CreativeTerrainProfileAmplitude value) noexcept {
  constexpr std::array<std::uint16_t, 5U> values{{1U, 2U, 4U, 8U, 16U}};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < values.size() ? values[index] : 0U;
}

std::uint16_t creativeTerrainProfileSpacingCells(
    CreativeTerrainProfileSpacing value) noexcept {
  constexpr std::array<std::uint16_t, 3U> values{{1U, 2U, 4U}};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < values.size() ? values[index] : 0U;
}

std::uint8_t creativeTerrainProfileFrequencyCycles(
    CreativeTerrainProfileFrequency value) noexcept {
  const std::size_t index = static_cast<std::size_t>(value);
  return index < static_cast<std::size_t>(CreativeTerrainProfileFrequency::Count)
             ? static_cast<std::uint8_t>(index + 1U)
             : 0U;
}

bool creativeTerrainProfileUsesDirection(
    CreativeTerrainProfileKind value) noexcept {
  return value == CreativeTerrainProfileKind::Ridge ||
         value == CreativeTerrainProfileKind::Wave;
}

bool creativeTerrainProfileUsesFrequency(
    CreativeTerrainProfileKind value) noexcept {
  return value == CreativeTerrainProfileKind::Wave ||
         value == CreativeTerrainProfileKind::Ripple;
}

CreativeTerrainProfilePlan buildCreativeTerrainProfilePlan(
    const CreativeTerrainProfileRequest& request) noexcept {
  CreativeTerrainProfilePlan plan;
  plan.requested = true;
  if (!validRequest(request)) {
    return rejectedPlan(plan, CreativeTerrainProfilePlanStatus::InvalidRequest,
                        "creative_terrain_profile_invalid_request");
  }
  if (oscillatory(request.profile) &&
      request.rodPolicy == CreativeTerrainProfileRodPolicy::Existing) {
    return rejectedPlan(plan,
                        CreativeTerrainProfilePlanStatus::SamplingUnproven,
                        "creative_terrain_profile_sampling_unproven");
  }
  if ((request.profile == CreativeTerrainProfileKind::Wave &&
       2U * request.frequency * request.spacingCells > request.radiusCells) ||
      (request.profile == CreativeTerrainProfileKind::Ripple &&
       4U * request.frequency * request.spacingCells > request.radiusCells)) {
    return rejectedPlan(plan, CreativeTerrainProfilePlanStatus::UnderSampled,
                        "creative_terrain_profile_under_sampled");
  }

  std::array<CreativeTerrainCoord2, kCreativeTerrainControlCapacity> lattice{};
  std::size_t latticeCount = 0U;
  if (request.rodPolicy == CreativeTerrainProfileRodPolicy::Fill) {
    const std::int32_t radius = request.radiusCells;
    const std::int32_t spacing = request.spacingCells;
    for (std::int32_t dz = -radius; dz <= radius; dz += spacing) {
      for (std::int32_t dx = -radius; dx <= radius; dx += spacing) {
        if (dx * dx + dz * dz > radius * radius) {
          continue;
        }
        CreativeTerrainCoord2 coord{};
        if (!offsetCoord(request.center, dx, dz, coord)) {
          return rejectedPlan(
              plan, CreativeTerrainProfilePlanStatus::CoordinateOverflow,
              "creative_terrain_profile_coordinate_overflow");
        }
        if (latticeCount >= lattice.size()) {
          return rejectedPlan(
              plan, CreativeTerrainProfilePlanStatus::CapacityExceeded,
              "creative_terrain_profile_candidate_capacity_exceeded");
        }
        lattice[latticeCount++] = coord;
      }
    }
  }

  std::array<CreativeTerrainCoord2, kCreativeTerrainControlCapacity> existing{};
  std::size_t existingCount = 0U;
  for (const CreativeTerrainControlPoint& control : request.field->controls()) {
    if (creativeTerrainInsideRadius(request.center, control.coord,
                                    request.radiusCells)) {
      existing[existingCount++] = control.coord;
    }
  }
  if (request.rodPolicy == CreativeTerrainProfileRodPolicy::Existing &&
      existingCount == 0U) {
    return rejectedPlan(plan,
                        CreativeTerrainProfilePlanStatus::NoControlsInBrush,
                        "creative_terrain_profile_no_controls");
  }

  std::array<CreativeTerrainCoord2, kCreativeTerrainControlCapacity> candidates{};
  std::size_t candidateCount = 0U;
  std::size_t existingIndex = 0U;
  std::size_t latticeIndex = 0U;
  while (existingIndex < existingCount || latticeIndex < latticeCount) {
    CreativeTerrainCoord2 next{};
    if (latticeIndex >= latticeCount ||
        (existingIndex < existingCount &&
         coordLess(existing[existingIndex], lattice[latticeIndex]))) {
      next = existing[existingIndex++];
    } else if (existingIndex >= existingCount ||
               coordLess(lattice[latticeIndex], existing[existingIndex])) {
      next = lattice[latticeIndex++];
    } else {
      next = existing[existingIndex++];
      ++latticeIndex;
    }
    if (candidateCount >= candidates.size()) {
      plan.candidateCount = static_cast<std::uint16_t>(candidateCount + 1U);
      return rejectedPlan(
          plan, CreativeTerrainProfilePlanStatus::CapacityExceeded,
          "creative_terrain_profile_candidate_capacity_exceeded");
    }
    candidates[candidateCount++] = next;
  }
  plan.candidateCount = static_cast<std::uint16_t>(candidateCount);

  std::size_t newControlCount = 0U;
  for (std::size_t index = 0U; index < candidateCount; ++index) {
    if (request.field->controlAt(candidates[index]) == nullptr) {
      ++newControlCount;
    }
  }
  if (request.field->controlCount() + newControlCount >
      kCreativeTerrainControlCapacity) {
    return rejectedPlan(plan, CreativeTerrainProfilePlanStatus::CapacityExceeded,
                        "creative_terrain_profile_field_capacity_exceeded");
  }

  for (std::size_t index = 0U; index < candidateCount; ++index) {
    const CreativeTerrainCoord2 coord = candidates[index];
    const CreativeTerrainControlPoint* existingControl =
        request.field->controlAt(coord);
    std::uint16_t baseline = request.baseHeightCells;
    if (request.blend == CreativeTerrainProfileBlend::Add) {
      if (existingControl != nullptr) {
        baseline = existingControl->heightCells;
      } else {
        const CreativeTerrainHeightSample sample =
            sampleCreativeTerrainHeight(*request.field, coord);
        if (sample.present) {
          baseline = sample.heightCells;
        }
      }
    }
    const std::int32_t weight = profileWeightQ15(request, coord);
    const std::int64_t delta = creativeTerrainRoundDivideSymmetric(
        static_cast<std::int64_t>(request.amplitudeCells) * weight,
        kCreativeTerrainQ15One);
    const std::uint16_t nextHeight = static_cast<std::uint16_t>(
        std::clamp<std::int64_t>(static_cast<std::int64_t>(baseline) + delta,
                                 kCreativeTerrainMinimumHeightCells,
                                 kCreativeTerrainMaximumHeightCells));
    const std::uint16_t nextRadius =
        existingControl != nullptr
            ? existingControl->radiusCells
            : static_cast<std::uint16_t>(request.spacingCells * 2U);
    const CreativeTerrainControlPoint next{coord, nextHeight, nextRadius};
    if (!isValidCreativeTerrainControlPoint(next)) {
      return rejectedPlan(
          plan, CreativeTerrainProfilePlanStatus::CoordinateOverflow,
          "creative_terrain_profile_control_invalid");
    }
    if (existingControl != nullptr && *existingControl == next) {
      continue;
    }
    if (plan.editCount >= plan.edits.size()) {
      return rejectedPlan(plan,
                          CreativeTerrainProfilePlanStatus::CapacityExceeded,
                          "creative_terrain_profile_edit_capacity_exceeded");
    }
    plan.edits[plan.editCount++] = {CreativeTerrainEditKind::Upsert, next};
  }

  plan.accepted = true;
  if (plan.editCount == 0U) {
    plan.status = CreativeTerrainProfilePlanStatus::NoChange;
    plan.reasonCode = "creative_terrain_profile_no_change";
    return plan;
  }
  plan.status = CreativeTerrainProfilePlanStatus::Ready;
  plan.reasonCode = "creative_terrain_profile_ready";
  return plan;
}

}  // namespace iggy3d::creative
