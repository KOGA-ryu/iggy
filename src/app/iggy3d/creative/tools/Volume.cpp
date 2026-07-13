#include "app/iggy3d/creative/tools/Volume.hpp"

#include <array>
#include <cstddef>
#include <utility>

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/iggy3d/creative/tools/VolumeInternal.hpp"

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

namespace {

[[nodiscard]] bool validOperation(
    CreativeVolumeOperationKind operation) noexcept {
  return operation < CreativeVolumeOperationKind::Count;
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
