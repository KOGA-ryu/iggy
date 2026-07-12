#include "app/iggy3d/creative/tools/ConnectedFill.hpp"
#include "app/iggy3d/creative/document/VoxelField.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool sameCell(cr::CreativeGridCoord3 lhs,
              cr::CreativeGridCoord3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool contains(const cr::CreativeConnectedFillPlan& plan,
              cr::CreativeGridCoord3 cell) {
  return std::any_of(plan.generatedCells().begin(),
                     plan.generatedCells().end(),
                     [cell](cr::CreativeGridCoord3 candidate) {
                       return sameCell(candidate, cell);
                     });
}

cr::CreativeVoxelField fieldWith(
    std::span<const cr::CreativeVoxelEdit> edits) {
  cr::CreativeVoxelField field;
  const cr::CreativeVoxelMutationReceipt receipt = field.apply(edits);
  if (!receipt.accepted) {
    std::cerr << "FAIL: test field setup\n";
  }
  return field;
}

bool plansOnlyTheConnectedSameMaterialComponent() {
  const std::array edits{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{-1, 0, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{0, 1, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{0, 1, 1}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{1, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{0, 0, 2}, cr::CreativeObjectKind::Wall},
  };
  const cr::CreativeVoxelField field = fieldWith(edits);
  const cr::CreativeConnectedFillPlan plan = cr::planCreativeConnectedFill(
      {&field, {0, 0, 0}, cr::CreativeConnectedFillLimit::Cells64});

  return expect(plan.accepted && plan.cellCount == 4U &&
                    plan.sourceMaterial == cr::CreativeObjectKind::Wall,
                "fill plans one same-material six-neighbor component") &&
         expect(sameCell(plan.cells[0], {0, 0, 0}) &&
                    sameCell(plan.cells[1], {-1, 0, 0}) &&
                    sameCell(plan.cells[2], {0, 1, 0}) &&
                    sameCell(plan.cells[3], {0, 1, 1}),
                "fill traversal order is deterministic") &&
         expect(!contains(plan, {1, 0, 0}) && !contains(plan, {0, 0, 2}),
                "other materials and disconnected cells are excluded") &&
         expect(sameCell(plan.minCell, {-1, 0, 0}) &&
                    sameCell(plan.maxCell, {0, 1, 1}),
                "plan records exact aggregate cell bounds");
}

bool exactLimitPassesAndOverflowFailsClosed() {
  std::vector<cr::CreativeVoxelEdit> edits;
  edits.reserve(65U);
  for (std::int32_t x = 0; x < 65; ++x) {
    edits.push_back({{x, 0, 0}, cr::CreativeObjectKind::Wall});
  }
  cr::CreativeVoxelField field = fieldWith(edits);
  const cr::CreativeVoxelEdit removeLast{{64, 0, 0},
                                         cr::CreativeObjectKind::Unknown};
  static_cast<void>(field.apply(std::span{&removeLast, 1U}));
  const cr::CreativeConnectedFillPlan exact = cr::planCreativeConnectedFill(
      {&field, {}, cr::CreativeConnectedFillLimit::Cells64});
  const cr::CreativeVoxelEdit restoreLast{{64, 0, 0},
                                          cr::CreativeObjectKind::Wall};
  static_cast<void>(field.apply(std::span{&restoreLast, 1U}));
  const cr::CreativeConnectedFillPlan exceeded =
      cr::planCreativeConnectedFill(
          {&field, {}, cr::CreativeConnectedFillLimit::Cells64});

  return expect(exact.accepted && exact.cellCount == 64U,
                "component exactly at the selected limit is accepted") &&
         expect(!exceeded.accepted && exceeded.cellCount == 0U &&
                    exceeded.status ==
                        cr::CreativeConnectedFillStatus::CapacityExceeded,
                "component above the selected limit fails closed");
}

bool invalidInputsAndCoordinateEdgesFailSafely() {
  cr::CreativeVoxelField empty;
  const cr::CreativeConnectedFillPlan nullField =
      cr::planCreativeConnectedFill({});
  const cr::CreativeConnectedFillPlan invalidLimit =
      cr::planCreativeConnectedFill(
          {&empty, {}, cr::CreativeConnectedFillLimit::Count});
  const cr::CreativeConnectedFillPlan emptySeed =
      cr::planCreativeConnectedFill(
          {&empty, {}, cr::CreativeConnectedFillLimit::Cells64});
  const cr::CreativeVoxelEdit edgeEdit{
      {std::numeric_limits<std::int32_t>::min(), 0, 0},
      cr::CreativeObjectKind::Wall};
  const cr::CreativeVoxelField edgeField =
      fieldWith(std::span{&edgeEdit, 1U});
  const cr::CreativeConnectedFillPlan edge = cr::planCreativeConnectedFill(
      {&edgeField, edgeEdit.cell,
       cr::CreativeConnectedFillLimit::Cells64});

  return expect(!nullField.accepted &&
                    nullField.status ==
                        cr::CreativeConnectedFillStatus::InvalidField,
                "null field is rejected") &&
         expect(!invalidLimit.accepted &&
                    invalidLimit.status ==
                        cr::CreativeConnectedFillStatus::InvalidLimit,
                "invalid limit is rejected before seed lookup") &&
         expect(!emptySeed.accepted &&
                    emptySeed.status ==
                        cr::CreativeConnectedFillStatus::EmptySeed,
                "empty seed is rejected") &&
         expect(edge.accepted && edge.cellCount == 1U &&
                    sameCell(edge.cells[0], edgeEdit.cell),
                "coordinate-edge traversal skips overflowing neighbors");
}

bool limitsAndLabelsAreStable() {
  return expect(cr::connectedFillCellLimit(
                    cr::CreativeConnectedFillLimit::Cells64) == 64U &&
                    cr::connectedFillCellLimit(
                        cr::CreativeConnectedFillLimit::Cells128) == 128U &&
                    cr::connectedFillCellLimit(
                        cr::CreativeConnectedFillLimit::Cells256) == 256U &&
                    cr::connectedFillCellLimit(
                        cr::CreativeConnectedFillLimit::Cells512) == 512U &&
                    cr::connectedFillCellLimit(
                        cr::CreativeConnectedFillLimit::Count) == 0U,
                "connected fill limits map to fixed capacities") &&
         expect(cr::toString(cr::CreativeConnectedFillLimit::Cells256) ==
                    "256 CELLS" &&
                    cr::toString(
                        cr::CreativeConnectedFillStatus::CapacityExceeded) ==
                        "CapacityExceeded",
                "connected fill labels remain stable") &&
         expect(std::is_trivially_copyable_v<
                    cr::CreativeConnectedFillPlan> &&
                    std::is_standard_layout_v<
                        cr::CreativeConnectedFillPlan>,
                "connected fill plan remains fixed-layout frame data");
}

}  // namespace

int main() {
  bool ok = true;
  ok = plansOnlyTheConnectedSameMaterialComponent() && ok;
  ok = exactLimitPassesAndOverflowFailsClosed() && ok;
  ok = invalidInputsAndCoordinateEdgesFailSafely() && ok;
  ok = limitsAndLabelsAreStable() && ok;
  return ok ? 0 : 1;
}
