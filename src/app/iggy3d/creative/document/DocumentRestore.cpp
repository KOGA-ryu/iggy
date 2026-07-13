#include "app/iggy3d/creative/document/Document.hpp"

#include "app/iggy3d/creative/document/DocumentInternal.hpp"

#include <utility>

namespace iggy3d::creative {

using document_internal::isValidGridSettings;
using document_internal::isValidRestoreObject;
using document_internal::isValidUnits;
using document_internal::isValidWorldBounds;
using document_internal::validateRestoredPathPayload;

namespace {

void setRestoreStatus(CreativeDocumentRestoreReceipt& receipt,
                      CreativeDocumentRestoreStatus status,
                      std::string_view reason) noexcept {
  receipt.status = status;
  receipt.message = reason;
  receipt.reasonCode = reason;
}

}  // namespace

std::string_view toString(CreativeDocumentRestoreStatus status) noexcept {
  switch (status) {
    case CreativeDocumentRestoreStatus::Unknown:
      return "Unknown";
    case CreativeDocumentRestoreStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeDocumentRestoreStatus::InvalidDocumentId:
      return "InvalidDocumentId";
    case CreativeDocumentRestoreStatus::InvalidSettings:
      return "InvalidSettings";
    case CreativeDocumentRestoreStatus::InvalidObject:
      return "InvalidObject";
    case CreativeDocumentRestoreStatus::DuplicateObjectId:
      return "DuplicateObjectId";
    case CreativeDocumentRestoreStatus::InvalidVoxelField:
      return "InvalidVoxelField";
    case CreativeDocumentRestoreStatus::InvalidTerrainField:
      return "InvalidTerrainField";
    case CreativeDocumentRestoreStatus::InvalidTerrainMaterialField:
      return "InvalidTerrainMaterialField";
    case CreativeDocumentRestoreStatus::InvalidNextObjectId:
      return "InvalidNextObjectId";
    case CreativeDocumentRestoreStatus::Restored:
      return "Restored";
  }
  return "Unknown";
}

CreativeDocumentRestoreReceipt CreativeDocument::restoreForLoad(
    const CreativeDocumentRestoreRequest& request) {
  CreativeDocumentRestoreReceipt receipt;
  receipt.requested = true;
  receipt.documentId = request.documentId;
  receipt.objectCount = request.objects.size();
  receipt.voxelCellCount = request.voxelField.occupiedCellCount();
  receipt.terrainControlCount = request.terrainField.controlCount();
  receipt.terrainMaterialOverrideCount =
      request.terrainMaterialField.overrideCount();
  receipt.nextObjectId = request.nextObjectId;

  if (!valid_) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidDocument,
                     "invalid_document");
    return receipt;
  }

  if (request.documentId == kInvalidDocumentId) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidDocumentId,
                     "invalid_document_id");
    return receipt;
  }

  if (!isValidUnits(request.units)) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidSettings,
                     "invalid_units");
    return receipt;
  }

  if (!isValidGridSettings(request.gridSettings)) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidSettings,
                     "invalid_grid_settings");
    return receipt;
  }

  if (!isValidCreativeDocumentSnapSettings(request.snapSettings)) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidSettings,
                     "invalid_document_snap_settings");
    return receipt;
  }

  if (!isValidWorldBounds(request.worldBounds)) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidSettings,
                     "invalid_world_bounds");
    return receipt;
  }

  std::unordered_map<CreativeObjectId, std::size_t> restoredIndex;
  restoredIndex.reserve(request.objects.size());
  CreativeObjectId maxObjectId = kInvalidObjectId;
  for (std::size_t index = 0; index < request.objects.size(); ++index) {
    const CreativeObject& object = request.objects[index];
    if (!isValidRestoreObject(object)) {
      setRestoreStatus(receipt,
                       CreativeDocumentRestoreStatus::InvalidObject,
                       "invalid_object");
      return receipt;
    }
    const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
    const std::string_view pathValidation =
        validateRestoredPathPayload(descriptor, object);
    if (!pathValidation.empty()) {
      setRestoreStatus(receipt,
                       CreativeDocumentRestoreStatus::InvalidObject,
                       pathValidation);
      return receipt;
    }
    if (restoredIndex.find(object.id) != restoredIndex.end()) {
      setRestoreStatus(receipt,
                       CreativeDocumentRestoreStatus::DuplicateObjectId,
                       "duplicate_object_id");
      return receipt;
    }
    restoredIndex.emplace(object.id, index);
    if (object.id > maxObjectId) {
      maxObjectId = object.id;
    }
  }

  const std::string_view parentGraphValidation =
      validateCreativeObjectParentGraph(request.objects);
  if (!parentGraphValidation.empty()) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidObject,
                     parentGraphValidation);
    return receipt;
  }

  if (!request.voxelField.validateInvariants()) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidVoxelField,
                     "invalid_voxel_field");
    return receipt;
  }

  if (!request.terrainField.validateInvariants()) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidTerrainField,
                     "invalid_terrain_field");
    return receipt;
  }

  if (!request.terrainMaterialField.validateInvariants()) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidTerrainMaterialField,
                     "invalid_terrain_material_field");
    return receipt;
  }

  if (request.nextObjectId == kInvalidObjectId ||
      request.nextObjectId <= maxObjectId) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidNextObjectId,
                     "invalid_next_object_id");
    return receipt;
  }

  valid_ = true;
  id_ = request.documentId;
  name_ = request.name;
  units_ = request.units;
  gridSettings_ = request.gridSettings;
  snapSettings_ = request.snapSettings;
  worldBounds_ = request.worldBounds;
  objects_ = request.objects;
  objectIndex_ = std::move(restoredIndex);
  nextObjectId_ = request.nextObjectId;
  voxelField_ = request.voxelField;
  terrainField_ = request.terrainField;
  terrainMaterialField_ = request.terrainMaterialField;
  revision_ = 0;
  dirtyFlags_ = 0;

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeDocumentRestoreStatus::Restored;
  receipt.message = "document_restored";
  receipt.reasonCode = "document_restored";
  return receipt;
}

}  // namespace iggy3d::creative
