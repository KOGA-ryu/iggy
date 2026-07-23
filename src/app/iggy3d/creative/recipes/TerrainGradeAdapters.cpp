#include "app/iggy3d/creative/recipes/TerrainGradeAdapters.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace iggy3d::creative {
namespace {

void reject(CreativeTerrainGradeAdapterPlan& plan,
            CreativeTerrainGradeAdapterStatus status,
            std::string_view reasonCode) noexcept {
  plan.status = status;
  plan.reasonCode = reasonCode;
}

[[nodiscard]] bool validRect(CreativeTerrainGradeRect rect) noexcept {
  return rect.minimum.x < rect.maximum.x &&
         rect.minimum.z < rect.maximum.z;
}

[[nodiscard]] bool inside(CreativeTerrainGradeRect rect,
                          CreativeTerrainCoord2 coord) noexcept {
  return coord.x >= rect.minimum.x && coord.x < rect.maximum.x &&
         coord.z >= rect.minimum.z && coord.z < rect.maximum.z;
}

[[nodiscard]] bool onPerimeter(CreativeTerrainGradeRect rect,
                               CreativeTerrainCoord2 coord) noexcept {
  return inside(rect, coord) &&
         (coord.x == rect.minimum.x || coord.x == rect.maximum.x - 1 ||
          coord.z == rect.minimum.z || coord.z == rect.maximum.z - 1);
}

[[nodiscard]] bool checkedOffset(std::int32_t value,
                                 std::int64_t offset,
                                 std::int32_t& output) noexcept {
  const std::int64_t result = static_cast<std::int64_t>(value) + offset;
  if (result < std::numeric_limits<std::int32_t>::min() ||
      result > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = static_cast<std::int32_t>(result);
  return true;
}

[[nodiscard]] bool checkedCenter(std::int32_t minimum,
                                 std::int32_t maximum,
                                 std::int32_t& output) noexcept {
  const std::int64_t first = minimum;
  const std::int64_t last = static_cast<std::int64_t>(maximum) - 1;
  const std::int64_t center = first + ((last - first) / 2);
  if (center < std::numeric_limits<std::int32_t>::min() ||
      center > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = static_cast<std::int32_t>(center);
  return true;
}

[[nodiscard]] CreativeTerrainGradeRecipe pathRecipe(
    const CreativeTerrainGradePathSegmentRequest& request) noexcept {
  CreativeTerrainGradeRecipe recipe;
  recipe.start = request.start;
  recipe.end = request.end;
  recipe.startHeightCells = request.startHeightCells;
  recipe.endHeightCells = request.endHeightCells;
  recipe.halfWidthCells = request.halfWidthCells;
  recipe.crossSlopePermille = request.crossSlopePermille;
  recipe.falloffCells = request.falloffCells;
  return recipe;
}

[[nodiscard]] CreativeTerrainGradeAdapterPlan oneRecipePlan(
    CreativeTerrainGradeAdapterKind kind,
    const CreativeTerrainGradePathSegmentRequest& request) noexcept {
  CreativeTerrainGradeAdapterPlan plan;
  plan.requested = true;
  plan.kind = kind;
  plan.recipes[0] = pathRecipe(request);
  if (!isValidCreativeTerrainGradeRecipe(plan.recipes[0])) {
    reject(plan, CreativeTerrainGradeAdapterStatus::RecipeRejected,
           "creative_terrain_grade_adapter_recipe_rejected");
    return plan;
  }
  plan.accepted = true;
  plan.status = CreativeTerrainGradeAdapterStatus::Ready;
  plan.recipeCount = 1U;
  plan.reasonCode = "creative_terrain_grade_adapter_ready";
  return plan;
}

}  // namespace

std::string_view toString(
    CreativeTerrainGradeAdapterStatus status) noexcept {
  switch (status) {
    case CreativeTerrainGradeAdapterStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainGradeAdapterStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTerrainGradeAdapterStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeTerrainGradeAdapterStatus::RecipeRejected:
      return "RecipeRejected";
    case CreativeTerrainGradeAdapterStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeTerrainGradeAdapterPlan planCreativeTerrainGradePathSegment(
    const CreativeTerrainGradePathSegmentRequest& request) noexcept {
  return oneRecipePlan(CreativeTerrainGradeAdapterKind::PathSegment, request);
}

CreativeTerrainGradeAdapterPlan planCreativeTerrainGradeBuildingPadApproach(
    const CreativeTerrainGradeBuildingPadApproachRequest& request) noexcept {
  CreativeTerrainGradeAdapterPlan plan;
  plan.requested = true;
  plan.kind = CreativeTerrainGradeAdapterKind::BuildingPadApproach;
  if (!validRect(request.padFootprint) ||
      inside(request.padFootprint, request.terrainEndpoint) ||
      !onPerimeter(request.padFootprint, request.padEndpoint)) {
    reject(plan, CreativeTerrainGradeAdapterStatus::InvalidRequest,
           "creative_terrain_grade_pad_approach_invalid");
    return plan;
  }
  CreativeTerrainGradePathSegmentRequest segment;
  segment.start = request.terrainEndpoint;
  segment.end = request.padEndpoint;
  segment.startHeightCells = request.terrainHeightCells;
  segment.endHeightCells = request.padHeightCells;
  segment.halfWidthCells = request.halfWidthCells;
  segment.crossSlopePermille = request.crossSlopePermille;
  segment.falloffCells = request.falloffCells;
  return oneRecipePlan(CreativeTerrainGradeAdapterKind::BuildingPadApproach,
                       segment);
}

CreativeTerrainGradeAdapterPlan planCreativeTerrainGradeBridgeApproaches(
    const CreativeTerrainGradeBridgeApproachRequest& request) noexcept {
  CreativeTerrainGradeAdapterPlan plan;
  plan.requested = true;
  plan.kind = CreativeTerrainGradeAdapterKind::BridgeApproaches;
  if (!validRect(request.bridgeFootprint) ||
      request.approachLengthCells == 0U) {
    reject(plan, CreativeTerrainGradeAdapterStatus::InvalidRequest,
           "creative_terrain_grade_bridge_approach_invalid");
    return plan;
  }
  const std::int64_t width =
      static_cast<std::int64_t>(request.bridgeFootprint.maximum.x) -
      request.bridgeFootprint.minimum.x;
  const std::int64_t depth =
      static_cast<std::int64_t>(request.bridgeFootprint.maximum.z) -
      request.bridgeFootprint.minimum.z;
  const std::int64_t minor = std::min(width, depth);
  if (minor <= 0 ||
      minor / 2 > kCreativeTerrainGradeMaximumHalfWidthCells) {
    reject(plan, CreativeTerrainGradeAdapterStatus::InvalidRequest,
           "creative_terrain_grade_bridge_width_invalid");
    return plan;
  }

  CreativeTerrainCoord2 firstDeck;
  CreativeTerrainCoord2 secondDeck;
  CreativeTerrainCoord2 firstTerrain;
  CreativeTerrainCoord2 secondTerrain;
  if (width >= depth) {
    std::int32_t centerZ = 0;
    if (!checkedCenter(request.bridgeFootprint.minimum.z,
                       request.bridgeFootprint.maximum.z, centerZ) ||
        !checkedOffset(request.bridgeFootprint.minimum.x,
                       -static_cast<std::int64_t>(request.approachLengthCells),
                       firstTerrain.x) ||
        !checkedOffset(request.bridgeFootprint.maximum.x - 1,
                       request.approachLengthCells, secondTerrain.x)) {
      reject(plan, CreativeTerrainGradeAdapterStatus::CoordinateOverflow,
             "creative_terrain_grade_bridge_coordinate_overflow");
      return plan;
    }
    firstDeck = {request.bridgeFootprint.minimum.x, centerZ};
    secondDeck = {request.bridgeFootprint.maximum.x - 1, centerZ};
    firstTerrain.z = centerZ;
    secondTerrain.z = centerZ;
  } else {
    std::int32_t centerX = 0;
    if (!checkedCenter(request.bridgeFootprint.minimum.x,
                       request.bridgeFootprint.maximum.x, centerX) ||
        !checkedOffset(request.bridgeFootprint.minimum.z,
                       -static_cast<std::int64_t>(request.approachLengthCells),
                       firstTerrain.z) ||
        !checkedOffset(request.bridgeFootprint.maximum.z - 1,
                       request.approachLengthCells, secondTerrain.z)) {
      reject(plan, CreativeTerrainGradeAdapterStatus::CoordinateOverflow,
             "creative_terrain_grade_bridge_coordinate_overflow");
      return plan;
    }
    firstDeck = {centerX, request.bridgeFootprint.minimum.z};
    secondDeck = {centerX, request.bridgeFootprint.maximum.z - 1};
    firstTerrain.x = centerX;
    secondTerrain.x = centerX;
  }

  const std::uint16_t halfWidth = static_cast<std::uint16_t>(minor / 2);
  CreativeTerrainGradePathSegmentRequest first;
  first.start = firstTerrain;
  first.end = firstDeck;
  first.startHeightCells = request.firstTerrainHeightCells;
  first.endHeightCells = request.deckHeightCells;
  first.halfWidthCells = halfWidth;
  first.crossSlopePermille = request.crossSlopePermille;
  first.falloffCells = request.falloffCells;
  CreativeTerrainGradePathSegmentRequest second;
  second.start = secondDeck;
  second.end = secondTerrain;
  second.startHeightCells = request.deckHeightCells;
  second.endHeightCells = request.secondTerrainHeightCells;
  second.halfWidthCells = halfWidth;
  second.crossSlopePermille = request.crossSlopePermille;
  second.falloffCells = request.falloffCells;
  plan.recipes[0] = pathRecipe(first);
  plan.recipes[1] = pathRecipe(second);
  if (!isValidCreativeTerrainGradeRecipe(plan.recipes[0]) ||
      !isValidCreativeTerrainGradeRecipe(plan.recipes[1])) {
    reject(plan, CreativeTerrainGradeAdapterStatus::RecipeRejected,
           "creative_terrain_grade_adapter_recipe_rejected");
    return plan;
  }
  plan.accepted = true;
  plan.status = CreativeTerrainGradeAdapterStatus::Ready;
  plan.recipeCount = 2U;
  plan.reasonCode = "creative_terrain_grade_adapter_ready";
  return plan;
}

}  // namespace iggy3d::creative
