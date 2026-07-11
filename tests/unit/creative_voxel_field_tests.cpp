#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/VoxelField.hpp"

#include <array>
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

bool chunksHandleNegativeCoordinates() {
  cr::CreativeVoxelField field;
  const std::array edits{
      cr::CreativeVoxelEdit{{-1, -1, -1}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{-16, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{-17, 0, 0}, cr::CreativeObjectKind::Crate},
      cr::CreativeVoxelEdit{{16, 16, 16}, cr::CreativeObjectKind::Wall},
  };
  const cr::CreativeVoxelMutationReceipt receipt = field.apply(edits);
  return expect(receipt.accepted && receipt.changed,
                "negative-coordinate edit applies") &&
         expect(receipt.createdCellCount == edits.size(),
                "all cells created") &&
         expect(receipt.chunkCountBefore == 0U &&
                    receipt.chunkCountAfter == 4U &&
                    receipt.stagedChunkCount == 4U,
                "one staged chunk per changed chunk") &&
         expect(field.occupiedCellCount() == edits.size(),
                "occupied count") &&
         expect(field.chunkCount() == 4U, "negative floor division chunks") &&
         expect(field.materialAt({-1, -1, -1}) ==
                    cr::CreativeObjectKind::Wall,
                "negative cell lookup") &&
         expect(field.materialAt({-17, 0, 0}) ==
                    cr::CreativeObjectKind::Crate,
                "cross-negative-chunk lookup") &&
         expect(field.validateInvariants(), "field invariants");
}

bool mutationsAreAtomicAndCanonical() {
  cr::CreativeVoxelField field;
  const cr::CreativeVoxelEdit initial{{2, 3, 4},
                                      cr::CreativeObjectKind::Wall};
  const cr::CreativeVoxelMutationReceipt created =
      field.apply(std::span{&initial, 1U});
  const std::uint64_t revision = field.revision();
  const std::array duplicate{
      cr::CreativeVoxelEdit{{3, 3, 3}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{3, 3, 3}, cr::CreativeObjectKind::Crate},
  };
  const cr::CreativeVoxelMutationReceipt duplicateReceipt =
      field.apply(duplicate);
  const cr::CreativeVoxelEdit invalid{{5, 5, 5},
                                      cr::CreativeObjectKind::Count};
  const cr::CreativeVoxelMutationReceipt invalidReceipt =
      field.apply(std::span{&invalid, 1U});
  const cr::CreativeVoxelEdit invalidCast{
      {6, 6, 6}, static_cast<cr::CreativeObjectKind>(-1)};
  const cr::CreativeVoxelMutationReceipt invalidCastReceipt =
      field.apply(std::span{&invalidCast, 1U});
  const cr::CreativeVoxelEdit unrepresentableCell{
      {std::numeric_limits<std::int32_t>::max(), 0, 0},
      cr::CreativeObjectKind::Wall};
  const cr::CreativeVoxelMutationReceipt unrepresentableReceipt =
      field.apply(std::span{&unrepresentableCell, 1U});
  const std::array mixedInvalid{
      cr::CreativeVoxelEdit{{7, 7, 7}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{8, 8, 8}, cr::CreativeObjectKind::Count},
  };
  const cr::CreativeVoxelMutationReceipt mixedInvalidReceipt =
      field.apply(mixedInvalid);
  const cr::CreativeVoxelMutationReceipt noChange =
      field.apply(std::span{&initial, 1U});

  return expect(created.accepted && created.changed, "initial edit") &&
         expect(!duplicateReceipt.accepted &&
                    duplicateReceipt.status ==
                        cr::CreativeVoxelMutationStatus::DuplicateCell,
                "duplicate batch rejected") &&
         expect(!invalidReceipt.accepted &&
                    invalidReceipt.status ==
                        cr::CreativeVoxelMutationStatus::InvalidMaterial,
                "invalid material rejected") &&
         expect(!invalidCastReceipt.accepted &&
                    invalidCastReceipt.status ==
                        cr::CreativeVoxelMutationStatus::InvalidMaterial,
                "out-of-range material enum rejected") &&
         expect(!unrepresentableReceipt.accepted &&
                    unrepresentableReceipt.status ==
                        cr::CreativeVoxelMutationStatus::InvalidCell,
                "cell without representable exclusive edge rejected") &&
         expect(!mixedInvalidReceipt.accepted &&
                    !field.occupied({7, 7, 7}),
                "mixed invalid batch is atomic") &&
         expect(noChange.accepted && !noChange.changed,
                "identical edit is no change") &&
         expect(noChange.stagedChunkCount == 0U,
                "no-change edit stages no chunks") &&
         expect(field.revision() == revision &&
                    field.occupiedCellCount() == 1U,
                "rejected edits preserve field");
}

bool singleChunkEditStagesConstantWorkAcrossLargeField() {
  constexpr std::int32_t kChunkCount = 256;
  std::vector<cr::CreativeVoxelEdit> initial;
  initial.reserve(kChunkCount);
  for (std::int32_t chunk = 0; chunk < kChunkCount; ++chunk) {
    initial.push_back({{chunk * cr::kCreativeVoxelChunkEdge, 0, 0},
                       cr::CreativeObjectKind::Wall});
  }

  cr::CreativeVoxelField field;
  const cr::CreativeVoxelMutationReceipt populated = field.apply(initial);
  const cr::CreativeVoxelEdit replacement{
      {(kChunkCount / 2) * cr::kCreativeVoxelChunkEdge, 0, 0},
      cr::CreativeObjectKind::Floor};
  const cr::CreativeVoxelMutationReceipt replaced =
      field.apply(std::span{&replacement, 1U});

  return expect(populated.accepted && populated.changed,
                "large field population applies") &&
         expect(populated.chunkCountBefore == 0U &&
                    populated.chunkCountAfter == kChunkCount &&
                    populated.stagedChunkCount == kChunkCount,
                "large population stages each changed chunk") &&
         expect(replaced.accepted && replaced.changed &&
                    replaced.replacedCellCount == 1U,
                "single existing cell replacement applies") &&
         expect(replaced.chunkCountBefore == kChunkCount &&
                    replaced.chunkCountAfter == kChunkCount,
                "single replacement preserves large field chunk count") &&
         expect(replaced.stagedChunkCount == 1U &&
                    replaced.dirtyChunks.size() == 1U,
                "single replacement stages constant chunk work") &&
         expect(field.chunks().front().revision == 1U &&
                    field.chunks()[kChunkCount / 2].revision == 2U &&
                    field.chunks().back().revision == 1U,
                "untouched chunk revisions remain stable") &&
         expect(field.validateInvariants(), "large field invariants");
}

bool chunkInsertionAndRemovalCommitTogether() {
  cr::CreativeVoxelField field;
  const cr::CreativeVoxelEdit initial{{0, 0, 0},
                                      cr::CreativeObjectKind::Wall};
  static_cast<void>(field.apply(std::span{&initial, 1U}));
  const std::array edits{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Unknown},
      cr::CreativeVoxelEdit{{-16, 0, 0}, cr::CreativeObjectKind::Floor},
  };
  const cr::CreativeVoxelMutationReceipt receipt = field.apply(edits);

  return expect(receipt.accepted && receipt.changed,
                "insert and removal batch applies") &&
         expect(receipt.stagedChunkCount == 2U &&
                    receipt.dirtyChunks.size() == 2U,
                "insert and removal stage two chunks") &&
         expect(receipt.createdCellCount == 1U &&
                    receipt.removedCellCount == 1U,
                "insert and removal facts") &&
         expect(receipt.chunkCountBefore == 1U &&
                    receipt.chunkCountAfter == 1U,
                "empty chunk erased as new chunk is inserted") &&
         expect(!field.occupied({0, 0, 0}) &&
                    field.materialAt({-16, 0, 0}) ==
                        cr::CreativeObjectKind::Floor,
                "insert and removal commit exact cells") &&
         expect(field.validateInvariants(), "insert and removal invariants");
}

bool cuboidsAreGreedyAndChunkLocal() {
  cr::CreativeVoxelField field;
  std::array<cr::CreativeVoxelEdit, 10> edits{};
  std::size_t edit = 0;
  for (std::int32_t z = 0; z < 2; ++z) {
    for (std::int32_t y = 0; y < 2; ++y) {
      for (std::int32_t x = 0; x < 2; ++x) {
        edits[edit++] = {{x, y, z}, cr::CreativeObjectKind::Wall};
      }
    }
  }
  edits[edit++] = {{15, 4, 4}, cr::CreativeObjectKind::Floor};
  edits[edit++] = {{16, 4, 4}, cr::CreativeObjectKind::Floor};
  static_cast<void>(field.apply(edits));

  const std::vector<cr::CreativeVoxelCuboid> cuboids =
      cr::buildCreativeVoxelCuboids(field);
  bool foundMergedBox = false;
  std::size_t floorCuboids = 0;
  for (const cr::CreativeVoxelCuboid& cuboid : cuboids) {
    foundMergedBox |= cuboid.material == cr::CreativeObjectKind::Wall &&
                      cuboid.minCell.x == 0 && cuboid.minCell.y == 0 &&
                      cuboid.minCell.z == 0 &&
                      cuboid.maxCellExclusive.x == 2 &&
                      cuboid.maxCellExclusive.y == 2 &&
                      cuboid.maxCellExclusive.z == 2;
    floorCuboids += cuboid.material == cr::CreativeObjectKind::Floor ? 1U : 0U;
  }
  return expect(foundMergedBox, "2x2x2 merges into one cuboid") &&
         expect(floorCuboids == 2U,
                "cross-chunk cells stay separate cuboids");
}

bool raycastUsesGridDda() {
  cr::CreativeVoxelField field;
  const cr::CreativeVoxelEdit edit{{2, 0, 0}, cr::CreativeObjectKind::Crate};
  static_cast<void>(field.apply(std::span{&edit, 1U}));

  cr::CreativeVoxelRaycastRequest request;
  request.rayOrigin = {-2.0, 0.5, 0.5};
  request.rayDirection = {4.0, 0.0, 0.0};
  request.maxDistance = 10.0;
  const cr::CreativeVoxelRaycastReceipt hit =
      cr::raycastCreativeVoxelField(field, request);
  request.rayOrigin = {2.5, 0.5, 0.5};
  const cr::CreativeVoxelRaycastReceipt inside =
      cr::raycastCreativeVoxelField(field, request);

  return expect(hit.accepted && hit.hit && !hit.startInside,
                "DDA ray hits") &&
         expect(hit.cell.x == 2 && hit.cell.y == 0 && hit.cell.z == 0,
                "DDA hit cell") &&
         expect(hit.distance == 4.0 && hit.faceNormal.x == -1.0,
                "DDA distance and face") &&
         expect(inside.hit && inside.startInside && inside.distance == 0.0,
                "DDA start-inside");
}

bool documentAdvancesOncePerBatch() {
  cr::CreativeDocument document = cr::CreativeDocument::create("voxels");
  static_cast<void>(document.assignId(7U));
  const std::array edits{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{1, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{2, 0, 0}, cr::CreativeObjectKind::Floor},
  };
  const cr::CreativeVoxelMutationReceipt receipt = document.applyVoxelEdits(edits);
  return expect(receipt.accepted && receipt.changed, "document batch applies") &&
         expect(document.revision() == 1U,
                "document revision advances once") &&
         expect(document.objectCount() == 0U,
                "voxels are not document objects") &&
         expect(document.voxelField().occupiedCellCount() == edits.size(),
                "document owns voxel field") &&
         expect(document.dirtyFlags() != 0U, "voxel edit marks dirty");
}

}  // namespace

int main() {
  return chunksHandleNegativeCoordinates() &&
                 mutationsAreAtomicAndCanonical() &&
                 singleChunkEditStagesConstantWorkAcrossLargeField() &&
                 chunkInsertionAndRemovalCommitTogether() &&
                 cuboidsAreGreedyAndChunkLocal() && raycastUsesGridDda() &&
                 documentAdvancesOncePerBatch()
             ? 0
             : 1;
}
