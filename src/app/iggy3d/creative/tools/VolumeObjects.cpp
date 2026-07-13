#include "app/iggy3d/creative/tools/VolumeInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/tools/Clipboard.hpp"

namespace iggy3d::creative::volume_internal {
namespace {

[[nodiscard]] bool pointInside(CreativeVec3 point,
                               CreativeBounds bounds) noexcept {
  return isFiniteCreativeVec3(point) && point.x >= bounds.min.x &&
         point.y >= bounds.min.y && point.z >= bounds.min.z &&
         point.x < bounds.max.x && point.y < bounds.max.y &&
         point.z < bounds.max.z;
}

[[nodiscard]] bool boundsInside(CreativeBounds inner,
                                CreativeBounds outer) noexcept {
  return isFiniteCreativeVec3(inner.min) &&
         isFiniteCreativeVec3(inner.max) &&
         inner.max.x >= inner.min.x && inner.max.y >= inner.min.y &&
         inner.max.z >= inner.min.z && inner.min.x >= outer.min.x &&
         inner.min.y >= outer.min.y && inner.min.z >= outer.min.z &&
         inner.max.x <= outer.max.x && inner.max.y <= outer.max.y &&
         inner.max.z <= outer.max.z;
}

[[nodiscard]] bool checkedAddCellCoordinate(std::int32_t lhs,
                                            std::int32_t rhs,
                                            std::int32_t& out) noexcept {
  const std::int64_t sum = static_cast<std::int64_t>(lhs) + rhs;
  if (sum < std::numeric_limits<std::int32_t>::min() ||
      sum > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  out = static_cast<std::int32_t>(sum);
  return true;
}

[[nodiscard]] std::vector<CreativeObjectId> containedObjectIds(
    const CreativeDocument& document,
    const CreativeVolumeSelection& selection) {
  const CreativeBounds worldBounds = creativeVolumeWorldBounds(selection);
  std::vector<CreativeObjectId> objectIds;
  objectIds.reserve(document.objects().size());
  for (const CreativeObject& object : document.objects()) {
    if (objectInsideVolume(object, worldBounds)) {
      objectIds.push_back(object.id);
    }
  }
  return objectIds;
}

}  // namespace

bool objectInsideVolume(const CreativeObject& object,
                        CreativeBounds volumeBounds) noexcept {
  const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
  if (descriptor.hasBounds) {
    const CreativeTransformedBounds resolved =
        resolveCreativeObjectBounds(object);
    return resolved.valid && boundsInside(resolved.worldBounds, volumeBounds);
  }
  if (!object.pathPoints.empty()) {
    return std::all_of(object.pathPoints.begin(), object.pathPoints.end(),
                       [volumeBounds](const CreativePathPoint& point) {
                         return pointInside(point.position, volumeBounds);
                       });
  }
  return descriptor.hasTransform &&
         pointInside(object.transform.position, volumeBounds);
}

CreativeVolumeOperationReceipt eraseVolumeObjects(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt) {
  std::vector<CreativeObjectId> objectIds =
      containedObjectIds(document, request.selection);
  receipt.matchedObjectCount = objectIds.size();
  const std::vector<CreativeVoxelCell> voxelCells = collectCreativeVoxelCells(
      document.voxelField(), inclusiveVolumeGridBounds(request.selection));
  receipt.matchedVoxelCellCount = voxelCells.size();
  if (affectedCountExceeds(objectIds.size(), voxelCells.size(),
                           request.maxAffectedObjects)) {
    reject(receipt, CreativeVolumeOperationStatus::OperationLimitExceeded,
           "creative_volume_erase_limit_exceeded");
    return receipt;
  }
  if (objectIds.empty() && voxelCells.empty()) {
    acceptNoChange(receipt, "creative_volume_erase_empty");
    return receipt;
  }

  CreativeDocument staged = document;
  if (!objectIds.empty()) {
    CreativeClipboard removedObjects;
    const CreativeClipboardCutReceipt cutReceipt =
        cutDocumentObjectsAtomically(staged, objectIds, removedObjects);
    if (!cutReceipt.accepted || !cutReceipt.changed) {
      receipt.failedObjectId = cutReceipt.failedObjectId;
      reject(receipt, CreativeVolumeOperationStatus::RemoveRejected,
             cutReceipt.reasonCode);
      return receipt;
    }
    receipt.removedObjectIds = objectIds;
  }

  if (!voxelCells.empty()) {
    std::vector<CreativeVoxelEdit> voxelEdits;
    voxelEdits.reserve(voxelCells.size());
    for (const CreativeVoxelCell& cell : voxelCells) {
      voxelEdits.push_back({cell.cell, CreativeObjectKind::Unknown});
    }
    const CreativeVoxelMutationReceipt voxelReceipt =
        staged.applyVoxelEdits(voxelEdits);
    if (!voxelReceipt.accepted || !voxelReceipt.changed) {
      reject(receipt, CreativeVolumeOperationStatus::VoxelMutationRejected,
             voxelReceipt.reasonCode);
      return receipt;
    }
    copyVoxelMutationFacts(receipt, voxelReceipt);
  }

  document = std::move(staged);
  acceptApplied(receipt, document, "creative_volume_erase_applied");
  return receipt;
}

CreativeVolumeOperationReceipt cloneVolumeObjects(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt) {
  const std::vector<CreativeObjectId> objectIds =
      containedObjectIds(document, request.selection);
  receipt.matchedObjectCount = objectIds.size();
  const std::vector<CreativeVoxelCell> voxelCells = collectCreativeVoxelCells(
      document.voxelField(), inclusiveVolumeGridBounds(request.selection));
  receipt.matchedVoxelCellCount = voxelCells.size();
  if (affectedCountExceeds(objectIds.size(), voxelCells.size(),
                           request.maxAffectedObjects)) {
    reject(receipt, CreativeVolumeOperationStatus::OperationLimitExceeded,
           "creative_volume_clone_limit_exceeded");
    return receipt;
  }
  if (objectIds.empty() && voxelCells.empty()) {
    acceptNoChange(receipt, "creative_volume_clone_empty");
    return receipt;
  }

  CreativeVec3 offset = request.cloneOffset;
  if (!request.hasCloneOffset) {
    const CreativeBounds bounds = creativeVolumeWorldBounds(request.selection);
    offset = {measureCreativeBounds(bounds).size.x, 0.0, 0.0};
  }
  if (!isFiniteCreativeVec3(offset)) {
    reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
           "creative_volume_clone_offset_invalid");
    return receipt;
  }

  CreativeGridCoord3 voxelOffset{};
  if (!voxelCells.empty()) {
    const double scaledX = offset.x / request.selection.cellSize;
    const double scaledY = offset.y / request.selection.cellSize;
    const double scaledZ = offset.z / request.selection.cellSize;
    const double roundedX = std::round(scaledX);
    const double roundedY = std::round(scaledY);
    const double roundedZ = std::round(scaledZ);
    constexpr double kCellOffsetEpsilon = 1.0e-9;
    if (std::fabs(scaledX - roundedX) > kCellOffsetEpsilon ||
        std::fabs(scaledY - roundedY) > kCellOffsetEpsilon ||
        std::fabs(scaledZ - roundedZ) > kCellOffsetEpsilon ||
        roundedX < std::numeric_limits<std::int32_t>::min() ||
        roundedX > std::numeric_limits<std::int32_t>::max() ||
        roundedY < std::numeric_limits<std::int32_t>::min() ||
        roundedY > std::numeric_limits<std::int32_t>::max() ||
        roundedZ < std::numeric_limits<std::int32_t>::min() ||
        roundedZ > std::numeric_limits<std::int32_t>::max()) {
      reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
             "creative_volume_clone_voxel_offset_invalid");
      return receipt;
    }
    voxelOffset = {static_cast<std::int32_t>(roundedX),
                   static_cast<std::int32_t>(roundedY),
                   static_cast<std::int32_t>(roundedZ)};
  }

  CreativeDocument staged = document;
  if (!objectIds.empty()) {
    CreativeClipboard clipboard;
    const CreativeClipboardCopyReceipt copyReceipt =
        copyDocumentObjectsToClipboard(document, objectIds, clipboard);
    if (!copyReceipt.accepted) {
      receipt.failedObjectId = copyReceipt.failedObjectId;
      reject(receipt, CreativeVolumeOperationStatus::CopyRejected,
             copyReceipt.reasonCode);
      return receipt;
    }

    CreativeClipboardPasteRequest pasteRequest;
    pasteRequest.offset = offset;
    pasteRequest.externalParentPolicy =
        CreativeClipboardExternalParentPolicy::PreserveIfPresent;
    const CreativeClipboardPasteReceipt pasteReceipt =
        pasteCreativeClipboardAtomically(staged, clipboard, pasteRequest);
    if (!pasteReceipt.accepted || !pasteReceipt.changed) {
      receipt.failedObjectId = pasteReceipt.failedObjectId;
      reject(receipt, CreativeVolumeOperationStatus::PasteRejected,
             pasteReceipt.reasonCode);
      return receipt;
    }
    receipt.createdObjectIds = pasteReceipt.pastedObjectIds;
  }

  if (!voxelCells.empty()) {
    std::vector<CreativeVoxelEdit> voxelEdits;
    voxelEdits.reserve(voxelCells.size());
    for (const CreativeVoxelCell& cell : voxelCells) {
      CreativeGridCoord3 targetCell;
      if (!checkedAddCellCoordinate(cell.cell.x, voxelOffset.x,
                                    targetCell.x) ||
          !checkedAddCellCoordinate(cell.cell.y, voxelOffset.y,
                                    targetCell.y) ||
          !checkedAddCellCoordinate(cell.cell.z, voxelOffset.z,
                                    targetCell.z)) {
        reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
               "creative_volume_clone_voxel_coordinate_overflow");
        return receipt;
      }
      voxelEdits.push_back({targetCell, cell.material});
    }
    const CreativeVoxelMutationReceipt voxelReceipt =
        staged.applyVoxelEdits(voxelEdits);
    if (!voxelReceipt.accepted) {
      reject(receipt, CreativeVolumeOperationStatus::VoxelMutationRejected,
             voxelReceipt.reasonCode);
      return receipt;
    }
    copyVoxelMutationFacts(receipt, voxelReceipt);
  }

  if (receipt.createdObjectIds.empty() &&
      receipt.createdVoxelCellCount == 0U &&
      receipt.replacedVoxelCellCount == 0U) {
    acceptNoChange(receipt, "creative_volume_clone_no_change");
    return receipt;
  }
  document = std::move(staged);
  acceptApplied(receipt, document, "creative_volume_clone_applied");
  return receipt;
}

}  // namespace iggy3d::creative::volume_internal
