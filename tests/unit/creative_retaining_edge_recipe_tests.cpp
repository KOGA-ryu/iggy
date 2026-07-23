#include "app/iggy3d/creative/recipes/RetainingEdgeRecipe.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
#include <string_view>
#include <vector>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool nearly(double lhs, double rhs) {
  return std::fabs(lhs - rhs) <= 1.0e-9;
}

bool hasTag(const cr::CreativeDocumentCreateRequest& request,
            std::string_view tag) {
  return std::find(request.tags.begin(), request.tags.end(), tag) !=
         request.tags.end();
}

bool hasStableKey(const cr::CreativeRecipePlan& plan,
                  std::string_view stableKey) {
  return std::any_of(plan.objects.begin(), plan.objects.end(),
                     [&](const cr::CreativeRecipeObjectPlan& object) {
                       return object.stableKey == stableKey;
                     });
}

cr::CreativeTerrainHeightField lShapeTerrain() {
  cr::CreativeTerrainHeightField terrain;
  const std::vector<std::uint16_t> heights{
      2U, 5U, 5U,
      2U, 5U, 5U,
      2U, 2U, 2U,
  };
  static_cast<void>(terrain.replace({{0, 0}, 3U, 3U}, heights));
  return terrain;
}

std::vector<cr::CreativeTerrainHardEdge> lShapeEdges() {
  return {
      cr::canonicalCreativeTerrainHardEdge({0, 0}, {1, 0}),
      cr::canonicalCreativeTerrainHardEdge({0, 1}, {1, 1}),
      cr::canonicalCreativeTerrainHardEdge({1, 1}, {1, 2}),
      cr::canonicalCreativeTerrainHardEdge({2, 1}, {2, 2}),
  };
}

cr::CreativeRetainingEdgeRecipeRequest defaultRequest(
    const cr::CreativeTerrainHeightField& terrain,
    const std::vector<cr::CreativeTerrainHardEdge>& edges) {
  cr::CreativeRetainingEdgeRecipeRequest request;
  request.instanceKey = "retaining.terrace.1";
  request.name = "Terrace Retaining Edge";
  request.grid = {{10.0, 1.0, -10.0}, 1.0, {32, 32, 32}};
  request.profileBounds = {{0, 0}, 3U, 3U};
  request.source.terrainProfileKey = "terrace.1";
  request.terrain = &terrain;
  request.hardEdges = edges;
  request.tags = {"world_layout:terrain_profile:terrace.1"};
  return request;
}

bool proceduralKitFollowsCanonicalSeams() {
  const cr::CreativeTerrainHeightField terrain = lShapeTerrain();
  const std::vector<cr::CreativeTerrainHardEdge> edges = lShapeEdges();
  const cr::CreativeRetainingEdgeRecipeRequest request =
      defaultRequest(terrain, edges);
  const cr::CreativeRetainingEdgeRecipeResult first =
      cr::planCreativeRetainingEdge(request);
  const cr::CreativeRetainingEdgeRecipeResult repeated =
      cr::planCreativeRetainingEdge(request);
  const cr::CreativeRecipeMaterializeResult materialized =
      first.receipt.accepted
          ? cr::materializeCreativeRecipe(first.structure, 1U)
          : cr::CreativeRecipeMaterializeResult{};

  const cr::CreativeDocumentCreateRequest& firstWall =
      first.structure.objects.front().createRequest;
  const cr::CreativeBoundsMetrics firstWallBounds =
      cr::measureCreativeBounds(firstWall.bounds);
  bool generatedFacts = first.structure.objects.size() == 7U;
  bool stableKeysMatch =
      repeated.structure.objects.size() == first.structure.objects.size();
  for (std::size_t index = 0U;
       stableKeysMatch && index < first.structure.objects.size(); ++index) {
    stableKeysMatch = repeated.structure.objects[index].stableKey ==
                      first.structure.objects[index].stableKey;
  }
  for (const cr::CreativeRecipeObjectPlan& object : first.structure.objects) {
    const cr::CreativeDocumentCreateRequest& create = object.createRequest;
    generatedFacts =
        generatedFacts &&
        object.role == cr::CreativeRecipeObjectRole::Generated &&
        create.hasBoundsOverride && create.hasTransformOverride &&
        create.hasVisibleOverride && create.visible && create.assetId.empty() &&
        hasTag(create, "creative_retaining_edge:generated") &&
        hasTag(create, "creative_structural_material:stone") &&
        hasTag(create, "world_layout:terrain_profile:terrace.1");
  }

  return expect(first.receipt.accepted &&
                    first.receipt.status ==
                        cr::CreativeRetainingEdgeRecipeStatus::Ready &&
                    first.structure.kind ==
                        cr::CreativeRecipeKind::RetainingEdge &&
                    first.receipt.selectedEdgeCount == 4U &&
                    first.receipt.wallSegmentCount == 4U &&
                    first.receipt.cornerCount == 1U &&
                    first.receipt.capCount == 2U &&
                    first.receipt.generatedObjectCount == 7U,
                "L seam emits four walls one corner and two end caps") &&
         expect(firstWall.kind == cr::CreativeObjectKind::Wall &&
                    nearly(firstWall.transform.position.x, 10.5) &&
                    nearly(firstWall.transform.position.y, 4.5) &&
                    nearly(firstWall.transform.position.z, -10.0) &&
                    nearly(firstWall.transform.rotationEulerRadians.y,
                           -std::numbers::pi * 0.5) &&
                    firstWallBounds.valid &&
                    nearly(firstWallBounds.size.x, 1.35) &&
                    nearly(firstWallBounds.size.y, 3.0) &&
                    nearly(firstWallBounds.size.z, 0.35),
                "wall uses exact half-grid seam and low/high terrain levels") &&
         expect(generatedFacts,
                "procedural output owns finite generated material-tagged boxes") &&
         expect(materialized.receipt.accepted &&
                    materialized.createRequests.size() == 7U &&
                    cr::creativeRecipeRequestHasInstanceProvenance(
                        materialized.createRequests.front(),
                        cr::CreativeRecipeKind::RetainingEdge,
                        request.instanceKey,
                        cr::CreativeRecipeObjectRole::Generated,
                        "segment.0.0.1.0"),
                "materialization adds retaining-edge instance provenance") &&
         expect(first.receipt.definitionFingerprint != 0U &&
                    repeated.receipt.accepted &&
                    repeated.receipt.definitionFingerprint ==
                        first.receipt.definitionFingerprint &&
                    stableKeysMatch,
                "retaining-edge regeneration is deterministic");
}

bool selectionAndInfrastructureKitAreExplicit() {
  const cr::CreativeTerrainHeightField terrain = lShapeTerrain();
  const std::vector<cr::CreativeTerrainHardEdge> edges = lShapeEdges();

  cr::CreativeRetainingEdgeRecipeRequest internal =
      defaultRequest(terrain, edges);
  internal.profileBounds = {{0, 0}, 2U, 2U};
  internal.source.settings.selection =
      cr::CreativeRetainingEdgeSelection::Internal;
  internal.source.settings.capEnds = false;
  internal.source.settings.closeCorners = false;
  const cr::CreativeRetainingEdgeRecipeResult internalResult =
      cr::planCreativeRetainingEdge(internal);

  cr::CreativeRetainingEdgeRecipeRequest perimeter = internal;
  perimeter.profileBounds = {{0, 0}, 1U, 2U};
  perimeter.source.settings.selection =
      cr::CreativeRetainingEdgeSelection::Perimeter;
  const cr::CreativeRetainingEdgeRecipeResult perimeterResult =
      cr::planCreativeRetainingEdge(perimeter);

  cr::CreativeRetainingEdgeRecipeRequest stone =
      defaultRequest(terrain, edges);
  stone.source.settings.kit =
      cr::CreativeRetainingEdgeKit::InfrastructureStone;
  const cr::CreativeRetainingEdgeRecipeResult stoneResult =
      cr::planCreativeRetainingEdge(stone);
  bool wallAssets = stoneResult.receipt.accepted;
  bool proceduralJoints = stoneResult.receipt.accepted;
  for (const cr::CreativeRecipeObjectPlan& object :
       stoneResult.structure.objects) {
    if (object.createRequest.kind == cr::CreativeObjectKind::Wall) {
      wallAssets =
          wallAssets && object.createRequest.assetId ==
                            "infrastructure/retaining_wall_2x1p2";
    } else {
      proceduralJoints = proceduralJoints && object.createRequest.assetId.empty();
    }
  }

  return expect(internalResult.receipt.accepted &&
                    internalResult.receipt.selectedEdgeCount == 2U &&
                    internalResult.receipt.wallSegmentCount == 2U,
                "Internal selects seams whose two cells belong to the profile") &&
         expect(perimeterResult.receipt.accepted &&
                    perimeterResult.receipt.selectedEdgeCount == 2U &&
                    perimeterResult.receipt.wallSegmentCount == 2U,
                "Perimeter selects seams with one profile cell") &&
         expect(stoneResult.receipt.accepted && wallAssets &&
                    proceduralJoints,
                "stone kit swaps straight assets but keeps exact-size joints");
}

bool transitionsReplaceOneWallAndRespectMovementGeometry() {
  const cr::CreativeTerrainHeightField terrain = lShapeTerrain();
  const std::vector<cr::CreativeTerrainHardEdge> edges = lShapeEdges();

  cr::CreativeRetainingEdgeRecipeRequest stair =
      defaultRequest(terrain, edges);
  stair.source.settings.transitionCount = 1U;
  stair.source.settings.transitions[0] = {
      edges.front(), cr::CreativeRetainingEdgeTransitionKind::Stair, 3U};
  stair.source.settings.kit =
      cr::CreativeRetainingEdgeKit::InfrastructureStone;
  const cr::CreativeRetainingEdgeRecipeResult stairResult =
      cr::planCreativeRetainingEdge(stair);
  const auto stairObject = std::find_if(
      stairResult.structure.objects.begin(), stairResult.structure.objects.end(),
      [](const cr::CreativeRecipeObjectPlan& object) {
        return object.createRequest.kind == cr::CreativeObjectKind::Stair;
      });

  cr::CreativeRetainingEdgeRecipeRequest ramp =
      defaultRequest(terrain, edges);
  ramp.source.settings.transitionCount = 1U;
  ramp.source.settings.transitions[0] = {
      edges.front(), cr::CreativeRetainingEdgeTransitionKind::Ramp, 6U};
  const cr::CreativeRetainingEdgeRecipeResult rampResult =
      cr::planCreativeRetainingEdge(ramp);

  return expect(stairResult.receipt.accepted &&
                    stairResult.receipt.wallSegmentCount == 3U &&
                    stairResult.receipt.stairCount == 1U &&
                    stairResult.receipt.rampCount == 0U &&
                    stairObject != stairResult.structure.objects.end() &&
                    stairObject->createRequest.assetId ==
                        "infrastructure/terrain_steps_2x3x1p2" &&
                    !hasStableKey(stairResult.structure,
                                  "segment.0.0.1.0") &&
                    !hasStableKey(stairResult.structure, "cap.1.-1"),
                "stair replaces its wall and omitted seam creates no closure") &&
         expect(rampResult.receipt.accepted &&
                    rampResult.receipt.wallSegmentCount == 3U &&
                    rampResult.receipt.rampCount == 1U &&
                    std::any_of(
                        rampResult.structure.objects.begin(),
                        rampResult.structure.objects.end(),
                        [](const cr::CreativeRecipeObjectPlan& object) {
                          return object.createRequest.kind ==
                                 cr::CreativeObjectKind::Ramp;
                        }),
                "walkable ramp replaces its exact hard-edge segment");
}

bool invalidAndUnboundedSourcesFailClosed() {
  const cr::CreativeTerrainHeightField terrain = lShapeTerrain();
  const std::vector<cr::CreativeTerrainHardEdge> edges = lShapeEdges();

  cr::CreativeRetainingEdgeRecipeRequest noMatch =
      defaultRequest(terrain, edges);
  noMatch.profileBounds = {{20, 20}, 2U, 2U};
  const cr::CreativeRetainingEdgeRecipeResult noMatchResult =
      cr::planCreativeRetainingEdge(noMatch);

  cr::CreativeRetainingEdgeRecipeRequest unmatched =
      defaultRequest(terrain, edges);
  unmatched.source.settings.transitionCount = 1U;
  unmatched.source.settings.transitions[0].edge =
      cr::canonicalCreativeTerrainHardEdge({7, 7}, {8, 7});
  const cr::CreativeRetainingEdgeRecipeResult unmatchedResult =
      cr::planCreativeRetainingEdge(unmatched);

  cr::CreativeRetainingEdgeRecipeRequest duplicate =
      defaultRequest(terrain, edges);
  duplicate.source.settings.transitionCount = 2U;
  duplicate.source.settings.transitions[0].edge = edges.front();
  duplicate.source.settings.transitions[1].edge = edges.front();
  const cr::CreativeRetainingEdgeRecipeResult duplicateResult =
      cr::planCreativeRetainingEdge(duplicate);

  cr::CreativeRetainingEdgeRecipeRequest tooHigh =
      defaultRequest(terrain, edges);
  tooHigh.source.settings.maximumHeightMeters = 2.9;
  const cr::CreativeRetainingEdgeRecipeResult tooHighResult =
      cr::planCreativeRetainingEdge(tooHigh);

  cr::CreativeRetainingEdgeRecipeRequest shortStair =
      defaultRequest(terrain, edges);
  shortStair.source.settings.transitionCount = 1U;
  shortStair.source.settings.transitions[0] = {
      edges.front(), cr::CreativeRetainingEdgeTransitionKind::Stair, 1U};
  const cr::CreativeRetainingEdgeRecipeResult shortStairResult =
      cr::planCreativeRetainingEdge(shortStair);

  cr::CreativeRetainingEdgeRecipeRequest shortRamp =
      defaultRequest(terrain, edges);
  shortRamp.source.settings.transitionCount = 1U;
  shortRamp.source.settings.transitions[0] = {
      edges.front(), cr::CreativeRetainingEdgeTransitionKind::Ramp, 1U};
  const cr::CreativeRetainingEdgeRecipeResult shortRampResult =
      cr::planCreativeRetainingEdge(shortRamp);

  cr::CreativeRetainingEdgeRecipeRequest nonFinite =
      defaultRequest(terrain, edges);
  nonFinite.source.settings.thicknessMeters =
      std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeRetainingEdgeRecipeResult nonFiniteResult =
      cr::planCreativeRetainingEdge(nonFinite);

  cr::CreativeTerrainHeightField capacityTerrain;
  std::vector<std::uint16_t> capacityHeights(4098U);
  for (std::size_t index = 0U; index < capacityHeights.size(); ++index) {
    capacityHeights[index] = index % 2U == 0U ? 2U : 3U;
  }
  static_cast<void>(capacityTerrain.replace({{0, 0}, 4098U, 1U},
                                            capacityHeights));
  std::vector<cr::CreativeTerrainHardEdge> capacityEdges;
  capacityEdges.reserve(4097U);
  for (std::int32_t x = 0; x < 4097; ++x) {
    capacityEdges.push_back(
        cr::canonicalCreativeTerrainHardEdge({x, 0}, {x + 1, 0}));
  }
  cr::CreativeRetainingEdgeRecipeRequest capacity =
      defaultRequest(capacityTerrain, capacityEdges);
  capacity.profileBounds = {{0, 0}, 4098U, 1U};
  capacity.source.settings.capEnds = false;
  capacity.source.settings.closeCorners = false;
  const cr::CreativeRetainingEdgeRecipeResult capacityResult =
      cr::planCreativeRetainingEdge(capacity);

  return expect(!noMatchResult.receipt.accepted &&
                    noMatchResult.receipt.status ==
                        cr::CreativeRetainingEdgeRecipeStatus::NoMatchingEdges,
                "empty selection rejects instead of fabricating structure") &&
         expect(!unmatchedResult.receipt.accepted &&
                    unmatchedResult.receipt.status ==
                        cr::CreativeRetainingEdgeRecipeStatus::TransitionInvalid,
                "transition must anchor to a selected canonical seam") &&
         expect(!duplicateResult.receipt.accepted &&
                    duplicateResult.receipt.status ==
                        cr::CreativeRetainingEdgeRecipeStatus::InvalidRequest,
                "duplicate transition anchors are invalid source state") &&
         expect(!tooHighResult.receipt.accepted &&
                    tooHighResult.receipt.status ==
                        cr::CreativeRetainingEdgeRecipeStatus::HeightExceeded,
                "maximum wall height is an enforced source limit") &&
         expect(!shortStairResult.receipt.accepted &&
                    shortStairResult.receipt.status ==
                        cr::CreativeRetainingEdgeRecipeStatus::TransitionInvalid &&
                    !shortRampResult.receipt.accepted &&
                    shortRampResult.receipt.status ==
                        cr::CreativeRetainingEdgeRecipeStatus::TransitionInvalid,
                "stairs and ramps reuse movement-safe geometry limits") &&
         expect(!nonFiniteResult.receipt.accepted &&
                    nonFiniteResult.receipt.status ==
                        cr::CreativeRetainingEdgeRecipeStatus::InvalidRequest,
                "non-finite settings fail before geometry") &&
         expect(!capacityResult.receipt.accepted &&
                    capacityResult.receipt.status ==
                        cr::CreativeRetainingEdgeRecipeStatus::
                            StructureCapacityExceeded &&
                    capacityResult.structure.objects.empty(),
                "generated structure capacity fails without partial output");
}

}  // namespace

int main() {
  const bool ok = proceduralKitFollowsCanonicalSeams() &&
                  selectionAndInfrastructureKitAreExplicit() &&
                  transitionsReplaceOneWallAndRespectMovementGeometry() &&
                  invalidAndUnboundedSourcesFailClosed();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "creative_retaining_edge_recipe_tests: PASS\n";
  return EXIT_SUCCESS;
}
