#include "app/iggy3d/creative/recipes/RoadRecipe.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeTerrainHeightField flatHeightField(
    cr::CreativeTerrainHeightFieldBounds bounds,
    std::uint16_t heightCells) {
  cr::CreativeTerrainHeightField field;
  const std::vector<std::uint16_t> heights(
      static_cast<std::size_t>(bounds.widthCells) * bounds.depthCells,
      heightCells);
  static_cast<void>(field.replace(bounds, heights));
  return field;
}

cr::CreativeRoadRecipeRequest defaultRequest() {
  cr::CreativeRoadRecipeRequest request;
  request.instanceKey = "road.main";
  request.name = "Main Road";
  request.grid = {{0.0, 0.0, 0.0}, 1.0, {32, 16, 32}};
  request.source.kind = cr::CreativeTerrainPathKind::Road;
  request.source.elevation = cr::CreativeTerrainPathElevation::Grade;
  request.source.curve = cr::CreativeTerrainPathCurvePolicy::Linear;
  request.source.crossSection = cr::CreativeTerrainPathCrossSection::Flat;
  request.source.falloffCells = 2U;
  request.source.material = cr::CreativeTerrainMaterial::Dirt;
  request.source.road.shoulderWidthCells = 1U;
  request.source.road.maximumGradePermille = 500U;
  request.source.road.edgeTreatment =
      cr::CreativeTerrainRoadEdgeTreatment::Curb;
  request.source.road.edgeWidthMeters = 0.2;
  request.source.road.edgeHeightMeters = 0.25;
  request.source.road.edgeMaterial = cr::CreativeStructuralMaterial::Stone;
  request.source.nextPointId = 3U;
  request.source.points = {
      {1U, {2, 5}, 4U, 1U, 0U, 0},
      {2U, {12, 5}, 6U, 1U, 0U, 0},
  };
  request.tags = {"world_layout:path:road.main"};
  return request;
}

bool hasTag(const cr::CreativeDocumentCreateRequest& request,
            std::string_view tag) {
  return std::find(request.tags.begin(), request.tags.end(), tag) !=
         request.tags.end();
}

bool validBounds(cr::CreativeBounds bounds) {
  return cr::isFiniteCreativeVec3(bounds.min) &&
         cr::isFiniteCreativeVec3(bounds.max) &&
         bounds.min.x < bounds.max.x && bounds.min.y < bounds.max.y &&
         bounds.min.z < bounds.max.z;
}

bool roadOwnsTerrainAndDeterministicCurbMembers() {
  const cr::CreativeTerrainHeightField base =
      flatHeightField({{0, 0}, 20U, 16U}, 4U);
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(base);
  const cr::CreativeTerrainMaterialField materials;
  const cr::CreativeRoadRecipeRequest request = defaultRequest();
  const cr::CreativeRoadRecipeResult first = cr::buildCreativeRoadRecipe(
      base, surface, materials, request);
  const cr::CreativeRoadRecipeResult repeated = cr::buildCreativeRoadRecipe(
      base, surface, materials, request);
  const cr::CreativeRecipeMaterializeResult materialized =
      cr::materializeCreativeRecipe(first.structure, 1U);

  std::unordered_set<std::string> stableKeys;
  bool geometryValid = first.structure.objects.size() == 20U;
  for (const cr::CreativeRecipeObjectPlan& object : first.structure.objects) {
    const cr::CreativeDocumentCreateRequest& create = object.createRequest;
    geometryValid = geometryValid &&
                    object.role == cr::CreativeRecipeObjectRole::Generated &&
                    stableKeys.insert(object.stableKey).second &&
                    create.kind == cr::CreativeObjectKind::Beam &&
                    create.hasBoundsOverride && create.hasTransformOverride &&
                    create.hasVisibleOverride && create.visible &&
                    validBounds(create.bounds) &&
                    cr::isFiniteCreativeVec3(create.transform.position) &&
                    cr::isFiniteCreativeVec3(
                        create.transform.rotationEulerRadians) &&
                    hasTag(create, "creative_road:curb") &&
                    hasTag(create, "creative_structural_material:stone") &&
                    hasTag(create, "world_layout:path:road.main");
  }

  const bool provenanceValid =
      materialized.receipt.accepted &&
      materialized.createRequests.size() == first.structure.objects.size() &&
      cr::creativeRecipeRequestHasInstanceProvenance(
          materialized.createRequests.front(), cr::CreativeRecipeKind::Road,
          request.instanceKey, cr::CreativeRecipeObjectRole::Generated,
          "curb.left.1") &&
      cr::creativeRecipeRequestHasInstanceProvenance(
          materialized.createRequests.back(), cr::CreativeRecipeKind::Road,
          request.instanceKey, cr::CreativeRecipeObjectRole::Generated,
          "curb.right.10");

  return expect(first.receipt.accepted && first.terrain.receipt.accepted &&
                    first.structure.kind == cr::CreativeRecipeKind::Road &&
                    first.receipt.centerlineSampleCount == 11U &&
                    first.receipt.edgePieceCount == 20U &&
                    first.receipt.terrainCellCount > 0U,
                "one road owner emits terrain and both curb sides") &&
         expect(geometryValid && stableKeys.size() == 20U,
                "curb members are finite unique beams with material provenance") &&
         expect(provenanceValid,
                "materialization adds exact Road instance provenance") &&
         expect(repeated.receipt.accepted &&
                    repeated.receipt.definitionFingerprint ==
                        first.receipt.definitionFingerprint &&
                    repeated.receipt.terrainHeightHash ==
                        first.receipt.terrainHeightHash &&
                    repeated.structure.objects.size() ==
                        first.structure.objects.size(),
                "terrain and structural road output are deterministic");
}

bool joinsSuppressOnlyEdgeMembersNearSharedSeams() {
  const cr::CreativeTerrainHeightField terrain =
      flatHeightField({{0, 0}, 20U, 16U}, 4U);
  cr::CreativeRoadRecipeRequest request = defaultRequest();
  request.source.points[1].heightCells = 4U;
  request.source.startJoin =
      cr::CreativeTerrainPathEndpointJoin::Intersection;
  request.source.endJoin = cr::CreativeTerrainPathEndpointJoin::Bridge;
  const cr::CreativeRoadStructureResult joined =
      cr::planCreativeRoadStructure(terrain, request);

  cr::CreativeRoadRecipeRequest shortJoin = request;
  shortJoin.source.points[1].coord = {4, 5};
  shortJoin.source.endJoin =
      cr::CreativeTerrainPathEndpointJoin::Intersection;
  const cr::CreativeRoadStructureResult fullySuppressed =
      cr::planCreativeRoadStructure(terrain, shortJoin);

  return expect(joined.receipt.accepted &&
                    joined.receipt.suppressedJoinPieceCount > 0U &&
                    joined.receipt.edgePieceCount > 0U &&
                    joined.receipt.suppressedJoinPieceCount +
                            joined.receipt.edgePieceCount ==
                        2U * (joined.receipt.centerlineSampleCount - 1U) &&
                    joined.plan.objects.size() ==
                        joined.receipt.edgePieceCount,
                "intersection and bridge joins leave deterministic curb gaps") &&
         expect(fullySuppressed.receipt.accepted &&
                    fullySuppressed.receipt.edgePieceCount == 0U &&
                    fullySuppressed.plan.objects.empty() &&
                    fullySuppressed.receipt.reasonCode ==
                        "creative_road_recipe_ready_without_edge_pieces",
                "a short shared seam may intentionally suppress every curb member");
}

bool invalidRoadRequestsFailClosed() {
  const cr::CreativeTerrainHeightField terrain =
      flatHeightField({{0, 0}, 20U, 16U}, 4U);
  cr::CreativeRoadRecipeRequest invalidKind = defaultRequest();
  invalidKind.source.kind = cr::CreativeTerrainPathKind::River;
  const cr::CreativeRoadStructureResult kindRejected =
      cr::planCreativeRoadStructure(terrain, invalidKind);

  cr::CreativeRoadRecipeRequest invalidMaterial = defaultRequest();
  invalidMaterial.source.road.edgeMaterial =
      cr::CreativeStructuralMaterial::Count;
  const cr::CreativeRoadStructureResult materialRejected =
      cr::planCreativeRoadStructure(terrain, invalidMaterial);

  cr::CreativeRoadRecipeRequest unsupported = defaultRequest();
  unsupported.version = 99U;
  const cr::CreativeRoadStructureResult versionRejected =
      cr::planCreativeRoadStructure(terrain, unsupported);

  cr::CreativeRoadRecipeRequest overCapacity = defaultRequest();
  overCapacity.source.road.shoulderWidthCells = 0U;
  overCapacity.source.falloffCells = 0U;
  overCapacity.source.nextPointId = 7U;
  overCapacity.source.points = {
      {1U, {0, 0}, 4U, 1U, 0U, 0},
      {2U, {255, 0}, 4U, 1U, 0U, 0},
      {3U, {510, 0}, 4U, 1U, 0U, 0},
      {4U, {765, 0}, 4U, 1U, 0U, 0},
      {5U, {1020, 0}, 4U, 1U, 0U, 0},
      {6U, {1275, 0}, 4U, 1U, 0U, 0},
  };
  const cr::CreativeRoadStructureResult capacityRejected =
      cr::planCreativeRoadStructure(terrain, overCapacity);

  return expect(!kindRejected.receipt.accepted &&
                    kindRejected.plan.objects.empty() &&
                    kindRejected.receipt.status ==
                        cr::CreativeRoadRecipeStatus::InvalidRequest,
                "non-road sources reject without structural output") &&
         expect(!materialRejected.receipt.accepted &&
                    materialRejected.plan.objects.empty() &&
                    materialRejected.receipt.status ==
                        cr::CreativeRoadRecipeStatus::InvalidRequest,
                "invalid curb material rejects without partial members") &&
         expect(!versionRejected.receipt.accepted &&
                    versionRejected.receipt.status ==
                        cr::CreativeRoadRecipeStatus::UnsupportedVersion,
                "unsupported Road recipe versions fail closed") &&
         expect(!capacityRejected.receipt.accepted &&
                    capacityRejected.plan.objects.empty() &&
                    capacityRejected.receipt.status ==
                        cr::CreativeRoadRecipeStatus::
                            StructureCapacityExceeded,
                "curb capacity rejects atomically before partial output");
}

}  // namespace

int main() {
  const bool ok = roadOwnsTerrainAndDeterministicCurbMembers() &&
                  joinsSuppressOnlyEdgeMembersNearSharedSeams() &&
                  invalidRoadRequestsFailClosed();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "creative_road_recipe_tests: PASS\n";
  return EXIT_SUCCESS;
}
