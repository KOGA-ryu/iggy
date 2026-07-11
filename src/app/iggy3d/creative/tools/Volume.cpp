#include "app/iggy3d/creative/tools/Volume.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/tools/Clipboard.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

namespace iggy3d::creative {
namespace {

struct CellKey {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::int32_t z = 0;

  [[nodiscard]] bool operator==(const CellKey&) const noexcept = default;
};

struct CellKeyHash {
  [[nodiscard]] std::size_t operator()(CellKey key) const noexcept {
    const std::size_t x = static_cast<std::uint32_t>(key.x);
    const std::size_t y = static_cast<std::uint32_t>(key.y);
    const std::size_t z = static_cast<std::uint32_t>(key.z);
    std::size_t hash = x * 0x9e3779b1U;
    hash ^= y + 0x9e3779b9U + (hash << 6U) + (hash >> 2U);
    hash ^= z + 0x85ebca6bU + (hash << 6U) + (hash >> 2U);
    return hash;
  }
};

[[nodiscard]] bool validObjectKind(CreativeObjectKind kind) noexcept {
  return kind != CreativeObjectKind::Unknown &&
         kind != CreativeObjectKind::Count;
}

[[nodiscard]] bool validOperation(
    CreativeVolumeOperationKind operation) noexcept {
  return operation < CreativeVolumeOperationKind::Count;
}

[[nodiscard]] bool sameDouble(double lhs, double rhs, double epsilon) noexcept {
  return std::fabs(lhs - rhs) <= epsilon;
}

[[nodiscard]] bool sameBounds(CreativeBounds lhs,
                              CreativeBounds rhs,
                              double epsilon) noexcept {
  return sameDouble(lhs.min.x, rhs.min.x, epsilon) &&
         sameDouble(lhs.min.y, rhs.min.y, epsilon) &&
         sameDouble(lhs.min.z, rhs.min.z, epsilon) &&
         sameDouble(lhs.max.x, rhs.max.x, epsilon) &&
         sameDouble(lhs.max.y, rhs.max.y, epsilon) &&
         sameDouble(lhs.max.z, rhs.max.z, epsilon);
}

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

[[nodiscard]] bool objectInsideVolume(const CreativeObject& object,
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

[[nodiscard]] bool hasTag(const CreativeObject& object,
                          std::string_view tag) {
  return std::find(object.tags.begin(), object.tags.end(), tag) !=
         object.tags.end();
}

void ensureTag(std::vector<std::string>& tags, std::string_view tag) {
  if (std::find(tags.begin(), tags.end(), tag) == tags.end()) {
    tags.emplace_back(tag);
  }
}

[[nodiscard]] CellKey toCellKey(CreativeGridCoord3 cell) noexcept {
  return {cell.x, cell.y, cell.z};
}

[[nodiscard]] bool volumeCellForObject(
    const CreativeObject& object,
    const CreativeVolumeSelection& selection,
    CreativeGridCoord3& outCell) noexcept {
  if (!hasTag(object, kCreativeVolumeCellTag) ||
      !describeObject(object.kind).hasBounds ||
      !tryCreativeVolumeCellFromWorld(object.bounds.min, selection.cellSize,
                                      selection.origin, outCell)) {
    return false;
  }
  const CreativeBounds expected = creativeVolumeCellBounds(
      outCell, selection.cellSize, selection.origin);
  const double epsilon = std::max(1.0, selection.cellSize) * 1.0e-9;
  return sameBounds(object.bounds, expected, epsilon);
}

[[nodiscard]] CreativeDocumentCreateRequest makeVolumeCellRequest(
    CreativeObjectKind kind,
    CreativeGridCoord3 cell,
    const CreativeVolumeSelection& selection,
    const CreativeObject* replacedObject = nullptr) {
  const CreativeObjectDescriptor& descriptor = describeObject(kind);
  const CreativeBounds cellBounds = creativeVolumeCellBounds(
      cell, selection.cellSize, selection.origin);
  const CreativeVec3 center = measureCreativeBounds(cellBounds).center;

  CreativeDocumentCreateRequest create;
  create.kind = kind;
  create.name = std::string(toString(kind)) + " volume " +
                std::to_string(cell.x) + "," + std::to_string(cell.y) + "," +
                std::to_string(cell.z);
  create.transform = descriptor.defaults.transform;
  create.transform.position = center;
  create.hasTransformOverride = descriptor.hasTransform;
  create.bounds = cellBounds;
  create.hasBoundsOverride = true;
  create.layerId = replacedObject != nullptr ? replacedObject->layerId
                                              : descriptor.defaults.layerId;
  create.hasLayerOverride = true;
  create.visible = replacedObject != nullptr ? replacedObject->visible : true;
  create.hasVisibleOverride = true;
  create.locked = false;
  create.hasLockedOverride = true;
  if (replacedObject != nullptr) {
    create.tags = replacedObject->tags;
  }
  ensureTag(create.tags, kCreativeVolumeCellTag);
  return create;
}

void acceptNoChange(CreativeVolumeOperationReceipt& receipt,
                    std::string_view reasonCode) noexcept {
  receipt.accepted = true;
  receipt.changed = false;
  receipt.status = CreativeVolumeOperationStatus::NoChange;
  receipt.reasonCode = reasonCode;
}

void acceptApplied(CreativeVolumeOperationReceipt& receipt,
                   const CreativeDocument& document,
                   std::string_view reasonCode) noexcept {
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeVolumeOperationStatus::Applied;
  receipt.revisionAfter = document.revision();
  receipt.createdObjectCount = receipt.createdObjectIds.size();
  receipt.removedObjectCount = receipt.removedObjectIds.size();
  receipt.reasonCode = reasonCode;
}

void reject(CreativeVolumeOperationReceipt& receipt,
            CreativeVolumeOperationStatus status,
            std::string_view reasonCode) noexcept {
  receipt.accepted = false;
  receipt.changed = false;
  receipt.status = status;
  receipt.createdObjectIds.clear();
  receipt.removedObjectIds.clear();
  receipt.createdObjectCount = 0;
  receipt.removedObjectCount = 0;
  receipt.createdVoxelCellCount = 0;
  receipt.removedVoxelCellCount = 0;
  receipt.replacedVoxelCellCount = 0;
  receipt.dirtyVoxelChunkCount = 0;
  receipt.reasonCode = reasonCode;
}

void copyVoxelMutationFacts(
    CreativeVolumeOperationReceipt& receipt,
    const CreativeVoxelMutationReceipt& voxelReceipt) noexcept {
  receipt.createdVoxelCellCount += voxelReceipt.createdCellCount;
  receipt.removedVoxelCellCount += voxelReceipt.removedCellCount;
  receipt.replacedVoxelCellCount += voxelReceipt.replacedCellCount;
  receipt.dirtyVoxelChunkCount += voxelReceipt.dirtyChunks.size();
}

[[nodiscard]] bool affectedCountExceeds(
    std::size_t objectCount,
    std::size_t voxelCount,
    std::uint64_t limit) noexcept {
  return objectCount > limit || voxelCount > limit - objectCount;
}

[[nodiscard]] CreativeGridBounds3 inclusiveVolumeGridBounds(
    const CreativeVolumeSelection& selection) noexcept {
  CreativeGridBounds3 bounds = creativeVolumeGridBounds(selection);
  --bounds.max.x;
  --bounds.max.y;
  --bounds.max.z;
  return bounds;
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

[[nodiscard]] CreativeVolumeOperationReceipt fillVolume(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt,
    bool hollow) {
  if (!creativeVolumeBrushSupported(request.objectKind)) {
    reject(receipt, CreativeVolumeOperationStatus::UnsupportedObjectKind,
           "creative_volume_brush_unsupported");
    return receipt;
  }

  CreativeShapeBrushPlanRequest filledRequest;
  filledRequest.kind = request.shapeKind;
  filledRequest.axis = request.shapeAxis;
  filledRequest.firstCell = request.selection.firstCell;
  filledRequest.secondCell = request.selection.secondCell;
  filledRequest.maxCandidateCellCount = request.maxAffectedObjects;
  filledRequest.maxGeneratedCellCount = request.maxAffectedObjects;
  const CreativeShapeBrushPlanReceipt filledPlan =
      planCreativeShapeBrush(filledRequest);
  receipt.shapeCandidateCellCount = filledPlan.candidateCellCount;
  if (!filledPlan.accepted) {
    const bool limitExceeded =
        filledPlan.status ==
            CreativeShapeBrushPlanStatus::CandidateLimitExceeded ||
        filledPlan.status ==
            CreativeShapeBrushPlanStatus::GeneratedLimitExceeded;
    reject(receipt,
           limitExceeded
               ? CreativeVolumeOperationStatus::OperationLimitExceeded
               : CreativeVolumeOperationStatus::InvalidRequest,
           limitExceeded ? "creative_volume_shape_limit_exceeded"
                         : "creative_volume_shape_plan_invalid");
    return receipt;
  }

  CreativeShapeBrushPlanReceipt hollowPlan;
  const CreativeShapeBrushPlanReceipt* plannedShape = &filledPlan;
  if (hollow) {
    CreativeShapeBrushPlanRequest hollowRequest = filledRequest;
    hollowRequest.hollow = true;
    hollowPlan = planCreativeShapeBrush(hollowRequest);
    if (!hollowPlan.accepted) {
      const bool limitExceeded =
          hollowPlan.status ==
              CreativeShapeBrushPlanStatus::CandidateLimitExceeded ||
          hollowPlan.status ==
              CreativeShapeBrushPlanStatus::GeneratedLimitExceeded;
      reject(receipt,
             limitExceeded
                 ? CreativeVolumeOperationStatus::OperationLimitExceeded
                 : CreativeVolumeOperationStatus::InvalidRequest,
             limitExceeded ? "creative_volume_shape_limit_exceeded"
                           : "creative_volume_shape_plan_invalid");
      return receipt;
    }
    plannedShape = &hollowPlan;
  }
  receipt.plannedCellCount = plannedShape->generatedCellCount;

  std::unordered_set<CellKey, CellKeyHash> filledCells;
  std::unordered_set<CellKey, CellKeyHash> plannedCells;
  if (hollow) {
    filledCells.reserve(filledPlan.cells.size());
    for (const CreativeGridCoord3 cell : filledPlan.generatedCells()) {
      filledCells.insert(toCellKey(cell));
    }
    plannedCells.reserve(plannedShape->cells.size());
    for (const CreativeGridCoord3 cell : plannedShape->generatedCells()) {
      plannedCells.insert(toCellKey(cell));
    }
  }
  std::unordered_set<CellKey, CellKeyHash> occupiedLegacyCells;
  occupiedLegacyCells.reserve(document.objects().size());
  std::vector<CreativeObjectId> interiorObjectIds;
  for (const CreativeObject& object : document.objects()) {
    CreativeGridCoord3 cell;
    if (volumeCellForObject(object, request.selection, cell)) {
      const CellKey key = toCellKey(cell);
      occupiedLegacyCells.insert(key);
      if (hollow && filledCells.contains(key)) {
        if (!plannedCells.contains(key)) {
          interiorObjectIds.push_back(object.id);
        }
      }
    }
  }

  std::vector<CreativeVoxelEdit> voxelEdits;
  voxelEdits.reserve(filledPlan.cells.size());
  if (hollow) {
    for (const CreativeGridCoord3 cell : filledPlan.generatedCells()) {
      const CellKey key = toCellKey(cell);
      if (plannedCells.contains(key)) {
        if (document.voxelField().occupied(cell) ||
            occupiedLegacyCells.contains(key)) {
          ++receipt.skippedOccupiedCellCount;
          continue;
        }
        voxelEdits.push_back({cell, request.objectKind});
      } else if (document.voxelField().occupied(cell)) {
        voxelEdits.push_back({cell, CreativeObjectKind::Unknown});
        ++receipt.matchedVoxelCellCount;
      }
    }
  } else {
    for (const CreativeGridCoord3 cell : plannedShape->generatedCells()) {
      if (document.voxelField().occupied(cell) ||
          occupiedLegacyCells.contains(toCellKey(cell))) {
        ++receipt.skippedOccupiedCellCount;
        continue;
      }
      voxelEdits.push_back({cell, request.objectKind});
    }
  }

  if (affectedCountExceeds(interiorObjectIds.size(), voxelEdits.size(),
                           request.maxAffectedObjects)) {
    reject(receipt, CreativeVolumeOperationStatus::OperationLimitExceeded,
           hollow ? "creative_volume_hollow_limit_exceeded"
                  : "creative_volume_fill_limit_exceeded");
    return receipt;
  }

  CreativeDocument staged = document;
  if (!interiorObjectIds.empty()) {
    CreativeClipboard removedObjects;
    const CreativeClipboardCutReceipt cutReceipt =
        cutDocumentObjectsAtomically(staged, interiorObjectIds, removedObjects);
    if (!cutReceipt.accepted || !cutReceipt.changed) {
      receipt.failedObjectId = cutReceipt.failedObjectId;
      reject(receipt, CreativeVolumeOperationStatus::RemoveRejected,
             cutReceipt.reasonCode);
      return receipt;
    }
    receipt.matchedObjectCount = interiorObjectIds.size();
    receipt.removedObjectIds = interiorObjectIds;
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

  if (receipt.createdVoxelCellCount == 0U &&
      receipt.removedVoxelCellCount == 0U &&
      receipt.replacedVoxelCellCount == 0U &&
      receipt.removedObjectIds.empty()) {
    acceptNoChange(receipt, "creative_volume_cells_already_occupied");
    return receipt;
  }
  document = std::move(staged);
  acceptApplied(receipt, document,
                hollow ? "creative_volume_hollow_applied"
                       : "creative_volume_fill_applied");
  return receipt;
}

[[nodiscard]] CreativeVolumeOperationReceipt replaceVolumeCells(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt) {
  if (!creativeVolumeBrushSupported(request.objectKind)) {
    reject(receipt, CreativeVolumeOperationStatus::UnsupportedObjectKind,
           "creative_volume_brush_unsupported");
    return receipt;
  }
  if (request.hasReplaceKindFilter &&
      !validObjectKind(request.replaceKindFilter)) {
    reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
           "creative_volume_replace_filter_invalid");
    return receipt;
  }

  const CreativeBounds worldBounds =
      creativeVolumeWorldBounds(request.selection);
  const CreativeGridBounds3 gridBounds =
      inclusiveVolumeGridBounds(request.selection);
  const std::vector<CreativeVoxelCell> voxelCandidates =
      collectCreativeVoxelCells(document.voxelField(), gridBounds);
  std::vector<CreativeVoxelEdit> voxelEdits;
  voxelEdits.reserve(voxelCandidates.size());
  for (const CreativeVoxelCell& cell : voxelCandidates) {
    if (request.hasReplaceKindFilter &&
        cell.material != request.replaceKindFilter) {
      continue;
    }
    ++receipt.matchedVoxelCellCount;
    if (cell.material != request.objectKind) {
      voxelEdits.push_back({cell.cell, request.objectKind});
    }
  }

  std::vector<CreativeObject> candidates;
  candidates.reserve(document.objects().size());
  for (const CreativeObject& object : document.objects()) {
    CreativeGridCoord3 cell;
    if (!volumeCellForObject(object, request.selection, cell) ||
        !objectInsideVolume(object, worldBounds) ||
        (request.hasReplaceKindFilter &&
         object.kind != request.replaceKindFilter)) {
      continue;
    }
    ++receipt.matchedObjectCount;
    if (object.kind != request.objectKind) {
      candidates.push_back(object);
    }
  }

  if (affectedCountExceeds(candidates.size(), voxelEdits.size(),
                           request.maxAffectedObjects)) {
    reject(receipt, CreativeVolumeOperationStatus::OperationLimitExceeded,
           "creative_volume_replace_limit_exceeded");
    return receipt;
  }
  if (candidates.empty() && voxelEdits.empty()) {
    acceptNoChange(receipt, "creative_volume_replace_no_change");
    return receipt;
  }

  for (const CreativeObject& candidate : candidates) {
    const bool ownsChild = std::any_of(
        document.objects().begin(), document.objects().end(),
        [&candidate](const CreativeObject& object) {
          return object.parentId == candidate.id;
        });
    if (candidate.locked || candidate.parentId.has_value() || ownsChild) {
      receipt.failedObjectId = candidate.id;
      reject(receipt, CreativeVolumeOperationStatus::RelationshipRejected,
             "creative_volume_replace_relationship_rejected");
      return receipt;
    }
  }

  CreativeDocument staged = document;
  receipt.removedObjectIds.reserve(candidates.size());
  for (const CreativeObject& candidate : candidates) {
    const CreativeDocumentRemoveReceipt removeReceipt =
        staged.removeDocumentObject(candidate.id);
    if (!removeReceipt.accepted || !removeReceipt.objectRemoved ||
        !removeReceipt.changed) {
      receipt.failedObjectId = candidate.id;
      reject(receipt, CreativeVolumeOperationStatus::RemoveRejected,
             removeReceipt.reasonCode);
      return receipt;
    }
    receipt.removedObjectIds.push_back(candidate.id);
  }

  receipt.createdObjectIds.reserve(candidates.size());
  for (const CreativeObject& candidate : candidates) {
    CreativeGridCoord3 cell;
    if (!volumeCellForObject(candidate, request.selection, cell)) {
      receipt.failedObjectId = candidate.id;
      reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
             "creative_volume_replace_cell_invalid");
      return receipt;
    }
    const CreativeDocumentCreateReceipt createReceipt = staged.createObject(
        makeVolumeCellRequest(request.objectKind, cell, request.selection,
                              &candidate));
    if (!createReceipt.accepted || !createReceipt.objectCreated ||
        !createReceipt.changed) {
      receipt.failedObjectId = candidate.id;
      reject(receipt, CreativeVolumeOperationStatus::CreateRejected,
             createReceipt.reasonCode);
      return receipt;
    }
    receipt.createdObjectIds.push_back(createReceipt.objectId);
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

  document = std::move(staged);
  acceptApplied(receipt, document, "creative_volume_replace_applied");
  return receipt;
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

[[nodiscard]] CreativeVolumeOperationReceipt eraseVolumeObjects(
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

[[nodiscard]] CreativeVolumeOperationReceipt cloneVolumeObjects(
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

using VolumeOperationHandler = CreativeVolumeOperationReceipt (*)(
    CreativeDocument&,
    const CreativeVolumeOperationRequest&,
    CreativeVolumeOperationReceipt);

[[nodiscard]] CreativeVolumeOperationReceipt fillHandler(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt) {
  return fillVolume(document, request, std::move(receipt), false);
}

[[nodiscard]] CreativeVolumeOperationReceipt hollowHandler(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt) {
  return fillVolume(document, request, std::move(receipt), true);
}

constexpr std::array<VolumeOperationHandler,
                     static_cast<std::size_t>(CreativeVolumeOperationKind::Count)>
    kVolumeOperationHandlers{
        fillHandler,
        hollowHandler,
        replaceVolumeCells,
        eraseVolumeObjects,
        cloneVolumeObjects,
    };

}  // namespace

std::string_view toString(CreativeVolumeOperationStatus status) noexcept {
  switch (status) {
    case CreativeVolumeOperationStatus::NotRequested: return "NotRequested";
    case CreativeVolumeOperationStatus::InvalidDocument: return "InvalidDocument";
    case CreativeVolumeOperationStatus::InvalidSelection: return "InvalidSelection";
    case CreativeVolumeOperationStatus::InvalidRequest: return "InvalidRequest";
    case CreativeVolumeOperationStatus::UnsupportedObjectKind:
      return "UnsupportedObjectKind";
    case CreativeVolumeOperationStatus::OperationLimitExceeded:
      return "OperationLimitExceeded";
    case CreativeVolumeOperationStatus::RelationshipRejected:
      return "RelationshipRejected";
    case CreativeVolumeOperationStatus::CreateRejected: return "CreateRejected";
    case CreativeVolumeOperationStatus::RemoveRejected: return "RemoveRejected";
    case CreativeVolumeOperationStatus::CopyRejected: return "CopyRejected";
    case CreativeVolumeOperationStatus::PasteRejected: return "PasteRejected";
    case CreativeVolumeOperationStatus::VoxelMutationRejected:
      return "VoxelMutationRejected";
    case CreativeVolumeOperationStatus::NoChange: return "NoChange";
    case CreativeVolumeOperationStatus::Applied: return "Applied";
  }
  return "Unknown";
}

bool creativeVolumeBrushSupported(CreativeObjectKind kind) noexcept {
  if (!validObjectKind(kind)) {
    return false;
  }
  const CreativeObjectDescriptor& descriptor = describeObject(kind);
  if (!descriptor.hasBounds ||
      !descriptorShowsInAuthoringBrushPalette(descriptor)) {
    return false;
  }
  return descriptor.placementPolicy.enabled &&
         descriptor.placementPolicy.storagePolicy ==
             CreativePlacementStoragePolicy::VoxelCell;
}

bool applyCreativeToolSettingsToVolumeRequest(
    CreativeVolumeOperationRequest& request,
    const CreativeToolSettings& settings) noexcept {
  if (!isValidCreativeToolSettings(settings)) {
    return false;
  }

  CreativeVolumeOperationRequest mapped = request;
  switch (request.operation) {
    case CreativeVolumeOperationKind::Fill:
    case CreativeVolumeOperationKind::Hollow:
      mapped.shapeKind = settings.shapeBrushKind;
      mapped.shapeAxis = settings.shapeBrushAxis;
      break;
    case CreativeVolumeOperationKind::Replace:
      mapped.hasReplaceKindFilter =
          settings.replaceSourceKind != CreativeObjectKind::Unknown;
      mapped.replaceKindFilter = settings.replaceSourceKind;
      break;
    case CreativeVolumeOperationKind::Clone: {
      CreativeToolWorldPoint offset;
      if (!tryCreativeCloneOffset(settings, request.selection.cellSize,
                                  offset)) {
        return false;
      }
      mapped.hasCloneOffset = true;
      mapped.cloneOffset = {offset.x, offset.y, offset.z};
      break;
    }
    case CreativeVolumeOperationKind::Erase:
      break;
    case CreativeVolumeOperationKind::Count:
      return false;
  }

  request = mapped;
  return true;
}

CreativeVolumeOperationReceipt executeCreativeVolumeOperation(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request) {
  CreativeVolumeOperationReceipt receipt;
  receipt.requested = true;
  receipt.operation = request.operation;
  receipt.shapeKind = request.shapeKind;
  receipt.shapeAxis = request.shapeAxis;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  receipt.volumeCellCount = creativeVolumeCellCount(request.selection);

  if (!document.isValid() || document.id() == kInvalidDocumentId) {
    reject(receipt, CreativeVolumeOperationStatus::InvalidDocument,
           "creative_volume_document_invalid");
    return receipt;
  }
  if (!creativeVolumeSelectionValid(request.selection)) {
    reject(receipt, CreativeVolumeOperationStatus::InvalidSelection,
           "creative_volume_selection_invalid");
    return receipt;
  }
  if (!validOperation(request.operation) ||
      request.maxAffectedObjects == 0U) {
    reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
           "creative_volume_request_invalid");
    return receipt;
  }

  const std::size_t operationIndex =
      static_cast<std::size_t>(request.operation);
  return kVolumeOperationHandlers[operationIndex](document, request,
                                                   std::move(receipt));
}

}  // namespace iggy3d::creative
