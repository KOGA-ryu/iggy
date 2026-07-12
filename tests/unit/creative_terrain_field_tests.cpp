#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/TerrainField.hpp"
#include "app/iggy3d/creative/tools/TerrainGrade.hpp"
#include "app/iggy3d/creative/tools/TerrainSculpt.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
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

bool samePoint(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  constexpr double epsilon = 1.0e-9;
  return std::fabs(lhs.x - rhs.x) <= epsilon &&
         std::fabs(lhs.y - rhs.y) <= epsilon &&
         std::fabs(lhs.z - rhs.z) <= epsilon;
}

cr::CreativeTerrainControlEdit upsert(std::int32_t x,
                                      std::int32_t z,
                                      std::uint16_t height,
                                      std::uint16_t radius) {
  return {cr::CreativeTerrainEditKind::Upsert, {{x, z}, height, radius}};
}

bool mutationsAreAtomicCanonicalAndBounded() {
  cr::CreativeTerrainField field;
  const std::array edits{upsert(4, 2, 6, 3), upsert(-3, 1, 2, 5)};
  const cr::CreativeTerrainMutationReceipt applied = field.apply(edits);
  const std::array duplicate{upsert(9, 9, 3, 2), upsert(9, 9, 4, 2)};
  const cr::CreativeTerrainMutationReceipt rejected = field.apply(duplicate);

  std::vector<cr::CreativeTerrainControlEdit> overflow;
  overflow.reserve(cr::kCreativeTerrainControlCapacity + 1U);
  for (std::size_t index = 0;
       index < cr::kCreativeTerrainControlCapacity + 1U; ++index) {
    overflow.push_back(upsert(static_cast<std::int32_t>(1000U + index), 0,
                              1, 1));
  }
  cr::CreativeTerrainField empty;
  const cr::CreativeTerrainMutationReceipt capacity = empty.apply(overflow);

  return expect(applied.accepted && applied.changed &&
                    applied.changedControlCount == 2U,
                "valid controls apply") &&
         expect(field.controls().size() == 2U &&
                    field.controls()[0].coord.x == -3 &&
                    field.controls()[1].coord.x == 4,
                "controls sort by z then x") &&
         expect(!rejected.accepted && !rejected.changed &&
                    rejected.status ==
                        cr::CreativeTerrainMutationStatus::DuplicateCoordinate &&
                    field.controlCount() == 2U,
                "duplicate batch rejects atomically") &&
         expect(!capacity.accepted && !capacity.changed &&
                    capacity.status ==
                        cr::CreativeTerrainMutationStatus::CapacityExceeded &&
                    empty.controlCount() == 0U,
                "capacity rejects atomically") &&
         expect(field.validateInvariants(), "field invariants remain valid");
}

bool singleRodCreatesAFlatCircularInfluencePatch() {
  cr::CreativeTerrainField field;
  const cr::CreativeTerrainControlEdit edit = upsert(0, 0, 6, 1);
  static_cast<void>(field.apply(std::span{&edit, 1U}));
  const cr::CreativeTerrainSurfacePlan plan =
      cr::buildCreativeTerrainSurfacePlan(field);

  return expect(plan.accepted &&
                    plan.status == cr::CreativeTerrainSurfacePlanStatus::Ready,
                "single control plans") &&
         expect(plan.contributionCount == 5U && plan.columns.size() == 5U,
                "radius one enumerates circular disk") &&
         expect(plan.cuboids.size() == 3U,
                "equal-height row cells merge") &&
         expect(plan.columns[0].heightCells == 6U &&
                    plan.columns[2].coord == cr::CreativeTerrainCoord2{0, 0} &&
                    plan.columns[2].heightCells == 6U,
                "single control owns exact height across influence") &&
         expect(plan.cuboids[1].minCell.x == -1 &&
                    plan.cuboids[1].maxCellExclusive.x == 2 &&
                    plan.cuboids[1].maxCellExclusive.y == 6 &&
                    plan.cuboids[1].material == cr::CreativeObjectKind::TerrainPatch,
                "middle row becomes one terrain cuboid");
}

bool overlappingRodsBlendWithDeterministicIntegerWeights() {
  cr::CreativeTerrainField field;
  const std::array edits{upsert(0, 0, 2, 2), upsert(2, 0, 6, 2)};
  static_cast<void>(field.apply(edits));
  const cr::CreativeTerrainSurfacePlan first =
      cr::buildCreativeTerrainSurfacePlan(field);
  const cr::CreativeTerrainSurfacePlan second =
      cr::buildCreativeTerrainSurfacePlan(field);
  const auto middle = std::find_if(
      first.columns.begin(), first.columns.end(),
      [](const cr::CreativeTerrainColumn& column) {
        return column.coord == cr::CreativeTerrainCoord2{1, 0};
      });

  return expect(middle != first.columns.end() && middle->heightCells == 4U,
                "symmetric overlap averages to four") &&
         expect(cr::sampleCreativeTerrainHeight(field, {1, 0}).present &&
                    cr::sampleCreativeTerrainHeight(field, {1, 0}).heightCells ==
                        middle->heightCells,
                "point sample uses the surface planner blend") &&
         expect(first.columns == second.columns &&
                    first.cuboids.size() == second.cuboids.size(),
                "rebuilding is deterministic");
}

bool raycastHitsTerrainTopsSidesAndFailsClosed() {
  cr::CreativeTerrainField field;
  const cr::CreativeTerrainControlEdit edit = upsert(0, 0, 6, 1);
  static_cast<void>(field.apply(std::span{&edit, 1U}));

  cr::CreativeTerrainRaycastRequest request;
  request.rayOrigin = {0.5, 10.0, 0.5};
  request.rayDirection = {0.0, -2.0, 0.0};
  request.gridOrigin = {};
  request.cellSize = 1.0;
  request.maxDistance = 20.0;
  const cr::CreativeTerrainRaycastReceipt top =
      cr::raycastCreativeTerrainField(field, request);

  request.rayOrigin = {-2.0, 3.0, 0.5};
  request.rayDirection = {5.0, 0.0, 0.0};
  const cr::CreativeTerrainRaycastReceipt side =
      cr::raycastCreativeTerrainField(field, request);

  request.rayOrigin = {8.5, 10.0, 8.5};
  request.rayDirection = {0.0, -1.0, 0.0};
  const cr::CreativeTerrainRaycastReceipt miss =
      cr::raycastCreativeTerrainField(field, request);

  request.rayOrigin = {-10.5, 3.0, 0.5};
  request.rayDirection = {1.0, 0.0, 0.0};
  request.maxVisitedCells = 2U;
  const cr::CreativeTerrainRaycastReceipt limited =
      cr::raycastCreativeTerrainField(field, request);

  request.rayDirection = {};
  const cr::CreativeTerrainRaycastReceipt invalid =
      cr::raycastCreativeTerrainField(field, request);

  return expect(top.accepted && top.hit && top.cell == cr::CreativeTerrainCoord2{} &&
                    top.heightCells == 6U && top.distance == 4.0 &&
                    top.hitPoint.y == 6.0 && top.faceNormal.y == 1.0,
                "vertical ray hits generated terrain top") &&
         expect(side.accepted && side.hit &&
                    side.cell == cr::CreativeTerrainCoord2{-1, 0} &&
                    side.distance == 1.0 && side.faceNormal.x == -1.0,
                "horizontal ray hits generated cliff side") &&
         expect(miss.accepted && !miss.hit &&
                    miss.status == cr::CreativeTerrainRaycastStatus::Miss,
                "ray outside all influence disks misses") &&
         expect(!limited.accepted && !limited.hit &&
                    limited.status ==
                        cr::CreativeTerrainRaycastStatus::TraversalLimitExceeded &&
                    limited.visitedCellCount == 2U,
                "bounded traversal fails closed") &&
         expect(!invalid.accepted && !invalid.hit &&
                    invalid.status ==
                        cr::CreativeTerrainRaycastStatus::InvalidRequest,
                "degenerate ray fails closed");
}

bool renderPlanBendsSharedCornersAndEnforcesBudget() {
  cr::CreativeTerrainField field;
  const std::array edits{upsert(0, 0, 2, 1), upsert(1, 0, 6, 1)};
  static_cast<void>(field.apply(edits));
  const cr::CreativeTerrainRenderPlan plan =
      cr::buildCreativeTerrainRenderPlan(field, {}, 1.0);
  const auto left = std::find_if(
      plan.patches.begin(), plan.patches.end(),
      [](const cr::CreativeTerrainSurfacePatch& patch) {
        return patch.coord == cr::CreativeTerrainCoord2{0, 0};
      });
  const auto right = std::find_if(
      plan.patches.begin(), plan.patches.end(),
      [](const cr::CreativeTerrainSurfacePatch& patch) {
        return patch.coord == cr::CreativeTerrainCoord2{1, 0};
      });
  const cr::CreativeTerrainRenderPlan limited =
      cr::buildCreativeTerrainRenderPlan(field, {}, 1.0, 1U);
  cr::CreativeTerrainField empty;
  const cr::CreativeTerrainRenderPlan emptyPlan =
      cr::buildCreativeTerrainRenderPlan(empty, {}, 1.0);

  return expect(plan.accepted &&
                    plan.status == cr::CreativeTerrainRenderPlanStatus::Ready &&
                    plan.patches.size() == plan.sourceColumnCount,
                "render plan emits one bounded patch per terrain column") &&
         expect(left != plan.patches.end() && right != plan.patches.end() &&
                    left->corners[0].y != left->corners[1].y,
                "neighbor height differences bend a tile") &&
         expect(left->center.y ==
                    cr::sampleCreativeTerrainHeight(field, left->coord)
                        .heightCells,
                "render patch center preserves the resolved column height") &&
         expect(samePoint(left->corners[1], right->corners[0]) &&
                    samePoint(left->corners[2], right->corners[3]),
                "neighbor patches share crack-free edge vertices") &&
         expect(!limited.accepted && limited.patches.empty() &&
                    limited.status ==
                        cr::CreativeTerrainRenderPlanStatus::CapacityExceeded,
                "render patch budget rejects before partial output") &&
         expect(emptyPlan.accepted && emptyPlan.patches.empty() &&
                    emptyPlan.status ==
                        cr::CreativeTerrainRenderPlanStatus::Empty,
                "empty terrain produces a valid empty render plan");
}

bool documentRevisionAdvancesOncePerTerrainBatch() {
  cr::CreativeDocument document = cr::CreativeDocument::create("terrain");
  static_cast<void>(document.assignId(19U));
  const std::array edits{upsert(1, 2, 4, 3), upsert(5, 2, 8, 4)};
  const cr::CreativeTerrainMutationReceipt receipt =
      document.applyTerrainControlEdits(edits);

  return expect(receipt.accepted && receipt.changed,
                "document terrain batch applies") &&
         expect(document.revision() == 1U,
                "document revision advances once") &&
         expect(document.objectCount() == 0U &&
                    document.voxelField().occupiedCellCount() == 0U &&
                    document.terrainField().controlCount() == 2U,
                "terrain truth is independent of objects and voxels") &&
         expect(document.dirtyFlags() != 0U, "terrain edit marks dirty");
}

bool sculptPlanIsSnapshotBasedBoundedAndCanonical() {
  constexpr std::array controls{
      cr::CreativeTerrainControlPoint{{-2, 0}, 1U, 2U},
      cr::CreativeTerrainControlPoint{{0, 0}, 4U, 3U},
      cr::CreativeTerrainControlPoint{{2, 0}, 9U, 4U},
      cr::CreativeTerrainControlPoint{{6, 0}, 5U, 5U},
  };
  const cr::CreativeTerrainSculptPlan flatten =
      cr::buildCreativeTerrainSculptPlan(
          {controls, {0, 0}, cr::CreativeTerrainSculptMode::Flatten, 2U, 2U,
           8U});
  const cr::CreativeTerrainSculptPlan smooth =
      cr::buildCreativeTerrainSculptPlan(
          {controls, {0, 0}, cr::CreativeTerrainSculptMode::Smooth, 2U, 2U,
           8U});
  const cr::CreativeTerrainSculptPlan raise =
      cr::buildCreativeTerrainSculptPlan(
          {controls, {0, 0}, cr::CreativeTerrainSculptMode::Raise, 2U, 4U,
           8U});
  const cr::CreativeTerrainSculptPlan lower =
      cr::buildCreativeTerrainSculptPlan(
          {controls, {0, 0}, cr::CreativeTerrainSculptMode::Lower, 2U, 4U,
           8U});
  const cr::CreativeTerrainSculptPlan noControls =
      cr::buildCreativeTerrainSculptPlan(
          {controls, {100, 100}, cr::CreativeTerrainSculptMode::Flatten, 2U,
           2U, 8U});
  const cr::CreativeTerrainSculptPlan noChange =
      cr::buildCreativeTerrainSculptPlan(
          {controls, {6, 0}, cr::CreativeTerrainSculptMode::Flatten, 1U, 2U,
           5U});
  constexpr std::array unsorted{
      cr::CreativeTerrainControlPoint{{1, 0}, 4U, 2U},
      cr::CreativeTerrainControlPoint{{0, 0}, 4U, 2U},
  };
  const cr::CreativeTerrainSculptPlan invalid =
      cr::buildCreativeTerrainSculptPlan(
          {unsorted, {}, cr::CreativeTerrainSculptMode::Count, 2U, 2U, 8U});
  constexpr std::array boundaryControls{
      cr::CreativeTerrainControlPoint{{0, 0},
                                      cr::kCreativeTerrainMaximumHeightCells,
                                      1U},
      cr::CreativeTerrainControlPoint{{10, 0},
                                      cr::kCreativeTerrainMinimumHeightCells,
                                      1U},
  };
  const cr::CreativeTerrainSculptPlan raiseAtMaximum =
      cr::buildCreativeTerrainSculptPlan(
          {boundaryControls, {0, 0}, cr::CreativeTerrainSculptMode::Raise, 1U,
           8U, 8U});
  const cr::CreativeTerrainSculptPlan lowerAtMinimum =
      cr::buildCreativeTerrainSculptPlan(
          {boundaryControls, {10, 0}, cr::CreativeTerrainSculptMode::Lower, 1U,
           8U, 8U});

  const auto hasHeights = [&controls](
                              const cr::CreativeTerrainSculptPlan& plan,
                              std::array<std::uint16_t, 3U> heights) {
    if (plan.items().size() != heights.size()) {
      return false;
    }
    for (std::size_t index = 0U; index < heights.size(); ++index) {
      if (plan.items()[index].kind != cr::CreativeTerrainEditKind::Upsert ||
          plan.items()[index].control.coord != controls[index].coord ||
          plan.items()[index].control.heightCells != heights[index] ||
          plan.items()[index].control.radiusCells !=
              controls[index].radiusCells) {
        return false;
      }
    }
    return true;
  };

  return expect(flatten.accepted && flatten.affectedControlCount == 3U &&
                    hasHeights(flatten, {3U, 6U, 8U}),
                "flatten moves existing rods toward target without creating rods") &&
         expect(smooth.accepted && smooth.affectedControlCount == 3U &&
                    hasHeights(smooth, {3U, 5U, 7U}),
                "smooth derives every output from the same input snapshot") &&
         expect(raise.accepted && raise.affectedControlCount == 3U &&
                    hasHeights(raise, {5U, 8U, 13U}),
                "raise adds strength to every existing rod in the brush") &&
         expect(lower.accepted && lower.affectedControlCount == 3U &&
                    lower.items().size() == 2U &&
                    lower.items()[0].control.coord == controls[1].coord &&
                    lower.items()[0].control.heightCells == 1U &&
                    lower.items()[1].control.coord == controls[2].coord &&
                    lower.items()[1].control.heightCells == 5U,
                "lower subtracts strength and clamps rods at minimum height") &&
         expect(!noControls.accepted && noControls.items().empty() &&
                    noControls.status ==
                        cr::CreativeTerrainSculptPlanStatus::NoControlsInBrush,
                "empty brush rejects without implicit terrain densification") &&
         expect(noChange.accepted && noChange.items().empty() &&
                    noChange.status ==
                        cr::CreativeTerrainSculptPlanStatus::NoChange,
                "already-flat brush is an accepted no-op") &&
         expect(!invalid.accepted && invalid.items().empty() &&
                    invalid.status ==
                        cr::CreativeTerrainSculptPlanStatus::InvalidRequest,
                "invalid or noncanonical sculpt input fails closed") &&
         expect(raiseAtMaximum.accepted && raiseAtMaximum.items().empty() &&
                    raiseAtMaximum.status ==
                        cr::CreativeTerrainSculptPlanStatus::NoChange &&
                    lowerAtMinimum.accepted &&
                    lowerAtMinimum.items().empty() &&
                    lowerAtMinimum.status ==
                        cr::CreativeTerrainSculptPlanStatus::NoChange,
                "raise and lower stop cleanly at authored height bounds") &&
         expect(cr::creativeTerrainSculptRadiusCells(
                    cr::CreativeTerrainSculptRadius::EightCells) == 8U &&
                    cr::creativeTerrainSculptStrengthCells(
                        cr::CreativeTerrainSculptStrength::FourCells) == 4U &&
                    cr::toString(cr::CreativeTerrainSculptMode::Raise) ==
                        "RAISE" &&
                    cr::toString(cr::CreativeTerrainSculptMode::Lower) ==
                        "LOWER" &&
                    cr::creativeTerrainSculptUsesTargetHeight(
                        cr::CreativeTerrainSculptMode::Flatten) &&
                    !cr::creativeTerrainSculptUsesTargetHeight(
                        cr::CreativeTerrainSculptMode::Raise),
                "sculpt options expose explicit values and target semantics");
}

bool gradePlanIsDeterministicBoundedAndValidated() {
  const cr::CreativeTerrainGradePlan ascending =
      cr::buildCreativeTerrainGradePlan({{0, 0}, {4, 2}, 2U, 8U, 3U});
  constexpr std::array expectedCoords{
      cr::CreativeTerrainCoord2{0, 0}, cr::CreativeTerrainCoord2{1, 0},
      cr::CreativeTerrainCoord2{2, 1}, cr::CreativeTerrainCoord2{3, 1},
      cr::CreativeTerrainCoord2{4, 2}};
  constexpr std::array<std::uint16_t, 5U> expectedHeights{2U, 4U, 5U, 7U,
                                                         8U};
  bool exact = ascending.items().size() == expectedCoords.size();
  for (std::size_t index = 0U;
       exact && index < expectedCoords.size(); ++index) {
    exact = ascending.items()[index].kind ==
                cr::CreativeTerrainEditKind::Upsert &&
            ascending.items()[index].control.coord == expectedCoords[index] &&
            ascending.items()[index].control.heightCells ==
                expectedHeights[index] &&
            ascending.items()[index].control.radiusCells == 3U;
  }

  const cr::CreativeTerrainGradePlan descending =
      cr::buildCreativeTerrainGradePlan({{4, 2}, {0, 0}, 8U, 2U, 3U});
  const cr::CreativeTerrainGradePlan point =
      cr::buildCreativeTerrainGradePlan({{7, -4}, {7, -4}, 2U, 9U, 2U});
  const cr::CreativeTerrainGradePlan tooLong =
      cr::buildCreativeTerrainGradePlan({{0, 0}, {256, 0}, 2U, 8U, 3U});
  const cr::CreativeTerrainGradePlan invalidHeight =
      cr::buildCreativeTerrainGradePlan({{0, 0}, {1, 0}, 0U, 8U, 3U});
  const std::int32_t maximum = std::numeric_limits<std::int32_t>::max();
  const cr::CreativeTerrainGradePlan invalidCoordinate =
      cr::buildCreativeTerrainGradePlan(
          {{maximum, 0}, {maximum, 0}, 2U, 8U, 3U});

  return expect(ascending.accepted &&
                    ascending.status ==
                        cr::CreativeTerrainGradePlanStatus::Ready &&
                    exact,
                "grade emits exact Bresenham coordinates and rounded heights") &&
         expect(descending.accepted && descending.items().size() == 5U &&
                    descending.items().front().control.heightCells == 8U &&
                    descending.items()[1].control.heightCells == 6U &&
                    descending.items().back().control.heightCells == 2U,
                "descending grade rounds symmetrically") &&
         expect(point.accepted && point.items().size() == 1U &&
                    point.items().front().control.heightCells == 9U,
                "zero-length grade applies the requested endpoint height") &&
         expect(!tooLong.accepted && tooLong.items().empty() &&
                    tooLong.status ==
                        cr::CreativeTerrainGradePlanStatus::CapacityExceeded,
                "grade rejects before exceeding 256 edits") &&
         expect(!invalidHeight.accepted && invalidHeight.items().empty() &&
                    invalidHeight.status ==
                        cr::CreativeTerrainGradePlanStatus::InvalidRequest,
                "grade rejects invalid height") &&
         expect(!invalidCoordinate.accepted &&
                    invalidCoordinate.items().empty() &&
                    invalidCoordinate.status ==
                        cr::CreativeTerrainGradePlanStatus::InvalidRequest,
                "grade rejects coordinates unsafe for terrain influence");
}

}  // namespace

int main() {
  return mutationsAreAtomicCanonicalAndBounded() &&
                 singleRodCreatesAFlatCircularInfluencePatch() &&
                 overlappingRodsBlendWithDeterministicIntegerWeights() &&
                 raycastHitsTerrainTopsSidesAndFailsClosed() &&
                 renderPlanBendsSharedCornersAndEnforcesBudget() &&
                 documentRevisionAdvancesOncePerTerrainBatch() &&
                 sculptPlanIsSnapshotBasedBoundedAndCanonical() &&
                 gradePlanIsDeterministicBoundedAndValidated()
             ? 0
             : 1;
}
