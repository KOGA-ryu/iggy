#include "app/iggy3d/creative/tools/VolumeInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <unordered_set>
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

[[nodiscard]] bool hasWorldLayoutOwnershipTag(
    const CreativeObject& object) noexcept {
  return std::any_of(
      object.tags.begin(), object.tags.end(), [](const std::string& tag) {
        return std::string_view{tag}.starts_with("creative_world_layout:");
      });
}

[[nodiscard]] const CreativePatternRecipe* patternRecipeForObject(
    const CreativeDocument& document,
    CreativeObjectId objectId) noexcept {
  const auto referencesObject = [objectId](const CreativePatternRecipe& recipe) {
    return std::find(recipe.sourceObjectIds.begin(),
                     recipe.sourceObjectIds.end(), objectId) !=
               recipe.sourceObjectIds.end() ||
           std::find(recipe.generatedObjectIds.begin(),
                     recipe.generatedObjectIds.end(), objectId) !=
               recipe.generatedObjectIds.end();
  };
  const auto found = std::find_if(document.patternRecipeStore().recipes.begin(),
                                  document.patternRecipeStore().recipes.end(),
                                  referencesObject);
  return found == document.patternRecipeStore().recipes.end() ? nullptr
                                                               : &*found;
}

[[nodiscard]] bool appendUniqueBounded(
    std::vector<CreativeObjectId>& values,
    CreativeObjectId value,
    std::uint64_t limit) {
  if (value == kInvalidObjectId ||
      std::find(values.begin(), values.end(), value) != values.end()) {
    return true;
  }
  if (values.size() >= limit) {
    return false;
  }
  values.push_back(value);
  return true;
}

void expandPatternCloneSeeds(const CreativeDocument& document,
                             std::vector<CreativeObjectId>& objectIds) {
  std::unordered_set<CreativeObjectId> closure(objectIds.begin(),
                                                objectIds.end());
  bool changed = true;
  while (changed) {
    changed = false;
    for (const CreativePatternRecipe& recipe :
         document.patternRecipeStore().recipes) {
      const auto selected = [&closure](CreativeObjectId objectId) {
        return closure.contains(objectId);
      };
      if (std::none_of(recipe.sourceObjectIds.begin(),
                       recipe.sourceObjectIds.end(), selected) &&
          std::none_of(recipe.generatedObjectIds.begin(),
                       recipe.generatedObjectIds.end(), selected)) {
        continue;
      }
      for (CreativeObjectId objectId : recipe.sourceObjectIds) {
        changed = closure.insert(objectId).second || changed;
      }
      for (CreativeObjectId objectId : recipe.generatedObjectIds) {
        changed = closure.insert(objectId).second || changed;
      }
    }
  }

  objectIds.clear();
  objectIds.reserve(closure.size());
  for (const CreativeObject& object : document.objects()) {
    if (closure.contains(object.id)) {
      objectIds.push_back(object.id);
    }
  }
}

[[nodiscard]] bool cloneClosureHasExternalReferences(
    const CreativeDocument& document,
    const CreativeClipboard& clipboard,
    std::uint64_t limit,
    CreativeVolumeOperationReceipt& receipt) {
  std::unordered_set<CreativeObjectId> closure;
  closure.reserve(clipboard.objects.size());
  for (const CreativeObject& object : clipboard.objects) {
    closure.insert(object.id);
  }

  bool withinLimit = true;
  for (const CreativeObject& object : clipboard.objects) {
    if (object.parentId.has_value() && !closure.contains(*object.parentId)) {
      withinLimit = appendUniqueBounded(receipt.blockedObjectIds, object.id,
                                        limit) &&
                    withinLimit;
      withinLimit = appendUniqueBounded(receipt.dependentSourceObjectIds,
                                        *object.parentId, limit) &&
                    withinLimit;
    }
  }
  for (const CreativeObject& object : document.objects()) {
    if (object.parentId.has_value() && closure.contains(*object.parentId) &&
        !closure.contains(object.id)) {
      withinLimit = appendUniqueBounded(receipt.blockedObjectIds,
                                        *object.parentId, limit) &&
                    withinLimit;
      withinLimit = appendUniqueBounded(receipt.dependentSourceObjectIds,
                                        object.id, limit) &&
                    withinLimit;
    }
  }
  for (const CreativeLogicLink& link : document.logicLinks()) {
    const bool sourceInside = closure.contains(link.sourceObjectId);
    const bool targetInside = closure.contains(link.targetObjectId);
    if (sourceInside == targetInside) {
      continue;
    }
    withinLimit = appendUniqueBounded(
                      receipt.blockedObjectIds,
                      sourceInside ? link.sourceObjectId : link.targetObjectId,
                      limit) &&
                  withinLimit;
    withinLimit = appendUniqueBounded(
                      receipt.dependentSourceObjectIds,
                      sourceInside ? link.targetObjectId : link.sourceObjectId,
                      limit) &&
                  withinLimit;
  }
  receipt.dependentSourceObjectCount =
      receipt.dependentSourceObjectIds.size();
  return withinLimit;
}

[[nodiscard]] bool cloneVoxelTargetCell(
    CreativeGridCoord3 source,
    CreativeGridBounds3 sourceBounds,
    CreativeGridCoord3 offset,
    std::uint8_t quarterTurns,
    bool mirrorX,
    bool mirrorZ,
    CreativeGridCoord3& target) noexcept {
  std::int64_t localX =
      2LL * (static_cast<std::int64_t>(source.x) - sourceBounds.min.x) + 1LL;
  std::int64_t localZ =
      2LL * (static_cast<std::int64_t>(source.z) - sourceBounds.min.z) + 1LL;
  if (mirrorX) {
    localX = -localX;
  }
  if (mirrorZ) {
    localZ = -localZ;
  }
  const std::int64_t beforeX = localX;
  switch (quarterTurns) {
    case 0U: break;
    case 1U:
      localX = localZ;
      localZ = -beforeX;
      break;
    case 2U:
      localX = -localX;
      localZ = -localZ;
      break;
    case 3U:
      localX = -localZ;
      localZ = beforeX;
      break;
    default: return false;
  }

  const std::int64_t targetCenterX =
      2LL * sourceBounds.min.x + localX + 2LL * offset.x;
  const std::int64_t targetCenterY =
      2LL * static_cast<std::int64_t>(source.y) + 1LL + 2LL * offset.y;
  const std::int64_t targetCenterZ =
      2LL * sourceBounds.min.z + localZ + 2LL * offset.z;
  if ((targetCenterX & 1LL) == 0LL || (targetCenterY & 1LL) == 0LL ||
      (targetCenterZ & 1LL) == 0LL) {
    return false;
  }
  const std::int64_t targetX = (targetCenterX - 1LL) / 2LL;
  const std::int64_t targetY = (targetCenterY - 1LL) / 2LL;
  const std::int64_t targetZ = (targetCenterZ - 1LL) / 2LL;
  if (targetX < std::numeric_limits<std::int32_t>::min() ||
      targetX > std::numeric_limits<std::int32_t>::max() ||
      targetY < std::numeric_limits<std::int32_t>::min() ||
      targetY > std::numeric_limits<std::int32_t>::max() ||
      targetZ < std::numeric_limits<std::int32_t>::min() ||
      targetZ > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  target = {static_cast<std::int32_t>(targetX),
            static_cast<std::int32_t>(targetY),
            static_cast<std::int32_t>(targetZ)};
  return true;
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
  if (request.hasEraseKindFilter &&
      !validObjectKind(request.eraseKindFilter)) {
    reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
           "creative_volume_erase_filter_invalid");
    return receipt;
  }

  const std::vector<CreativeObjectId> containedIds =
      containedObjectIds(document, request.selection);
  const std::vector<CreativeVoxelCell> containedVoxels =
      collectCreativeVoxelCells(
      document.voxelField(), inclusiveVolumeGridBounds(request.selection));
  if (affectedCountExceeds(containedIds.size(), containedVoxels.size(),
                           request.maxAffectedObjects)) {
    reject(receipt, CreativeVolumeOperationStatus::OperationLimitExceeded,
           "creative_volume_erase_limit_exceeded");
    return receipt;
  }

  std::vector<CreativeObjectId> objectIds;
  objectIds.reserve(containedIds.size());
  bool dependencyFactsWithinLimit = true;
  for (CreativeObjectId objectId : containedIds) {
    const CreativeObject* object = document.findObject(objectId);
    if (object == nullptr) {
      receipt.failedObjectId = objectId;
      reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
             "creative_volume_erase_object_missing");
      return receipt;
    }
    if (!creativeVolumeMemberMaskIncludesObjects(request.eraseMemberMask) ||
        (request.hasEraseKindFilter &&
         object->kind != request.eraseKindFilter)) {
      ++receipt.excludedObjectCount;
      continue;
    }

    const CreativePatternRecipe* recipe =
        patternRecipeForObject(document, objectId);
    if (recipe != nullptr || hasWorldLayoutOwnershipTag(*object)) {
      ++receipt.excludedObjectCount;
      receipt.protectedObjectIds.push_back(objectId);
      if (recipe != nullptr) {
        for (CreativeObjectId sourceId : recipe->sourceObjectIds) {
          dependencyFactsWithinLimit =
              appendUniqueBounded(receipt.dependentSourceObjectIds, sourceId,
                                  request.maxAffectedObjects) &&
              dependencyFactsWithinLimit;
        }
      }
      continue;
    }
    objectIds.push_back(objectId);
  }
  receipt.protectedObjectCount = receipt.protectedObjectIds.size();

  std::vector<CreativeVoxelCell> voxelCells;
  voxelCells.reserve(containedVoxels.size());
  for (const CreativeVoxelCell& cell : containedVoxels) {
    if (!creativeVolumeMemberMaskIncludesVoxels(request.eraseMemberMask) ||
        (request.hasEraseKindFilter &&
         cell.material != request.eraseKindFilter)) {
      ++receipt.excludedVoxelCellCount;
      continue;
    }
    voxelCells.push_back(cell);
    receipt.removedVoxelCells.push_back(cell.cell);
  }
  receipt.matchedObjectCount = objectIds.size();
  receipt.matchedVoxelCellCount = voxelCells.size();

  std::erase_if(receipt.dependentSourceObjectIds,
                [&receipt](CreativeObjectId objectId) {
                  return std::find(receipt.protectedObjectIds.begin(),
                                   receipt.protectedObjectIds.end(), objectId) !=
                         receipt.protectedObjectIds.end();
                });
  receipt.dependentSourceObjectCount =
      receipt.dependentSourceObjectIds.size();
  if (!dependencyFactsWithinLimit) {
    reject(receipt, CreativeVolumeOperationStatus::OperationLimitExceeded,
           "creative_volume_erase_dependency_limit_exceeded");
    return receipt;
  }

  if (objectIds.empty() && voxelCells.empty()) {
    acceptNoChange(receipt,
                   receipt.protectedObjectCount > 0U
                       ? "creative_volume_erase_source_owned"
                       : "creative_volume_erase_filtered_empty");
    return receipt;
  }

  std::unordered_set<CreativeObjectId> objectSet(objectIds.begin(),
                                                  objectIds.end());
  for (CreativeObjectId objectId : objectIds) {
    const CreativeObject* object = document.findObject(objectId);
    if (object != nullptr && object->locked) {
      receipt.blockedObjectIds.push_back(objectId);
    }
    for (const CreativeObject& candidate : document.objects()) {
      if (candidate.parentId == objectId && !objectSet.contains(candidate.id)) {
        static_cast<void>(appendUniqueBounded(
            receipt.blockedObjectIds, objectId, request.maxAffectedObjects));
        dependencyFactsWithinLimit =
            appendUniqueBounded(receipt.dependentSourceObjectIds, candidate.id,
                                request.maxAffectedObjects) &&
            dependencyFactsWithinLimit;
      }
    }
  }
  for (const CreativeLogicLink& link : document.logicLinks()) {
    const bool sourceSelected = objectSet.contains(link.sourceObjectId);
    const bool targetSelected = objectSet.contains(link.targetObjectId);
    if (sourceSelected == targetSelected) {
      continue;
    }
    const CreativeObjectId selectedId =
        sourceSelected ? link.sourceObjectId : link.targetObjectId;
    const CreativeObjectId dependentId =
        sourceSelected ? link.targetObjectId : link.sourceObjectId;
    dependencyFactsWithinLimit =
        appendUniqueBounded(receipt.blockedObjectIds, selectedId,
                            request.maxAffectedObjects) &&
        dependencyFactsWithinLimit;
    dependencyFactsWithinLimit =
        appendUniqueBounded(receipt.dependentSourceObjectIds, dependentId,
                            request.maxAffectedObjects) &&
        dependencyFactsWithinLimit;
  }
  receipt.dependentSourceObjectCount =
      receipt.dependentSourceObjectIds.size();
  if (!dependencyFactsWithinLimit) {
    reject(receipt, CreativeVolumeOperationStatus::OperationLimitExceeded,
           "creative_volume_erase_dependency_limit_exceeded");
    return receipt;
  }
  if (!receipt.blockedObjectIds.empty()) {
    receipt.failedObjectId = receipt.blockedObjectIds.front();
    reject(receipt, CreativeVolumeOperationStatus::RelationshipRejected,
           "creative_volume_erase_dependency_rejected");
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

  if (!document.commitStagedMutation(std::move(staged))) {
    reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
           "creative_volume_erase_commit_rejected");
    return receipt;
  }
  acceptApplied(receipt, document, "creative_volume_erase_applied");
  return receipt;
}

CreativeVolumeOperationReceipt cloneVolumeObjects(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt) {
  if (request.cloneMemberMask >= CreativeVolumeMemberMask::Count ||
      request.cloneVoxelOverlapPolicy >=
          CreativeVolumeCloneVoxelOverlapPolicy::Count ||
      request.cloneQuarterTurns > 3U) {
    reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
           "creative_volume_clone_settings_invalid");
    return receipt;
  }

  std::vector<CreativeObjectId> objectIds;
  const std::vector<CreativeObjectId> containedIds =
      containedObjectIds(document, request.selection);
  objectIds.reserve(containedIds.size());
  for (CreativeObjectId objectId : containedIds) {
    const CreativeObject* object = document.findObject(objectId);
    if (!creativeVolumeMemberMaskIncludesObjects(request.cloneMemberMask)) {
      ++receipt.excludedObjectCount;
    } else if (object == nullptr) {
      receipt.failedObjectId = objectId;
      reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
             "creative_volume_clone_object_missing");
      return receipt;
    } else if (hasWorldLayoutOwnershipTag(*object)) {
      ++receipt.excludedObjectCount;
      receipt.protectedObjectIds.push_back(objectId);
    } else {
      objectIds.push_back(objectId);
    }
  }
  receipt.protectedObjectCount = receipt.protectedObjectIds.size();
  if (!objectIds.empty()) {
    expandPatternCloneSeeds(document, objectIds);
  }

  std::vector<CreativeVoxelCell> voxelCells = collectCreativeVoxelCells(
      document.voxelField(), inclusiveVolumeGridBounds(request.selection));
  if (!creativeVolumeMemberMaskIncludesVoxels(request.cloneMemberMask)) {
    receipt.excludedVoxelCellCount = voxelCells.size();
    voxelCells.clear();
  }
  receipt.matchedVoxelCellCount = voxelCells.size();

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

  CreativeClipboard clipboard;
  if (!objectIds.empty()) {
    const CreativeClipboardCopyReceipt copyReceipt =
        copyDocumentObjectsToClipboard(document, objectIds, clipboard);
    if (!copyReceipt.accepted) {
      receipt.failedObjectId = copyReceipt.failedObjectId;
      reject(receipt, CreativeVolumeOperationStatus::CopyRejected,
             copyReceipt.reasonCode);
      return receipt;
    }
    for (const CreativeObject& object : clipboard.objects) {
      if (hasWorldLayoutOwnershipTag(object)) {
        receipt.failedObjectId = object.id;
        receipt.protectedObjectIds.push_back(object.id);
        receipt.protectedObjectCount = receipt.protectedObjectIds.size();
        reject(receipt, CreativeVolumeOperationStatus::RelationshipRejected,
               "creative_volume_clone_generated_source_rejected");
        return receipt;
      }
    }
    receipt.matchedObjectCount = clipboard.objects.size();
    if (affectedCountExceeds(clipboard.objects.size(), voxelCells.size(),
                             request.maxAffectedObjects)) {
      reject(receipt, CreativeVolumeOperationStatus::OperationLimitExceeded,
             "creative_volume_clone_limit_exceeded");
      return receipt;
    }
    if (!cloneClosureHasExternalReferences(document, clipboard,
                                           request.maxAffectedObjects,
                                           receipt)) {
      reject(receipt, CreativeVolumeOperationStatus::OperationLimitExceeded,
             "creative_volume_clone_dependency_limit_exceeded");
      return receipt;
    }
    if (!receipt.blockedObjectIds.empty()) {
      receipt.failedObjectId = receipt.blockedObjectIds.front();
      reject(receipt, CreativeVolumeOperationStatus::RelationshipRejected,
             "creative_volume_clone_external_reference_rejected");
      return receipt;
    }
    if (!clipboard.patternRecipes.empty() &&
        (request.cloneQuarterTurns != 0U || request.cloneMirrorX ||
         request.cloneMirrorZ)) {
      reject(receipt, CreativeVolumeOperationStatus::RelationshipRejected,
             "creative_volume_clone_pattern_transform_unsupported");
      return receipt;
    }
  } else if (affectedCountExceeds(0U, voxelCells.size(),
                                  request.maxAffectedObjects)) {
    reject(receipt, CreativeVolumeOperationStatus::OperationLimitExceeded,
           "creative_volume_clone_limit_exceeded");
    return receipt;
  }

  const CreativeGridBounds3 sourceBounds =
      inclusiveVolumeGridBounds(request.selection);
  std::vector<CreativeVoxelEdit> voxelEdits;
  voxelEdits.reserve(voxelCells.size());
  for (const CreativeVoxelCell& cell : voxelCells) {
    CreativeGridCoord3 targetCell;
    if (!cloneVoxelTargetCell(cell.cell, sourceBounds, voxelOffset,
                              request.cloneQuarterTurns,
                              request.cloneMirrorX, request.cloneMirrorZ,
                              targetCell)) {
      reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
             "creative_volume_clone_voxel_coordinate_overflow");
      return receipt;
    }
    const CreativeObjectKind existing =
        document.voxelField().materialAt(targetCell);
    if (existing != CreativeObjectKind::Unknown) {
      switch (request.cloneVoxelOverlapPolicy) {
        case CreativeVolumeCloneVoxelOverlapPolicy::RejectOccupied:
          receipt.blockedVoxelCells.push_back(targetCell);
          continue;
        case CreativeVolumeCloneVoxelOverlapPolicy::PreserveExisting:
          ++receipt.skippedOccupiedCellCount;
          receipt.unchangedVoxelCells.push_back(targetCell);
          continue;
        case CreativeVolumeCloneVoxelOverlapPolicy::ReplaceExisting:
          if (existing == cell.material) {
            ++receipt.unchangedMaterialCellCount;
            receipt.unchangedVoxelCells.push_back(targetCell);
            continue;
          }
          break;
        case CreativeVolumeCloneVoxelOverlapPolicy::Count:
          break;
      }
    } else {
      receipt.createdVoxelCells.push_back(targetCell);
    }
    receipt.changedVoxelCells.push_back(targetCell);
    voxelEdits.push_back({targetCell, cell.material});
  }
  if (!receipt.blockedVoxelCells.empty()) {
    reject(receipt, CreativeVolumeOperationStatus::RelationshipRejected,
           "creative_volume_clone_voxel_occupied");
    return receipt;
  }
  if (clipboard.objects.empty() && voxelEdits.empty()) {
    acceptNoChange(receipt,
                   receipt.protectedObjectCount > 0U
                       ? "creative_volume_clone_source_owned"
                       : "creative_volume_clone_no_change");
    return receipt;
  }

  CreativeDocument staged = document;
  if (!clipboard.objects.empty()) {

    CreativeClipboardPasteRequest pasteRequest;
    pasteRequest.offset = offset;
    pasteRequest.hasTransformAnchor = true;
    pasteRequest.transformAnchor =
        creativeVolumeWorldBounds(request.selection).min;
    pasteRequest.quarterTurns = request.cloneQuarterTurns;
    pasteRequest.mirrorX = request.cloneMirrorX;
    pasteRequest.mirrorZ = request.cloneMirrorZ;
    pasteRequest.externalParentPolicy =
        CreativeClipboardExternalParentPolicy::Detach;
    const CreativeClipboardPasteReceipt pasteReceipt =
        pasteCreativeClipboardAtomically(staged, clipboard, pasteRequest);
    if (!pasteReceipt.accepted || !pasteReceipt.changed) {
      receipt.failedObjectId = pasteReceipt.failedObjectId;
      reject(receipt, CreativeVolumeOperationStatus::PasteRejected,
             pasteReceipt.reasonCode);
      return receipt;
    }
    receipt.createdObjectIds = pasteReceipt.pastedObjectIds;
    receipt.clonedLogicLinkCount = pasteReceipt.pastedLogicLinkCount;
    receipt.clonedPatternRecipeCount =
        pasteReceipt.pastedPatternRecipeCount;
    receipt.clonedObjectIdRemaps.reserve(pasteReceipt.idRemaps.size());
    for (const CreativeClipboardIdRemap& remap : pasteReceipt.idRemaps) {
      receipt.clonedObjectIdRemaps.push_back(
          {remap.sourceObjectId, remap.pastedObjectId});
    }
    receipt.clonedPatternRecipeIdRemaps.reserve(
        pasteReceipt.patternRecipeIdRemaps.size());
    for (const CreativeClipboardPatternRecipeIdRemap& remap :
         pasteReceipt.patternRecipeIdRemaps) {
      receipt.clonedPatternRecipeIdRemaps.push_back(
          {remap.sourceRecipeId, remap.pastedRecipeId});
    }
  }

  if (!voxelEdits.empty()) {
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
  if (!document.commitStagedMutation(std::move(staged))) {
    reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
           "creative_volume_clone_commit_rejected");
    return receipt;
  }
  acceptApplied(receipt, document, "creative_volume_clone_applied");
  return receipt;
}

}  // namespace iggy3d::creative::volume_internal
