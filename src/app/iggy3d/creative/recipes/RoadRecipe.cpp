#include "app/iggy3d/creative/recipes/RoadRecipe.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr double kRoadGeometryEpsilon = 1.0e-9;
constexpr double kJoinStructureThreshold = 0.5;

void reject(CreativeRoadRecipeReceipt& receipt,
            CreativeRoadRecipeStatus status,
            std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
}

void appendTagOnce(std::vector<std::string>& tags, std::string tag) {
  if (!tag.empty() &&
      std::find(tags.begin(), tags.end(), tag) == tags.end()) {
    tags.push_back(std::move(tag));
  }
}

[[nodiscard]] bool validGrid(CreativeGridSettings grid) noexcept {
  return isFiniteCreativeVec3(grid.origin) &&
         std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0 &&
         grid.size.width > 0 && grid.size.height > 0 && grid.size.depth > 0;
}

[[nodiscard]] bool validRequest(
    const CreativeRoadRecipeRequest& request) noexcept {
  return request.version == kCreativeRoadRecipeVersion &&
         !request.instanceKey.empty() && !request.name.empty() &&
         validGrid(request.grid) &&
         request.source.kind == CreativeTerrainPathKind::Road &&
         isValidCreativeTerrainPathSourceRecipe(request.source);
}

struct RoadEdgePoint {
  CreativeVec3 position{};
  bool valid = false;
};

[[nodiscard]] std::uint16_t terrainHeightAtEdge(
    const CreativeTerrainHeightField& terrain,
    const CreativeTerrainPathSourceSample& sample,
    double gridX,
    double gridZ) noexcept {
  const auto roundedX = std::llround(gridX);
  const auto roundedZ = std::llround(gridZ);
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
  return static_cast<std::uint16_t>(std::clamp<long long>(
      std::llround(sample.heightCells), kCreativeTerrainMinimumHeightCells,
      kCreativeTerrainMaximumHeightCells));
}

[[nodiscard]] RoadEdgePoint roadEdgePoint(
    const CreativeTerrainHeightField& terrain,
    const CreativeRoadRecipeRequest& request,
    const CreativeTerrainPathSourceSample& sample,
    double sideSign) noexcept {
  const double tangentLength =
      std::hypot(sample.tangentX, sample.tangentZ);
  if (!std::isfinite(tangentLength) || tangentLength <= 0.0 ||
      !std::isfinite(sample.halfWidthCells)) {
    return {};
  }
  const double normalX = -static_cast<double>(sample.tangentZ) / tangentLength;
  const double normalZ = static_cast<double>(sample.tangentX) / tangentLength;
  const double offsetCells =
      sample.halfWidthCells +
      request.source.road.edgeWidthMeters /
          (request.grid.cellSizeMeters * 2.0);
  const double gridX = sample.coord.x + sideSign * normalX * offsetCells;
  const double gridZ = sample.coord.z + sideSign * normalZ * offsetCells;
  const std::uint16_t height =
      terrainHeightAtEdge(terrain, sample, gridX, gridZ);
  RoadEdgePoint result;
  result.position = {
      request.grid.origin.x + gridX * request.grid.cellSizeMeters,
      request.grid.origin.y + height * request.grid.cellSizeMeters,
      request.grid.origin.z + gridZ * request.grid.cellSizeMeters,
  };
  result.valid = isFiniteCreativeVec3(result.position);
  return result;
}

[[nodiscard]] bool appendEdgePiece(
    CreativeRecipePlan& plan,
    const CreativeRoadRecipeRequest& request,
    RoadEdgePoint from,
    RoadEdgePoint to,
    std::string_view side,
    std::size_t pairIndex) {
  if (!from.valid || !to.valid) {
    return false;
  }
  const CreativeVec3 delta{to.position.x - from.position.x,
                           to.position.y - from.position.y,
                           to.position.z - from.position.z};
  const double length =
      std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
  if (!std::isfinite(length) || length <= kRoadGeometryEpsilon) {
    return true;
  }
  if (plan.objects.size() >= kCreativeRoadGeneratedEdgePieceCapacity) {
    return false;
  }

  const CreativeVec3 center{(from.position.x + to.position.x) * 0.5,
                            (from.position.y + to.position.y) * 0.5 +
                                request.source.road.edgeHeightMeters * 0.5,
                            (from.position.z + to.position.z) * 0.5};
  const double yaw = -std::asin(std::clamp(delta.z / length, -1.0, 1.0));
  const double pitch = std::atan2(
      delta.y, std::hypot(delta.x, delta.z));
  CreativeDocumentCreateRequest create;
  create.kind = CreativeObjectKind::Beam;
  create.name = request.name + " " + std::string(side) + " Curb " +
                std::to_string(pairIndex + 1U);
  const double overlapLength = length + request.source.road.edgeWidthMeters;
  create.bounds = {
      {center.x - overlapLength * 0.5,
       center.y - request.source.road.edgeHeightMeters * 0.5,
       center.z - request.source.road.edgeWidthMeters * 0.5},
      {center.x + overlapLength * 0.5,
       center.y + request.source.road.edgeHeightMeters * 0.5,
       center.z + request.source.road.edgeWidthMeters * 0.5},
  };
  create.hasBoundsOverride = true;
  create.transform.position = center;
  create.transform.rotationEulerRadians = {0.0, yaw, pitch};
  create.hasTransformOverride = true;
  create.visible = true;
  create.hasVisibleOverride = true;
  create.tags = request.tags;
  appendTagOnce(create.tags, "creative_road:curb");
  appendTagOnce(create.tags, "creative_road:side:" + std::string(side));
  appendTagOnce(create.tags, creativeStructuralMaterialTag(
                                 request.source.road.edgeMaterial));

  CreativeRecipeObjectPlan object;
  object.createRequest = std::move(create);
  object.role = CreativeRecipeObjectRole::Generated;
  object.stableKey = "curb." + std::string(side) + "." +
                     std::to_string(pairIndex + 1U);
  plan.objects.push_back(std::move(object));
  return true;
}

}  // namespace

std::string_view toString(CreativeRoadRecipeStatus status) noexcept {
  switch (status) {
    case CreativeRoadRecipeStatus::NotRequested: return "NotRequested";
    case CreativeRoadRecipeStatus::UnsupportedVersion:
      return "UnsupportedVersion";
    case CreativeRoadRecipeStatus::InvalidRequest: return "InvalidRequest";
    case CreativeRoadRecipeStatus::PathRejected: return "PathRejected";
    case CreativeRoadRecipeStatus::StructureCapacityExceeded:
      return "StructureCapacityExceeded";
    case CreativeRoadRecipeStatus::InvalidStructure: return "InvalidStructure";
    case CreativeRoadRecipeStatus::Ready: return "Ready";
  }
  return "Unknown";
}

CreativeRoadStructureResult planCreativeRoadStructure(
    const CreativeTerrainHeightField& generatedTerrain,
    const CreativeRoadRecipeRequest& request) {
  CreativeRoadStructureResult result;
  CreativeRoadRecipeReceipt& receipt = result.receipt;
  receipt.requested = true;
  if (request.version != kCreativeRoadRecipeVersion) {
    reject(receipt, CreativeRoadRecipeStatus::UnsupportedVersion,
           "creative_road_recipe_version_unsupported");
    return result;
  }
  if (!validRequest(request) || !generatedTerrain.validateInvariants()) {
    reject(receipt, CreativeRoadRecipeStatus::InvalidRequest,
           "creative_road_recipe_request_invalid");
    return result;
  }

  result.plan.kind = CreativeRecipeKind::Road;
  result.plan.instanceKey = request.instanceKey;
  result.plan.instanceName = request.name;
  if (request.source.road.edgeTreatment ==
      CreativeTerrainRoadEdgeTreatment::None) {
    receipt.accepted = true;
    receipt.status = CreativeRoadRecipeStatus::Ready;
    receipt.reasonCode = "creative_road_recipe_ready_without_edges";
    return result;
  }

  const CreativeTerrainPathSourceSamplingResult sampled =
      sampleCreativeTerrainPathSourceRecipe(request.source);
  if (!sampled.accepted) {
    reject(receipt, CreativeRoadRecipeStatus::PathRejected,
           sampled.reasonCode);
    return result;
  }
  receipt.centerlineSampleCount = sampled.samples.size();
  for (std::size_t index = 0U; index + 1U < sampled.samples.size(); ++index) {
    const CreativeTerrainPathSourceSample& from = sampled.samples[index];
    const CreativeTerrainPathSourceSample& to = sampled.samples[index + 1U];
    if (from.profileWeight < kJoinStructureThreshold ||
        to.profileWeight < kJoinStructureThreshold) {
      receipt.suppressedJoinPieceCount += 2U;
      continue;
    }
    if (!appendEdgePiece(result.plan, request,
                         roadEdgePoint(generatedTerrain, request, from, 1.0),
                         roadEdgePoint(generatedTerrain, request, to, 1.0),
                         "left", index) ||
        !appendEdgePiece(result.plan, request,
                         roadEdgePoint(generatedTerrain, request, from, -1.0),
                         roadEdgePoint(generatedTerrain, request, to, -1.0),
                         "right", index)) {
      result.plan = {};
      reject(receipt, CreativeRoadRecipeStatus::StructureCapacityExceeded,
             "creative_road_recipe_edge_capacity_exceeded");
      return result;
    }
  }
  receipt.edgePieceCount = result.plan.objects.size();
  if (result.plan.objects.empty()) {
    receipt.accepted = true;
    receipt.status = CreativeRoadRecipeStatus::Ready;
    receipt.reasonCode = "creative_road_recipe_ready_without_edge_pieces";
    return result;
  }
  result.plan.definitionFingerprint = fingerprintCreativeRecipePlan(result.plan);
  if (result.plan.definitionFingerprint == 0U) {
    result.plan = {};
    reject(receipt, CreativeRoadRecipeStatus::InvalidStructure,
           "creative_road_recipe_fingerprint_invalid");
    return result;
  }
  const CreativeRecipeMaterializeResult materialized =
      materializeCreativeRecipe(result.plan, 1U);
  if (!materialized.receipt.accepted) {
    result.plan = {};
    reject(receipt, CreativeRoadRecipeStatus::InvalidStructure,
           materialized.receipt.reasonCode);
    return result;
  }
  receipt.definitionFingerprint = result.plan.definitionFingerprint;
  receipt.accepted = true;
  receipt.status = CreativeRoadRecipeStatus::Ready;
  receipt.reasonCode = "creative_road_recipe_ready";
  return result;
}

CreativeRoadRecipeResult buildCreativeRoadRecipe(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainMaterialField& materialSource,
    const CreativeRoadRecipeRequest& request,
    CreativeTerrainPathSourceCache* cache) {
  CreativeRoadRecipeResult result;
  result.receipt.requested = true;
  if (request.version != kCreativeRoadRecipeVersion) {
    reject(result.receipt, CreativeRoadRecipeStatus::UnsupportedVersion,
           "creative_road_recipe_version_unsupported");
    return result;
  }
  if (!validRequest(request)) {
    reject(result.receipt, CreativeRoadRecipeStatus::InvalidRequest,
           "creative_road_recipe_request_invalid");
    return result;
  }
  result.terrain = buildCreativeTerrainPathSourceRecipe(
      existingAuthored, canonicalSource, materialSource, request.source, cache);
  if (!result.terrain.receipt.accepted) {
    reject(result.receipt, CreativeRoadRecipeStatus::PathRejected,
           result.terrain.receipt.reasonCode);
    return result;
  }
  CreativeRoadStructureResult structure =
      planCreativeRoadStructure(result.terrain.heightField, request);
  if (!structure.receipt.accepted) {
    result.terrain = {};
    result.receipt = structure.receipt;
    return result;
  }
  result.structure = std::move(structure.plan);
  result.receipt = structure.receipt;
  result.receipt.terrainCellCount = result.terrain.receipt.outputCellCount;
  result.receipt.terrainHeightHash = result.terrain.receipt.outputHeightHash;
  return result;
}

}  // namespace iggy3d::creative
