#include "app/iggy3d/creative/recipes/WatercourseRecipe.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
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

cr::CreativeWatercourseRecipeRequest defaultRequest() {
  cr::CreativeWatercourseRecipeRequest request;
  request.instanceKey = "watercourse.creek";
  request.name = "Creek";
  request.grid = {{0.0, 0.0, 0.0}, 1.0, {32, 32, 32}};
  request.source.kind = cr::CreativeTerrainPathKind::River;
  request.source.elevation = cr::CreativeTerrainPathElevation::Grade;
  request.source.curve = cr::CreativeTerrainPathCurvePolicy::Linear;
  request.source.crossSection = cr::CreativeTerrainPathCrossSection::Channel;
  request.source.falloffCells = 1U;
  request.source.paintSurface = true;
  request.source.material = cr::CreativeTerrainMaterial::Sand;
  request.source.watercourse.bankSlopeCells = 2U;
  request.source.watercourse.drainageDirection =
      cr::CreativeTerrainWatercourseDrainageDirection::StartToEnd;
  request.source.watercourse.surfacePolicy =
      cr::CreativeTerrainWaterSurfacePolicy::Reserved;
  request.source.watercourse.surfaceInsetCells = 1U;
  request.source.watercourse.nextCrossingId = 2U;
  request.source.watercourse.crossings = {{1U, 2U, 1U, 1U, 2U}};
  request.source.nextPointId = 4U;
  request.source.points = {
      {1U, {3, 8}, 10U, 1U, 3U, 0},
      {2U, {9, 8}, 9U, 1U, 3U, 0},
      {3U, {15, 8}, 8U, 1U, 3U, 0},
  };
  return request;
}

bool nearly(double lhs, double rhs) {
  return std::fabs(lhs - rhs) <= 1.0e-9;
}

bool watercourseOwnsCutSurfaceFlowAndCrossingFrames() {
  const cr::CreativeTerrainHeightField base =
      flatHeightField({{0, 0}, 24U, 20U}, 12U);
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(base);
  const cr::CreativeTerrainMaterialField materials;
  const cr::CreativeWatercourseRecipeRequest request = defaultRequest();
  const cr::CreativeWatercourseRecipeResult first =
      cr::buildCreativeWatercourseRecipe(base, surface, materials, request);
  const cr::CreativeWatercourseRecipeResult repeated =
      cr::buildCreativeWatercourseRecipe(base, surface, materials, request);

  const cr::CreativeWatercourseCrossingFrame* crossing =
      first.plan.crossings.empty() ? nullptr : &first.plan.crossings.front();
  const bool exactFrame =
      crossing != nullptr && crossing->id == 1U &&
      crossing->sourcePointId == 2U &&
      nearly(crossing->crossingAxis.x, 0.0) &&
      nearly(crossing->crossingAxis.z, 1.0) &&
      nearly(crossing->spanMeters, 8.0) &&
      nearly(crossing->leftBankMeters.z, 12.0) &&
      nearly(crossing->rightBankMeters.z, 4.0) &&
      nearly(crossing->leftApproachMeters.z, 14.0) &&
      nearly(crossing->rightApproachMeters.z, 2.0) &&
      nearly(crossing->centerMeters.y, 13.0) &&
      crossing->bridgeTransform.position.x == crossing->centerMeters.x &&
      crossing->bridgeTransform.position.y == crossing->centerMeters.y &&
      crossing->bridgeTransform.position.z == crossing->centerMeters.z;

  return expect(first.receipt.accepted && first.terrain.receipt.accepted &&
                    first.plan.kind == cr::CreativeRecipeKind::Watercourse &&
                    first.plan.pathKind == cr::CreativeTerrainPathKind::River &&
                    first.plan.drainageDirection ==
                        cr::CreativeTerrainWatercourseDrainageDirection::
                            StartToEnd &&
                    first.plan.surfacePolicy ==
                        cr::CreativeTerrainWaterSurfacePolicy::Reserved &&
                    first.receipt.centerlineSampleCount == 13U &&
                    first.receipt.reservedSurfaceSampleCount == 13U &&
                    first.receipt.crossingCount == 1U,
                "one watercourse owner emits cut surface flow and crossings") &&
         expect(first.terrain.heightField.heightAt({9, 8}) == 6U &&
                    first.terrain.heightField.heightAt({9, 9}) == 6U &&
                    first.terrain.heightField.heightAt({9, 10}) == 8U &&
                    first.terrain.heightField.heightAt({9, 11}) == 9U,
                "bed width bank slope and outer bank are explicit") &&
         expect(!first.plan.surfaceSamples.empty() &&
                    nearly(first.plan.surfaceSamples.front().flowProgress, 0.0) &&
                    nearly(first.plan.surfaceSamples.back().flowProgress, 1.0) &&
                    first.plan.surfaceSamples[6U].reservedSurfaceHeightCells ==
                        8.0 &&
                    first.plan.surfaceSamples[6U].bedHeightCells == 6.0,
                "reserved water elevation is separate from the terrain bed") &&
         expect(exactFrame,
                "stable point crossing derives both banks approaches and span") &&
         expect(repeated.receipt.accepted &&
                    repeated.receipt.definitionFingerprint ==
                        first.receipt.definitionFingerprint &&
                    repeated.receipt.terrainHeightHash ==
                        first.receipt.terrainHeightHash &&
                    repeated.plan.crossings.size() ==
                        first.plan.crossings.size(),
                "watercourse terrain flow and crossing output are deterministic");
}

bool drainageDirectionIsValidatedAndReversible() {
  const cr::CreativeTerrainHeightField base =
      flatHeightField({{0, 0}, 24U, 20U}, 12U);
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainHeightSurfacePlan(base);
  const cr::CreativeTerrainMaterialField materials;
  cr::CreativeWatercourseRecipeRequest uphill = defaultRequest();
  uphill.source.watercourse.drainageDirection =
      cr::CreativeTerrainWatercourseDrainageDirection::EndToStart;
  const cr::CreativeWatercourseRecipeResult rejected =
      cr::buildCreativeWatercourseRecipe(base, surface, materials, uphill);

  cr::CreativeWatercourseRecipeRequest reversed = uphill;
  reversed.source.points[0].heightCells = 8U;
  reversed.source.points[1].heightCells = 9U;
  reversed.source.points[2].heightCells = 10U;
  const cr::CreativeWatercourseRecipeResult accepted =
      cr::buildCreativeWatercourseRecipe(base, surface, materials, reversed);

  return expect(!rejected.receipt.accepted &&
                    rejected.receipt.status ==
                        cr::CreativeWatercourseRecipeStatus::InvalidRequest,
                "authored flow direction rejects an uphill water surface") &&
         expect(accepted.receipt.accepted &&
                    !accepted.plan.surfaceSamples.empty() &&
                    nearly(accepted.plan.surfaceSamples.front().flowProgress,
                           1.0) &&
                    nearly(accepted.plan.surfaceSamples.back().flowProgress,
                           0.0),
                "reverse drainage preserves geometry and reverses flow order") &&
         expect(cr::toString(
                    cr::CreativeTerrainWatercourseDrainageDirection::StartToEnd) ==
                        "START_TO_END" &&
                    cr::toString(
                        cr::CreativeTerrainWaterSurfacePolicy::Reserved) ==
                        "RESERVED",
                "watercourse semantics expose closed creator labels");
}

bool invalidSurfaceAndCrossingContractsFailClosed() {
  const cr::CreativeTerrainHeightField terrain =
      flatHeightField({{0, 0}, 24U, 20U}, 12U);
  cr::CreativeWatercourseRecipeRequest badSurface = defaultRequest();
  badSurface.source.watercourse.surfaceInsetCells = 3U;
  const cr::CreativeWatercoursePlanResult surfaceRejected =
      cr::planCreativeWatercourse(terrain, badSurface);

  cr::CreativeWatercourseRecipeRequest duplicate = defaultRequest();
  duplicate.source.watercourse.nextCrossingId = 3U;
  duplicate.source.watercourse.crossings.push_back({2U, 2U, 1U, 1U, 2U});
  const cr::CreativeWatercoursePlanResult crossingRejected =
      cr::planCreativeWatercourse(terrain, duplicate);

  cr::CreativeWatercourseRecipeRequest unsupported = defaultRequest();
  unsupported.version = 99U;
  const cr::CreativeWatercoursePlanResult versionRejected =
      cr::planCreativeWatercourse(terrain, unsupported);

  return expect(!surfaceRejected.receipt.accepted &&
                    surfaceRejected.plan.surfaceSamples.empty() &&
                    surfaceRejected.plan.crossings.empty(),
                "water surface must remain above the authored bed") &&
         expect(!crossingRejected.receipt.accepted &&
                    crossingRejected.plan.surfaceSamples.empty() &&
                    crossingRejected.plan.crossings.empty(),
                "duplicate crossing point ownership rejects atomically") &&
         expect(!versionRejected.receipt.accepted &&
                    versionRejected.receipt.status ==
                        cr::CreativeWatercourseRecipeStatus::UnsupportedVersion,
                "unsupported watercourse recipe versions fail closed");
}

}  // namespace

int main() {
  const bool ok = watercourseOwnsCutSurfaceFlowAndCrossingFrames() &&
                  drainageDirectionIsValidatedAndReversible() &&
                  invalidSurfaceAndCrossingContractsFailClosed();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "creative_watercourse_recipe_tests: PASS\n";
  return EXIT_SUCCESS;
}
