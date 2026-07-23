#include "app/iggy3d/creative/tools/Volume.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <utility>

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/iggy3d/creative/tools/VolumeInternal.hpp"
#include "core/hash/StableHash.hpp"

namespace iggy3d::creative::volume_internal {

bool validObjectKind(CreativeObjectKind kind) noexcept {
  return kind != CreativeObjectKind::Unknown &&
         kind != CreativeObjectKind::Count;
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
  receipt.changedObjectIds.clear();
  receipt.unchangedObjectIds.clear();
  receipt.changedVoxelCells.clear();
  receipt.unchangedVoxelCells.clear();
  receipt.removedVoxelCells.clear();
  receipt.createdVoxelCells.clear();
  receipt.clonedObjectIdRemaps.clear();
  receipt.clonedPatternRecipeIdRemaps.clear();
  receipt.createdObjectCount = 0;
  receipt.removedObjectCount = 0;
  receipt.replacedObjectCount = 0;
  receipt.unchangedObjectCount = 0;
  receipt.clonedLogicLinkCount = 0;
  receipt.clonedPatternRecipeCount = 0;
  receipt.createdVoxelCellCount = 0;
  receipt.removedVoxelCellCount = 0;
  receipt.replacedVoxelCellCount = 0;
  receipt.unchangedMaterialCellCount = 0;
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

bool affectedCountExceeds(std::size_t objectCount,
                          std::size_t voxelCount,
                          std::uint64_t limit) noexcept {
  return objectCount > limit || voxelCount > limit - objectCount;
}

CreativeGridBounds3 inclusiveVolumeGridBounds(
    const CreativeVolumeSelection& selection) noexcept {
  CreativeGridBounds3 bounds = creativeVolumeGridBounds(selection);
  --bounds.max.x;
  --bounds.max.y;
  --bounds.max.z;
  return bounds;
}

}  // namespace iggy3d::creative::volume_internal

namespace iggy3d::creative {

using volume_internal::cloneVolumeObjects;
using volume_internal::eraseVolumeObjects;
using volume_internal::fillHandler;
using volume_internal::hollowHandler;
using volume_internal::reject;
using volume_internal::replaceVolumeCells;
using volume_internal::validObjectKind;

std::uint64_t fingerprintCreativeVolumeOperationRequest(
    const CreativeVolumeOperationRequest& request) noexcept {
  StableHasher hasher;
  hasher.addString("creative_volume_operation_request_v1");
  hasher.addU64(static_cast<std::uint8_t>(request.operation));
  hasher.addU64(static_cast<std::uint8_t>(request.selection.phase));
  hasher.addI64(request.selection.firstCell.x);
  hasher.addI64(request.selection.firstCell.y);
  hasher.addI64(request.selection.firstCell.z);
  hasher.addI64(request.selection.secondCell.x);
  hasher.addI64(request.selection.secondCell.y);
  hasher.addI64(request.selection.secondCell.z);
  const auto addDouble = [&](double value) {
    if (!std::isfinite(value)) {
      return false;
    }
    hasher.addU64(std::bit_cast<std::uint64_t>(value == 0.0 ? 0.0 : value));
    return true;
  };
  if (!addDouble(request.selection.origin.x) ||
      !addDouble(request.selection.origin.y) ||
      !addDouble(request.selection.origin.z) ||
      !addDouble(request.selection.cellSize)) {
    return 0U;
  }
  hasher.addU64(static_cast<std::uint32_t>(request.objectKind));
  hasher.addU64(static_cast<std::uint8_t>(request.shapeKind));
  hasher.addU64(static_cast<std::uint8_t>(request.shapeAxis));
  hasher.addU64(static_cast<std::uint8_t>(request.fillOverlapPolicy));
  hasher.addU64(static_cast<std::uint8_t>(request.hollowThickness));
  hasher.addU64(static_cast<std::uint8_t>(request.hollowAlignment));
  hasher.addU64(static_cast<std::uint8_t>(request.hollowOpening));
  hasher.addU64(static_cast<std::uint8_t>(request.hollowCornerRule));
  hasher.addBool(request.hasReplaceKindFilter);
  hasher.addU64(static_cast<std::uint32_t>(request.replaceKindFilter));
  hasher.addU64(static_cast<std::uint8_t>(request.replaceMemberMask));
  hasher.addBool(request.hasEraseKindFilter);
  hasher.addU64(static_cast<std::uint32_t>(request.eraseKindFilter));
  hasher.addU64(static_cast<std::uint8_t>(request.eraseMemberMask));
  hasher.addBool(request.hasCloneOffset);
  if (!addDouble(request.cloneOffset.x) ||
      !addDouble(request.cloneOffset.y) ||
      !addDouble(request.cloneOffset.z)) {
    return 0U;
  }
  hasher.addU64(request.cloneQuarterTurns);
  hasher.addBool(request.cloneMirrorX);
  hasher.addBool(request.cloneMirrorZ);
  hasher.addU64(static_cast<std::uint8_t>(request.cloneMemberMask));
  hasher.addU64(static_cast<std::uint8_t>(request.cloneVoxelOverlapPolicy));
  hasher.addU64(request.maxAffectedObjects);
  return hasher.value();
}

namespace {

[[nodiscard]] bool validOperation(
    CreativeVolumeOperationKind operation) noexcept {
  return operation < CreativeVolumeOperationKind::Count;
}

[[nodiscard]] bool validFillOverlapPolicy(
    CreativeVolumeFillOverlapPolicy policy) noexcept {
  return policy < CreativeVolumeFillOverlapPolicy::Count;
}

[[nodiscard]] bool validHollowSettings(
    const CreativeVolumeOperationRequest& request) noexcept {
  return request.hollowThickness < CreativeVolumeHollowThickness::Count &&
         request.hollowAlignment < CreativeVolumeHollowAlignment::Count &&
         request.hollowOpening < CreativeVolumeHollowOpening::Count &&
         request.hollowCornerRule < CreativeVolumeHollowCornerRule::Count &&
         creativeVolumeHollowThicknessCells(request.hollowThickness) > 0U;
}

[[nodiscard]] bool validCloneSettings(
    const CreativeVolumeOperationRequest& request) noexcept {
  return request.cloneQuarterTurns <= 3U &&
         request.cloneMemberMask < CreativeVolumeMemberMask::Count &&
         request.cloneVoxelOverlapPolicy <
             CreativeVolumeCloneVoxelOverlapPolicy::Count;
}

[[nodiscard]] bool validMemberMask(
    CreativeVolumeMemberMask mask) noexcept {
  return mask < CreativeVolumeMemberMask::Count;
}

using VolumeOperationHandler = CreativeVolumeOperationReceipt (*)(
    CreativeDocument&,
    const CreativeVolumeOperationRequest&,
    CreativeVolumeOperationReceipt);

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
      mapped.shapeKind = settings.shapeBrushKind;
      mapped.shapeAxis = settings.shapeBrushAxis;
      mapped.fillOverlapPolicy = settings.volumeFillOverlapPolicy;
      break;
    case CreativeVolumeOperationKind::Hollow:
      mapped.shapeKind = settings.shapeBrushKind;
      mapped.shapeAxis = settings.shapeBrushAxis;
      mapped.hollowThickness = settings.volumeHollowThickness;
      mapped.hollowAlignment = settings.volumeHollowAlignment;
      mapped.hollowOpening = settings.volumeHollowOpening;
      mapped.hollowCornerRule = settings.volumeHollowCornerRule;
      break;
    case CreativeVolumeOperationKind::Replace:
      mapped.hasReplaceKindFilter =
          settings.replaceSourceKind != CreativeObjectKind::Unknown;
      mapped.replaceKindFilter = settings.replaceSourceKind;
      mapped.replaceMemberMask = settings.volumeReplaceMemberMask;
      break;
    case CreativeVolumeOperationKind::Erase:
      mapped.hasEraseKindFilter =
          settings.eraseSourceKind != CreativeObjectKind::Unknown;
      mapped.eraseKindFilter = settings.eraseSourceKind;
      mapped.eraseMemberMask = settings.volumeEraseMemberMask;
      break;
    case CreativeVolumeOperationKind::Clone: {
      CreativeToolWorldPoint offset;
      if (!tryCreativeCloneOffset(settings, request.selection.cellSize,
                                  offset)) {
        return false;
      }
      mapped.hasCloneOffset = true;
      mapped.cloneOffset = {offset.x, offset.y, offset.z};
      mapped.cloneQuarterTurns =
          static_cast<std::uint8_t>(settings.cloneRotation);
      mapped.cloneMirrorX =
          settings.cloneMirror == CreativeCloneMirror::X ||
          settings.cloneMirror == CreativeCloneMirror::XAndZ;
      mapped.cloneMirrorZ =
          settings.cloneMirror == CreativeCloneMirror::Z ||
          settings.cloneMirror == CreativeCloneMirror::XAndZ;
      mapped.cloneMemberMask = settings.volumeCloneMemberMask;
      mapped.cloneVoxelOverlapPolicy = settings.cloneVoxelOverlapPolicy;
      break;
    }
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
  receipt.objectKind = request.objectKind;
  receipt.fillOverlapPolicy = request.fillOverlapPolicy;
  receipt.hollowThickness = request.hollowThickness;
  receipt.hollowAlignment = request.hollowAlignment;
  receipt.hollowOpening = request.hollowOpening;
  receipt.hollowCornerRule = request.hollowCornerRule;
  receipt.replaceMemberMask = request.replaceMemberMask;
  receipt.eraseMemberMask = request.eraseMemberMask;
  receipt.cloneMemberMask = request.cloneMemberMask;
  receipt.cloneVoxelOverlapPolicy = request.cloneVoxelOverlapPolicy;
  receipt.cloneOffset = request.cloneOffset;
  receipt.cloneQuarterTurns = request.cloneQuarterTurns;
  receipt.cloneMirrorX = request.cloneMirrorX;
  receipt.cloneMirrorZ = request.cloneMirrorZ;
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
      !validFillOverlapPolicy(request.fillOverlapPolicy) ||
      !validHollowSettings(request) ||
      !validCloneSettings(request) ||
      !validMemberMask(request.replaceMemberMask) ||
      !validMemberMask(request.eraseMemberMask) ||
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

CreativeVolumeOperationReceipt previewCreativeVolumeOperation(
    const CreativeDocument& document,
    const CreativeVolumeOperationRequest& request) {
  CreativeDocument staged = document;
  return executeCreativeVolumeOperation(staged, request);
}

std::uint64_t creativeVolumeChangedMemberCount(
    const CreativeVolumeOperationReceipt& receipt) noexcept {
  const std::uint64_t voxelChanges = receipt.createdVoxelCellCount +
                                     receipt.removedVoxelCellCount +
                                     receipt.replacedVoxelCellCount;
  switch (receipt.operation) {
    case CreativeVolumeOperationKind::Fill:
    case CreativeVolumeOperationKind::Hollow:
    case CreativeVolumeOperationKind::Erase:
      return voxelChanges + receipt.removedObjectCount;
    case CreativeVolumeOperationKind::Replace:
      return voxelChanges + receipt.replacedObjectCount;
    case CreativeVolumeOperationKind::Clone:
      return voxelChanges + receipt.createdObjectCount;
    case CreativeVolumeOperationKind::Count:
      break;
  }
  return 0U;
}

}  // namespace iggy3d::creative
