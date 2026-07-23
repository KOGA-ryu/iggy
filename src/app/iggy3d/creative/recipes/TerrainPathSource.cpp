#include "app/iggy3d/creative/recipes/TerrainPathSource.hpp"

#include "app/iggy3d/creative/recipes/TerrainGradeAdapters.hpp"
#include "app/iggy3d/creative/tools/TerrainBrushKernel.hpp"
#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

template <typename Enum>
[[nodiscard]] bool validEnum(Enum value, Enum count) noexcept {
  return static_cast<std::size_t>(value) < static_cast<std::size_t>(count);
}

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
    if (source.columns[index].heightCells <
            kCreativeTerrainMinimumHeightCells ||
        source.columns[index].heightCells >
            kCreativeTerrainMaximumHeightCells ||
        (index > 0U &&
         !coordLess(source.columns[index - 1U].coord,
                    source.columns[index].coord))) {
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

[[nodiscard]] bool checkedCoord(std::int64_t x,
                                std::int64_t z,
                                CreativeTerrainCoord2& output) noexcept {
  if (x < std::numeric_limits<std::int32_t>::min() ||
      x > std::numeric_limits<std::int32_t>::max() ||
      z < std::numeric_limits<std::int32_t>::min() ||
      z > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)};
  return true;
}

using CenterlineSample = CreativeTerrainPathSourceSample;

[[nodiscard]] double interpolate(double from,
                                 double to,
                                 double progress) noexcept {
  return from + (to - from) * progress;
}

[[nodiscard]] double smoothstep01(double value) noexcept {
  const double clamped = std::clamp(value, 0.0, 1.0);
  return clamped * clamped * (3.0 - 2.0 * clamped);
}

[[nodiscard]] double effectiveHalfWidth(
    const CreativeTerrainPathSourceRecipe& recipe,
    double authoredHalfWidth) noexcept {
  if (recipe.kind == CreativeTerrainPathKind::Road) {
    return authoredHalfWidth + recipe.road.shoulderWidthCells;
  }
  if (recipe.kind == CreativeTerrainPathKind::River ||
      recipe.kind == CreativeTerrainPathKind::Trench) {
    return authoredHalfWidth + recipe.watercourse.bankSlopeCells;
  }
  return authoredHalfWidth;
}

[[nodiscard]] bool validRoadSettings(
    const CreativeTerrainRoadSettings& settings) noexcept {
  return settings.shoulderWidthCells <=
             kCreativeTerrainRoadMaximumShoulderWidthCells &&
         settings.maximumGradePermille <=
             kCreativeTerrainRoadMaximumGradePermille &&
         validEnum(settings.edgeTreatment,
                   CreativeTerrainRoadEdgeTreatment::Count) &&
         std::isfinite(settings.edgeWidthMeters) &&
         settings.edgeWidthMeters >=
             kCreativeTerrainRoadMinimumEdgeDimensionMeters &&
         settings.edgeWidthMeters <=
             kCreativeTerrainRoadMaximumEdgeDimensionMeters &&
         std::isfinite(settings.edgeHeightMeters) &&
         settings.edgeHeightMeters >=
             kCreativeTerrainRoadMinimumEdgeDimensionMeters &&
         settings.edgeHeightMeters <=
             kCreativeTerrainRoadMaximumEdgeDimensionMeters &&
         settings.edgeMaterial < CreativeStructuralMaterial::Count;
}

[[nodiscard]] bool roadGradeWithinLimit(
    const CreativeTerrainPathSourceRecipe& recipe) noexcept {
  if (recipe.kind != CreativeTerrainPathKind::Road ||
      recipe.points.size() < 2U ||
      recipe.road.maximumGradePermille == 0U) {
    return true;
  }
  for (std::size_t index = 0U; index + 1U < recipe.points.size(); ++index) {
    const CreativeTerrainPathSourcePoint& from = recipe.points[index];
    const CreativeTerrainPathSourcePoint& to = recipe.points[index + 1U];
    const double length = std::hypot(
        static_cast<double>(to.coord.x) - from.coord.x,
        static_cast<double>(to.coord.z) - from.coord.z);
    const double rise = std::abs(static_cast<double>(to.heightCells) -
                                 from.heightCells);
    if (!std::isfinite(length) || length <= 0.0 ||
        rise * 1000.0 >
            length * static_cast<double>(recipe.road.maximumGradePermille) +
                1.0e-9) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool isWatercourseKind(
    CreativeTerrainPathKind kind) noexcept {
  return kind == CreativeTerrainPathKind::River ||
         kind == CreativeTerrainPathKind::Trench;
}

[[nodiscard]] bool validWatercourseSettings(
    const CreativeTerrainPathSourceRecipe& recipe) noexcept {
  const CreativeTerrainWatercourseSettings& settings = recipe.watercourse;
  if (settings.bankSlopeCells >
          kCreativeTerrainWatercourseMaximumBankSlopeCells ||
      settings.surfaceInsetCells >
          kCreativeTerrainWatercourseMaximumSurfaceInsetCells ||
      !validEnum(settings.drainageDirection,
                 CreativeTerrainWatercourseDrainageDirection::Count) ||
      !validEnum(settings.surfacePolicy,
                 CreativeTerrainWaterSurfacePolicy::Count) ||
      settings.nextCrossingId ==
          kInvalidCreativeTerrainWatercourseCrossingId ||
      settings.crossings.size() >
          kCreativeTerrainWatercourseCrossingCapacity) {
    return false;
  }
  if (!isWatercourseKind(recipe.kind) &&
      (!settings.crossings.empty() || settings.bankSlopeCells != 0U ||
       settings.drainageDirection !=
           CreativeTerrainWatercourseDrainageDirection::Unspecified ||
       settings.surfacePolicy != CreativeTerrainWaterSurfacePolicy::None)) {
    return false;
  }

  CreativeTerrainWatercourseCrossingId maximumId =
      kInvalidCreativeTerrainWatercourseCrossingId;
  for (std::size_t index = 0U; index < settings.crossings.size(); ++index) {
    const CreativeTerrainWatercourseCrossing& crossing =
        settings.crossings[index];
    if (crossing.id == kInvalidCreativeTerrainWatercourseCrossingId ||
        crossing.pointId == kInvalidCreativeTerrainPathSourcePointId ||
        crossing.bankClearanceCells >
            kCreativeTerrainWatercourseMaximumCrossingClearanceCells ||
        crossing.deckClearanceCells >
            kCreativeTerrainWatercourseMaximumCrossingClearanceCells ||
        crossing.approachLengthCells >
            kCreativeTerrainWatercourseMaximumCrossingClearanceCells ||
        std::none_of(recipe.points.begin(), recipe.points.end(),
                     [&crossing](const CreativeTerrainPathSourcePoint& point) {
                       return point.id == crossing.pointId;
                     })) {
      return false;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      if (settings.crossings[prior].id == crossing.id ||
          settings.crossings[prior].pointId == crossing.pointId) {
        return false;
      }
    }
    maximumId = std::max(maximumId, crossing.id);
  }
  return settings.nextCrossingId > maximumId;
}

[[nodiscard]] bool watercourseDrainageWithinDirection(
    const CreativeTerrainPathSourceRecipe& recipe) noexcept {
  if (!isWatercourseKind(recipe.kind) ||
      recipe.watercourse.drainageDirection ==
          CreativeTerrainWatercourseDrainageDirection::Unspecified) {
    return true;
  }
  if (recipe.elevation == CreativeTerrainPathElevation::Follow) {
    return false;
  }
  for (std::size_t index = 0U; index + 1U < recipe.points.size(); ++index) {
    const std::uint16_t from = recipe.points[index].heightCells;
    const std::uint16_t to = recipe.points[index + 1U].heightCells;
    if ((recipe.watercourse.drainageDirection ==
             CreativeTerrainWatercourseDrainageDirection::StartToEnd &&
         to > from) ||
        (recipe.watercourse.drainageDirection ==
             CreativeTerrainWatercourseDrainageDirection::EndToStart &&
         from > to)) {
      return false;
    }
  }
  return true;
}

void applyEndpointJoin(CenterlineSample& sample,
                       CreativeTerrainPathEndpointJoin join,
                       double distanceFromEndpoint,
                       std::uint16_t falloffCells) noexcept {
  if (join == CreativeTerrainPathEndpointJoin::Open) {
    return;
  }
  const double extent = std::max(
      1.0, sample.halfWidthCells + static_cast<double>(falloffCells));
  const double ramp = smoothstep01(distanceFromEndpoint / extent);
  if (join == CreativeTerrainPathEndpointJoin::Blend) {
    sample.terrainWeight = std::min(sample.terrainWeight, ramp);
    return;
  }
  if (join == CreativeTerrainPathEndpointJoin::Intersection) {
    const double expansion = std::max(1.0, sample.halfWidthCells * 0.5);
    const double widened = std::min(
        static_cast<double>(kCreativeTerrainPathSourceMaximumHalfWidthCells),
        sample.halfWidthCells + expansion * (1.0 - ramp));
    sample.halfWidthCells = std::max(sample.halfWidthCells, widened);
  }
  if (join == CreativeTerrainPathEndpointJoin::Intersection ||
      join == CreativeTerrainPathEndpointJoin::Bridge ||
      join == CreativeTerrainPathEndpointJoin::BuildingPad) {
    sample.profileWeight = std::min(sample.profileWeight, ramp);
  }
}

void applyEndpointJoins(const CreativeTerrainPathSourceRecipe& recipe,
                        CenterlineSample& sample) noexcept {
  if (sample.segmentIndex == 0U) {
    const CreativeTerrainCoord2 from = recipe.points[0U].coord;
    const CreativeTerrainCoord2 to = recipe.points[1U].coord;
    const double segmentLength = std::hypot(
        static_cast<double>(to.x) - from.x,
        static_cast<double>(to.z) - from.z);
    applyEndpointJoin(sample, recipe.startJoin,
                      segmentLength * sample.progress,
                      recipe.falloffCells);
  }
  const std::size_t lastSegment = recipe.points.size() - 2U;
  if (sample.segmentIndex == lastSegment) {
    const CreativeTerrainCoord2 from = recipe.points[lastSegment].coord;
    const CreativeTerrainCoord2 to = recipe.points[lastSegment + 1U].coord;
    const double segmentLength = std::hypot(
        static_cast<double>(to.x) - from.x,
        static_cast<double>(to.z) - from.z);
    applyEndpointJoin(sample, recipe.endJoin,
                      segmentLength * (1.0 - sample.progress),
                      recipe.falloffCells);
  }
}

[[nodiscard]] double catmullRom(double p0,
                                double p1,
                                double p2,
                                double p3,
                                double t) noexcept {
  const double t2 = t * t;
  const double t3 = t2 * t;
  return 0.5 * ((2.0 * p1) + (-p0 + p2) * t +
                (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
                (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
}

[[nodiscard]] bool appendSample(std::vector<CenterlineSample>& output,
                                CenterlineSample sample) {
  if (!output.empty() && output.back().coord == sample.coord) {
    return true;
  }
  if (output.size() >= kCreativeTerrainPathGeneratedCenterlineCapacity) {
    return false;
  }
  output.push_back(sample);
  return true;
}

[[nodiscard]] bool appendLinearSegment(
    std::vector<CenterlineSample>& output,
    const CreativeTerrainPathSourceRecipe& recipe,
    const CreativeTerrainPathSourcePoint& from,
    const CreativeTerrainPathSourcePoint& to,
    std::size_t segmentIndex) {
  const CreativeTerrainGridLine line =
      rasterizeCreativeTerrainGridLine(from.coord, to.coord);
  if (!line.accepted || line.count < 2U) {
    return false;
  }
  const double denominator = static_cast<double>(line.count - 1U);
  for (std::size_t index = 0U; index < line.count; ++index) {
    const double progress = static_cast<double>(index) / denominator;
    CenterlineSample sample;
    sample.coord = line.coords[index];
    sample.segmentIndex = segmentIndex;
    sample.progress = progress;
    sample.heightCells =
        interpolate(from.heightCells, to.heightCells, progress);
    sample.profileHalfWidthCells =
        interpolate(from.halfWidthCells, to.halfWidthCells, progress);
    sample.halfWidthCells =
        effectiveHalfWidth(recipe, sample.profileHalfWidthCells);
    sample.amplitudeCells =
        interpolate(from.amplitudeCells, to.amplitudeCells, progress);
    sample.bankPermille =
        interpolate(from.bankPermille, to.bankPermille, progress);
    sample.tangentX = to.coord.x - from.coord.x;
    sample.tangentZ = to.coord.z - from.coord.z;
    if (!appendSample(output, sample)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool appendCurvedSegment(
    std::vector<CenterlineSample>& output,
    const CreativeTerrainPathSourceRecipe& recipe,
    std::size_t segmentIndex) {
  const CreativeTerrainPathSourcePoint& from = recipe.points[segmentIndex];
  const CreativeTerrainPathSourcePoint& to = recipe.points[segmentIndex + 1U];
  const CreativeTerrainPathSourcePoint& previous =
      recipe.points[segmentIndex == 0U ? 0U : segmentIndex - 1U];
  const CreativeTerrainPathSourcePoint& next =
      recipe.points[std::min(segmentIndex + 2U, recipe.points.size() - 1U)];
  const std::uint64_t span = static_cast<std::uint64_t>(std::max(
      std::llabs(static_cast<std::int64_t>(to.coord.x) - from.coord.x),
      std::llabs(static_cast<std::int64_t>(to.coord.z) - from.coord.z)));
  const std::uint64_t stepCount = std::max<std::uint64_t>(span * 4U, 1U);
  if (stepCount > kCreativeTerrainPathGeneratedCenterlineCapacity) {
    return false;
  }
  for (std::uint64_t step = 0U; step <= stepCount; ++step) {
    const double progress =
        static_cast<double>(step) / static_cast<double>(stepCount);
    const double x = catmullRom(previous.coord.x, from.coord.x, to.coord.x,
                                next.coord.x, progress);
    const double z = catmullRom(previous.coord.z, from.coord.z, to.coord.z,
                                next.coord.z, progress);
    if (!std::isfinite(x) || !std::isfinite(z)) {
      return false;
    }
    CreativeTerrainCoord2 coord;
    if (!checkedCoord(std::llround(x), std::llround(z), coord)) {
      return false;
    }
    CenterlineSample sample;
    sample.coord = coord;
    sample.segmentIndex = segmentIndex;
    sample.progress = progress;
    sample.heightCells =
        interpolate(from.heightCells, to.heightCells, progress);
    sample.profileHalfWidthCells =
        interpolate(from.halfWidthCells, to.halfWidthCells, progress);
    sample.halfWidthCells =
        effectiveHalfWidth(recipe, sample.profileHalfWidthCells);
    sample.amplitudeCells =
        interpolate(from.amplitudeCells, to.amplitudeCells, progress);
    sample.bankPermille =
        interpolate(from.bankPermille, to.bankPermille, progress);
    const double nextProgress =
        std::min(progress + 1.0 / static_cast<double>(stepCount), 1.0);
    sample.tangentX = static_cast<std::int32_t>(std::llround(
        catmullRom(previous.coord.x, from.coord.x, to.coord.x, next.coord.x,
                   nextProgress) -
        x));
    sample.tangentZ = static_cast<std::int32_t>(std::llround(
        catmullRom(previous.coord.z, from.coord.z, to.coord.z, next.coord.z,
                   nextProgress) -
        z));
    if (sample.tangentX == 0 && sample.tangentZ == 0) {
      sample.tangentX = to.coord.x - from.coord.x;
      sample.tangentZ = to.coord.z - from.coord.z;
    }
    if (!appendSample(output, sample)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::uint64_t hashCachedSegment(
    CreativeTerrainPathSourcePointId startPointId,
    CreativeTerrainPathSourcePointId endPointId,
    std::span<const CenterlineSample> samples) noexcept {
  StableHasher hasher;
  hasher.addU64(startPointId);
  hasher.addU64(endPointId);
  for (const CenterlineSample& sample : samples) {
    hasher.addI64(sample.coord.x);
    hasher.addI64(sample.coord.z);
    hasher.addU64(std::bit_cast<std::uint64_t>(sample.progress));
    hasher.addU64(std::bit_cast<std::uint64_t>(sample.heightCells));
    hasher.addU64(
        std::bit_cast<std::uint64_t>(sample.profileHalfWidthCells));
    hasher.addU64(std::bit_cast<std::uint64_t>(sample.halfWidthCells));
    hasher.addU64(std::bit_cast<std::uint64_t>(sample.amplitudeCells));
    hasher.addU64(std::bit_cast<std::uint64_t>(sample.bankPermille));
    hasher.addI64(sample.tangentX);
    hasher.addI64(sample.tangentZ);
  }
  return hasher.value();
}

[[nodiscard]] bool validCachedSegment(
    const CreativeTerrainPathSourceSegmentCache& segment,
    const CreativeTerrainPathSourcePoint& from,
    const CreativeTerrainPathSourcePoint& to) noexcept {
  if (segment.startPointId != from.id || segment.endPointId != to.id ||
      segment.samples.empty() ||
      segment.samples.size() >
          kCreativeTerrainPathGeneratedCenterlineCapacity ||
      segment.samples.front().coord != from.coord ||
      segment.samples.back().coord != to.coord ||
      segment.sourceHash != hashCachedSegment(from.id, to.id,
                                              segment.samples)) {
    return false;
  }
  return std::all_of(segment.samples.begin(), segment.samples.end(),
                     [](const CenterlineSample& sample) {
                       return std::isfinite(sample.progress) &&
                              sample.progress >= 0.0 &&
                              sample.progress <= 1.0 &&
                              std::isfinite(sample.heightCells) &&
                              std::isfinite(sample.profileHalfWidthCells) &&
                              std::isfinite(sample.halfWidthCells) &&
                              std::isfinite(sample.amplitudeCells) &&
                              std::isfinite(sample.bankPermille) &&
                              sample.terrainWeight == 1.0 &&
                              sample.profileWeight == 1.0;
                     });
}

[[nodiscard]] bool centerlineForRecipe(
    const CreativeTerrainPathSourceRecipe& recipe,
    std::vector<CenterlineSample>& centerline,
    std::vector<CreativeTerrainPathSegmentReceipt>& segments,
    const CreativeTerrainPathSourceCache* previousCache,
    CreativeTerrainPathSourceCache& nextCache) {
  centerline.clear();
  segments.clear();
  segments.resize(recipe.points.size() - 1U);
  nextCache = {};
  nextCache.recipe = recipe;
  nextCache.segments.resize(recipe.points.size() - 1U);

  const bool cacheShapeValid =
      previousCache != nullptr && previousCache->valid &&
      isValidCreativeTerrainPathSourceRecipe(previousCache->recipe) &&
      previousCache->recipe.points.size() >= 2U &&
      previousCache->segments.size() ==
          previousCache->recipe.points.size() - 1U;
  if (cacheShapeValid) {
    nextCache.dirtySegments = diffCreativeTerrainPathSourceSegments(
        previousCache->recipe, recipe);
  } else {
    nextCache.dirtySegments.changed = true;
    nextCache.dirtySegments.allSegments = true;
    nextCache.dirtySegments.segmentCount = recipe.points.size() - 1U;
  }

  const auto segmentIsDirty = [&nextCache](std::size_t index) noexcept {
    const CreativeTerrainPathDirtySegments& dirty =
        nextCache.dirtySegments;
    return dirty.allSegments ||
           (dirty.changed && index >= dirty.firstSegment &&
            index < dirty.firstSegment + dirty.segmentCount);
  };
  const auto cachedSegmentFor =
      [previousCache](const CreativeTerrainPathSourcePoint& from,
                      const CreativeTerrainPathSourcePoint& to)
      -> const CreativeTerrainPathSourceSegmentCache* {
    if (previousCache == nullptr) {
      return nullptr;
    }
    const auto found = std::find_if(
        previousCache->segments.begin(), previousCache->segments.end(),
        [&from, &to](const auto& segment) {
          return validCachedSegment(segment, from, to);
        });
    return found == previousCache->segments.end() ? nullptr : &*found;
  };

  for (std::size_t index = 0U; index + 1U < recipe.points.size(); ++index) {
    CreativeTerrainPathSourceSegmentCache& segmentCache =
        nextCache.segments[index];
    segmentCache.startPointId = recipe.points[index].id;
    segmentCache.endPointId = recipe.points[index + 1U].id;
    const CreativeTerrainPathSourceSegmentCache* reusable =
        !segmentIsDirty(index)
            ? cachedSegmentFor(recipe.points[index],
                               recipe.points[index + 1U])
            : nullptr;
    if (reusable != nullptr) {
      segmentCache = *reusable;
      ++nextCache.reusedSegmentCount;
    } else {
      const bool sampled =
          recipe.curve == CreativeTerrainPathCurvePolicy::Linear
              ? appendLinearSegment(segmentCache.samples, recipe,
                                    recipe.points[index],
                                    recipe.points[index + 1U], index)
              : appendCurvedSegment(segmentCache.samples, recipe, index);
      if (!sampled || segmentCache.samples.empty()) {
        return false;
      }
      ++nextCache.rebuiltSegmentCount;
    }

    segmentCache.sourceHash = hashCachedSegment(
        segmentCache.startPointId, segmentCache.endPointId,
        segmentCache.samples);

    const std::size_t begin = centerline.size();
    for (CenterlineSample sample : segmentCache.samples) {
      sample.segmentIndex = index;
      sample.terrainWeight = 1.0;
      sample.profileWeight = 1.0;
      if (!appendSample(centerline, sample)) {
        return false;
      }
    }
    if (centerline.size() == begin) {
      return false;
    }
    for (std::size_t sampleIndex = begin; sampleIndex < centerline.size();
         ++sampleIndex) {
      applyEndpointJoins(recipe, centerline[sampleIndex]);
    }
    CreativeTerrainPathSegmentReceipt& segment = segments[index];
    segment.startPointId = recipe.points[index].id;
    segment.endPointId = recipe.points[index + 1U].id;
    segment.centerlineCellCount =
        static_cast<std::uint32_t>(centerline.size() - begin);
    std::int64_t minimumX = std::numeric_limits<std::int64_t>::max();
    std::int64_t minimumZ = std::numeric_limits<std::int64_t>::max();
    std::int64_t maximumX = std::numeric_limits<std::int64_t>::min();
    std::int64_t maximumZ = std::numeric_limits<std::int64_t>::min();
    StableHasher hasher;
    hasher.addU64(segment.startPointId);
    hasher.addU64(segment.endPointId);
    for (std::size_t sampleIndex = begin; sampleIndex < centerline.size();
         ++sampleIndex) {
      hasher.addI64(centerline[sampleIndex].coord.x);
      hasher.addI64(centerline[sampleIndex].coord.z);
      const std::int64_t outer = static_cast<std::int64_t>(
          std::ceil(centerline[sampleIndex].halfWidthCells)) +
          recipe.falloffCells;
      minimumX = std::min(
          minimumX,
          static_cast<std::int64_t>(centerline[sampleIndex].coord.x) - outer);
      minimumZ = std::min(
          minimumZ,
          static_cast<std::int64_t>(centerline[sampleIndex].coord.z) - outer);
      maximumX = std::max(
          maximumX,
          static_cast<std::int64_t>(centerline[sampleIndex].coord.x) + outer);
      maximumZ = std::max(
          maximumZ,
          static_cast<std::int64_t>(centerline[sampleIndex].coord.z) + outer);
    }
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
    segment.impactBounds = {
        {static_cast<std::int32_t>(minimumX),
         static_cast<std::int32_t>(minimumZ)},
        static_cast<std::uint16_t>(width),
        static_cast<std::uint16_t>(depth)};
    segment.sourceHash = hasher.value();
  }
  return true;
}

[[nodiscard]] bool pathBounds(
    const CreativeTerrainPathSourceRecipe& recipe,
    std::span<const CenterlineSample> centerline,
    CreativeTerrainHeightFieldBounds& bounds) noexcept {
  std::int64_t minimumX = std::numeric_limits<std::int64_t>::max();
  std::int64_t minimumZ = std::numeric_limits<std::int64_t>::max();
  std::int64_t maximumX = std::numeric_limits<std::int64_t>::min();
  std::int64_t maximumZ = std::numeric_limits<std::int64_t>::min();
  for (const CenterlineSample& sample : centerline) {
    const std::int64_t outer =
        static_cast<std::int64_t>(std::ceil(sample.halfWidthCells)) +
        recipe.falloffCells;
    minimumX = std::min(minimumX,
                        static_cast<std::int64_t>(sample.coord.x) - outer);
    minimumZ = std::min(minimumZ,
                        static_cast<std::int64_t>(sample.coord.z) - outer);
    maximumX = std::max(maximumX,
                        static_cast<std::int64_t>(sample.coord.x) + outer);
    maximumZ = std::max(maximumZ,
                        static_cast<std::int64_t>(sample.coord.z) + outer);
  }
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
  return true;
}

[[nodiscard]] bool unionBounds(
    const CreativeTerrainHeightField& existing,
    CreativeTerrainHeightFieldBounds path,
    CreativeTerrainHeightFieldBounds& output) noexcept {
  if (existing.cellCount() == 0U) {
    output = path;
    return true;
  }
  const CreativeTerrainHeightFieldBounds current = existing.bounds();
  const std::int64_t minimumX =
      std::min<std::int64_t>(current.minimum.x, path.minimum.x);
  const std::int64_t minimumZ =
      std::min<std::int64_t>(current.minimum.z, path.minimum.z);
  const std::int64_t maximumX = std::max(
      static_cast<std::int64_t>(current.minimum.x) + current.widthCells,
      static_cast<std::int64_t>(path.minimum.x) + path.widthCells);
  const std::int64_t maximumZ = std::max(
      static_cast<std::int64_t>(current.minimum.z) + current.depthCells,
      static_cast<std::int64_t>(path.minimum.z) + path.depthCells);
  const std::int64_t width = maximumX - minimumX;
  const std::int64_t depth = maximumZ - minimumZ;
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
  return true;
}

[[nodiscard]] double crossSectionOffset(
    const CreativeTerrainPathSourceRecipe& recipe,
    double amplitude,
    double distance,
    double profileHalfWidth,
    double effectiveHalfWidth) noexcept {
  const CreativeTerrainPathCrossSection section = recipe.crossSection;
  if (isWatercourseKind(recipe.kind) &&
      recipe.watercourse.bankSlopeCells > 0U &&
      (section == CreativeTerrainPathCrossSection::Channel ||
       section == CreativeTerrainPathCrossSection::Cut)) {
    if (distance <= profileHalfWidth) {
      return -amplitude;
    }
    const double slopeWidth = effectiveHalfWidth - profileHalfWidth;
    const double progress =
        slopeWidth <= 0.0
            ? 1.0
            : std::clamp((distance - profileHalfWidth) / slopeWidth, 0.0, 1.0);
    return -amplitude * (1.0 - smoothstep01(progress));
  }
  const double normalized =
      effectiveHalfWidth <= 0.0
          ? 0.0
          : std::clamp(distance / effectiveHalfWidth, 0.0, 1.0);
  const double bell = 0.5 * (std::cos(normalized * 3.14159265358979323846) +
                             1.0);
  switch (section) {
    case CreativeTerrainPathCrossSection::Flat:
      return 0.0;
    case CreativeTerrainPathCrossSection::Crowned:
    case CreativeTerrainPathCrossSection::Berm:
      return amplitude * bell;
    case CreativeTerrainPathCrossSection::Channel:
      return -amplitude * bell;
    case CreativeTerrainPathCrossSection::Cut:
      return -amplitude;
    case CreativeTerrainPathCrossSection::Count:
      break;
  }
  return 0.0;
}

[[nodiscard]] CreativeTerrainMaterial resolvedMaterial(
    const CreativeTerrainPathSourceRecipe& recipe) noexcept {
  if (isValidCreativeTerrainMaterial(recipe.material)) {
    return recipe.material;
  }
  switch (recipe.kind) {
    case CreativeTerrainPathKind::Road:
      return CreativeTerrainMaterial::Dirt;
    case CreativeTerrainPathKind::River:
      return CreativeTerrainMaterial::Sand;
    case CreativeTerrainPathKind::Ridge:
    case CreativeTerrainPathKind::Trench:
    case CreativeTerrainPathKind::Count:
      return CreativeTerrainMaterial::Count;
  }
  return CreativeTerrainMaterial::Count;
}

[[nodiscard]] bool globalFieldsEqual(
    const CreativeTerrainPathSourceRecipe& lhs,
    const CreativeTerrainPathSourceRecipe& rhs) noexcept {
  return lhs.version == rhs.version && lhs.kind == rhs.kind &&
         lhs.elevation == rhs.elevation && lhs.curve == rhs.curve &&
         lhs.crossSection == rhs.crossSection &&
         lhs.startJoin == rhs.startJoin && lhs.endJoin == rhs.endJoin &&
         lhs.falloffCells == rhs.falloffCells &&
         lhs.paintSurface == rhs.paintSurface && lhs.material == rhs.material &&
         lhs.road == rhs.road && lhs.watercourse == rhs.watercourse;
}

}  // namespace

bool isValidCreativeTerrainPathSourceRecipe(
    const CreativeTerrainPathSourceRecipe& recipe) noexcept {
  if (recipe.version != kCreativeTerrainPathSourceVersion ||
      !validEnum(recipe.kind, CreativeTerrainPathKind::Count) ||
      !validEnum(recipe.elevation, CreativeTerrainPathElevation::Count) ||
      !validEnum(recipe.curve, CreativeTerrainPathCurvePolicy::Count) ||
      !validEnum(recipe.crossSection, CreativeTerrainPathCrossSection::Count) ||
      !validEnum(recipe.startJoin, CreativeTerrainPathEndpointJoin::Count) ||
      !validEnum(recipe.endJoin, CreativeTerrainPathEndpointJoin::Count) ||
      recipe.falloffCells > kCreativeTerrainPathSourceMaximumFalloffCells ||
      !validRoadSettings(recipe.road) ||
      !validWatercourseSettings(recipe) ||
      recipe.nextPointId == kInvalidCreativeTerrainPathSourcePointId ||
      recipe.points.size() < 2U ||
      recipe.points.size() > kCreativeTerrainPathPointCapacity ||
      (recipe.material != CreativeTerrainMaterial::Count &&
       !isValidCreativeTerrainMaterial(recipe.material))) {
    return false;
  }
  CreativeTerrainPathSourcePointId maximumId =
      kInvalidCreativeTerrainPathSourcePointId;
  for (std::size_t index = 0U; index < recipe.points.size(); ++index) {
    const CreativeTerrainPathSourcePoint& point = recipe.points[index];
    if (point.id == kInvalidCreativeTerrainPathSourcePointId ||
        point.heightCells < kCreativeTerrainMinimumHeightCells ||
        point.heightCells > kCreativeTerrainMaximumHeightCells ||
        point.halfWidthCells > kCreativeTerrainPathSourceMaximumHalfWidthCells ||
        (recipe.kind == CreativeTerrainPathKind::Road &&
         static_cast<std::uint32_t>(point.halfWidthCells) +
                 recipe.road.shoulderWidthCells >
             kCreativeTerrainPathSourceMaximumHalfWidthCells) ||
        (isWatercourseKind(recipe.kind) &&
         static_cast<std::uint32_t>(point.halfWidthCells) +
                 recipe.watercourse.bankSlopeCells >
             kCreativeTerrainPathSourceMaximumHalfWidthCells) ||
        point.amplitudeCells > kCreativeTerrainMaximumHeightCells ||
        std::abs(static_cast<std::int64_t>(point.bankPermille)) >
            kCreativeTerrainPathSourceMaximumBankPermille ||
        (index > 0U && point.coord == recipe.points[index - 1U].coord)) {
      return false;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      if (recipe.points[prior].id == point.id) {
        return false;
      }
    }
    const std::uint64_t span =
        index == 0U
            ? 0U
            : static_cast<std::uint64_t>(std::max(
                  std::llabs(static_cast<std::int64_t>(point.coord.x) -
                             recipe.points[index - 1U].coord.x),
                  std::llabs(static_cast<std::int64_t>(point.coord.z) -
                             recipe.points[index - 1U].coord.z)));
    if (span > kCreativeTerrainPathMaximumSegmentCells) {
      return false;
    }
    maximumId = std::max(maximumId, point.id);
  }
  if (recipe.watercourse.surfacePolicy ==
      CreativeTerrainWaterSurfacePolicy::Reserved) {
    for (const CreativeTerrainPathSourcePoint& point : recipe.points) {
      if (point.amplitudeCells <= recipe.watercourse.surfaceInsetCells ||
          point.heightCells < point.amplitudeCells) {
        return false;
      }
    }
  }
  return recipe.nextPointId > maximumId && roadGradeWithinLimit(recipe) &&
         watercourseDrainageWithinDirection(recipe);
}

std::string_view toString(CreativeTerrainPathCurvePolicy value) noexcept {
  constexpr std::array names{"LINEAR", "CATMULL-ROM"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainPathCrossSection value) noexcept {
  constexpr std::array names{"FLAT", "CROWNED", "CHANNEL", "BERM", "CUT"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainPathEndpointJoin value) noexcept {
  constexpr std::array names{"OPEN", "BLEND", "INTERSECTION", "BRIDGE",
                             "BUILDING_PAD"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainRoadEdgeTreatment value) noexcept {
  constexpr std::array names{"NONE", "CURB"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(
    CreativeTerrainWatercourseDrainageDirection value) noexcept {
  constexpr std::array names{"UNSPECIFIED", "START_TO_END", "END_TO_START"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainWaterSurfacePolicy value) noexcept {
  constexpr std::array names{"NONE", "RESERVED"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

bool parseCreativeTerrainPathKind(
    std::string_view text,
    CreativeTerrainPathKind& output) noexcept {
  for (std::uint8_t index = 0U;
       index < static_cast<std::uint8_t>(CreativeTerrainPathKind::Count);
       ++index) {
    const auto candidate = static_cast<CreativeTerrainPathKind>(index);
    if (toString(candidate) == text) {
      output = candidate;
      return true;
    }
  }
  return false;
}

bool parseCreativeTerrainPathElevation(
    std::string_view text,
    CreativeTerrainPathElevation& output) noexcept {
  for (std::uint8_t index = 0U;
       index < static_cast<std::uint8_t>(CreativeTerrainPathElevation::Count);
       ++index) {
    const auto candidate = static_cast<CreativeTerrainPathElevation>(index);
    if (toString(candidate) == text) {
      output = candidate;
      return true;
    }
  }
  return false;
}

bool parseCreativeTerrainPathCurvePolicy(
    std::string_view text,
    CreativeTerrainPathCurvePolicy& output) noexcept {
  for (std::uint8_t index = 0U;
       index < static_cast<std::uint8_t>(CreativeTerrainPathCurvePolicy::Count);
       ++index) {
    const auto candidate = static_cast<CreativeTerrainPathCurvePolicy>(index);
    if (toString(candidate) == text) {
      output = candidate;
      return true;
    }
  }
  return false;
}

bool parseCreativeTerrainPathCrossSection(
    std::string_view text,
    CreativeTerrainPathCrossSection& output) noexcept {
  for (std::uint8_t index = 0U;
       index < static_cast<std::uint8_t>(CreativeTerrainPathCrossSection::Count);
       ++index) {
    const auto candidate = static_cast<CreativeTerrainPathCrossSection>(index);
    if (toString(candidate) == text) {
      output = candidate;
      return true;
    }
  }
  return false;
}

bool parseCreativeTerrainPathEndpointJoin(
    std::string_view text,
    CreativeTerrainPathEndpointJoin& output) noexcept {
  for (std::uint8_t index = 0U;
       index < static_cast<std::uint8_t>(CreativeTerrainPathEndpointJoin::Count);
       ++index) {
    const auto candidate = static_cast<CreativeTerrainPathEndpointJoin>(index);
    if (toString(candidate) == text) {
      output = candidate;
      return true;
    }
  }
  return false;
}

std::string_view toString(CreativeTerrainPathSourceStatus value) noexcept {
  constexpr std::array names{
      "NotRequested",      "UnsupportedVersion", "InvalidRecipe",
      "InvalidSource",     "CoordinateOverflow", "CapacityExceeded",
      "HeightOutOfRange",  "MaterialRejected",   "OutputRejected",
      "Ready",
  };
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "Unknown";
}

std::uint64_t hashCreativeTerrainPathSourceRecipe(
    const CreativeTerrainPathSourceRecipe& recipe) noexcept {
  StableHasher hasher;
  hasher.addU64(recipe.version);
  hasher.addU64(static_cast<std::uint8_t>(recipe.kind));
  hasher.addU64(static_cast<std::uint8_t>(recipe.elevation));
  hasher.addU64(static_cast<std::uint8_t>(recipe.curve));
  hasher.addU64(static_cast<std::uint8_t>(recipe.crossSection));
  hasher.addU64(static_cast<std::uint8_t>(recipe.startJoin));
  hasher.addU64(static_cast<std::uint8_t>(recipe.endJoin));
  hasher.addU64(recipe.falloffCells);
  hasher.addBool(recipe.paintSurface);
  hasher.addU64(static_cast<std::uint8_t>(recipe.material));
  hasher.addU64(recipe.road.shoulderWidthCells);
  hasher.addU64(recipe.road.maximumGradePermille);
  hasher.addU64(static_cast<std::uint8_t>(recipe.road.edgeTreatment));
  hasher.addU64(std::bit_cast<std::uint64_t>(recipe.road.edgeWidthMeters));
  hasher.addU64(std::bit_cast<std::uint64_t>(recipe.road.edgeHeightMeters));
  hasher.addU64(static_cast<std::uint8_t>(recipe.road.edgeMaterial));
  hasher.addU64(recipe.watercourse.bankSlopeCells);
  hasher.addU64(
      static_cast<std::uint8_t>(recipe.watercourse.drainageDirection));
  hasher.addU64(static_cast<std::uint8_t>(recipe.watercourse.surfacePolicy));
  hasher.addU64(recipe.watercourse.surfaceInsetCells);
  hasher.addU64(recipe.watercourse.nextCrossingId);
  hasher.addU64(recipe.watercourse.crossings.size());
  for (const CreativeTerrainWatercourseCrossing& crossing :
       recipe.watercourse.crossings) {
    hasher.addU64(crossing.id);
    hasher.addU64(crossing.pointId);
    hasher.addU64(crossing.bankClearanceCells);
    hasher.addU64(crossing.deckClearanceCells);
    hasher.addU64(crossing.approachLengthCells);
  }
  hasher.addU64(recipe.nextPointId);
  hasher.addU64(recipe.points.size());
  for (const CreativeTerrainPathSourcePoint& point : recipe.points) {
    hasher.addU64(point.id);
    hasher.addI64(point.coord.x);
    hasher.addI64(point.coord.z);
    hasher.addU64(point.heightCells);
    hasher.addU64(point.halfWidthCells);
    hasher.addU64(point.amplitudeCells);
    hasher.addI64(point.bankPermille);
  }
  return hasher.value();
}

CreativeTerrainPathDirtySegments diffCreativeTerrainPathSourceSegments(
    const CreativeTerrainPathSourceRecipe& before,
    const CreativeTerrainPathSourceRecipe& after) noexcept {
  CreativeTerrainPathDirtySegments dirty;
  if (before == after) {
    return dirty;
  }
  if (globalFieldsEqual(before, after) && before.points == after.points) {
    return dirty;
  }
  dirty.changed = true;
  const std::size_t afterSegments =
      after.points.empty() ? 0U : after.points.size() - 1U;
  if (!globalFieldsEqual(before, after) || before.points.size() < 2U ||
      after.points.size() < 2U) {
    dirty.allSegments = true;
    dirty.segmentCount = afterSegments;
    return dirty;
  }
  const std::size_t common = std::min(before.points.size(), after.points.size());
  std::size_t firstChangedPoint = common;
  for (std::size_t index = 0U; index < common; ++index) {
    if (before.points[index] != after.points[index]) {
      firstChangedPoint = index;
      break;
    }
  }
  std::size_t unchangedSuffix = 0U;
  while (unchangedSuffix < common - firstChangedPoint &&
         before.points[before.points.size() - 1U - unchangedSuffix] ==
             after.points[after.points.size() - 1U - unchangedSuffix]) {
    ++unchangedSuffix;
  }
  const std::size_t lastChangedPointExclusive =
      after.points.size() - unchangedSuffix;
  dirty.firstSegment = firstChangedPoint > 1U ? firstChangedPoint - 2U : 0U;
  const std::size_t lastSegmentExclusive = std::min(
      afterSegments, lastChangedPointExclusive + 1U);
  dirty.segmentCount =
      lastSegmentExclusive > dirty.firstSegment
          ? lastSegmentExclusive - dirty.firstSegment
          : 0U;
  dirty.allSegments = dirty.segmentCount == afterSegments;
  return dirty;
}

CreativeTerrainPathSourceSamplingResult sampleCreativeTerrainPathSourceRecipe(
    const CreativeTerrainPathSourceRecipe& recipe,
    const CreativeTerrainPathSourceCache* previousCache) {
  CreativeTerrainPathSourceSamplingResult result;
  if (recipe.version != kCreativeTerrainPathSourceVersion) {
    result.status = CreativeTerrainPathSourceStatus::UnsupportedVersion;
    result.reasonCode = "creative_terrain_path_source_version_unsupported";
    return result;
  }
  if (recipe.kind == CreativeTerrainPathKind::Road &&
      !roadGradeWithinLimit(recipe)) {
    result.status = CreativeTerrainPathSourceStatus::InvalidRecipe;
    result.reasonCode = "creative_terrain_path_source_grade_limit_exceeded";
    return result;
  }
  if (isWatercourseKind(recipe.kind) &&
      !watercourseDrainageWithinDirection(recipe)) {
    result.status = CreativeTerrainPathSourceStatus::InvalidRecipe;
    result.reasonCode =
        "creative_terrain_path_source_drainage_direction_invalid";
    return result;
  }
  if (!isValidCreativeTerrainPathSourceRecipe(recipe)) {
    result.status = CreativeTerrainPathSourceStatus::InvalidRecipe;
    result.reasonCode = "creative_terrain_path_source_recipe_invalid";
    return result;
  }
  result.samples.reserve(256U);
  if (!centerlineForRecipe(recipe, result.samples, result.segments,
                           previousCache, result.cache)) {
    result.samples.clear();
    result.segments.clear();
    result.cache = {};
    result.status = CreativeTerrainPathSourceStatus::CapacityExceeded;
    result.reasonCode = "creative_terrain_path_source_centerline_rejected";
    return result;
  }
  result.cache.valid = true;
  result.accepted = true;
  result.status = CreativeTerrainPathSourceStatus::Ready;
  result.reasonCode = "creative_terrain_path_source_sampling_ready";
  return result;
}

CreativeTerrainPathSourceResult buildCreativeTerrainPathSourceRecipe(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainMaterialField& materialSource,
    const CreativeTerrainPathSourceRecipe& recipe,
    CreativeTerrainPathSourceCache* cache) {
  CreativeTerrainPathSourceResult result;
  CreativeTerrainPathSourceReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.sourceColumnCount = canonicalSource.columns.size();
  CreativeTerrainPathSourceSamplingResult sampling =
      sampleCreativeTerrainPathSourceRecipe(recipe, cache);
  if (!sampling.accepted) {
    receipt.status = sampling.status;
    receipt.reasonCode = sampling.reasonCode;
    return result;
  }
  if (!existingAuthored.validateInvariants() ||
      !validCanonicalSource(canonicalSource) ||
      !materialSource.validateInvariants()) {
    receipt.status = CreativeTerrainPathSourceStatus::InvalidSource;
    receipt.reasonCode = "creative_terrain_path_source_input_invalid";
    return result;
  }

  std::vector<CenterlineSample>& centerline = sampling.samples;
  result.segments = std::move(sampling.segments);
  receipt.segmentCount = result.segments.size();
  receipt.centerlineCellCount = centerline.size();
  receipt.rebuiltSegmentCount = sampling.cache.rebuiltSegmentCount;
  receipt.reusedSegmentCount = sampling.cache.reusedSegmentCount;

  // Roads use the shared grade adapter for every source segment. The path
  // kernel adds curvature and variable profiles, but does not invent a second
  // endpoint-elevation contract.
  if (recipe.kind == CreativeTerrainPathKind::Road) {
    for (std::size_t index = 0U; index + 1U < recipe.points.size(); ++index) {
      CreativeTerrainGradePathSegmentRequest grade;
      grade.start = recipe.points[index].coord;
      grade.end = recipe.points[index + 1U].coord;
      grade.startHeightCells = recipe.points[index].heightCells;
      grade.endHeightCells = recipe.points[index + 1U].heightCells;
      grade.halfWidthCells = static_cast<std::uint16_t>(
          std::max(recipe.points[index].halfWidthCells,
                   recipe.points[index + 1U].halfWidthCells) +
          recipe.road.shoulderWidthCells);
      grade.crossSlopePermille = static_cast<std::int32_t>(
          (static_cast<std::int64_t>(recipe.points[index].bankPermille) +
           recipe.points[index + 1U].bankPermille) /
          2);
      grade.falloffCells = recipe.falloffCells;
      if (!planCreativeTerrainGradePathSegment(grade).accepted) {
        result.segments.clear();
        receipt.status = CreativeTerrainPathSourceStatus::InvalidRecipe;
        receipt.reasonCode = "creative_terrain_path_source_grade_rejected";
        return result;
      }
    }
  }

  CreativeTerrainHeightFieldBounds pathRegion;
  CreativeTerrainHeightFieldBounds outputBounds;
  if (!pathBounds(recipe, centerline, pathRegion) ||
      !unionBounds(existingAuthored, pathRegion, outputBounds)) {
    result.segments.clear();
    receipt.status = CreativeTerrainPathSourceStatus::CoordinateOverflow;
    receipt.reasonCode = "creative_terrain_path_source_bounds_invalid";
    return result;
  }
  const std::uint64_t outputCellCount =
      static_cast<std::uint64_t>(outputBounds.widthCells) *
      outputBounds.depthCells;
  if (outputCellCount > kCreativeTerrainHeightFieldCellCapacity) {
    result.segments.clear();
    receipt.status = CreativeTerrainPathSourceStatus::CapacityExceeded;
    receipt.reasonCode = "creative_terrain_path_source_capacity_exceeded";
    return result;
  }

  const std::uint16_t maximumOuter = static_cast<std::uint16_t>(
      kCreativeTerrainPathSourceMaximumHalfWidthCells + recipe.falloffCells);
  const CreativeTerrainMaterial material = resolvedMaterial(recipe);
  std::vector<std::int32_t> centerlineLookup(
      static_cast<std::size_t>(outputCellCount), -1);
  for (std::size_t index = 0U; index < centerline.size(); ++index) {
    const std::int64_t localX =
        static_cast<std::int64_t>(centerline[index].coord.x) -
        outputBounds.minimum.x;
    const std::int64_t localZ =
        static_cast<std::int64_t>(centerline[index].coord.z) -
        outputBounds.minimum.z;
    if (localX < 0 || localX >= outputBounds.widthCells || localZ < 0 ||
        localZ >= outputBounds.depthCells) {
      continue;
    }
    std::int32_t& lookup = centerlineLookup[
        static_cast<std::size_t>(localZ) * outputBounds.widthCells +
        static_cast<std::size_t>(localX)];
    if (lookup < 0) {
      lookup = static_cast<std::int32_t>(index);
    }
  }
  std::vector<std::uint16_t> heights;
  heights.reserve(static_cast<std::size_t>(outputCellCount));
  StableHasher outputHasher;
  outputHasher.addI64(outputBounds.minimum.x);
  outputHasher.addI64(outputBounds.minimum.z);
  outputHasher.addU64(outputBounds.widthCells);
  outputHasher.addU64(outputBounds.depthCells);
  for (std::uint16_t z = 0U; z < outputBounds.depthCells; ++z) {
    for (std::uint16_t x = 0U; x < outputBounds.widthCells; ++x) {
      const CreativeTerrainCoord2 coord{
          outputBounds.minimum.x + static_cast<std::int32_t>(x),
          outputBounds.minimum.z + static_cast<std::int32_t>(z)};
      const std::uint16_t source =
          sourceHeightAt(existingAuthored, canonicalSource, coord);
      const CenterlineSample* nearest = nullptr;
      double nearestDistanceSquared = std::numeric_limits<double>::max();
      for (std::int32_t dz = -static_cast<std::int32_t>(maximumOuter);
           dz <= static_cast<std::int32_t>(maximumOuter); ++dz) {
        const std::int32_t sampleZ = static_cast<std::int32_t>(z) + dz;
        if (sampleZ < 0 || sampleZ >= outputBounds.depthCells) {
          continue;
        }
        for (std::int32_t dx = -static_cast<std::int32_t>(maximumOuter);
             dx <= static_cast<std::int32_t>(maximumOuter); ++dx) {
          const std::int32_t sampleX = static_cast<std::int32_t>(x) + dx;
          if (sampleX < 0 || sampleX >= outputBounds.widthCells) {
            continue;
          }
          const std::int32_t lookup = centerlineLookup[
              static_cast<std::size_t>(sampleZ) * outputBounds.widthCells +
              static_cast<std::size_t>(sampleX)];
          if (lookup < 0) {
            continue;
          }
          const CenterlineSample& sample =
              centerline[static_cast<std::size_t>(lookup)];
          const double distanceSquared =
              static_cast<double>(dx * dx + dz * dz);
          const double outer = sample.halfWidthCells + recipe.falloffCells;
          if (distanceSquared > outer * outer ||
              distanceSquared >= nearestDistanceSquared) {
            continue;
          }
          nearest = &sample;
          nearestDistanceSquared = distanceSquared;
        }
      }

      std::uint16_t output = source;
      if (nearest != nullptr) {
        ++receipt.evaluatedCellCount;
        ++receipt.generatedControlCount;
        ++result.segments[nearest->segmentIndex].generatedCellCount;
        const double distance = std::sqrt(nearestDistanceSquared);
        const double radialWeight =
            distance <= nearest->halfWidthCells || recipe.falloffCells == 0U
                ? 1.0
                : std::clamp(
                      (nearest->halfWidthCells + recipe.falloffCells -
                       distance) /
                          recipe.falloffCells,
                      0.0, 1.0);
        const double weight = radialWeight * nearest->terrainWeight;
        double baseline = nearest->heightCells;
        if (recipe.elevation == CreativeTerrainPathElevation::Follow) {
          const std::uint16_t followed = sourceHeightAt(
              existingAuthored, canonicalSource, nearest->coord);
          // Empty columns carry no terrain elevation. Keep the authored point
          // height as the fallback, matching the original path kernel and
          // allowing a path to establish terrain in an empty region.
          if (followed != kCreativeTerrainEmptyHeightCells) {
            baseline = followed;
          }
        } else if (recipe.elevation == CreativeTerrainPathElevation::Level) {
          baseline = recipe.points.front().heightCells;
        }
        const double tangentLength =
            std::hypot(nearest->tangentX, nearest->tangentZ);
        const double signedLateral =
            tangentLength <= 0.0
                ? 0.0
                : (static_cast<double>(nearest->tangentX) *
                       (coord.z - nearest->coord.z) -
                   static_cast<double>(nearest->tangentZ) *
                       (coord.x - nearest->coord.x)) /
                      tangentLength;
        const double bank = signedLateral * nearest->bankPermille / 1000.0 *
                            nearest->profileWeight;
        const double shape =
            crossSectionOffset(recipe, nearest->amplitudeCells, distance,
                               nearest->profileHalfWidthCells,
                               nearest->halfWidthCells) *
            nearest->profileWeight;
        const long long target = std::llround(baseline + bank + shape);
        if (target < kCreativeTerrainMinimumHeightCells ||
            target > kCreativeTerrainMaximumHeightCells) {
          result.segments.clear();
          result.materialEdits.clear();
          receipt.status = CreativeTerrainPathSourceStatus::HeightOutOfRange;
          receipt.reasonCode = "creative_terrain_path_source_height_out_of_range";
          return result;
        }
        const long long blended = std::llround(
            static_cast<double>(source) * (1.0 - weight) +
            static_cast<double>(target) * weight);
        output = static_cast<std::uint16_t>(std::clamp<long long>(
            blended, kCreativeTerrainEmptyHeightCells,
            kCreativeTerrainMaximumHeightCells));
        if (recipe.paintSurface &&
            material < CreativeTerrainMaterial::Count &&
            distance <= nearest->halfWidthCells &&
            nearest->terrainWeight >= 0.5 && output != 0U &&
            materialSource.materialAt(coord) != material) {
          result.materialEdits.push_back(
              {CreativeTerrainMaterialEditKind::Set, coord, material, {}});
        }
      }
      receipt.modifiedCellCount += output != source ? 1U : 0U;
      outputHasher.addU64(output);
      heights.push_back(output);
    }
  }

  if (result.materialEdits.size() > kCreativeTerrainMaterialOverrideCapacity) {
    result.segments.clear();
    result.materialEdits.clear();
    receipt.status = CreativeTerrainPathSourceStatus::MaterialRejected;
    receipt.reasonCode = "creative_terrain_path_source_material_capacity";
    return result;
  }
  CreativeTerrainMaterialField stagedMaterials = materialSource;
  if (!result.materialEdits.empty() &&
      !stagedMaterials.apply(result.materialEdits).accepted) {
    result.segments.clear();
    result.materialEdits.clear();
    receipt.status = CreativeTerrainPathSourceStatus::MaterialRejected;
    receipt.reasonCode = "creative_terrain_path_source_material_rejected";
    return result;
  }
  const CreativeTerrainHeightFieldReplaceReceipt replaced =
      result.heightField.replace(outputBounds, heights);
  if (!replaced.accepted) {
    result.segments.clear();
    result.materialEdits.clear();
    receipt.status = CreativeTerrainPathSourceStatus::OutputRejected;
    receipt.reasonCode = "creative_terrain_path_source_output_rejected";
    return result;
  }
  receipt.accepted = true;
  receipt.status = CreativeTerrainPathSourceStatus::Ready;
  receipt.materialEditCount = result.materialEdits.size();
  receipt.outputCellCount = outputCellCount;
  receipt.outputHeightHash = outputHasher.value();
  receipt.recipeHash = hashCreativeTerrainPathSourceRecipe(recipe);
  receipt.reasonCode = "creative_terrain_path_source_ready";
  result.samples = centerline;
  sampling.cache.generatedControlCount = receipt.generatedControlCount;
  if (cache != nullptr) {
    *cache = std::move(sampling.cache);
  }
  return result;
}

}  // namespace iggy3d::creative
