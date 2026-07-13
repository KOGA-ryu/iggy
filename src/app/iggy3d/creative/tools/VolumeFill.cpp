#include "app/iggy3d/creative/tools/VolumeInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/tools/Clipboard.hpp"

namespace iggy3d::creative::volume_internal {
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

}  // namespace

CreativeVolumeOperationReceipt replaceVolumeCells(
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

CreativeVolumeOperationReceipt fillHandler(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt) {
  return fillVolume(document, request, std::move(receipt), false);
}

CreativeVolumeOperationReceipt hollowHandler(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt) {
  return fillVolume(document, request, std::move(receipt), true);
}

}  // namespace iggy3d::creative::volume_internal
