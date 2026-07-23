#include "app/iggy3d/creative/recipes/WatercourseRecipe.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/recipes/TerrainOperation.hpp"
#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace iggy3d::creative {
namespace {

void reject(CreativeWatercourseRecipeReceipt& receipt,
            CreativeWatercourseRecipeStatus status,
            std::string_view reasonCode) {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
}

[[nodiscard]] bool validGrid(CreativeGridSettings grid) noexcept {
  return isFiniteCreativeVec3(grid.origin) &&
         std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0;
}

[[nodiscard]] bool validRequest(
    const CreativeWatercourseRecipeRequest& request) noexcept {
  return request.version == kCreativeWatercourseRecipeVersion &&
         !request.instanceKey.empty() && !request.name.empty() &&
         validGrid(request.grid) &&
         (request.source.kind == CreativeTerrainPathKind::River ||
          request.source.kind == CreativeTerrainPathKind::Trench) &&
         isValidCreativeTerrainPathSourceRecipe(request.source);
}

[[nodiscard]] double terrainHeightCellsAt(
    const CreativeTerrainHeightField& terrain,
    double gridX,
    double gridZ,
    double fallback) noexcept {
  const long long roundedX = std::llround(gridX);
  const long long roundedZ = std::llround(gridZ);
  if (roundedX >= std::numeric_limits<std::int32_t>::min() &&
      roundedX <= std::numeric_limits<std::int32_t>::max() &&
      roundedZ >= std::numeric_limits<std::int32_t>::min() &&
      roundedZ <= std::numeric_limits<std::int32_t>::max()) {
    const auto height = terrain.heightAt(
        {static_cast<std::int32_t>(roundedX),
         static_cast<std::int32_t>(roundedZ)});
    if (height.has_value() && *height != kCreativeTerrainEmptyHeightCells) {
      return *height;
    }
  }
  return fallback;
}

[[nodiscard]] CreativeVec3 worldPoint(CreativeGridSettings grid,
                                      double gridX,
                                      double heightCells,
                                      double gridZ) noexcept {
  return {grid.origin.x + gridX * grid.cellSizeMeters,
          grid.origin.y + heightCells * grid.cellSizeMeters,
          grid.origin.z + gridZ * grid.cellSizeMeters};
}

[[nodiscard]] const CreativeTerrainPathSourcePoint* sourcePoint(
    const CreativeTerrainPathSourceRecipe& source,
    CreativeTerrainPathSourcePointId id,
    std::size_t& pointIndex) noexcept {
  for (std::size_t index = 0U; index < source.points.size(); ++index) {
    if (source.points[index].id == id) {
      pointIndex = index;
      return &source.points[index];
    }
  }
  return nullptr;
}

[[nodiscard]] bool appendCrossing(
    CreativeWatercoursePlan& plan,
    const CreativeTerrainHeightField& terrain,
    const CreativeWatercourseRecipeRequest& request,
    const CreativeTerrainWatercourseCrossing& crossing) {
  std::size_t pointIndex = 0U;
  const CreativeTerrainPathSourcePoint* point =
      sourcePoint(request.source, crossing.pointId, pointIndex);
  if (point == nullptr) {
    return false;
  }
  const CreativeTerrainCoord2 before =
      request.source.points[pointIndex == 0U ? 0U : pointIndex - 1U].coord;
  const CreativeTerrainCoord2 after =
      request.source.points[std::min(pointIndex + 1U,
                                     request.source.points.size() - 1U)]
          .coord;
  const double tangentX = static_cast<double>(after.x) - before.x;
  const double tangentZ = static_cast<double>(after.z) - before.z;
  const double tangentLength = std::hypot(tangentX, tangentZ);
  if (!std::isfinite(tangentLength) || tangentLength <= 0.0) {
    return false;
  }
  const double axisX = -tangentZ / tangentLength;
  const double axisZ = tangentX / tangentLength;
  const double bankOffsetCells =
      static_cast<double>(point->halfWidthCells) +
      request.source.watercourse.bankSlopeCells +
      crossing.bankClearanceCells;
  const double leftX = point->coord.x + axisX * bankOffsetCells;
  const double leftZ = point->coord.z + axisZ * bankOffsetCells;
  const double rightX = point->coord.x - axisX * bankOffsetCells;
  const double rightZ = point->coord.z - axisZ * bankOffsetCells;
  const double leftHeight = terrainHeightCellsAt(
      terrain, leftX, leftZ, point->heightCells);
  const double rightHeight = terrainHeightCellsAt(
      terrain, rightX, rightZ, point->heightCells);
  const double deckHeightCells =
      std::max(leftHeight, rightHeight) + crossing.deckClearanceCells;
  const double approachOffsetCells =
      bankOffsetCells + crossing.approachLengthCells;
  const double leftApproachX =
      point->coord.x + axisX * approachOffsetCells;
  const double leftApproachZ =
      point->coord.z + axisZ * approachOffsetCells;
  const double rightApproachX =
      point->coord.x - axisX * approachOffsetCells;
  const double rightApproachZ =
      point->coord.z - axisZ * approachOffsetCells;
  const double leftApproachHeight = terrainHeightCellsAt(
      terrain, leftApproachX, leftApproachZ, leftHeight);
  const double rightApproachHeight = terrainHeightCellsAt(
      terrain, rightApproachX, rightApproachZ, rightHeight);
  const double channelBedHeight = terrainHeightCellsAt(
      terrain, point->coord.x, point->coord.z,
      point->heightCells - point->amplitudeCells);
  const double reservedSurfaceHeight =
      point->heightCells - request.source.watercourse.surfaceInsetCells;
  const double clearanceReferenceHeight =
      request.source.watercourse.surfacePolicy ==
              CreativeTerrainWaterSurfacePolicy::Reserved
          ? reservedSurfaceHeight
          : channelBedHeight;

  CreativeWatercourseCrossingFrame frame;
  frame.id = crossing.id;
  frame.sourcePointId = crossing.pointId;
  frame.crossingAxis = {axisX, 0.0, axisZ};
  frame.centerMeters = worldPoint(request.grid, point->coord.x,
                                  deckHeightCells, point->coord.z);
  frame.leftBankMeters = worldPoint(request.grid, leftX, leftHeight, leftZ);
  frame.rightBankMeters = worldPoint(request.grid, rightX, rightHeight, rightZ);
  frame.leftApproachMeters = worldPoint(
      request.grid, leftApproachX, leftApproachHeight, leftApproachZ);
  frame.rightApproachMeters = worldPoint(
      request.grid, rightApproachX, rightApproachHeight, rightApproachZ);
  frame.centerGrid = {static_cast<double>(point->coord.x), deckHeightCells,
                      static_cast<double>(point->coord.z)};
  frame.leftBankGrid = {leftX, leftHeight, leftZ};
  frame.rightBankGrid = {rightX, rightHeight, rightZ};
  frame.leftApproachGrid = {leftApproachX, leftApproachHeight, leftApproachZ};
  frame.rightApproachGrid = {
      rightApproachX, rightApproachHeight, rightApproachZ};
  frame.channelBedGrid = {static_cast<double>(point->coord.x),
                          channelBedHeight,
                          static_cast<double>(point->coord.z)};
  frame.clearanceReferenceGrid = {
      static_cast<double>(point->coord.x), clearanceReferenceHeight,
      static_cast<double>(point->coord.z)};
  frame.channelBedMeters = worldPoint(
      request.grid, point->coord.x, channelBedHeight, point->coord.z);
  frame.clearanceReferenceMeters = worldPoint(
      request.grid, point->coord.x, clearanceReferenceHeight,
      point->coord.z);
  frame.bridgeTransform.position = frame.centerMeters;
  frame.bridgeTransform.rotationEulerRadians.y = -std::atan2(axisZ, axisX);
  frame.spanMeters = 2.0 * bankOffsetCells * request.grid.cellSizeMeters;
  if (!isFiniteCreativeVec3(frame.centerMeters) ||
      !isFiniteCreativeVec3(frame.crossingAxis) ||
      !isFiniteCreativeVec3(frame.leftBankMeters) ||
      !isFiniteCreativeVec3(frame.rightBankMeters) ||
      !isFiniteCreativeVec3(frame.leftApproachMeters) ||
      !isFiniteCreativeVec3(frame.rightApproachMeters) ||
      !isFiniteCreativeVec3(frame.centerGrid) ||
      !isFiniteCreativeVec3(frame.leftBankGrid) ||
      !isFiniteCreativeVec3(frame.rightBankGrid) ||
      !isFiniteCreativeVec3(frame.leftApproachGrid) ||
      !isFiniteCreativeVec3(frame.rightApproachGrid) ||
      !isFiniteCreativeVec3(frame.channelBedGrid) ||
      !isFiniteCreativeVec3(frame.clearanceReferenceGrid) ||
      !isFiniteCreativeVec3(frame.channelBedMeters) ||
      !isFiniteCreativeVec3(frame.clearanceReferenceMeters) ||
      !isFiniteCreativeVec3(frame.bridgeTransform.position) ||
      !isFiniteCreativeVec3(frame.bridgeTransform.rotationEulerRadians) ||
      !isFiniteCreativeVec3(frame.bridgeTransform.scale) ||
      !std::isfinite(frame.spanMeters) || frame.spanMeters <= 0.0) {
    return false;
  }
  plan.crossings.push_back(frame);
  return true;
}

[[nodiscard]] std::uint64_t fingerprint(
    const CreativeWatercoursePlan& plan,
    const CreativeTerrainPathSourceRecipe& source,
    std::uint64_t terrainHeightHash) noexcept {
  StableHasher hasher;
  hasher.addU64(static_cast<std::uint8_t>(plan.kind));
  hasher.addU64(static_cast<std::uint8_t>(plan.pathKind));
  hasher.addU64(static_cast<std::uint8_t>(plan.drainageDirection));
  hasher.addU64(static_cast<std::uint8_t>(plan.surfacePolicy));
  hasher.addString(plan.instanceKey);
  hasher.addU64(hashCreativeTerrainPathSourceRecipe(source));
  hasher.addU64(terrainHeightHash);
  hasher.addU64(plan.surfaceSamples.size());
  for (const CreativeWatercourseSurfaceSample& sample : plan.surfaceSamples) {
    hasher.addI64(sample.coord.x);
    hasher.addI64(sample.coord.z);
    hasher.addU64(std::bit_cast<std::uint64_t>(sample.flowProgress));
    hasher.addU64(std::bit_cast<std::uint64_t>(sample.bankHeightCells));
    hasher.addU64(std::bit_cast<std::uint64_t>(sample.bedHeightCells));
    hasher.addU64(
        std::bit_cast<std::uint64_t>(sample.reservedSurfaceHeightCells));
  }
  hasher.addU64(plan.crossings.size());
  const auto addVec3 = [&hasher](CreativeVec3 value) {
    hasher.addU64(std::bit_cast<std::uint64_t>(value.x));
    hasher.addU64(std::bit_cast<std::uint64_t>(value.y));
    hasher.addU64(std::bit_cast<std::uint64_t>(value.z));
  };
  for (const CreativeWatercourseCrossingFrame& crossing : plan.crossings) {
    hasher.addU64(crossing.id);
    hasher.addU64(crossing.sourcePointId);
    addVec3(crossing.centerMeters);
    addVec3(crossing.crossingAxis);
    addVec3(crossing.leftBankMeters);
    addVec3(crossing.rightBankMeters);
    addVec3(crossing.leftApproachMeters);
    addVec3(crossing.rightApproachMeters);
    addVec3(crossing.centerGrid);
    addVec3(crossing.leftBankGrid);
    addVec3(crossing.rightBankGrid);
    addVec3(crossing.leftApproachGrid);
    addVec3(crossing.rightApproachGrid);
    addVec3(crossing.channelBedGrid);
    addVec3(crossing.clearanceReferenceGrid);
    addVec3(crossing.channelBedMeters);
    addVec3(crossing.clearanceReferenceMeters);
    hasher.addU64(std::bit_cast<std::uint64_t>(crossing.spanMeters));
  }
  return hasher.value();
}

}  // namespace

std::string_view toString(CreativeWatercourseRecipeStatus status) noexcept {
  switch (status) {
    case CreativeWatercourseRecipeStatus::NotRequested: return "NotRequested";
    case CreativeWatercourseRecipeStatus::UnsupportedVersion:
      return "UnsupportedVersion";
    case CreativeWatercourseRecipeStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeWatercourseRecipeStatus::PathRejected: return "PathRejected";
    case CreativeWatercourseRecipeStatus::TerrainRejected:
      return "TerrainRejected";
    case CreativeWatercourseRecipeStatus::CrossingRejected:
      return "CrossingRejected";
    case CreativeWatercourseRecipeStatus::Ready: return "Ready";
  }
  return "Unknown";
}

CreativeWatercoursePlanResult planCreativeWatercourse(
    const CreativeTerrainHeightField& generatedTerrain,
    const CreativeWatercourseRecipeRequest& request) {
  CreativeWatercoursePlanResult result;
  CreativeWatercourseRecipeReceipt& receipt = result.receipt;
  receipt.requested = true;
  if (request.version != kCreativeWatercourseRecipeVersion) {
    reject(receipt, CreativeWatercourseRecipeStatus::UnsupportedVersion,
           "creative_watercourse_recipe_version_unsupported");
    return result;
  }
  if (!validRequest(request)) {
    reject(receipt, CreativeWatercourseRecipeStatus::InvalidRequest,
           "creative_watercourse_recipe_request_invalid");
    return result;
  }
  if (!generatedTerrain.validateInvariants()) {
    reject(receipt, CreativeWatercourseRecipeStatus::TerrainRejected,
           "creative_watercourse_recipe_terrain_invalid");
    return result;
  }
  const CreativeTerrainPathSourceSamplingResult sampled =
      sampleCreativeTerrainPathSourceRecipe(request.source);
  if (!sampled.accepted) {
    reject(receipt, CreativeWatercourseRecipeStatus::PathRejected,
           sampled.reasonCode);
    return result;
  }

  result.plan.instanceKey = request.instanceKey;
  result.plan.pathKind = request.source.kind;
  result.plan.drainageDirection =
      request.source.watercourse.drainageDirection;
  result.plan.surfacePolicy = request.source.watercourse.surfacePolicy;
  receipt.centerlineSampleCount = sampled.samples.size();
  result.plan.surfaceSamples.reserve(sampled.samples.size());
  const bool reverseFlow =
      request.source.watercourse.drainageDirection ==
      CreativeTerrainWatercourseDrainageDirection::EndToStart;
  const double denominator =
      sampled.samples.size() > 1U
          ? static_cast<double>(sampled.samples.size() - 1U)
          : 1.0;
  for (std::size_t index = 0U; index < sampled.samples.size(); ++index) {
    const CreativeTerrainPathSourceSample& sourceSample = sampled.samples[index];
    CreativeWatercourseSurfaceSample surface;
    surface.coord = sourceSample.coord;
    const double orderedProgress = static_cast<double>(index) / denominator;
    surface.flowProgress = reverseFlow ? 1.0 - orderedProgress : orderedProgress;
    surface.bankHeightCells = sourceSample.heightCells;
    surface.bedHeightCells = terrainHeightCellsAt(
        generatedTerrain, sourceSample.coord.x, sourceSample.coord.z,
        sourceSample.heightCells - sourceSample.amplitudeCells);
    surface.reservedSurfaceHeightCells =
        sourceSample.heightCells -
        request.source.watercourse.surfaceInsetCells;
    surface.bedPositionMeters = worldPoint(
        request.grid, sourceSample.coord.x, surface.bedHeightCells,
        sourceSample.coord.z);
    surface.reservedSurfacePositionMeters = worldPoint(
        request.grid, sourceSample.coord.x,
        surface.reservedSurfaceHeightCells, sourceSample.coord.z);
    if (!std::isfinite(surface.flowProgress) ||
        !std::isfinite(surface.bankHeightCells) ||
        !std::isfinite(surface.bedHeightCells) ||
        !std::isfinite(surface.reservedSurfaceHeightCells) ||
        !isFiniteCreativeVec3(surface.bedPositionMeters) ||
        !isFiniteCreativeVec3(surface.reservedSurfacePositionMeters)) {
      result.plan = {};
      reject(receipt, CreativeWatercourseRecipeStatus::TerrainRejected,
             "creative_watercourse_recipe_surface_sample_invalid");
      return result;
    }
    result.plan.surfaceSamples.push_back(surface);
  }
  if (request.source.watercourse.surfacePolicy ==
      CreativeTerrainWaterSurfacePolicy::Reserved) {
    receipt.reservedSurfaceSampleCount = result.plan.surfaceSamples.size();
  }

  result.plan.crossings.reserve(request.source.watercourse.crossings.size());
  for (const CreativeTerrainWatercourseCrossing& crossing :
       request.source.watercourse.crossings) {
    if (!appendCrossing(result.plan, generatedTerrain, request, crossing)) {
      result.plan = {};
      reject(receipt, CreativeWatercourseRecipeStatus::CrossingRejected,
             "creative_watercourse_recipe_crossing_invalid");
      return result;
    }
  }
  receipt.crossingCount = result.plan.crossings.size();
  receipt.terrainCellCount = generatedTerrain.cellCount();
  receipt.terrainHeightHash = hashCreativeTerrainHeightField(generatedTerrain);
  receipt.definitionFingerprint =
      fingerprint(result.plan, request.source, receipt.terrainHeightHash);
  if (receipt.definitionFingerprint == 0U) {
    result.plan = {};
    reject(receipt, CreativeWatercourseRecipeStatus::InvalidRequest,
           "creative_watercourse_recipe_fingerprint_invalid");
    return result;
  }
  result.plan.definitionFingerprint = receipt.definitionFingerprint;
  receipt.accepted = true;
  receipt.status = CreativeWatercourseRecipeStatus::Ready;
  receipt.reasonCode = "creative_watercourse_recipe_ready";
  return result;
}

CreativeWatercourseRecipeResult buildCreativeWatercourseRecipe(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainMaterialField& materialSource,
    const CreativeWatercourseRecipeRequest& request,
    CreativeTerrainPathSourceCache* cache) {
  CreativeWatercourseRecipeResult result;
  result.receipt.requested = true;
  if (request.version != kCreativeWatercourseRecipeVersion) {
    reject(result.receipt, CreativeWatercourseRecipeStatus::UnsupportedVersion,
           "creative_watercourse_recipe_version_unsupported");
    return result;
  }
  if (!validRequest(request)) {
    reject(result.receipt, CreativeWatercourseRecipeStatus::InvalidRequest,
           "creative_watercourse_recipe_request_invalid");
    return result;
  }
  result.terrain = buildCreativeTerrainPathSourceRecipe(
      existingAuthored, canonicalSource, materialSource, request.source, cache);
  if (!result.terrain.receipt.accepted) {
    reject(result.receipt, CreativeWatercourseRecipeStatus::PathRejected,
           result.terrain.receipt.reasonCode);
    return result;
  }
  CreativeWatercoursePlanResult planned =
      planCreativeWatercourse(result.terrain.heightField, request);
  if (!planned.receipt.accepted) {
    result.terrain = {};
    result.receipt = std::move(planned.receipt);
    return result;
  }
  result.plan = std::move(planned.plan);
  result.receipt = std::move(planned.receipt);
  result.receipt.terrainCellCount = result.terrain.receipt.outputCellCount;
  result.receipt.terrainHeightHash = result.terrain.receipt.outputHeightHash;
  return result;
}

}  // namespace iggy3d::creative
