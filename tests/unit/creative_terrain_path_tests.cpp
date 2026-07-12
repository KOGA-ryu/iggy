#include "app/iggy3d/creative/tools/TerrainPath.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const cr::CreativeTerrainControlPoint* controlAt(
    const cr::CreativeTerrainPathPlan& plan,
    cr::CreativeTerrainCoord2 coord) {
  const auto found = std::find_if(
      plan.finalControls().begin(), plan.finalControls().end(),
      [coord](const auto& control) { return control.coord == coord; });
  return found == plan.finalControls().end() ? nullptr : &*found;
}

cr::CreativeTerrainPathRequest requestFor(
    const cr::CreativeTerrainField& field,
    std::span<const cr::CreativeTerrainPathPoint> points) {
  cr::CreativeTerrainPathRequest request;
  request.field = &field;
  request.points = points;
  request.elevation = cr::CreativeTerrainPathElevation::Level;
  request.halfWidthCells = 1U;
  request.amplitudeCells = 2U;
  return request;
}

bool optionValuesAndLabelsAreClosed() {
  return expect(cr::creativeTerrainPathHalfWidthCells(
                    cr::CreativeTerrainPathWidth::SevenCells) == 3U &&
                    cr::creativeTerrainPathWidthCells(
                        cr::CreativeTerrainPathWidth::FiveCells) == 5U &&
                    cr::creativeTerrainPathAmplitudeCells(
                        cr::CreativeTerrainPathAmplitude::EightCells) == 8U,
                "path option enums resolve to bounded scalar values") &&
         expect(cr::toString(cr::CreativeTerrainPathKind::Road) == "ROAD" &&
                    cr::toString(cr::CreativeTerrainPathKind::Trench) ==
                        "TRENCH" &&
                    cr::toString(cr::CreativeTerrainPathElevation::Grade) ==
                        "GRADE" &&
                    cr::toString(cr::CreativeTerrainPathWidth::ThreeCells) ==
                        "3 CELLS" &&
                    cr::toString(cr::CreativeTerrainPathKind::Count) ==
                        "INVALID",
                "path enums expose explicit creator labels") &&
         expect(cr::creativeTerrainPathUsesDepth(
                    cr::CreativeTerrainPathKind::River) &&
                    cr::creativeTerrainPathUsesDepth(
                        cr::CreativeTerrainPathKind::Trench) &&
                    !cr::creativeTerrainPathUsesDepth(
                        cr::CreativeTerrainPathKind::Road),
                "depth-bearing path kinds are explicit");
}

bool roadRasterIsCanonicalAndIdempotent() {
  cr::CreativeTerrainField field;
  constexpr std::array points{
      cr::CreativeTerrainPathPoint{{0, 0}, 10U},
      cr::CreativeTerrainPathPoint{{4, 0}, 20U},
  };
  cr::CreativeTerrainPathRequest request = requestFor(field, points);
  const cr::CreativeTerrainPathPlan first =
      cr::buildCreativeTerrainPathPlan(request);
  bool canonical = true;
  for (std::size_t index = 1U; index < first.finalControls().size(); ++index) {
    const auto before = first.finalControls()[index - 1U].coord;
    const auto after = first.finalControls()[index].coord;
    canonical = canonical &&
                (before.z < after.z ||
                 (before.z == after.z && before.x < after.x));
  }
  const cr::CreativeTerrainMutationReceipt applied = field.apply(first.items());
  request.field = &field;
  const cr::CreativeTerrainPathPlan repeated =
      cr::buildCreativeTerrainPathPlan(request);
  const cr::CreativeTerrainControlPoint* center = controlAt(first, {2, 0});
  const cr::CreativeTerrainControlPoint* edge = controlAt(first, {2, 1});
  return expect(first.accepted &&
                    first.status == cr::CreativeTerrainPathPlanStatus::Ready &&
                    first.centerlineCount == 5U && first.controlCount == 17U &&
                    first.editCount == 17U && canonical,
                "straight road raster is bounded deduplicated and canonical") &&
         expect(center != nullptr && edge != nullptr &&
                    center->heightCells == 12U && edge->heightCells == 12U &&
                    center->radiusCells == 2U,
                "road emits a flat width-three deck at level plus rise") &&
         expect(applied.changed && field.controlCount() == 17U &&
                    repeated.accepted && repeated.items().empty() &&
                    repeated.status ==
                        cr::CreativeTerrainPathPlanStatus::NoChange,
                "one terrain batch applies and exact replay is idempotent");
}

bool crossSectionsAndElevationModesArePinned() {
  cr::CreativeTerrainField field;
  constexpr std::array flatPoints{
      cr::CreativeTerrainPathPoint{{0, 0}, 20U},
      cr::CreativeTerrainPathPoint{{2, 0}, 20U},
  };
  cr::CreativeTerrainPathRequest request = requestFor(field, flatPoints);
  request.halfWidthCells = 2U;
  request.amplitudeCells = 4U;

  request.kind = cr::CreativeTerrainPathKind::River;
  const cr::CreativeTerrainPathPlan river =
      cr::buildCreativeTerrainPathPlan(request);
  request.kind = cr::CreativeTerrainPathKind::Ridge;
  const cr::CreativeTerrainPathPlan ridge =
      cr::buildCreativeTerrainPathPlan(request);
  request.kind = cr::CreativeTerrainPathKind::Trench;
  const cr::CreativeTerrainPathPlan trench =
      cr::buildCreativeTerrainPathPlan(request);

  constexpr std::array gradedPoints{
      cr::CreativeTerrainPathPoint{{0, 0}, 10U},
      cr::CreativeTerrainPathPoint{{4, 0}, 20U},
  };
  cr::CreativeTerrainPathRequest grade = requestFor(field, gradedPoints);
  grade.kind = cr::CreativeTerrainPathKind::Ridge;
  grade.elevation = cr::CreativeTerrainPathElevation::Grade;
  grade.halfWidthCells = 0U;
  grade.amplitudeCells = 1U;
  const cr::CreativeTerrainPathPlan graded =
      cr::buildCreativeTerrainPathPlan(grade);

  return expect(controlAt(river, {1, 0})->heightCells == 16U &&
                    controlAt(river, {1, 2})->heightCells == 20U,
                "river is a smooth bowl returning to base at its bank") &&
         expect(controlAt(ridge, {1, 0})->heightCells == 24U &&
                    controlAt(ridge, {1, 2})->heightCells == 20U,
                "ridge is the positive smooth cross-section") &&
         expect(controlAt(trench, {1, 0})->heightCells == 16U &&
                    controlAt(trench, {1, 2})->heightCells == 16U,
                "trench keeps a flat full-depth cut across its width") &&
         expect(controlAt(graded, {0, 0})->heightCells == 11U &&
                    controlAt(graded, {2, 0})->heightCells == 16U &&
                    controlAt(graded, {4, 0})->heightCells == 21U,
                "grade interpolates endpoint elevations by path progress");
}

bool bendsDeduplicateAndFollowSamplesLiveTerrain() {
  cr::CreativeTerrainField field;
  const std::array existing{
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{0, 0}, 8U, 1U}},
      cr::CreativeTerrainControlEdit{cr::CreativeTerrainEditKind::Upsert,
                                     {{2, 2}, 14U, 1U}},
  };
  static_cast<void>(field.apply(existing));
  constexpr std::array points{
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
      cr::CreativeTerrainPathPoint{{2, 0}, 4U},
      cr::CreativeTerrainPathPoint{{2, 2}, 4U},
  };
  cr::CreativeTerrainPathRequest request = requestFor(field, points);
  request.kind = cr::CreativeTerrainPathKind::Road;
  request.elevation = cr::CreativeTerrainPathElevation::Follow;
  request.halfWidthCells = 0U;
  request.amplitudeCells = 1U;
  const cr::CreativeTerrainPathPlan plan =
      cr::buildCreativeTerrainPathPlan(request);
  return expect(plan.accepted && plan.centerlineCount == 5U &&
                    plan.controlCount == 5U,
                "joined segments own their bend cell exactly once") &&
         expect(controlAt(plan, {0, 0})->heightCells == 9U &&
                    controlAt(plan, {2, 2})->heightCells == 15U,
                "follow samples current terrain along the centerline");
}

bool rejectedPlansAreAtomicAndSpecific() {
  cr::CreativeTerrainField empty;
  constexpr std::array duplicate{
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
  };
  const auto invalid =
      cr::buildCreativeTerrainPathPlan(requestFor(empty, duplicate));

  constexpr std::array longSegment{
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
      cr::CreativeTerrainPathPoint{{256, 0}, 4U},
  };
  const auto tooLong =
      cr::buildCreativeTerrainPathPlan(requestFor(empty, longSegment));

  constexpr std::array wideSegment{
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
      cr::CreativeTerrainPathPoint{{255, 0}, 4U},
  };
  cr::CreativeTerrainPathRequest capacityRequest =
      requestFor(empty, wideSegment);
  capacityRequest.halfWidthCells = 3U;
  const auto capacity =
      cr::buildCreativeTerrainPathPlan(capacityRequest);

  constexpr std::array overflowPoints{
      cr::CreativeTerrainPathPoint{
          {std::numeric_limits<std::int32_t>::max(), 0}, 4U},
      cr::CreativeTerrainPathPoint{
          {std::numeric_limits<std::int32_t>::max() - 1, 0}, 4U},
  };
  cr::CreativeTerrainPathRequest overflowRequest =
      requestFor(empty, overflowPoints);
  overflowRequest.halfWidthCells = 1U;
  const auto overflow =
      cr::buildCreativeTerrainPathPlan(overflowRequest);

  std::vector<cr::CreativeTerrainControlEdit> fullEdits;
  fullEdits.reserve(cr::kCreativeTerrainControlCapacity);
  for (std::size_t index = 0U; index < cr::kCreativeTerrainControlCapacity;
       ++index) {
    fullEdits.push_back(
        {cr::CreativeTerrainEditKind::Upsert,
         {{1000 + static_cast<std::int32_t>(index), 1000}, 4U, 1U}});
  }
  cr::CreativeTerrainField full;
  static_cast<void>(full.apply(fullEdits));
  constexpr std::array shortPath{
      cr::CreativeTerrainPathPoint{{0, 0}, 4U},
      cr::CreativeTerrainPathPoint{{1, 0}, 4U},
  };
  const auto fieldCapacity =
      cr::buildCreativeTerrainPathPlan(requestFor(full, shortPath));

  return expect(!invalid.accepted && invalid.items().empty() &&
                    invalid.status ==
                        cr::CreativeTerrainPathPlanStatus::InvalidRequest,
                "duplicate adjacent points reject atomically") &&
         expect(!tooLong.accepted && tooLong.items().empty() &&
                    tooLong.status ==
                        cr::CreativeTerrainPathPlanStatus::PathTooLong,
                "unbounded segments reject before traversal") &&
         expect(!capacity.accepted && capacity.items().empty() &&
                    capacity.status ==
                        cr::CreativeTerrainPathPlanStatus::CapacityExceeded,
                "wide long corridors reject instead of partially stamping") &&
         expect(!overflow.accepted && overflow.items().empty() &&
                    overflow.status ==
                        cr::CreativeTerrainPathPlanStatus::CoordinateOverflow,
                "corridor offset overflow rejects with zero edits") &&
         expect(!fieldCapacity.accepted && fieldCapacity.items().empty() &&
                    fieldCapacity.status ==
                        cr::CreativeTerrainPathPlanStatus::CapacityExceeded,
                "full authored field rejects a path before partial edits");
}

}  // namespace

int main() {
  bool ok = true;
  ok = optionValuesAndLabelsAreClosed() && ok;
  ok = roadRasterIsCanonicalAndIdempotent() && ok;
  ok = crossSectionsAndElevationModesArePinned() && ok;
  ok = bendsDeduplicateAndFollowSamplesLiveTerrain() && ok;
  ok = rejectedPlansAreAtomicAndSpecific() && ok;
  return ok ? 0 : 1;
}
