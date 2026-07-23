#include "app/iggy3d/creative/document/TerrainContours.hpp"

#include <cmath>
#include <iostream>
#include <limits>
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

bool analysisBuildsLabelsSlopeBandsAndCutFill() {
  const cr::CreativeTerrainSurfacePlan candidate = surface({
      {{0, 0}, 1U},
      {{1, 0}, 3U},
      {{0, 1}, 1U},
      {{1, 1}, 3U},
  });
  const cr::CreativeTerrainSurfacePlan reference = surface({
      {{0, 0}, 2U},
      {{1, 0}, 2U},
      {{0, 1}, 1U},
      {{1, 1}, 4U},
  });
  cr::CreativeTerrainAnalysisRequest request;
  request.contours = {1U, 2U, 32U};
  const cr::CreativeTerrainAnalysisPlan first =
      cr::buildCreativeTerrainAnalysisPlan(candidate, request, &reference);
  const cr::CreativeTerrainAnalysisPlan second =
      cr::buildCreativeTerrainAnalysisPlan(candidate, request, &reference);

  return expect(first.accepted &&
                    first.status == cr::CreativeTerrainAnalysisPlanStatus::Ready &&
                    first.hasReference && first.cells.size() == 4U,
                "analysis retains the bounded candidate/reference union") &&
         expect(first.contours.accepted && first.labels.size() == 1U &&
                    first.labels.front().levelCells == 2U &&
                    near(first.labels.front().point.x, 0.75) &&
                    near(first.labels.front().point.z, 1.0),
                "one deterministic label anchors the longest index contour") &&
         expect(first.cutCellCount == 2U && first.fillCellCount == 1U &&
                    first.cells[0].deltaCells == -1 &&
                    first.cells[1].deltaCells == 1 &&
                    first.cells[2].deltaCells == 0 &&
                    first.cells[3].deltaCells == -1,
                "signed cut and fill deltas compare exact cell truth") &&
         expect(first.cells[0].slopeBand ==
                        cr::CreativeTerrainSlopeBand::Extreme &&
                    first.maximumSlopeDegrees > 63.4 &&
                    first.maximumSlopeDegrees < 63.5,
                "slope bands derive from deterministic local gradients") &&
         expect(first.cells == second.cells && first.labels == second.labels,
                "analysis output is deterministic");
}

bool analysisHitTestingPrefersContoursThenHeightHandles() {
  cr::CreativeTerrainAnalysisRequest request;
  request.contours = {1U, 2U, 32U};
  const cr::CreativeTerrainAnalysisPlan analysis =
      cr::buildCreativeTerrainAnalysisPlan(surface({
          {{0, 0}, 1U},
          {{1, 0}, 3U},
          {{0, 1}, 1U},
          {{1, 1}, 3U},
      }), request);
  const cr::CreativeTerrainAnalysisHit contour =
      cr::hitCreativeTerrainAnalysis(
          analysis, {0.79, 0.8}, 0.05,
          cr::CreativeTerrainAnalysisHitMode::ContourThenHeightHandle);
  const cr::CreativeTerrainAnalysisHit handle =
      cr::hitCreativeTerrainAnalysis(
          analysis, {0.51, 0.51}, 0.01,
          cr::CreativeTerrainAnalysisHitMode::ContourThenHeightHandle);
  const cr::CreativeTerrainAnalysisHit forcedHandle =
      cr::hitCreativeTerrainAnalysis(
          analysis, {0.79, 0.8}, 0.05,
          cr::CreativeTerrainAnalysisHitMode::HeightHandleOnly);
  const cr::CreativeTerrainAnalysisHit contourOnlyMiss =
      cr::hitCreativeTerrainAnalysis(
          analysis, {0.51, 0.51}, 0.01,
          cr::CreativeTerrainAnalysisHitMode::ContourOnly);
  const cr::CreativeTerrainAnalysisHit invalid =
      cr::hitCreativeTerrainAnalysis(
          analysis, {std::numeric_limits<double>::quiet_NaN(), 0.0}, 0.1,
          cr::CreativeTerrainAnalysisHitMode::ContourThenHeightHandle);

  return expect(contour.accepted &&
                    contour.kind == cr::CreativeTerrainAnalysisHitKind::Contour &&
                    contour.targetHeightCells == 2U &&
                    contour.distanceCells < 0.05,
                "contour proximity wins over the containing terrain cell") &&
         expect(handle.accepted &&
                    handle.kind ==
                        cr::CreativeTerrainAnalysisHitKind::HeightHandle &&
                    handle.coord == cr::CreativeTerrainCoord2{0, 0} &&
                    handle.targetHeightCells == 1U,
                "cell fallback exposes the exact authored height handle") &&
         expect(forcedHandle.accepted &&
                    forcedHandle.kind ==
                        cr::CreativeTerrainAnalysisHitKind::HeightHandle &&
                    forcedHandle.targetHeightCells == 1U,
                "explicit height mode bypasses a nearby contour") &&
         expect(!contourOnlyMiss.accepted && !invalid.accepted,
                "disabled fallback and invalid coordinates fail closed");
}

bool analysisCapacityAndReferenceFailuresAreAtomic() {
  const cr::CreativeTerrainSurfacePlan candidate = surface({
      {{0, 0}, 1U},
      {{1, 0}, 3U},
      {{0, 1}, 1U},
      {{1, 1}, 3U},
  });
  cr::CreativeTerrainAnalysisRequest smallCells;
  smallCells.maxCellCount = 3U;
  const cr::CreativeTerrainAnalysisPlan cellOverflow =
      cr::buildCreativeTerrainAnalysisPlan(candidate, smallCells);
  cr::CreativeTerrainAnalysisRequest badThresholds;
  badThresholds.flatMaximumDegrees = 20.0;
  badThresholds.gentleMaximumDegrees = 10.0;
  const cr::CreativeTerrainAnalysisPlan invalidThresholds =
      cr::buildCreativeTerrainAnalysisPlan(candidate, badThresholds);
  cr::CreativeTerrainSurfacePlan invalidReference = surface({
      {{1, 0}, 2U},
      {{0, 0}, 2U},
  });
  const cr::CreativeTerrainAnalysisPlan badReference =
      cr::buildCreativeTerrainAnalysisPlan(candidate, {}, &invalidReference);

  return expect(!cellOverflow.accepted && cellOverflow.cells.empty() &&
                    cellOverflow.labels.empty() &&
                    cellOverflow.status ==
                        cr::CreativeTerrainAnalysisPlanStatus::CapacityExceeded,
                "analysis cell overflow rejects all output atomically") &&
         expect(!invalidThresholds.accepted &&
                    invalidThresholds.status ==
                        cr::CreativeTerrainAnalysisPlanStatus::InvalidRequest,
                "unordered slope thresholds are rejected") &&
         expect(!badReference.accepted && badReference.cells.empty() &&
                    badReference.status ==
                        cr::CreativeTerrainAnalysisPlanStatus::InvalidReference,
                "noncanonical comparison terrain cannot fabricate deltas");
}

}  // namespace

int main() {
  bool ok = true;
  ok = slopeProducesCanonicalMinorAndMajorLines() && ok;
  ok = holesAndFlatTerrainDoNotFabricateContours() && ok;
  ok = saddlesResolveDeterministicallyAndCapacityIsAtomic() && ok;
  ok = invalidSurfaceAndRequestsFailClosed() && ok;
  ok = analysisBuildsLabelsSlopeBandsAndCutFill() && ok;
  ok = analysisHitTestingPrefersContoursThenHeightHandles() && ok;
  ok = analysisCapacityAndReferenceFailuresAreAtomic() && ok;
  return ok ? 0 : 1;
}
