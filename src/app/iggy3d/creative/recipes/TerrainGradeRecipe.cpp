#include "app/iggy3d/creative/recipes/TerrainGradeRecipe.hpp"

#include "core/hash/StableHash.hpp"
#include "runtime/movement/MovementParams.hpp"
#include "runtime/movement/MovementPolicy.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool coordLess(CreativeTerrainCoord2 lhs,
                             CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
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

[[nodiscard]] bool gradeBounds(const CreativeTerrainGradeRecipe& recipe,
                               CreativeTerrainHeightFieldBounds& bounds) noexcept {
  const std::int64_t padding =
      static_cast<std::int64_t>(recipe.halfWidthCells) + recipe.falloffCells;
  const std::int64_t minimumX =
      std::min<std::int64_t>(recipe.start.x, recipe.end.x) - padding;
  const std::int64_t minimumZ =
      std::min<std::int64_t>(recipe.start.z, recipe.end.z) - padding;
  const std::int64_t maximumX =
      std::max<std::int64_t>(recipe.start.x, recipe.end.x) + padding;
  const std::int64_t maximumZ =
      std::max<std::int64_t>(recipe.start.z, recipe.end.z) + padding;
  const std::int64_t width = maximumX - minimumX + 1;
  const std::int64_t depth = maximumZ - minimumZ + 1;
  if (minimumX < std::numeric_limits<std::int32_t>::min() ||
      minimumX > std::numeric_limits<std::int32_t>::max() ||
      minimumZ < std::numeric_limits<std::int32_t>::min() ||
      minimumZ > std::numeric_limits<std::int32_t>::max() || width <= 0 ||
      depth <= 0 || width > std::numeric_limits<std::uint16_t>::max() ||
      depth > std::numeric_limits<std::uint16_t>::max()) {
    return false;
  }
  bounds = {{static_cast<std::int32_t>(minimumX),
             static_cast<std::int32_t>(minimumZ)},
            static_cast<std::uint16_t>(width),
            static_cast<std::uint16_t>(depth)};
  return isValidCreativeTerrainHeightFieldBounds(bounds);
}

[[nodiscard]] bool unionBounds(
    const CreativeTerrainHeightField& existing,
    CreativeTerrainHeightFieldBounds grade,
    CreativeTerrainHeightFieldBounds& output,
    bool& expanded) noexcept {
  if (existing.cellCount() == 0U) {
    output = grade;
    expanded = false;
    return isValidCreativeTerrainHeightFieldBounds(output);
  }
  const CreativeTerrainHeightFieldBounds current = existing.bounds();
  const std::int64_t minimumX =
      std::min<std::int64_t>(current.minimum.x, grade.minimum.x);
  const std::int64_t minimumZ =
      std::min<std::int64_t>(current.minimum.z, grade.minimum.z);
  const std::int64_t maximumXExclusive = std::max(
      static_cast<std::int64_t>(current.minimum.x) + current.widthCells,
      static_cast<std::int64_t>(grade.minimum.x) + grade.widthCells);
  const std::int64_t maximumZExclusive = std::max(
      static_cast<std::int64_t>(current.minimum.z) + current.depthCells,
      static_cast<std::int64_t>(grade.minimum.z) + grade.depthCells);
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

struct GradeSample {
  bool influenced = false;
  bool corridor = false;
  double targetHeight = 0.0;
  double weight = 0.0;
};

[[nodiscard]] GradeSample sampleGrade(
    const CreativeTerrainGradeRecipe& recipe,
    CreativeTerrainCoord2 coord) noexcept {
  const double dx = static_cast<double>(recipe.end.x) - recipe.start.x;
  const double dz = static_cast<double>(recipe.end.z) - recipe.start.z;
  const double lengthSquared = dx * dx + dz * dz;
  const double length = std::sqrt(lengthSquared);
  const double relativeX = static_cast<double>(coord.x) - recipe.start.x;
  const double relativeZ = static_cast<double>(coord.z) - recipe.start.z;
  const double projection =
      std::clamp((relativeX * dx + relativeZ * dz) / lengthSquared, 0.0, 1.0);
  const double closestX = recipe.start.x + projection * dx;
  const double closestZ = recipe.start.z + projection * dz;
  const double offsetX = static_cast<double>(coord.x) - closestX;
  const double offsetZ = static_cast<double>(coord.z) - closestZ;
  const double distance = std::hypot(offsetX, offsetZ);
  const double halfWidth = recipe.halfWidthCells;
  const double outer = halfWidth + recipe.falloffCells;
  GradeSample sample;
  if (!std::isfinite(distance) || distance > outer) {
    return sample;
  }
  sample.influenced = true;
  sample.corridor = distance <= halfWidth;
  sample.weight = sample.corridor || recipe.falloffCells == 0U
                      ? 1.0
                      : std::clamp((outer - distance) /
                                       static_cast<double>(recipe.falloffCells),
                                   0.0, 1.0);
  const double longitudinal =
      recipe.startHeightCells +
      projection * (static_cast<double>(recipe.endHeightCells) -
                    recipe.startHeightCells);
  const double signedLateral = (dx * relativeZ - dz * relativeX) / length;
  sample.targetHeight =
      longitudinal + signedLateral * recipe.crossSlopePermille / 1000.0;
  return sample;
}

[[nodiscard]] iggy3d::Vec3 triangleNormal(CreativeVec3 center,
                                          CreativeVec3 first,
                                          CreativeVec3 second) noexcept {
  const double ux = first.x - center.x;
  const double uy = first.y - center.y;
  const double uz = first.z - center.z;
  const double vx = second.x - center.x;
  const double vy = second.y - center.y;
  const double vz = second.z - center.z;
  iggy3d::Vec3 normal{
      static_cast<float>(uy * vz - uz * vy),
      static_cast<float>(uz * vx - ux * vz),
      static_cast<float>(ux * vy - uy * vx),
  };
  if (normal.y < 0.0F) {
    normal = normal * -1.0F;
  }
  return normal;
}

void populateWalkabilityReadout(
    CreativeTerrainGradeRecipeReceipt& receipt,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainHeightField& output,
    const CreativeTerrainGradeRecipe& recipe,
    double maximumWalkableSlopeDegrees) {
  CreativeTerrainSurfacePlan composed =
      replaceCreativeTerrainSurfaceRegion(canonicalSource, output);
  const CreativeTerrainRenderPlan render = buildCreativeTerrainRenderPlan(
      composed, {}, 1.0, kCreativeTerrainRenderPatchCapacity);
  receipt.readout.maximumWalkableSlopeDegrees = maximumWalkableSlopeDegrees;
  if (!render.accepted) {
    return;
  }

  iggy3d::MovementParams params;
  params.maxWalkableSlopeDegrees =
      static_cast<float>(maximumWalkableSlopeDegrees);
  float maximumSlope = 0.0F;
  std::string_view maximumBand = "flat";
  bool sampled = false;
  bool walkable = true;
  constexpr std::array<std::array<std::size_t, 2U>, 4U> pairs{{
      {{0U, 1U}}, {{1U, 2U}}, {{2U, 3U}}, {{3U, 0U}},
  }};
  for (const CreativeTerrainSurfacePatch& patch : render.patches) {
    if (!sampleGrade(recipe, patch.coord).corridor) {
      continue;
    }
    for (const auto pair : pairs) {
      const iggy3d::SlopeSample slope = iggy3d::sampleSlope(
          triangleNormal(patch.center, patch.corners[pair[0]],
                         patch.corners[pair[1]]),
          params);
      if (!slope.valid) {
        walkable = false;
        continue;
      }
      sampled = true;
      walkable = walkable && slope.walkable;
      if (slope.angleDegrees >= maximumSlope) {
        maximumSlope = slope.angleDegrees;
        maximumBand = slope.bandId;
      }
    }
  }
  receipt.readout.maximumCollisionSlopeDegrees = maximumSlope;
  receipt.readout.movementBand = sampled ? maximumBand : "invalid";
  receipt.readout.walkable = sampled && walkable;
}

}  // namespace

bool isValidCreativeTerrainGradeRecipe(
    const CreativeTerrainGradeRecipe& recipe) noexcept {
  return recipe.version == kCreativeTerrainGradeRecipeVersion &&
         recipe.start != recipe.end &&
         recipe.startHeightCells >= kCreativeTerrainMinimumHeightCells &&
         recipe.startHeightCells <= kCreativeTerrainMaximumHeightCells &&
         recipe.endHeightCells >= kCreativeTerrainMinimumHeightCells &&
         recipe.endHeightCells <= kCreativeTerrainMaximumHeightCells &&
         recipe.halfWidthCells <= kCreativeTerrainGradeMaximumHalfWidthCells &&
         std::abs(static_cast<std::int64_t>(recipe.crossSlopePermille)) <=
             kCreativeTerrainGradeMaximumCrossSlopePermille &&
         recipe.falloffCells <= kCreativeTerrainGradeMaximumFalloffCells;
}

std::string_view toString(CreativeTerrainGradeRecipeStatus status) noexcept {
  switch (status) {
    case CreativeTerrainGradeRecipeStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainGradeRecipeStatus::UnsupportedVersion:
      return "UnsupportedVersion";
    case CreativeTerrainGradeRecipeStatus::InvalidRecipe:
      return "InvalidRecipe";
    case CreativeTerrainGradeRecipeStatus::InvalidSource:
      return "InvalidSource";
    case CreativeTerrainGradeRecipeStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainGradeRecipeStatus::HeightOutOfRange:
      return "HeightOutOfRange";
    case CreativeTerrainGradeRecipeStatus::OutputRejected:
      return "OutputRejected";
    case CreativeTerrainGradeRecipeStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeTerrainGradeRecipeResult buildCreativeTerrainGradeRecipe(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainGradeRecipe& recipe,
    double maximumWalkableSlopeDegrees) {
  CreativeTerrainGradeRecipeResult result;
  result.recipe = recipe;
  CreativeTerrainGradeRecipeReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.sourceColumnCount = canonicalSource.columns.size();
  if (recipe.version != kCreativeTerrainGradeRecipeVersion) {
    receipt.status = CreativeTerrainGradeRecipeStatus::UnsupportedVersion;
    receipt.reasonCode = "creative_terrain_grade_version_unsupported";
    return result;
  }
  if (!isValidCreativeTerrainGradeRecipe(recipe) ||
      !std::isfinite(maximumWalkableSlopeDegrees) ||
      maximumWalkableSlopeDegrees < 0.0 ||
      maximumWalkableSlopeDegrees > 90.0) {
    receipt.status = CreativeTerrainGradeRecipeStatus::InvalidRecipe;
    receipt.reasonCode = "creative_terrain_grade_recipe_invalid";
    return result;
  }
  if (!existingAuthored.validateInvariants() ||
      !validCanonicalSource(canonicalSource)) {
    receipt.status = CreativeTerrainGradeRecipeStatus::InvalidSource;
    receipt.reasonCode = "creative_terrain_grade_source_invalid";
    return result;
  }

  CreativeTerrainHeightFieldBounds gradeRegion;
  CreativeTerrainHeightFieldBounds outputBounds;
  if (!gradeBounds(recipe, gradeRegion) ||
      !unionBounds(existingAuthored, gradeRegion, outputBounds,
                   receipt.boundsExpanded)) {
    receipt.status = CreativeTerrainGradeRecipeStatus::CapacityExceeded;
    receipt.reasonCode = "creative_terrain_grade_capacity_exceeded";
    return result;
  }
  const std::uint64_t cellCount =
      static_cast<std::uint64_t>(outputBounds.widthCells) *
      outputBounds.depthCells;
  if (cellCount > kCreativeTerrainHeightFieldCellCapacity) {
    receipt.status = CreativeTerrainGradeRecipeStatus::CapacityExceeded;
    receipt.reasonCode = "creative_terrain_grade_capacity_exceeded";
    return result;
  }

  const double length = std::hypot(
      static_cast<double>(recipe.end.x) - recipe.start.x,
      static_cast<double>(recipe.end.z) - recipe.start.z);
  receipt.readout.lengthCells = length;
  receipt.readout.longitudinalSlopePercent =
      (static_cast<double>(recipe.endHeightCells) - recipe.startHeightCells) /
      length * 100.0;
  receipt.readout.crossSlopePercent = recipe.crossSlopePermille / 10.0;

  std::vector<std::uint16_t> heights;
  heights.reserve(static_cast<std::size_t>(cellCount));
  for (std::uint16_t z = 0U; z < outputBounds.depthCells; ++z) {
    for (std::uint16_t x = 0U; x < outputBounds.widthCells; ++x) {
      const CreativeTerrainCoord2 coord{
          outputBounds.minimum.x + static_cast<std::int32_t>(x),
          outputBounds.minimum.z + static_cast<std::int32_t>(z),
      };
      const std::uint16_t source =
          sourceHeightAt(existingAuthored, canonicalSource, coord);
      const GradeSample sample = sampleGrade(recipe, coord);
      std::uint16_t output = source;
      if (sample.influenced) {
        ++receipt.evaluatedCellCount;
        if (sample.corridor) {
          ++receipt.corridorCellCount;
        } else {
          ++receipt.falloffCellCount;
        }
        const long long roundedTarget = std::llround(sample.targetHeight);
        if (roundedTarget < kCreativeTerrainMinimumHeightCells ||
            roundedTarget > kCreativeTerrainMaximumHeightCells) {
          receipt.status = CreativeTerrainGradeRecipeStatus::HeightOutOfRange;
          receipt.reasonCode = "creative_terrain_grade_height_out_of_range";
          return result;
        }
        const long long blended = std::llround(
            static_cast<double>(source) * (1.0 - sample.weight) +
            static_cast<double>(roundedTarget) * sample.weight);
        output = static_cast<std::uint16_t>(std::clamp<long long>(
            blended, kCreativeTerrainEmptyHeightCells,
            kCreativeTerrainMaximumHeightCells));
      }
      receipt.modifiedCellCount += output != source ? 1U : 0U;
      heights.push_back(output);
    }
  }

  const CreativeTerrainHeightFieldReplaceReceipt replaced =
      result.heightField.replace(outputBounds, heights);
  if (!replaced.accepted) {
    receipt.status = CreativeTerrainGradeRecipeStatus::OutputRejected;
    receipt.reasonCode = "creative_terrain_grade_output_rejected";
    return result;
  }
  receipt.outputCellCount = cellCount;
  receipt.heightHash = heightHash(outputBounds, heights);
  populateWalkabilityReadout(receipt, canonicalSource, result.heightField,
                             recipe, maximumWalkableSlopeDegrees);
  receipt.accepted = true;
  receipt.status = CreativeTerrainGradeRecipeStatus::Ready;
  receipt.reasonCode = "creative_terrain_grade_ready";
  return result;
}

}  // namespace iggy3d::creative
