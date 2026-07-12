#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <algorithm>
#include <array>
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
         expect(first.columns == second.columns &&
                    first.cuboids.size() == second.cuboids.size(),
                "rebuilding is deterministic");
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
                 documentRevisionAdvancesOncePerTerrainBatch()
             ? 0
             : 1;
}
