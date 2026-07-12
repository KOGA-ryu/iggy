#include "app/iggy3d/creative/tools/SurfaceExtrude.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

#include "app/iggy3d/creative/document/VoxelField.hpp"

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

cr::CreativeVoxelField fieldWith(
    std::span<const cr::CreativeVoxelEdit> edits) {
  cr::CreativeVoxelField field;
  const cr::CreativeVoxelMutationReceipt receipt = field.apply(edits);
  if (!receipt.accepted) {
    std::cerr << "FAIL: surface fixture setup\n";
  }
  return field;
}

cr::CreativeSurfaceExtrudePlan plan(
    const cr::CreativeVoxelField* field,
    cr::CreativeGridCoord3 seed,
    cr::CreativeGridCoord3 outward,
    cr::CreativeSurfaceExtrudeKind kind =
        cr::CreativeSurfaceExtrudeKind::Extrude,
    cr::CreativeSurfaceExtrudeDepth depth =
        cr::CreativeSurfaceExtrudeDepth::OneCell,
    cr::CreativeConnectedFillLimit limit =
        cr::CreativeConnectedFillLimit::Cells256) {
  return cr::planCreativeSurfaceExtrude(
      {field, seed, outward, kind, depth, limit});
}

bool exposedPatchAndExtrusionAreDeterministic() {
  const std::array edits{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{1, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{0, 0, 1}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{1, 0, 1}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{3, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{0, 1, 1}, cr::CreativeObjectKind::Wall},
  };
  const cr::CreativeVoxelField field = fieldWith(edits);
  const cr::CreativeSurfaceExtrudePlan result = plan(
      &field, {}, {0, 1, 0}, cr::CreativeSurfaceExtrudeKind::Extrude,
      cr::CreativeSurfaceExtrudeDepth::TwoCells);

  return expect(result.accepted && result.surfaceCellCount == 3U &&
                    result.mutationCellCount == 6U &&
                    result.sourceMaterial == cr::CreativeObjectKind::Floor,
                "planner finds the connected coplanar exposed patch") &&
         expect(sameCell(result.surfaceCells[0], {0, 0, 0}) &&
                    sameCell(result.surfaceCells[1], {1, 0, 0}) &&
                    sameCell(result.surfaceCells[2], {1, 0, 1}),
                "surface traversal has stable tangent order") &&
         expect(sameCell(result.mutationCells[0], {0, 1, 0}) &&
                    sameCell(result.mutationCells[1], {1, 1, 0}) &&
                    sameCell(result.mutationCells[2], {1, 1, 1}) &&
                    sameCell(result.mutationCells[3], {0, 2, 0}) &&
                    sameCell(result.mutationCells[5], {1, 2, 1}),
                "extrusion emits complete layers in stable order") &&
         expect(sameCell(result.minMutationCell, {0, 1, 0}) &&
                    sameCell(result.maxMutationCell, {1, 2, 1}),
                "extrusion records exact aggregate mutation bounds");
}

bool insetPlansCompleteMaterialLayers() {
  std::vector<cr::CreativeVoxelEdit> edits;
  for (std::int32_t y : {0, -1}) {
    edits.push_back({{0, y, 0}, cr::CreativeObjectKind::Wall});
    edits.push_back({{1, y, 0}, cr::CreativeObjectKind::Wall});
  }
  const cr::CreativeVoxelField field = fieldWith(edits);
  const cr::CreativeSurfaceExtrudePlan inset = plan(
      &field, {}, {0, 1, 0}, cr::CreativeSurfaceExtrudeKind::Inset,
      cr::CreativeSurfaceExtrudeDepth::TwoCells);

  const std::array shallowEdits{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{1, 0, 0}, cr::CreativeObjectKind::Wall},
  };
  const cr::CreativeVoxelField shallow = fieldWith(shallowEdits);
  const cr::CreativeSurfaceExtrudePlan missing = plan(
      &shallow, {}, {0, 1, 0}, cr::CreativeSurfaceExtrudeKind::Inset,
      cr::CreativeSurfaceExtrudeDepth::TwoCells);

  return expect(inset.accepted && inset.surfaceCellCount == 2U &&
                    inset.mutationCellCount == 4U &&
                    sameCell(inset.mutationCells[0], {0, 0, 0}) &&
                    sameCell(inset.mutationCells[1], {1, 0, 0}) &&
                    sameCell(inset.mutationCells[2], {0, -1, 0}) &&
                    sameCell(inset.mutationCells[3], {1, -1, 0}),
                "inset validates and emits every inward source layer") &&
         expect(!missing.accepted && missing.mutationCellCount == 0U &&
                    missing.status ==
                        cr::CreativeSurfaceExtrudeStatus::SourceMissing,
                "inset rejects atomically when an inward layer is missing");
}

bool invalidDestinationsAndLimitsFailClosed() {
  const std::array occupiedEdits{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{0, 2, 0}, cr::CreativeObjectKind::Wall},
  };
  const cr::CreativeVoxelField occupied = fieldWith(occupiedEdits);
  const cr::CreativeSurfaceExtrudePlan blocked = plan(
      &occupied, {}, {0, 1, 0}, cr::CreativeSurfaceExtrudeKind::Extrude,
      cr::CreativeSurfaceExtrudeDepth::TwoCells);

  std::vector<cr::CreativeVoxelEdit> largeEdits;
  for (std::int32_t x = 0; x < 33; ++x) {
    largeEdits.push_back({{x, 0, 0}, cr::CreativeObjectKind::Floor});
  }
  const cr::CreativeVoxelField large = fieldWith(largeEdits);
  const cr::CreativeSurfaceExtrudePlan overLimit = plan(
      &large, {}, {0, 1, 0}, cr::CreativeSurfaceExtrudeKind::Extrude,
      cr::CreativeSurfaceExtrudeDepth::TwoCells,
      cr::CreativeConnectedFillLimit::Cells64);

  return expect(!blocked.accepted && blocked.mutationCellCount == 0U &&
                    blocked.status ==
                        cr::CreativeSurfaceExtrudeStatus::
                            DestinationOccupied,
                "occupied later destination rejects the whole extrusion") &&
         expect(!overLimit.accepted && overLimit.surfaceCellCount == 0U &&
                    overLimit.status ==
                        cr::CreativeSurfaceExtrudeStatus::CapacityExceeded,
                "affected-cell limit includes every extrusion layer");
}

bool invalidInputsAndCoordinateEdgesAreSafe() {
  cr::CreativeVoxelField empty;
  const cr::CreativeSurfaceExtrudePlan nullField =
      plan(nullptr, {}, {0, 1, 0});
  const cr::CreativeSurfaceExtrudePlan invalidFace =
      plan(&empty, {}, {1, 1, 0});
  const cr::CreativeSurfaceExtrudePlan invalidDepth = plan(
      &empty, {}, {0, 1, 0}, cr::CreativeSurfaceExtrudeKind::Extrude,
      cr::CreativeSurfaceExtrudeDepth::Count);
  const cr::CreativeSurfaceExtrudePlan emptySeed =
      plan(&empty, {}, {0, 1, 0});

  const cr::CreativeVoxelEdit edgeEdit{
      {std::numeric_limits<std::int32_t>::min(), 0, 0},
      cr::CreativeObjectKind::Wall};
  const cr::CreativeVoxelField edgeField =
      fieldWith(std::span{&edgeEdit, 1U});
  const cr::CreativeSurfaceExtrudePlan edge = plan(
      &edgeField, edgeEdit.cell, {-1, 0, 0});
  const cr::CreativeVoxelEdit upperEdgeEdit{
      {std::numeric_limits<std::int32_t>::max() - 1, 0, 0},
      cr::CreativeObjectKind::Wall};
  const cr::CreativeVoxelField upperEdgeField =
      fieldWith(std::span{&upperEdgeEdit, 1U});
  const cr::CreativeSurfaceExtrudePlan upperEdge = plan(
      &upperEdgeField, upperEdgeEdit.cell, {1, 0, 0});

  return expect(!nullField.accepted &&
                    nullField.status ==
                        cr::CreativeSurfaceExtrudeStatus::InvalidField,
                "null surface field is rejected") &&
         expect(!invalidFace.accepted &&
                    invalidFace.status ==
                        cr::CreativeSurfaceExtrudeStatus::InvalidFace,
                "non-axis face is rejected") &&
         expect(!invalidDepth.accepted &&
                    invalidDepth.status ==
                        cr::CreativeSurfaceExtrudeStatus::InvalidDepth,
                "invalid depth is rejected") &&
         expect(!emptySeed.accepted &&
                    emptySeed.status ==
                        cr::CreativeSurfaceExtrudeStatus::EmptySeed,
                "empty surface seed is rejected") &&
         expect(!edge.accepted &&
                    edge.status ==
                        cr::CreativeSurfaceExtrudeStatus::CoordinateOverflow,
                "lower coordinate overflow fails closed") &&
         expect(!upperEdge.accepted &&
                    upperEdge.status ==
                        cr::CreativeSurfaceExtrudeStatus::CoordinateOverflow,
                "reserved upper voxel coordinate fails before commit");
}

bool labelsAndLayoutStayStable() {
  return expect(cr::creativeSurfaceExtrudeDepthCells(
                    cr::CreativeSurfaceExtrudeDepth::OneCell) == 1U &&
                    cr::creativeSurfaceExtrudeDepthCells(
                        cr::CreativeSurfaceExtrudeDepth::TwoCells) == 2U &&
                    cr::creativeSurfaceExtrudeDepthCells(
                        cr::CreativeSurfaceExtrudeDepth::FourCells) == 4U,
                "surface depths map to bounded layer counts") &&
         expect(cr::toString(cr::CreativeSurfaceExtrudeDepth::TwoCells) ==
                    "2 CELLS" &&
                    cr::toString(cr::CreativeSurfaceExtrudeKind::Inset) ==
                        "Inset" &&
                    cr::toString(
                        cr::CreativeSurfaceExtrudeStatus::FaceOccluded) ==
                        "FaceOccluded",
                "surface plan labels remain stable") &&
         expect(std::is_trivially_copyable_v<
                    cr::CreativeSurfaceExtrudePlan> &&
                    std::is_standard_layout_v<
                        cr::CreativeSurfaceExtrudePlan>,
                "surface plan remains fixed-layout frame data");
}

}  // namespace

int main() {
  bool ok = true;
  ok = exposedPatchAndExtrusionAreDeterministic() && ok;
  ok = insetPlansCompleteMaterialLayers() && ok;
  ok = invalidDestinationsAndLimitsFailClosed() && ok;
  ok = invalidInputsAndCoordinateEdgesAreSafe() && ok;
  ok = labelsAndLayoutStayStable() && ok;
  return ok ? 0 : 1;
}
