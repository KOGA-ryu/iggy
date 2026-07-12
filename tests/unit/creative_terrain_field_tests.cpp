#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
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

}  // namespace

int main() {
  return mutationsAreAtomicCanonicalAndBounded() &&
                 singleRodCreatesAFlatCircularInfluencePatch() &&
                 overlappingRodsBlendWithDeterministicIntegerWeights() &&
                 raycastHitsTerrainTopsSidesAndFailsClosed() &&
                 renderPlanBendsSharedCornersAndEnforcesBudget() &&
                 documentRevisionAdvancesOncePerTerrainBatch()
             ? 0
             : 1;
}
