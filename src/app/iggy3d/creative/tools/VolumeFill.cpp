#include "app/iggy3d/creative/tools/VolumeInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <unordered_map>
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

struct HollowPlanBounds {
  bool valid = false;
  CreativeGridCoord3 exteriorMinimum{};
  CreativeGridCoord3 exteriorMaximum{};
  CreativeGridBounds3 interiorBounds{};
  CreativeGridBounds3 exteriorBounds{};
  std::string_view reasonCode = "creative_volume_hollow_bounds_invalid";
};

[[nodiscard]] bool checkedCoord(std::int64_t value,
                                std::int32_t& output) noexcept {
  if (value < std::numeric_limits<std::int32_t>::min() ||
      value > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = static_cast<std::int32_t>(value);
  return true;
}

[[nodiscard]] HollowPlanBounds planHollowBounds(
    const CreativeVolumeOperationRequest& request) noexcept {
  HollowPlanBounds result;
  if (request.shapeKind == CreativeShapeBrushKind::Line) {
    result.reasonCode = "creative_volume_hollow_line_has_no_interior";
    return result;
  }
  const CreativeVolumeRegionFacts region =
      inspectCreativeVolumeRegion(request.selection);
  const std::uint8_t thickness =
      creativeVolumeHollowThicknessCells(request.hollowThickness);
  if (!region.valid || thickness == 0U) {
    return result;
  }

  const std::int64_t minX = region.inclusiveMinimum.x;
  const std::int64_t minY = region.inclusiveMinimum.y;
  const std::int64_t minZ = region.inclusiveMinimum.z;
  const std::int64_t maxX = region.inclusiveMaximum.x;
  const std::int64_t maxY = region.inclusiveMaximum.y;
  const std::int64_t maxZ = region.inclusiveMaximum.z;
  const std::int64_t shell = thickness;

  std::int64_t exteriorMinX = minX;
  std::int64_t exteriorMinY = minY;
  std::int64_t exteriorMinZ = minZ;
  std::int64_t exteriorMaxX = maxX;
  std::int64_t exteriorMaxY = maxY;
  std::int64_t exteriorMaxZ = maxZ;
  std::int64_t interiorMinX = minX + shell;
  std::int64_t interiorMinY = minY + shell;
  std::int64_t interiorMinZ = minZ + shell;
  std::int64_t interiorMaxX = maxX + 1 - shell;
  std::int64_t interiorMaxY = maxY + 1 - shell;
  std::int64_t interiorMaxZ = maxZ + 1 - shell;
  if (request.hollowAlignment == CreativeVolumeHollowAlignment::Outward) {
    exteriorMinX -= shell;
    exteriorMinY -= shell;
    exteriorMinZ -= shell;
    exteriorMaxX += shell;
    exteriorMaxY += shell;
    exteriorMaxZ += shell;
    interiorMinX = minX;
    interiorMinY = minY;
    interiorMinZ = minZ;
    interiorMaxX = maxX + 1;
    interiorMaxY = maxY + 1;
    interiorMaxZ = maxZ + 1;
  } else if (interiorMinX >= interiorMaxX ||
             interiorMinY >= interiorMaxY ||
             interiorMinZ >= interiorMaxZ) {
    result.reasonCode = "creative_volume_hollow_shell_does_not_fit";
    return result;
  }

  if (!checkedCoord(exteriorMinX, result.exteriorMinimum.x) ||
      !checkedCoord(exteriorMinY, result.exteriorMinimum.y) ||
      !checkedCoord(exteriorMinZ, result.exteriorMinimum.z) ||
      !checkedCoord(exteriorMaxX, result.exteriorMaximum.x) ||
      !checkedCoord(exteriorMaxY, result.exteriorMaximum.y) ||
      !checkedCoord(exteriorMaxZ, result.exteriorMaximum.z) ||
      !checkedCoord(interiorMinX, result.interiorBounds.min.x) ||
      !checkedCoord(interiorMinY, result.interiorBounds.min.y) ||
      !checkedCoord(interiorMinZ, result.interiorBounds.min.z) ||
      !checkedCoord(interiorMaxX, result.interiorBounds.max.x) ||
      !checkedCoord(interiorMaxY, result.interiorBounds.max.y) ||
      !checkedCoord(interiorMaxZ, result.interiorBounds.max.z) ||
      !checkedCoord(exteriorMinX, result.exteriorBounds.min.x) ||
      !checkedCoord(exteriorMinY, result.exteriorBounds.min.y) ||
      !checkedCoord(exteriorMinZ, result.exteriorBounds.min.z) ||
      !checkedCoord(exteriorMaxX + 1, result.exteriorBounds.max.x) ||
      !checkedCoord(exteriorMaxY + 1, result.exteriorBounds.max.y) ||
      !checkedCoord(exteriorMaxZ + 1, result.exteriorBounds.max.z)) {
    result.reasonCode = "creative_volume_hollow_bounds_overflow";
    return result;
  }
  result.valid = true;
  result.reasonCode = "creative_volume_hollow_bounds_ready";
  return result;
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
  HollowPlanBounds hollowBounds;
  if (hollow) {
    hollowBounds = planHollowBounds(request);
    if (!hollowBounds.valid) {
      reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
             hollowBounds.reasonCode);
      return receipt;
    }
    filledRequest.firstCell = hollowBounds.exteriorMinimum;
    filledRequest.secondCell = hollowBounds.exteriorMaximum;
    receipt.hollowBoundsValid = true;
    receipt.hollowInteriorBounds = hollowBounds.interiorBounds;
    receipt.hollowExteriorBounds = hollowBounds.exteriorBounds;
  }
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
    hollowRequest.shellThicknessCells =
        creativeVolumeHollowThicknessCells(request.hollowThickness);
    hollowRequest.shellOpening = request.hollowOpening;
    hollowRequest.shellCornerRule = request.hollowCornerRule;
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
  std::unordered_map<CellKey, std::vector<CreativeObjectId>, CellKeyHash>
      legacyObjectsByCell;
  legacyObjectsByCell.reserve(document.objects().size());
  std::vector<CreativeObjectId> objectIdsToRemove;
  for (const CreativeObject& object : document.objects()) {
    CreativeGridCoord3 cell;
    if (volumeCellForObject(object, request.selection, cell)) {
      const CellKey key = toCellKey(cell);
      legacyObjectsByCell[key].push_back(object.id);
      if (hollow && filledCells.contains(key)) {
        if (!plannedCells.contains(key)) {
          objectIdsToRemove.push_back(object.id);
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
            legacyObjectsByCell.contains(key)) {
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
      const CellKey key = toCellKey(cell);
      const CreativeObjectKind currentMaterial =
          document.voxelField().materialAt(cell);
      const auto legacy = legacyObjectsByCell.find(key);
      const bool hasLegacy = legacy != legacyObjectsByCell.end();
      if (request.fillOverlapPolicy ==
              CreativeVolumeFillOverlapPolicy::PreserveExisting &&
          (currentMaterial != CreativeObjectKind::Unknown || hasLegacy)) {
        ++receipt.skippedOccupiedCellCount;
        continue;
      }
      if (request.fillOverlapPolicy ==
              CreativeVolumeFillOverlapPolicy::ReplaceExisting &&
          hasLegacy) {
        objectIdsToRemove.insert(objectIdsToRemove.end(),
                                 legacy->second.begin(), legacy->second.end());
      }
      if (currentMaterial == request.objectKind) {
        ++receipt.unchangedMaterialCellCount;
      } else {
        voxelEdits.push_back({cell, request.objectKind});
      }
    }
  }

  if (affectedCountExceeds(objectIdsToRemove.size(), voxelEdits.size(),
                           request.maxAffectedObjects)) {
    reject(receipt, CreativeVolumeOperationStatus::OperationLimitExceeded,
           hollow ? "creative_volume_hollow_limit_exceeded"
                  : "creative_volume_fill_limit_exceeded");
    return receipt;
  }

  CreativeDocument staged = document;
  if (!objectIdsToRemove.empty()) {
    CreativeClipboard removedObjects;
    const CreativeClipboardCutReceipt cutReceipt =
        cutDocumentObjectsAtomically(staged, objectIdsToRemove, removedObjects);
    if (!cutReceipt.accepted || !cutReceipt.changed) {
      receipt.failedObjectId = cutReceipt.failedObjectId;
      reject(receipt, CreativeVolumeOperationStatus::RemoveRejected,
             cutReceipt.reasonCode);
      return receipt;
    }
    receipt.matchedObjectCount = objectIdsToRemove.size();
    receipt.removedObjectIds = objectIdsToRemove;
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
    acceptNoChange(
        receipt,
        hollow ? "creative_volume_hollow_already_applied"
               : request.fillOverlapPolicy ==
                         CreativeVolumeFillOverlapPolicy::ReplaceExisting
                     ? "creative_volume_fill_material_already_applied"
                     : "creative_volume_cells_already_occupied");
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
    if (!creativeVolumeMemberMaskIncludesVoxels(
            request.replaceMemberMask)) {
      ++receipt.excludedVoxelCellCount;
      continue;
    }
    if (request.hasReplaceKindFilter &&
        cell.material != request.replaceKindFilter) {
      ++receipt.excludedVoxelCellCount;
      continue;
    }
    ++receipt.matchedVoxelCellCount;
    if (cell.material == request.objectKind) {
      ++receipt.unchangedMaterialCellCount;
      receipt.unchangedVoxelCells.push_back(cell.cell);
      continue;
    }
    voxelEdits.push_back({cell.cell, request.objectKind});
    receipt.changedVoxelCells.push_back(cell.cell);
  }

  std::vector<CreativeObject> candidates;
  candidates.reserve(document.objects().size());
  for (const CreativeObject& object : document.objects()) {
    CreativeGridCoord3 cell;
    if (!volumeCellForObject(object, request.selection, cell) ||
        !objectInsideVolume(object, worldBounds)) {
      continue;
    }
    if (!creativeVolumeBrushSupported(object.kind) ||
        !creativeVolumeMemberMaskIncludesObjects(
            request.replaceMemberMask) ||
        (request.hasReplaceKindFilter &&
         object.kind != request.replaceKindFilter)) {
      ++receipt.excludedObjectCount;
      continue;
    }
    ++receipt.matchedObjectCount;
    if (object.kind == request.objectKind) {
      ++receipt.unchangedObjectCount;
      receipt.unchangedObjectIds.push_back(object.id);
      continue;
    }
    candidates.push_back(object);
    receipt.changedObjectIds.push_back(object.id);
  }

  if (affectedCountExceeds(
          static_cast<std::size_t>(receipt.matchedObjectCount),
          static_cast<std::size_t>(receipt.matchedVoxelCellCount),
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
  CreativeObjectDirtyFlags objectDirtyFlags = 0U;
  for (const CreativeObject& candidate : candidates) {
    CreativeObject* replacement = staged.findObject(candidate.id);
    if (replacement == nullptr || replacement->kind != candidate.kind) {
      receipt.failedObjectId = candidate.id;
      reject(receipt, CreativeVolumeOperationStatus::InvalidRequest,
             "creative_volume_replace_object_missing");
      return receipt;
    }
    objectDirtyFlags |= dirtyFlagsForCreation(candidate.kind);
    objectDirtyFlags |= dirtyFlagsForCreation(request.objectKind);
    replacement->kind = request.objectKind;
  }
  receipt.replacedObjectCount = candidates.size();

  if (!voxelEdits.empty()) {
    const CreativeVoxelMutationReceipt voxelReceipt =
        staged.applyVoxelEdits(voxelEdits, objectDirtyFlags);
    if (!voxelReceipt.accepted) {
      reject(receipt, CreativeVolumeOperationStatus::VoxelMutationRejected,
             voxelReceipt.reasonCode);
      return receipt;
    }
    copyVoxelMutationFacts(receipt, voxelReceipt);
  } else if (objectDirtyFlags != 0U) {
    staged.markObjectMutationChanged(objectDirtyFlags);
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
