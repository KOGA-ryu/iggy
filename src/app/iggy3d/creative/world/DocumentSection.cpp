#include "app/iggy3d/creative/world/DocumentSection.hpp"
#include "app/iggy3d/creative/world/DocumentSectionInternal.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "core/hash/StableHash.hpp"
#include "runtime/save/SaveCodec.hpp"

#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d {

using document_section_internal::parseUnits;
using document_section_internal::toCreativeBounds;
using document_section_internal::toCreativeGridSettings;
using document_section_internal::toCreativeLogicLink;
using document_section_internal::toCreativeMeasurementAnnotationStore;
using document_section_internal::toCreativeObject;
using document_section_internal::toCreativePatternRecipeStore;
using document_section_internal::toCreativeSnapSettings;
using document_section_internal::toCreativeTerrainField;
using document_section_internal::toCreativeTerrainHeightField;
using document_section_internal::toCreativeTerrainHardEdges;
using document_section_internal::toCreativeTerrainMaterialField;
using document_section_internal::toCreativeTerrainOperationStack;
using document_section_internal::toCreativeVoxelField;
using document_section_internal::toSaveBounds;
using document_section_internal::toSaveGridSettings;
using document_section_internal::toSaveLogicLink;
using document_section_internal::toSaveMeasurementAnnotations;
using document_section_internal::toSaveObject;
using document_section_internal::toSavePatternRecipes;
using document_section_internal::toSaveSnapSettings;
using document_section_internal::toSaveTerrainControls;
using document_section_internal::toSaveTerrainHeightField;
using document_section_internal::toSaveTerrainHardEdges;
using document_section_internal::toSaveTerrainMaterials;
using document_section_internal::toSaveTerrainOperations;
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
    case creative::CreativeDocumentRestoreStatus::InvalidTerrainHeightField:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidTerrainData,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidTerrainOperationStack:
      setStatus(
          receipt,
          ProductCreativeDocumentSectionStatus::InvalidTerrainOperationData,
          reason);
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidPatternRecipeStore:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidPatternRecipeData,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::
        InvalidMeasurementAnnotationStore:
      setStatus(
          receipt,
          ProductCreativeDocumentSectionStatus::InvalidMeasurementAnnotationData,
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
    case ProductCreativeDocumentSectionStatus::InvalidTerrainOperationData:
      return "InvalidTerrainOperationData";
    case ProductCreativeDocumentSectionStatus::InvalidPatternRecipeData:
      return "InvalidPatternRecipeData";
    case ProductCreativeDocumentSectionStatus::InvalidMeasurementAnnotationData:
      return "InvalidMeasurementAnnotationData";
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
  receipt.terrainHeightCellCount =
      document.terrainHeightField().cellCount();
  receipt.terrainHardEdgeCount = document.terrainHardEdges().size();
  receipt.terrainOperationCount =
      document.terrainOperationStack().operations.size();
  receipt.patternRecipeCount =
      document.patternRecipeStore().recipes.size();
  receipt.measurementAnnotationCount =
      document.measurementAnnotationStore().annotations.size();
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
  result.section.terrainHeightField =
      toSaveTerrainHeightField(document.terrainHeightField());
  result.section.terrainHardEdges =
      toSaveTerrainHardEdges(document.terrainHardEdges());
  result.section.terrainOperationStackVersion =
      document.terrainOperationStack().version;
  result.section.nextTerrainOperationId =
      document.terrainOperationStack().nextOperationId;
  result.section.terrainOperationBaseHeightField =
      toSaveTerrainHeightField(
          document.terrainOperationStack().baseHeightField);
  result.section.terrainOperationBaseHardEdges =
      toSaveTerrainHardEdges(
          document.terrainOperationStack().baseHardEdges);
  result.section.terrainOperationBaseMaterials =
      toSaveTerrainMaterials(
          document.terrainOperationStack().baseMaterialField);
  result.section.terrainOperations =
      toSaveTerrainOperations(document.terrainOperationStack());
  result.section.patternRecipeStoreVersion =
      document.patternRecipeStore().version;
  result.section.nextPatternRecipeId =
      document.patternRecipeStore().nextRecipeId;
  result.section.patternRecipes =
      toSavePatternRecipes(document.patternRecipeStore());
  result.section.measurementAnnotationStoreVersion =
      document.measurementAnnotationStore().version;
  result.section.nextMeasurementAnnotationId =
      document.measurementAnnotationStore().nextAnnotationId;
  result.section.measurementAnnotations =
      toSaveMeasurementAnnotations(document.measurementAnnotationStore());
  result.section.terrainMaterials =
      toSaveTerrainMaterials(document.terrainMaterialField());

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = ProductCreativeDocumentSectionStatus::Converted;
  receipt.objectCount = result.section.objects.size();
  receipt.logicLinkCount = result.section.logicLinks.size();
  receipt.voxelCellCount = document.voxelField().occupiedCellCount();
  receipt.terrainControlCount = document.terrainField().controlCount();
  receipt.terrainHeightCellCount =
      document.terrainHeightField().cellCount();
  receipt.terrainHardEdgeCount = document.terrainHardEdges().size();
  receipt.terrainOperationCount = result.section.terrainOperations.size();
  receipt.patternRecipeCount = result.section.patternRecipes.size();
  receipt.measurementAnnotationCount =
      result.section.measurementAnnotations.size();
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
  receipt.terrainHeightCellCount =
      section.terrainHeightField.heights.size();
  receipt.terrainHardEdgeCount = section.terrainHardEdges.size();
  receipt.terrainOperationCount = section.terrainOperations.size();
  receipt.patternRecipeCount = section.patternRecipes.size();
  receipt.measurementAnnotationCount = section.measurementAnnotations.size();
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
  if (!toCreativeTerrainHeightField(section.terrainHeightField,
                                    request.terrainHeightField)) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidTerrainData,
              "invalid_terrain_height_data");
    return result;
  }
  if (!toCreativeTerrainHardEdges(section.terrainHardEdges,
                                  request.terrainHardEdges)) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidTerrainData,
              "invalid_terrain_hard_edge_data");
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
  if (!toCreativeTerrainOperationStack(
          section.terrainOperations,
          section.version,
          section.terrainOperationStackVersion,
          section.nextTerrainOperationId,
          section.terrainOperationBaseHeightField,
          section.terrainOperationBaseHardEdges,
          section.terrainOperationBaseMaterials,
          request.terrainMaterialField,
          request.terrainOperationStack)) {
    setStatus(
        receipt,
        ProductCreativeDocumentSectionStatus::InvalidTerrainOperationData,
        "invalid_terrain_operation_data");
    return result;
  }
  if (section.version < kSaveCreativeDocumentTerrainHardEdgeVersion &&
      !request.terrainOperationStack.operations.empty()) {
    const creative::CreativeTerrainOperationReplayResult migratedReplay =
        creative::replayCreativeTerrainOperations(
            request.terrainField, request.terrainOperationStack);
    if (!migratedReplay.receipt.accepted) {
      setStatus(
          receipt,
          ProductCreativeDocumentSectionStatus::InvalidTerrainOperationData,
          "invalid_legacy_terrain_operation_topology");
      return result;
    }
    request.terrainHardEdges = migratedReplay.hardEdges;
    receipt.terrainHardEdgeCount = request.terrainHardEdges.size();
  }
  if (!toCreativePatternRecipeStore(
          section.patternRecipes,
          section.patternRecipeStoreVersion,
          section.nextPatternRecipeId,
          request.patternRecipeStore)) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidPatternRecipeData,
              "invalid_pattern_recipe_data");
    return result;
  }
  if (!toCreativeMeasurementAnnotationStore(
          section.measurementAnnotations,
          section.measurementAnnotationStoreVersion,
          section.nextMeasurementAnnotationId,
          request.measurementAnnotationStore)) {
    setStatus(
        receipt,
        ProductCreativeDocumentSectionStatus::InvalidMeasurementAnnotationData,
        "invalid_measurement_annotation_data");
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
    if (!toCreativeObject(objectRecord, section.version, object)) {
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

std::uint64_t fingerprintSaveCreativeDocumentSection(
    const creative::CreativeDocument& document) {
  ProductCreativeDocumentSectionBuildResult built =
      buildSaveCreativeDocumentSection(document);
  if (!built.receipt.accepted) {
    return 0U;
  }

  SaveEnvelope envelope;
  envelope.creativeDocument = std::move(built.section);
  const SaveEncodeResult encoded = encodeSaveEnvelope(envelope);
  if (encoded.status != SaveCodecStatus::Ok) {
    return 0U;
  }

  StableHasher hasher;
  hasher.addString("creative_document_save_section_v1");
  hasher.addString(encoded.encodedText);
  const std::uint64_t fingerprint = hasher.value();
  return fingerprint != 0U ? fingerprint : 0U;
}

}  // namespace iggy3d
