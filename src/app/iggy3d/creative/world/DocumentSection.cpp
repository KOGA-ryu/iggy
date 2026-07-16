#include "app/iggy3d/creative/world/DocumentSection.hpp"
#include "app/iggy3d/creative/world/DocumentSectionInternal.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d {

using document_section_internal::parseUnits;
using document_section_internal::toCreativeBounds;
using document_section_internal::toCreativeGridSettings;
using document_section_internal::toCreativeLogicLink;
using document_section_internal::toCreativeObject;
using document_section_internal::toCreativeSnapSettings;
using document_section_internal::toCreativeTerrainField;
using document_section_internal::toCreativeTerrainMaterialField;
using document_section_internal::toCreativeVoxelField;
using document_section_internal::toSaveBounds;
using document_section_internal::toSaveGridSettings;
using document_section_internal::toSaveLogicLink;
using document_section_internal::toSaveObject;
using document_section_internal::toSaveSnapSettings;
using document_section_internal::toSaveTerrainControls;
using document_section_internal::toSaveTerrainMaterials;
using document_section_internal::toSaveUnits;
using document_section_internal::toSaveVoxelChunks;

namespace {

constexpr std::uint32_t kMovingPlatformPathSectionVersion = 8U;

void setStatus(ProductCreativeDocumentSectionReceipt& receipt,
               ProductCreativeDocumentSectionStatus status,
               std::string_view reason) noexcept {
  receipt.status = status;
  receipt.message = reason;
  receipt.reasonCode = reason;
}

[[nodiscard]] bool objectIdsAreUniqueAndNextIdIsValid(
    std::span<const SaveCreativeDocumentObjectRecord> objects,
    creative::CreativeObjectId nextObjectId,
    ProductCreativeDocumentSectionReceipt& receipt) {
  creative::CreativeObjectId maxObjectId = creative::kInvalidObjectId;
  for (std::size_t index = 0; index < objects.size(); ++index) {
    const creative::CreativeObjectId id = objects[index].id;
    if (id == creative::kInvalidObjectId) {
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidObject,
                "invalid_object");
      return false;
    }
    for (std::size_t prior = 0; prior < index; ++prior) {
      if (objects[prior].id == id) {
        setStatus(receipt,
                  ProductCreativeDocumentSectionStatus::DuplicateObjectId,
                  "duplicate_object_id");
        return false;
      }
    }
    if (id > maxObjectId) {
      maxObjectId = id;
    }
  }

  if (nextObjectId == creative::kInvalidObjectId ||
      nextObjectId <= maxObjectId) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidNextObjectId,
              "invalid_next_object_id");
    return false;
  }
  return true;
}

void mirrorRestoreFailure(ProductCreativeDocumentSectionReceipt& receipt,
                          creative::CreativeDocumentRestoreStatus status,
                          std::string_view reason) noexcept {
  switch (status) {
    case creative::CreativeDocumentRestoreStatus::InvalidDocument:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidDocument,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidDocumentId:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidDocumentId,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidSettings:
      if (reason == "invalid_grid_settings") {
        setStatus(receipt,
                  ProductCreativeDocumentSectionStatus::InvalidGrid,
                  reason);
      } else if (reason == "invalid_document_snap_settings") {
        setStatus(receipt,
                  ProductCreativeDocumentSectionStatus::InvalidSnap,
                  reason);
      } else if (reason == "invalid_world_bounds") {
        setStatus(receipt,
                  ProductCreativeDocumentSectionStatus::InvalidWorldBounds,
                  reason);
      } else if (reason == "invalid_units") {
        setStatus(receipt,
                  ProductCreativeDocumentSectionStatus::InvalidUnits,
                  reason);
      } else {
        setStatus(receipt,
                  ProductCreativeDocumentSectionStatus::InvalidDocument,
                  reason);
      }
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidObject:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidObject,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidLogicLink:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidLogicLink,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::DuplicateObjectId:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::DuplicateObjectId,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidVoxelField:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidVoxelData,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidTerrainField:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidTerrainData,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidTerrainMaterialField:
      setStatus(
          receipt,
          ProductCreativeDocumentSectionStatus::InvalidTerrainMaterialData,
          reason);
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidNextObjectId:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidNextObjectId,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::Unknown:
    case creative::CreativeDocumentRestoreStatus::Restored:
      break;
  }
  setStatus(receipt,
            ProductCreativeDocumentSectionStatus::InvalidDocument,
            reason);
}

[[nodiscard]] bool migrateLegacyMovingPlatformPath(
    std::uint32_t sectionVersion,
    creative::CreativeObject& object) {
  if (sectionVersion >= kMovingPlatformPathSectionVersion ||
      object.kind != creative::CreativeObjectKind::MovingPlatform ||
      !object.pathPoints.empty()) {
    return true;
  }
  const creative::CreativeBoundsMetrics bounds =
      creative::measureCreativeBounds(object.bounds);
  if (!bounds.valid) {
    return false;
  }
  object.pathPoints = {
      {bounds.center},
      {{bounds.center.x, bounds.center.y + 3.0, bounds.center.z}},
  };
  return true;
}

}  // namespace

std::string_view toString(
    ProductCreativeDocumentSectionStatus status) noexcept {
  switch (status) {
    case ProductCreativeDocumentSectionStatus::Unknown:
      return "Unknown";
    case ProductCreativeDocumentSectionStatus::MissingSection:
      return "MissingSection";
    case ProductCreativeDocumentSectionStatus::InvalidDocument:
      return "InvalidDocument";
    case ProductCreativeDocumentSectionStatus::InvalidDocumentId:
      return "InvalidDocumentId";
    case ProductCreativeDocumentSectionStatus::InvalidUnits:
      return "InvalidUnits";
    case ProductCreativeDocumentSectionStatus::InvalidGrid:
      return "InvalidGrid";
    case ProductCreativeDocumentSectionStatus::InvalidSnap:
      return "InvalidSnap";
    case ProductCreativeDocumentSectionStatus::InvalidWorldBounds:
      return "InvalidWorldBounds";
    case ProductCreativeDocumentSectionStatus::InvalidObject:
      return "InvalidObject";
    case ProductCreativeDocumentSectionStatus::InvalidObjectKind:
      return "InvalidObjectKind";
    case ProductCreativeDocumentSectionStatus::InvalidLogicLink:
      return "InvalidLogicLink";
    case ProductCreativeDocumentSectionStatus::DuplicateObjectId:
      return "DuplicateObjectId";
    case ProductCreativeDocumentSectionStatus::InvalidVoxelData:
      return "InvalidVoxelData";
    case ProductCreativeDocumentSectionStatus::InvalidTerrainData:
      return "InvalidTerrainData";
    case ProductCreativeDocumentSectionStatus::InvalidTerrainMaterialData:
      return "InvalidTerrainMaterialData";
    case ProductCreativeDocumentSectionStatus::InvalidNextObjectId:
      return "InvalidNextObjectId";
    case ProductCreativeDocumentSectionStatus::Converted:
      return "Converted";
  }
  return "Unknown";
}

ProductCreativeDocumentSectionBuildResult buildSaveCreativeDocumentSection(
    const creative::CreativeDocument& document) {
  ProductCreativeDocumentSectionBuildResult result;
  ProductCreativeDocumentSectionReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.documentId = document.id();
  receipt.objectCount = document.objectCount();
  receipt.logicLinkCount = document.logicLinks().size();
  receipt.voxelCellCount = document.voxelField().occupiedCellCount();
  receipt.terrainControlCount = document.terrainField().controlCount();
  receipt.terrainMaterialOverrideCount =
      document.terrainMaterialField().overrideCount();
  receipt.nextObjectId = document.nextObjectId();

  if (!document.isValid()) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidDocument,
              "invalid_document");
    return result;
  }

  if (document.id() == creative::kInvalidDocumentId) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidDocumentId,
              "invalid_document_id");
    return result;
  }

  const std::string_view units = toSaveUnits(document.units());
  if (units.empty()) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidUnits,
              "invalid_units");
    return result;
  }

  result.section.present = true;
  result.section.version = kSaveCreativeDocumentSectionVersion;
  result.section.documentId = document.id();
  result.section.name = std::string{document.name()};
  result.section.units = std::string{units};
  if (!toSaveGridSettings(document.gridSettings(), result.section)) {
    result.section = {};
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidGrid,
              "invalid_grid_settings");
    return result;
  }
  if (!toSaveSnapSettings(document.documentSnapSettings(), result.section)) {
    result.section = {};
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidSnap,
              "invalid_document_snap_settings");
    return result;
  }
  result.section.worldBounds = toSaveBounds(document.worldBounds());
  result.section.nextObjectId = document.nextObjectId();
  result.section.objects.reserve(document.objects().size());
  for (const creative::CreativeObject& object : document.objects()) {
    result.section.objects.push_back(toSaveObject(object));
  }
  result.section.logicLinks.reserve(document.logicLinks().size());
  for (const creative::CreativeLogicLink& link : document.logicLinks()) {
    result.section.logicLinks.push_back(toSaveLogicLink(link));
  }
  result.section.voxelChunks = toSaveVoxelChunks(document.voxelField());
  result.section.terrainControls =
      toSaveTerrainControls(document.terrainField());
  result.section.terrainMaterials =
      toSaveTerrainMaterials(document.terrainMaterialField());

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = ProductCreativeDocumentSectionStatus::Converted;
  receipt.objectCount = result.section.objects.size();
  receipt.logicLinkCount = result.section.logicLinks.size();
  receipt.voxelCellCount = document.voxelField().occupiedCellCount();
  receipt.terrainControlCount = document.terrainField().controlCount();
  receipt.terrainMaterialOverrideCount =
      document.terrainMaterialField().overrideCount();
  receipt.nextObjectId = result.section.nextObjectId;
  receipt.message = "creative_document_section_converted";
  receipt.reasonCode = "creative_document_section_converted";
  return result;
}

ProductCreativeDocumentSectionRestoreResult restoreCreativeDocumentFromSaveSection(
    const SaveCreativeDocumentSection& section) {
  ProductCreativeDocumentSectionRestoreResult result;
  ProductCreativeDocumentSectionReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.documentId = section.documentId;
  receipt.objectCount = section.objects.size();
  receipt.logicLinkCount = section.logicLinks.size();
  for (const SaveCreativeDocumentVoxelChunkRecord& chunk :
       section.voxelChunks) {
    receipt.voxelCellCount += chunk.cells.size();
  }
  receipt.terrainControlCount = section.terrainControls.size();
  receipt.terrainMaterialOverrideCount = section.terrainMaterials.size();
  receipt.nextObjectId = section.nextObjectId;

  if (!section.present) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::MissingSection,
              "missing_creative_document_section");
    return result;
  }

  creative::CreativeDocumentRestoreRequest request;
  request.documentId = section.documentId;
  request.name = section.name;
  if (!parseUnits(section.units, request.units)) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidUnits,
              "invalid_units");
    return result;
  }
  if (!toCreativeGridSettings(section, request.gridSettings)) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidGrid,
              "invalid_grid_settings");
    return result;
  }
  if (!toCreativeSnapSettings(section, request.snapSettings)) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidSnap,
              "invalid_document_snap_settings");
    return result;
  }
  request.worldBounds = toCreativeBounds(section.worldBounds);
  request.nextObjectId = section.nextObjectId;
  if (!toCreativeVoxelField(section.voxelChunks, request.voxelField)) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidVoxelData,
              "invalid_voxel_data");
    return result;
  }
  if (!toCreativeTerrainField(section.terrainControls, request.terrainField)) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidTerrainData,
              "invalid_terrain_data");
    return result;
  }
  if (!toCreativeTerrainMaterialField(section.terrainMaterials,
                                      request.terrainMaterialField)) {
    setStatus(
        receipt,
        ProductCreativeDocumentSectionStatus::InvalidTerrainMaterialData,
        "invalid_terrain_material_data");
    return result;
  }

  if (!objectIdsAreUniqueAndNextIdIsValid(section.objects,
                                          section.nextObjectId,
                                          receipt)) {
    return result;
  }

  request.objects.reserve(section.objects.size());
  for (const SaveCreativeDocumentObjectRecord& objectRecord : section.objects) {
    creative::CreativeObject object;
    if (!toCreativeObject(objectRecord, object)) {
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidObjectKind,
                "invalid_object_kind");
      return result;
    }
    if (!migrateLegacyMovingPlatformPath(section.version, object)) {
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidObject,
                "invalid_moving_platform_legacy_bounds");
      return result;
    }
    request.objects.push_back(std::move(object));
  }
  request.logicLinks.reserve(section.logicLinks.size());
  for (const SaveCreativeDocumentLogicLinkRecord& linkRecord :
       section.logicLinks) {
    creative::CreativeLogicLink link;
    if (!toCreativeLogicLink(linkRecord, link)) {
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidLogicLink,
                "creative_logic_link_action_invalid");
      return result;
    }
    request.logicLinks.push_back(link);
  }

  const creative::CreativeDocumentRestoreReceipt restored =
      result.document.restoreForLoad(request);
  if (!restored.accepted) {
    mirrorRestoreFailure(receipt, restored.status, restored.reasonCode);
    return result;
  }

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = ProductCreativeDocumentSectionStatus::Converted;
  receipt.message = "creative_document_section_converted";
  receipt.reasonCode = "creative_document_section_converted";
  return result;
}

}  // namespace iggy3d
