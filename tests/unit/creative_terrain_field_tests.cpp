#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/TerrainField.hpp"
#include "app/iggy3d/creative/tools/TerrainGrade.hpp"
#include "app/iggy3d/creative/tools/TerrainSeed.hpp"
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

bool terrainSeedPlansMissingRodsAndClearAtomically() {
  cr::CreativeTerrainField empty;
  const cr::CreativeTerrainSeedRequest seedRequest{
      &empty, {0, 0}, cr::CreativeTerrainSeedOperation::SeedMissing, 2U, 2U,
      4U, 3U};
  const cr::CreativeTerrainSeedPlan seed =
      cr::buildCreativeTerrainSeedPlan(seedRequest);
  constexpr std::array expectedCoords{
      cr::CreativeTerrainCoord2{0, -2}, cr::CreativeTerrainCoord2{-2, 0},
      cr::CreativeTerrainCoord2{0, 0}, cr::CreativeTerrainCoord2{2, 0},
      cr::CreativeTerrainCoord2{0, 2},
  };
  bool exact = seed.items().size() == expectedCoords.size();
  for (std::size_t index = 0U; exact && index < expectedCoords.size(); ++index) {
    exact = seed.items()[index].kind == cr::CreativeTerrainEditKind::Upsert &&
            seed.items()[index].control.coord == expectedCoords[index] &&
            seed.items()[index].control.heightCells == 4U &&
            seed.items()[index].control.radiusCells == 3U;
  }
  static_cast<void>(empty.apply(seed.items()));
  const cr::CreativeTerrainSeedPlan repeated =
      cr::buildCreativeTerrainSeedPlan(
          {&empty, {0, 0}, cr::CreativeTerrainSeedOperation::SeedMissing, 2U,
           2U, 8U, 1U});
  const cr::CreativeTerrainSeedPlan clear =
      cr::buildCreativeTerrainSeedPlan(
          {&empty, {0, 0}, cr::CreativeTerrainSeedOperation::Clear, 2U, 2U, 8U,
           1U});

  cr::CreativeTerrainField surface;
  const cr::CreativeTerrainControlEdit surfaceControl = upsert(0, 0, 6U, 2U);
  static_cast<void>(surface.apply(std::span{&surfaceControl, 1U}));
  const cr::CreativeTerrainSeedPlan sampled =
      cr::buildCreativeTerrainSeedPlan(
          {&surface, {0, 0}, cr::CreativeTerrainSeedOperation::SeedMissing, 2U,
           1U, 2U, 1U});
  const bool sampledSurface = std::all_of(
      sampled.items().begin(), sampled.items().end(),
      [](const cr::CreativeTerrainControlEdit& edit) {
        return edit.control.coord != cr::CreativeTerrainCoord2{0, 0} &&
               edit.control.heightCells == 6U;
      });

  cr::CreativeTerrainField nearlyFull;
  std::vector<cr::CreativeTerrainControlEdit> capacityControls;
  capacityControls.reserve(250U);
  for (std::int32_t index = 0; index < 250; ++index) {
    capacityControls.push_back(upsert(1000 + index, 0, 4U, 1U));
  }
  static_cast<void>(nearlyFull.apply(capacityControls));
  const cr::CreativeTerrainSeedPlan capacity =
      cr::buildCreativeTerrainSeedPlan(
          {&nearlyFull, {0, 0}, cr::CreativeTerrainSeedOperation::SeedMissing,
           2U, 1U, 4U, 1U});
  const cr::CreativeTerrainSeedPlan invalid =
      cr::buildCreativeTerrainSeedPlan(
          {&empty,
           {std::numeric_limits<std::int32_t>::max(), 0},
           cr::CreativeTerrainSeedOperation::SeedMissing, 8U, 1U, 4U, 1U});

  return expect(seed.accepted && seed.candidateCount == 5U && exact,
                "seed emits the exact canonical circular lattice") &&
         expect(repeated.accepted && repeated.items().empty() &&
                    repeated.status ==
                        cr::CreativeTerrainSeedPlanStatus::NoChange,
                "seed preserves existing rods instead of overwriting them") &&
         expect(clear.accepted && clear.items().size() == 5U &&
                    std::all_of(clear.items().begin(), clear.items().end(),
                                [](const auto& edit) {
                                  return edit.kind ==
                                         cr::CreativeTerrainEditKind::Remove;
                                }),
                "clear removes every existing rod in the circular disk") &&
         expect(sampled.accepted && sampled.items().size() == 12U &&
                    sampledSurface,
                "new rods sample pre-edit derived height when terrain exists") &&
         expect(!capacity.accepted && capacity.items().empty() &&
                    capacity.status ==
                        cr::CreativeTerrainSeedPlanStatus::CapacityExceeded &&
                    nearlyFull.controlCount() == 250U,
                "over-capacity seed rejects atomically") &&
         expect(!invalid.accepted && invalid.items().empty() &&
                    invalid.status ==
                        cr::CreativeTerrainSeedPlanStatus::InvalidRequest,
                "coordinate overflow rejects before emitting edits") &&
         expect(cr::creativeTerrainSeedRadiusCells(
                    cr::CreativeTerrainSeedRadius::EightCells) == 8U &&
                    cr::creativeTerrainSeedSpacingCells(
                        cr::CreativeTerrainSeedSpacing::FourCells) == 4U,
                "seed option enums resolve to explicit cell values");
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

bool sculptFalloffIsDeterministicSymmetricAndSharedByModes() {
  constexpr std::array raisedControls{
      cr::CreativeTerrainControlPoint{{-8, 0}, 10U, 2U},
      cr::CreativeTerrainControlPoint{{-6, 0}, 10U, 2U},
      cr::CreativeTerrainControlPoint{{-2, 0}, 10U, 2U},
      cr::CreativeTerrainControlPoint{{0, 0}, 10U, 2U},
      cr::CreativeTerrainControlPoint{{2, 0}, 10U, 2U},
      cr::CreativeTerrainControlPoint{{6, 0}, 10U, 2U},
      cr::CreativeTerrainControlPoint{{8, 0}, 10U, 2U},
  };
  const auto raisePlan =
      [&raisedControls](cr::CreativeTerrainSculptFalloff falloff) {
        return cr::buildCreativeTerrainSculptPlan(
            {raisedControls,
             {0, 0},
             cr::CreativeTerrainSculptMode::Raise,
             8U,
             8U,
             20U,
             falloff});
      };
  const cr::CreativeTerrainSculptPlan uniform =
      raisePlan(cr::CreativeTerrainSculptFalloff::Uniform);
  const cr::CreativeTerrainSculptPlan linear =
      raisePlan(cr::CreativeTerrainSculptFalloff::Linear);
  const cr::CreativeTerrainSculptPlan smooth =
      raisePlan(cr::CreativeTerrainSculptFalloff::Smooth);

  const auto editedHeight = [](const cr::CreativeTerrainSculptPlan &plan,
                               std::int32_t x) {
    const auto found = std::find_if(
        plan.items().begin(), plan.items().end(), [x](const auto &edit) {
          return edit.control.coord == cr::CreativeTerrainCoord2{x, 0};
        });
    return found == plan.items().end() ? std::uint16_t{0U}
                                       : found->control.heightCells;
  };

  constexpr std::array loweredControls{
      cr::CreativeTerrainControlPoint{{-8, 0}, 20U, 2U},
      cr::CreativeTerrainControlPoint{{-6, 0}, 20U, 2U},
      cr::CreativeTerrainControlPoint{{-2, 0}, 20U, 2U},
      cr::CreativeTerrainControlPoint{{0, 0}, 20U, 2U},
      cr::CreativeTerrainControlPoint{{2, 0}, 20U, 2U},
      cr::CreativeTerrainControlPoint{{6, 0}, 20U, 2U},
      cr::CreativeTerrainControlPoint{{8, 0}, 20U, 2U},
  };
  const cr::CreativeTerrainSculptPlan lower =
      cr::buildCreativeTerrainSculptPlan(
          {loweredControls,
           {0, 0},
           cr::CreativeTerrainSculptMode::Lower,
           8U,
           8U,
           20U,
           cr::CreativeTerrainSculptFalloff::Smooth});
  const cr::CreativeTerrainSculptPlan flatten =
      cr::buildCreativeTerrainSculptPlan(
          {raisedControls,
           {0, 0},
           cr::CreativeTerrainSculptMode::Flatten,
           8U,
           8U,
           30U,
           cr::CreativeTerrainSculptFalloff::Smooth});

  constexpr std::array unevenControls{
      cr::CreativeTerrainControlPoint{{0, 0}, 10U, 2U},
      cr::CreativeTerrainControlPoint{{2, 0}, 30U, 2U},
      cr::CreativeTerrainControlPoint{{6, 0}, 30U, 2U},
      cr::CreativeTerrainControlPoint{{8, 0}, 10U, 2U},
  };
  const cr::CreativeTerrainSculptPlan averaged =
      cr::buildCreativeTerrainSculptPlan(
          {unevenControls,
           {0, 0},
           cr::CreativeTerrainSculptMode::Smooth,
           8U,
           8U,
           20U,
           cr::CreativeTerrainSculptFalloff::Smooth});
  const cr::CreativeTerrainSculptPlan invalid =
      cr::buildCreativeTerrainSculptPlan(
          {raisedControls,
           {0, 0},
           cr::CreativeTerrainSculptMode::Raise,
           8U,
           8U,
           20U,
           cr::CreativeTerrainSculptFalloff::Count});

  return expect(uniform.accepted && uniform.items().size() == 7U &&
                    editedHeight(uniform, -8) == 18U &&
                    editedHeight(uniform, 8) == 18U,
                "uniform falloff preserves full strength through the brush "
                "edge") &&
         expect(linear.accepted && linear.items().size() == 5U &&
                    editedHeight(linear, -6) == 12U &&
                    editedHeight(linear, -2) == 16U &&
                    editedHeight(linear, 0) == 18U &&
                    editedHeight(linear, 2) == 16U &&
                    editedHeight(linear, 6) == 12U &&
                    editedHeight(linear, -8) == 0U &&
                    editedHeight(linear, 8) == 0U,
                "linear falloff is symmetric with zero strength at the edge") &&
         expect(smooth.accepted && smooth.items().size() == 5U &&
                    editedHeight(smooth, -6) == 11U &&
                    editedHeight(smooth, -2) == 17U &&
                    editedHeight(smooth, 0) == 18U &&
                    editedHeight(smooth, 2) == 17U &&
                    editedHeight(smooth, 6) == 11U,
                "smoothstep falloff rounds deterministic center and shoulder "
                "weights") &&
         expect(lower.accepted && editedHeight(lower, -6) == 19U &&
                    editedHeight(lower, -2) == 13U &&
                    editedHeight(lower, 0) == 12U &&
                    editedHeight(lower, 2) == 13U &&
                    editedHeight(lower, 6) == 19U,
                "lower consumes the shared smooth falloff strength") &&
         expect(flatten.accepted && editedHeight(flatten, -6) == 11U &&
                    editedHeight(flatten, -2) == 17U &&
                    editedHeight(flatten, 0) == 18U &&
                    editedHeight(flatten, 2) == 17U &&
                    editedHeight(flatten, 6) == 11U,
                "flatten consumes the shared smooth falloff strength") &&
         expect(
             averaged.accepted && averaged.items().size() == 3U &&
                 editedHeight(averaged, 0) == 18U &&
                 editedHeight(averaged, 2) == 23U &&
                 editedHeight(averaged, 6) == 29U &&
                 editedHeight(averaged, 8) == 0U,
             "smooth averages one snapshot before applying radial strength") &&
         expect(!invalid.accepted && invalid.items().empty() &&
                    invalid.status ==
                        cr::CreativeTerrainSculptPlanStatus::InvalidRequest &&
                    cr::toString(cr::CreativeTerrainSculptFalloff::Uniform) ==
                        "UNIFORM" &&
                    cr::toString(cr::CreativeTerrainSculptFalloff::Linear) ==
                        "LINEAR" &&
                    cr::toString(cr::CreativeTerrainSculptFalloff::Smooth) ==
                        "SMOOTH",
                "falloff values are closed validated and explicitly named");
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
                 terrainSeedPlansMissingRodsAndClearAtomically() &&
                 sculptPlanIsSnapshotBasedBoundedAndCanonical() &&
                 sculptFalloffIsDeterministicSymmetricAndSharedByModes() &&
                 gradePlanIsDeterministicBoundedAndValidated()
             ? 0
             : 1;
}
