#include "app/iggy3d/creative/document/TerrainContours.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double actual, double expected, double epsilon = 1.0e-9) {
  return std::fabs(actual - expected) <= epsilon;
}

cr::CreativeTerrainSurfacePlan surface(
    std::initializer_list<cr::CreativeTerrainColumn> columns) {
  cr::CreativeTerrainSurfacePlan result;
  result.requested = true;
  result.accepted = true;
  result.status = columns.size() == 0U
                      ? cr::CreativeTerrainSurfacePlanStatus::Empty
                      : cr::CreativeTerrainSurfacePlanStatus::Ready;
  result.sourceRevision = 17U;
  result.columns.assign(columns.begin(), columns.end());
  return result;
}

bool slopeProducesCanonicalMinorAndMajorLines() {
  const cr::CreativeTerrainSurfacePlan input = surface({
      {{0, 0}, 1U},
      {{1, 0}, 3U},
      {{0, 1}, 1U},
      {{1, 1}, 3U},
  });
  const cr::CreativeTerrainContourPlan first =
      cr::buildCreativeTerrainContourPlan(input, {1U, 2U, 32U});
  const cr::CreativeTerrainContourPlan second =
      cr::buildCreativeTerrainContourPlan(input, {1U, 2U, 32U});

  return expect(first.accepted &&
                    first.status == cr::CreativeTerrainContourPlanStatus::Ready,
                "slope contour plan is ready") &&
         expect(first.sourceRevision == 17U &&
                    first.sourceColumnCount == 4U &&
                    first.evaluatedSquareCount == 1U &&
                    first.contourLevelCount == 2U,
                "slope contour facts are reported") &&
         expect(first.segments.size() == 2U &&
                    first.segments[0].levelCells == 2U &&
                    first.segments[0].major &&
                    first.segments[1].levelCells == 3U &&
                    !first.segments[1].major,
                "interval and major cadence classify lines") &&
         expect(near(first.segments[0].start.x, 0.75) &&
                    near(first.segments[0].start.z, 0.5) &&
                    near(first.segments[0].end.x, 0.75) &&
                    near(first.segments[0].end.z, 1.5) &&
                    near(first.segments[1].start.x, 1.25) &&
                    near(first.segments[1].end.x, 1.25),
                "contours interpolate between column centers") &&
         expect(first.segments == second.segments,
                "contour output is deterministic");
}

bool holesAndFlatTerrainDoNotFabricateContours() {
  const cr::CreativeTerrainContourPlan hole =
      cr::buildCreativeTerrainContourPlan(surface({
          {{0, 0}, 1U},
          {{1, 0}, 4U},
          {{0, 1}, 1U},
      }));
  const cr::CreativeTerrainContourPlan flat =
      cr::buildCreativeTerrainContourPlan(surface({
          {{0, 0}, 4U},
          {{1, 0}, 4U},
          {{0, 1}, 4U},
          {{1, 1}, 4U},
      }));

  return expect(hole.accepted && hole.segments.empty() &&
                    hole.evaluatedSquareCount == 0U &&
                    hole.status == cr::CreativeTerrainContourPlanStatus::Empty,
                "terrain holes are not bridged") &&
         expect(flat.accepted && flat.segments.empty() &&
                    flat.evaluatedSquareCount == 1U &&
                    flat.status == cr::CreativeTerrainContourPlanStatus::Empty,
                "flat terrain has no fabricated isolines");
}

bool saddlesResolveDeterministicallyAndCapacityIsAtomic() {
  const cr::CreativeTerrainSurfacePlan input = surface({
      {{0, 0}, 3U},
      {{1, 0}, 1U},
      {{0, 1}, 1U},
      {{1, 1}, 3U},
  });
  const cr::CreativeTerrainContourPlan saddle =
      cr::buildCreativeTerrainContourPlan(input, {1U, 5U, 16U});
  const cr::CreativeTerrainContourPlan overflow =
      cr::buildCreativeTerrainContourPlan(input, {1U, 5U, 3U});

  return expect(saddle.accepted && saddle.segments.size() == 4U &&
                    saddle.ambiguousCaseCount == 2U,
                "saddle levels resolve into two segments each") &&
         expect(!overflow.accepted && overflow.segments.empty() &&
                    overflow.status ==
                        cr::CreativeTerrainContourPlanStatus::CapacityExceeded,
                "contour overflow rejects atomically");
}

bool invalidSurfaceAndRequestsFailClosed() {
  cr::CreativeTerrainSurfacePlan invalidSurface = surface({
      {{1, 0}, 2U},
      {{0, 0}, 1U},
  });
  const cr::CreativeTerrainContourPlan badSurface =
      cr::buildCreativeTerrainContourPlan(invalidSurface);
  cr::CreativeTerrainSurfacePlan mismatchedEmpty = surface({});
  mismatchedEmpty.status = cr::CreativeTerrainSurfacePlanStatus::Ready;
  const cr::CreativeTerrainContourPlan badEmpty =
      cr::buildCreativeTerrainContourPlan(mismatchedEmpty);
  const cr::CreativeTerrainContourPlan badInterval =
      cr::buildCreativeTerrainContourPlan(surface({}), {0U, 5U, 16U});
  const cr::CreativeTerrainContourPlan badMajor =
      cr::buildCreativeTerrainContourPlan(surface({}), {1U, 0U, 16U});
  const cr::CreativeTerrainContourPlan badCapacity =
      cr::buildCreativeTerrainContourPlan(
          surface({}),
          {1U, 5U, cr::kCreativeTerrainContourSegmentCapacity + 1U});

  return expect(!badSurface.accepted && !badEmpty.accepted &&
                    badSurface.status ==
                        cr::CreativeTerrainContourPlanStatus::InvalidSurface &&
                    badEmpty.status ==
                        cr::CreativeTerrainContourPlanStatus::InvalidSurface,
                "noncanonical surface fails closed") &&
         expect(!badInterval.accepted && !badMajor.accepted &&
                    !badCapacity.accepted &&
                    badInterval.status ==
                        cr::CreativeTerrainContourPlanStatus::InvalidRequest &&
                    badMajor.status ==
                        cr::CreativeTerrainContourPlanStatus::InvalidRequest &&
                    badCapacity.status ==
                        cr::CreativeTerrainContourPlanStatus::InvalidRequest,
                "invalid contour settings fail closed");
}

}  // namespace

int main() {
  bool ok = true;
  ok = slopeProducesCanonicalMinorAndMajorLines() && ok;
  ok = holesAndFlatTerrainDoNotFabricateContours() && ok;
  ok = saddlesResolveDeterministicallyAndCapacityIsAtomic() && ok;
  ok = invalidSurfaceAndRequestsFailClosed() && ok;
  return ok ? 0 : 1;
}
