#include "app/iggy3d/creative/tools/TerrainProfile.hpp"

#include "app/iggy3d/creative/tools/TerrainBrushKernel.hpp"

#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <vector>

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

[[nodiscard]] bool profileCoordinatesSafe(CreativeTerrainCoord2 center,
                                          std::uint16_t radiusCells,
                                          std::uint16_t spacingCells) noexcept {
  const std::int64_t border =
      static_cast<std::int64_t>(radiusCells) + spacingCells;
  return static_cast<std::int64_t>(center.x) - border >=
             std::numeric_limits<std::int32_t>::min() &&
         static_cast<std::int64_t>(center.x) + border <=
             std::numeric_limits<std::int32_t>::max() &&
         static_cast<std::int64_t>(center.z) - border >=
             std::numeric_limits<std::int32_t>::min() &&
         static_cast<std::int64_t>(center.z) + border <=
             std::numeric_limits<std::int32_t>::max();
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

[[nodiscard]] bool validRecipeValues(
    const CreativeTerrainProfileRecipe& recipe) noexcept {
  return recipe.version == kCreativeTerrainProfileRecipeVersion &&
         validEnum(recipe.profile, CreativeTerrainProfileKind::Count) &&
         validEnum(recipe.blend, CreativeTerrainProfileBlend::Count) &&
         validEnum(recipe.rodPolicy,
                   CreativeTerrainProfileRodPolicy::Count) &&
         validEnum(recipe.direction,
                   CreativeTerrainProfileDirection::Count) &&
         recipe.baseHeightCells >= kCreativeTerrainMinimumHeightCells &&
         recipe.baseHeightCells <= kCreativeTerrainMaximumHeightCells &&
         recipe.radiusCells >= 1U &&
         recipe.radiusCells <= kCreativeTerrainProfileMaximumRadiusCells &&
         recipe.amplitudeCells >= 1U &&
         recipe.amplitudeCells <= kCreativeTerrainMaximumHeightCells &&
         recipe.spacingCells >= 1U &&
         recipe.spacingCells <= kCreativeTerrainProfileMaximumSpacingCells &&
         recipe.frequency >= 1U &&
         recipe.frequency <= kCreativeTerrainProfileMaximumFrequency;
}

[[nodiscard]] bool oscillatory(CreativeTerrainProfileKind profile) noexcept {
  return profile == CreativeTerrainProfileKind::Wave ||
         profile == CreativeTerrainProfileKind::Ripple;
}

template <typename Enum, std::size_t Extent>
[[nodiscard]] bool parseNamedEnum(
    std::string_view text,
    std::span<const Enum, Extent> values,
    Enum& output) noexcept {
  for (const Enum value : values) {
    if (text == toString(value)) {
      output = value;
      return true;
    }
  }
  return false;
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

[[nodiscard]] std::uint32_t profilePhaseQ16(std::uint64_t seed) noexcept {
  if (seed == 0U) {
    return 0U;
  }
  std::uint64_t mixed = seed + 0x9e3779b97f4a7c15ULL;
  mixed = (mixed ^ (mixed >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  mixed = (mixed ^ (mixed >> 27U)) * 0x94d049bb133111ebULL;
  mixed ^= mixed >> 31U;
  return static_cast<std::uint32_t>(mixed & 0xffffU);
}

[[nodiscard]] std::int32_t rawProfileWeightQ15(
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
            2) + profilePhaseQ16(request.seed);
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
              static_cast<std::int64_t>(radialQ16) * request.frequency +
              profilePhaseQ16(request.seed)),
          bell);
    case CreativeTerrainProfileKind::Count:
      break;
  }
  return 0;
}

[[nodiscard]] std::int64_t floorDiv(std::int64_t value,
                                    std::int64_t divisor) noexcept {
  const std::int64_t quotient = value / divisor;
  const std::int64_t remainder = value % divisor;
  return remainder < 0 ? quotient - 1 : quotient;
}

[[nodiscard]] std::int32_t interpolateProfileWeight(
    std::int32_t first,
    std::int32_t second,
    std::int64_t numerator,
    std::int64_t denominator) noexcept {
  return static_cast<std::int32_t>(creativeTerrainRoundDivideSymmetric(
      static_cast<std::int64_t>(first) * (denominator - numerator) +
          static_cast<std::int64_t>(second) * numerator,
      denominator));
}

[[nodiscard]] std::int32_t profileWeightQ15(
    const CreativeTerrainProfileRequest& request,
    CreativeTerrainCoord2 coord) noexcept {
  if (request.spacingCells <= 1U) {
    return rawProfileWeightQ15(request, coord);
  }
  const std::int64_t spacing = request.spacingCells;
  const std::int64_t relativeX =
      static_cast<std::int64_t>(coord.x) - request.center.x;
  const std::int64_t relativeZ =
      static_cast<std::int64_t>(coord.z) - request.center.z;
  const std::int64_t lowerX = floorDiv(relativeX, spacing) * spacing;
  const std::int64_t lowerZ = floorDiv(relativeZ, spacing) * spacing;
  const std::int64_t fractionX = relativeX - lowerX;
  const std::int64_t fractionZ = relativeZ - lowerZ;
  const auto sample = [&](std::int64_t x, std::int64_t z) {
    return rawProfileWeightQ15(
        request,
        {static_cast<std::int32_t>(
             static_cast<std::int64_t>(request.center.x) + x),
         static_cast<std::int32_t>(
             static_cast<std::int64_t>(request.center.z) + z)});
  };
  const std::int32_t lower = interpolateProfileWeight(
      sample(lowerX, lowerZ), sample(lowerX + spacing, lowerZ), fractionX,
      spacing);
  const std::int32_t upper = interpolateProfileWeight(
      sample(lowerX, lowerZ + spacing),
      sample(lowerX + spacing, lowerZ + spacing), fractionX, spacing);
  return interpolateProfileWeight(lower, upper, fractionZ, spacing);
}

[[nodiscard]] bool validCanonicalSource(
    const CreativeTerrainSurfacePlan& source) noexcept {
  if (!source.accepted ||
      (source.status != CreativeTerrainSurfacePlanStatus::Empty &&
       source.status != CreativeTerrainSurfacePlanStatus::Ready)) {
    return false;
  }
  for (std::size_t index = 0U; index < source.columns.size(); ++index) {
    const CreativeTerrainColumn& column = source.columns[index];
    if (column.heightCells < kCreativeTerrainMinimumHeightCells ||
        column.heightCells > kCreativeTerrainMaximumHeightCells ||
        (index > 0U &&
         !coordLess(source.columns[index - 1U].coord, column.coord))) {
      return false;
    }
  }
  return source.status == CreativeTerrainSurfacePlanStatus::Empty
             ? source.columns.empty()
             : !source.columns.empty();
}

[[nodiscard]] std::uint16_t sourceHeightAt(
    const CreativeTerrainHeightField& existing,
    const CreativeTerrainSurfacePlan& canonical,
    CreativeTerrainCoord2 coord) noexcept {
  if (existing.contains(coord)) {
    return existing.heightAt(coord).value_or(kCreativeTerrainEmptyHeightCells);
  }
  const auto found = std::lower_bound(
      canonical.columns.begin(), canonical.columns.end(), coord,
      [](const CreativeTerrainColumn& column, CreativeTerrainCoord2 candidate) {
        return coordLess(column.coord, candidate);
      });
  return found != canonical.columns.end() && found->coord == coord
             ? found->heightCells
             : kCreativeTerrainEmptyHeightCells;
}

[[nodiscard]] bool profileBounds(
    const CreativeTerrainProfileRecipe& recipe,
    CreativeTerrainHeightFieldBounds& output) noexcept {
  const std::int64_t minimumX =
      static_cast<std::int64_t>(recipe.center.x) - recipe.radiusCells;
  const std::int64_t minimumZ =
      static_cast<std::int64_t>(recipe.center.z) - recipe.radiusCells;
  const std::uint64_t diameter =
      static_cast<std::uint64_t>(recipe.radiusCells) * 2U + 1U;
  if (minimumX < std::numeric_limits<std::int32_t>::min() ||
      minimumZ < std::numeric_limits<std::int32_t>::min() ||
      diameter > std::numeric_limits<std::uint16_t>::max()) {
    return false;
  }
  output = {{static_cast<std::int32_t>(minimumX),
             static_cast<std::int32_t>(minimumZ)},
            static_cast<std::uint16_t>(diameter),
            static_cast<std::uint16_t>(diameter)};
  return isValidCreativeTerrainHeightFieldBounds(output);
}

[[nodiscard]] bool unionBounds(
    const CreativeTerrainHeightField& existing,
    CreativeTerrainHeightFieldBounds profile,
    CreativeTerrainHeightFieldBounds& output,
    bool& expanded) noexcept {
  if (existing.cellCount() == 0U) {
    output = profile;
    expanded = false;
    return isValidCreativeTerrainHeightFieldBounds(output);
  }
  const CreativeTerrainHeightFieldBounds current = existing.bounds();
  const std::int64_t minimumX =
      std::min<std::int64_t>(current.minimum.x, profile.minimum.x);
  const std::int64_t minimumZ =
      std::min<std::int64_t>(current.minimum.z, profile.minimum.z);
  const std::int64_t maximumXExclusive = std::max(
      static_cast<std::int64_t>(current.minimum.x) + current.widthCells,
      static_cast<std::int64_t>(profile.minimum.x) + profile.widthCells);
  const std::int64_t maximumZExclusive = std::max(
      static_cast<std::int64_t>(current.minimum.z) + current.depthCells,
      static_cast<std::int64_t>(profile.minimum.z) + profile.depthCells);
  const std::int64_t width = maximumXExclusive - minimumX;
  const std::int64_t depth = maximumZExclusive - minimumZ;
  if (minimumX < std::numeric_limits<std::int32_t>::min() ||
      minimumX > std::numeric_limits<std::int32_t>::max() ||
      minimumZ < std::numeric_limits<std::int32_t>::min() ||
      minimumZ > std::numeric_limits<std::int32_t>::max() || width <= 0 ||
      depth <= 0 || width > std::numeric_limits<std::uint16_t>::max() ||
      depth > std::numeric_limits<std::uint16_t>::max()) {
    return false;
  }
  output = {{static_cast<std::int32_t>(minimumX),
             static_cast<std::int32_t>(minimumZ)},
            static_cast<std::uint16_t>(width),
            static_cast<std::uint16_t>(depth)};
  expanded = output != current;
  return isValidCreativeTerrainHeightFieldBounds(output);
}

[[nodiscard]] std::uint64_t heightHash(
    CreativeTerrainHeightFieldBounds bounds,
    const std::vector<std::uint16_t>& heights) noexcept {
  StableHasher hasher;
  hasher.addI64(bounds.minimum.x);
  hasher.addI64(bounds.minimum.z);
  hasher.addU64(bounds.widthCells);
  hasher.addU64(bounds.depthCells);
  for (const std::uint16_t height : heights) {
    hasher.addU64(height);
  }
  return hasher.value();
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

bool parseCreativeTerrainProfileKind(
    std::string_view text,
    CreativeTerrainProfileKind& output) noexcept {
  constexpr std::array values{
      CreativeTerrainProfileKind::Hill,   CreativeTerrainProfileKind::Basin,
      CreativeTerrainProfileKind::Ring,   CreativeTerrainProfileKind::Crater,
      CreativeTerrainProfileKind::Ridge,  CreativeTerrainProfileKind::Wave,
      CreativeTerrainProfileKind::Ripple,
  };
  return parseNamedEnum(text, std::span{values}, output);
}

std::string_view toString(CreativeTerrainProfileBlend value) noexcept {
  constexpr std::array names{"SET", "ADD"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

bool parseCreativeTerrainProfileBlend(
    std::string_view text,
    CreativeTerrainProfileBlend& output) noexcept {
  constexpr std::array values{CreativeTerrainProfileBlend::Set,
                              CreativeTerrainProfileBlend::Add};
  return parseNamedEnum(text, std::span{values}, output);
}

std::string_view toString(CreativeTerrainProfileRodPolicy value) noexcept {
  constexpr std::array names{"FILL", "EXISTING"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

bool parseCreativeTerrainProfileRodPolicy(
    std::string_view text,
    CreativeTerrainProfileRodPolicy& output) noexcept {
  constexpr std::array values{CreativeTerrainProfileRodPolicy::Fill,
                              CreativeTerrainProfileRodPolicy::Existing};
  return parseNamedEnum(text, std::span{values}, output);
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

bool parseCreativeTerrainProfileDirection(
    std::string_view text,
    CreativeTerrainProfileDirection& output) noexcept {
  constexpr std::array values{
      CreativeTerrainProfileDirection::PositiveX,
      CreativeTerrainProfileDirection::PositiveXPositiveZ,
      CreativeTerrainProfileDirection::PositiveZ,
      CreativeTerrainProfileDirection::NegativeXPositiveZ,
      CreativeTerrainProfileDirection::NegativeX,
      CreativeTerrainProfileDirection::NegativeXNegativeZ,
      CreativeTerrainProfileDirection::NegativeZ,
      CreativeTerrainProfileDirection::PositiveXNegativeZ,
  };
  return parseNamedEnum(text, std::span{values}, output);
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

bool creativeTerrainProfileUsesSeed(
    CreativeTerrainProfileKind value) noexcept {
  return creativeTerrainProfileUsesFrequency(value);
}

CreativeTerrainProfilePlan buildCreativeTerrainProfilePlan(
    const CreativeTerrainProfileRequest& request) noexcept {
  CreativeTerrainProfilePlan plan;
  plan.requested = true;
  if (!validRequest(request)) {
    return rejectedPlan(plan, CreativeTerrainProfilePlanStatus::InvalidRequest,
                        "creative_terrain_profile_invalid_request");
  }
  if (!profileCoordinatesSafe(request.center, request.radiusCells,
                              request.spacingCells)) {
    return rejectedPlan(
        plan, CreativeTerrainProfilePlanStatus::CoordinateOverflow,
        "creative_terrain_profile_coordinate_overflow");
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

bool isValidCreativeTerrainProfileRecipe(
    const CreativeTerrainProfileRecipe& recipe) noexcept {
  return validRecipeValues(recipe) &&
         profileCoordinatesSafe(recipe.center, recipe.radiusCells,
                                recipe.spacingCells);
}

std::string_view toString(
    CreativeTerrainProfileRecipeStatus status) noexcept {
  switch (status) {
    case CreativeTerrainProfileRecipeStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainProfileRecipeStatus::UnsupportedVersion:
      return "UnsupportedVersion";
    case CreativeTerrainProfileRecipeStatus::InvalidRecipe:
      return "InvalidRecipe";
    case CreativeTerrainProfileRecipeStatus::InvalidSource:
      return "InvalidSource";
    case CreativeTerrainProfileRecipeStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeTerrainProfileRecipeStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainProfileRecipeStatus::UnderSampled:
      return "UnderSampled";
    case CreativeTerrainProfileRecipeStatus::NoSourceInFootprint:
      return "NoSourceInFootprint";
    case CreativeTerrainProfileRecipeStatus::OutputRejected:
      return "OutputRejected";
    case CreativeTerrainProfileRecipeStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeTerrainProfileRecipeResult buildCreativeTerrainProfileRecipe(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainProfileRecipe& recipe) {
  CreativeTerrainProfileRecipeResult result;
  result.recipe = recipe;
  CreativeTerrainProfileRecipeReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.sourceColumnCount = canonicalSource.columns.size();
  if (recipe.version != kCreativeTerrainProfileRecipeVersion) {
    receipt.status = CreativeTerrainProfileRecipeStatus::UnsupportedVersion;
    receipt.reasonCode = "creative_terrain_profile_recipe_version_unsupported";
    return result;
  }
  if (!validRecipeValues(recipe)) {
    receipt.status = CreativeTerrainProfileRecipeStatus::InvalidRecipe;
    receipt.reasonCode = "creative_terrain_profile_recipe_invalid";
    return result;
  }
  if (!profileCoordinatesSafe(recipe.center, recipe.radiusCells,
                              recipe.spacingCells)) {
    receipt.status = CreativeTerrainProfileRecipeStatus::CoordinateOverflow;
    receipt.reasonCode = "creative_terrain_profile_recipe_coordinate_overflow";
    return result;
  }
  if (!existingAuthored.validateInvariants() ||
      !validCanonicalSource(canonicalSource)) {
    receipt.status = CreativeTerrainProfileRecipeStatus::InvalidSource;
    receipt.reasonCode = "creative_terrain_profile_recipe_source_invalid";
    return result;
  }
  if ((recipe.profile == CreativeTerrainProfileKind::Wave &&
       2U * recipe.frequency * recipe.spacingCells > recipe.radiusCells) ||
      (recipe.profile == CreativeTerrainProfileKind::Ripple &&
       4U * recipe.frequency * recipe.spacingCells > recipe.radiusCells)) {
    receipt.status = CreativeTerrainProfileRecipeStatus::UnderSampled;
    receipt.reasonCode = "creative_terrain_profile_recipe_under_sampled";
    return result;
  }

  CreativeTerrainHeightFieldBounds footprint;
  if (!profileBounds(recipe, footprint)) {
    const std::uint64_t diameter =
        static_cast<std::uint64_t>(recipe.radiusCells) * 2U + 1U;
    receipt.status = diameter * diameter >
                             kCreativeTerrainHeightFieldCellCapacity
                         ? CreativeTerrainProfileRecipeStatus::CapacityExceeded
                         : CreativeTerrainProfileRecipeStatus::CoordinateOverflow;
    receipt.reasonCode =
        receipt.status == CreativeTerrainProfileRecipeStatus::CapacityExceeded
            ? "creative_terrain_profile_recipe_capacity_exceeded"
            : "creative_terrain_profile_recipe_coordinate_overflow";
    return result;
  }
  CreativeTerrainHeightFieldBounds outputBounds;
  if (!unionBounds(existingAuthored, footprint, outputBounds,
                   receipt.boundsExpanded)) {
    receipt.status = CreativeTerrainProfileRecipeStatus::CapacityExceeded;
    receipt.reasonCode = "creative_terrain_profile_recipe_capacity_exceeded";
    return result;
  }

  CreativeTerrainProfileRequest shape;
  shape.center = recipe.center;
  shape.profile = recipe.profile;
  shape.direction = recipe.direction;
  shape.radiusCells = recipe.radiusCells;
  shape.spacingCells = recipe.spacingCells;
  shape.frequency = recipe.frequency;
  shape.seed = recipe.seed;

  const std::uint64_t cellCount =
      static_cast<std::uint64_t>(outputBounds.widthCells) *
      outputBounds.depthCells;
  std::vector<std::uint16_t> heights;
  heights.reserve(static_cast<std::size_t>(cellCount));
  bool foundSourceInFootprint = false;
  for (std::uint16_t z = 0U; z < outputBounds.depthCells; ++z) {
    for (std::uint16_t x = 0U; x < outputBounds.widthCells; ++x) {
      const CreativeTerrainCoord2 coord{
          outputBounds.minimum.x + static_cast<std::int32_t>(x),
          outputBounds.minimum.z + static_cast<std::int32_t>(z),
      };
      const std::uint16_t source =
          sourceHeightAt(existingAuthored, canonicalSource, coord);
      std::uint16_t output = source;
      if (creativeTerrainInsideRadius(recipe.center, coord,
                                      recipe.radiusCells)) {
        ++receipt.evaluatedCellCount;
        foundSourceInFootprint = foundSourceInFootprint ||
                                 source != kCreativeTerrainEmptyHeightCells;
        if (recipe.rodPolicy == CreativeTerrainProfileRodPolicy::Fill ||
            source != kCreativeTerrainEmptyHeightCells) {
          ++receipt.affectedCellCount;
          const std::uint16_t baseline =
              recipe.blend == CreativeTerrainProfileBlend::Add &&
                      source != kCreativeTerrainEmptyHeightCells
                  ? source
                  : recipe.baseHeightCells;
          const std::int64_t delta = creativeTerrainRoundDivideSymmetric(
              static_cast<std::int64_t>(recipe.amplitudeCells) *
                  profileWeightQ15(shape, coord),
              kCreativeTerrainQ15One);
          output = static_cast<std::uint16_t>(std::clamp<std::int64_t>(
              static_cast<std::int64_t>(baseline) + delta,
              kCreativeTerrainMinimumHeightCells,
              kCreativeTerrainMaximumHeightCells));
        }
      }
      receipt.modifiedCellCount += output != source ? 1U : 0U;
      receipt.materializedCellCount +=
          source == kCreativeTerrainEmptyHeightCells &&
                  output != kCreativeTerrainEmptyHeightCells
              ? 1U
              : 0U;
      heights.push_back(output);
    }
  }
  if (recipe.rodPolicy == CreativeTerrainProfileRodPolicy::Existing &&
      !foundSourceInFootprint) {
    receipt.status =
        CreativeTerrainProfileRecipeStatus::NoSourceInFootprint;
    receipt.reasonCode = "creative_terrain_profile_recipe_no_source";
    return result;
  }

  const CreativeTerrainHeightFieldReplaceReceipt replaced =
      result.heightField.replace(outputBounds, heights);
  if (!replaced.accepted) {
    receipt.status = CreativeTerrainProfileRecipeStatus::OutputRejected;
    receipt.reasonCode = "creative_terrain_profile_recipe_output_rejected";
    return result;
  }
  receipt.accepted = true;
  receipt.status = CreativeTerrainProfileRecipeStatus::Ready;
  receipt.outputCellCount = cellCount;
  receipt.heightHash = heightHash(outputBounds, heights);
  receipt.reasonCode = "creative_terrain_profile_recipe_ready";
  return result;
}

}  // namespace iggy3d::creative
