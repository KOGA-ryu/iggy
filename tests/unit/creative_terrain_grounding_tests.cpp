#include "app/iggy3d/creative/recipes/TerrainGrounding.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeTerrainSurfacePlan surface(
    std::initializer_list<cr::CreativeTerrainColumn> columns) {
  cr::CreativeTerrainSurfacePlan plan;
  plan.requested = true;
  plan.accepted = true;
  plan.status = columns.size() == 0U
                    ? cr::CreativeTerrainSurfacePlanStatus::Empty
                    : cr::CreativeTerrainSurfacePlanStatus::Ready;
  plan.columns.assign(columns);
  return plan;
}

bool flatAndSlopedFootprintsResolveDeterministically() {
  const cr::CreativeTerrainSurfacePlan flat = surface(
      {{{0, 0}, 3U}, {{1, 0}, 3U}, {{0, 1}, 3U}, {{1, 1}, 3U}});
  cr::CreativeTerrainGroundingRequest request{&flat, {0, 0}, {2, 2}, 0.5,
                                               2U};
  const cr::CreativeTerrainGroundingPlan first =
      cr::planCreativeTerrainGrounding(request);
  const cr::CreativeTerrainGroundingPlan repeated =
      cr::planCreativeTerrainGrounding(request);

  const cr::CreativeTerrainSurfacePlan slope = surface(
      {{{0, 0}, 2U}, {{1, 0}, 3U}, {{0, 1}, 3U}, {{1, 1}, 4U}});
  request.terrain = &slope;
  request.authoredGroundLayer = 1.0;
  const cr::CreativeTerrainGroundingPlan sloped =
      cr::planCreativeTerrainGrounding(request);

  return expect(first.accepted && first.sampleCount == 4U &&
                    first.minimumHeightCells == 3U &&
                    first.maximumHeightCells == 3U &&
                    first.reliefCells == 0U &&
                    first.targetGroundLayer == 3.0 &&
                    first.verticalOffsetLayers == 2.5,
                "flat footprint resolves highest terrain plane") &&
         expect(repeated.accepted &&
                    repeated.sampleCount == first.sampleCount &&
                    repeated.verticalOffsetLayers == first.verticalOffsetLayers,
                "grounding result is deterministic") &&
         expect(sloped.accepted && sloped.minimumHeightCells == 2U &&
                    sloped.maximumHeightCells == 4U &&
                    sloped.reliefCells == 2U &&
                    sloped.verticalOffsetLayers == 3.0,
                "bounded relief resolves to highest terrain plane");
}

bool failuresAreExplicitAndFailClosed() {
  const cr::CreativeTerrainSurfacePlan hole =
      surface({{{0, 0}, 2U}, {{1, 0}, 2U}, {{0, 1}, 2U}});
  const cr::CreativeTerrainGroundingPlan missing =
      cr::planCreativeTerrainGrounding({&hole, {0, 0}, {2, 2}, 0.0, 2U});

  const cr::CreativeTerrainSurfacePlan cliff = surface(
      {{{0, 0}, 1U}, {{1, 0}, 5U}, {{0, 1}, 1U}, {{1, 1}, 5U}});
  const cr::CreativeTerrainGroundingPlan relief =
      cr::planCreativeTerrainGrounding({&cliff, {0, 0}, {2, 2}, 0.0, 3U});

  cr::CreativeTerrainSurfacePlan invalid = surface(
      {{{1, 0}, 2U}, {{0, 0}, 2U}});
  const cr::CreativeTerrainGroundingPlan invalidTerrain =
      cr::planCreativeTerrainGrounding({&invalid, {0, 0}, {1, 1}, 0.0, 0U});
  cr::CreativeTerrainSurfacePlan contradictory = surface({{{0, 0}, 2U}});
  contradictory.status = cr::CreativeTerrainSurfacePlanStatus::Empty;
  const cr::CreativeTerrainGroundingPlan contradictoryTerrain =
      cr::planCreativeTerrainGrounding(
          {&contradictory, {0, 0}, {1, 1}, 0.0, 0U});
  const cr::CreativeTerrainGroundingPlan invalidRequest =
      cr::planCreativeTerrainGrounding(
          {&cliff, {0, 0}, {0, 2}, std::nan(""), 3U});
  const cr::CreativeTerrainGroundingPlan oversized =
      cr::planCreativeTerrainGrounding({
          &cliff, {0, 0},
          {static_cast<std::int32_t>(cr::kCreativeTerrainHeightFieldCellCapacity),
           2},
          0.0, 3U});

  return expect(!missing.accepted &&
                    missing.status ==
                        cr::CreativeTerrainGroundingStatus::MissingSurface,
                "missing terrain cell rejects grounding") &&
         expect(!relief.accepted && relief.sampleCount == 4U &&
                    relief.reliefCells == 4U &&
                    relief.status ==
                        cr::CreativeTerrainGroundingStatus::ReliefExceeded,
                "excess relief rejects grounding") &&
         expect(!invalidTerrain.accepted &&
                    invalidTerrain.status ==
                        cr::CreativeTerrainGroundingStatus::InvalidTerrain,
                "noncanonical terrain rejects grounding") &&
         expect(!contradictoryTerrain.accepted &&
                    contradictoryTerrain.status ==
                        cr::CreativeTerrainGroundingStatus::InvalidTerrain,
                "terrain status and columns must agree") &&
         expect(!invalidRequest.accepted &&
                    invalidRequest.status ==
                        cr::CreativeTerrainGroundingStatus::InvalidRequest,
                "invalid footprint and layer reject grounding") &&
         expect(!oversized.accepted &&
                    oversized.status ==
                        cr::CreativeTerrainGroundingStatus::FootprintTooLarge,
                "oversized footprint rejects before sampling");
}

bool emptyTerrainUsesTheAuthoredGridDatum() {
  const cr::CreativeTerrainSurfacePlan empty = surface({});
  const cr::CreativeTerrainGroundingPlan grounded =
      cr::planCreativeTerrainGrounding({&empty, {-2, -1}, {3, 2}, -0.05, 0U});
  return expect(grounded.accepted && grounded.sampleCount == 15U &&
                    grounded.minimumHeightCells == 0U &&
                    grounded.maximumHeightCells == 0U &&
                    grounded.targetGroundLayer == 0.0 &&
                    grounded.verticalOffsetLayers == 0.05,
                "empty terrain retains the explicit grid datum");
}

}  // namespace

int main() {
  const bool ok = flatAndSlopedFootprintsResolveDeterministically() &&
                  failuresAreExplicitAndFailClosed() &&
                  emptyTerrainUsesTheAuthoredGridDatum();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
