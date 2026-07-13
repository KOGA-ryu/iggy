#include "app/iggy3d/creative/world/MapTemplate.hpp"

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

constexpr double kTerrainTopMeters = 3.0;
constexpr double kFloorTopMeters = 3.25;
constexpr double kWallTopMeters = 6.25;
constexpr double kWallHalfThickness = 0.125;
constexpr double kInteriorWallHalfThickness = 0.1;

void setStatus(CreativeMapTemplateResult& result,
               CreativeMapTemplateStatus status,
               std::string_view reasonCode,
               bool accepted = false) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
  result.accepted = accepted;
}

CreativeVec3 boundsCenter(CreativeBounds bounds) noexcept {
  return {(bounds.min.x + bounds.max.x) * 0.5,
          (bounds.min.y + bounds.max.y) * 0.5,
          (bounds.min.z + bounds.max.z) * 0.5};
}

void appendBox(std::vector<CreativeDocumentCreateRequest>& requests,
               CreativeObjectKind kind,
               std::string name,
               CreativeBounds bounds,
               std::string assetId = {}) {
  CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.assetId = std::move(assetId);
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  const CreativeObjectDescriptor& descriptor = describeObject(kind);
  if (descriptor.hasTransform) {
    request.transform.position = boundsCenter(bounds);
    request.hasTransformOverride = true;
  }
  request.visible = true;
  request.hasVisibleOverride = true;
  requests.push_back(std::move(request));
}

void appendMarker(std::vector<CreativeDocumentCreateRequest>& requests,
                  CreativeObjectKind kind,
                  std::string name,
                  CreativeVec3 position) {
  CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.transform.position = position;
  request.hasTransformOverride = true;
  request.visible = true;
  request.hasVisibleOverride = true;
  requests.push_back(std::move(request));
}

void appendWallAlongX(std::vector<CreativeDocumentCreateRequest>& requests,
                      std::string name,
                      double minimumX,
                      double maximumX,
                      double z,
                      double minimumY = kFloorTopMeters,
                      double maximumY = kWallTopMeters,
                      double halfThickness = kWallHalfThickness) {
  appendBox(requests, CreativeObjectKind::Wall, std::move(name),
            {{minimumX, minimumY, z - halfThickness},
             {maximumX, maximumY, z + halfThickness}});
}

void appendWallAlongZ(std::vector<CreativeDocumentCreateRequest>& requests,
                      std::string name,
                      double x,
                      double minimumZ,
                      double maximumZ,
                      double minimumY = kFloorTopMeters,
                      double maximumY = kWallTopMeters,
                      double halfThickness = kWallHalfThickness) {
  appendBox(requests, CreativeObjectKind::Wall, std::move(name),
            {{x - halfThickness, minimumY, minimumZ},
             {x + halfThickness, maximumY, maximumZ}});
}

void appendWindowAlongX(std::vector<CreativeDocumentCreateRequest>& requests,
                        std::string name,
                        double minimumX,
                        double maximumX,
                        double z) {
  appendWallAlongX(requests, name + " Sill", minimumX, maximumX, z,
                   kFloorTopMeters, 4.15);
  appendBox(requests, CreativeObjectKind::Window, name,
            {{minimumX, 4.15, z - kWallHalfThickness},
             {maximumX, 5.25, z + kWallHalfThickness}});
  appendWallAlongX(requests, name + " Lintel", minimumX, maximumX, z,
                   5.25, kWallTopMeters);
}

void appendWindowAlongZ(std::vector<CreativeDocumentCreateRequest>& requests,
                        std::string name,
                        double x,
                        double minimumZ,
                        double maximumZ) {
  appendWallAlongZ(requests, name + " Sill", x, minimumZ, maximumZ,
                   kFloorTopMeters, 4.15);
  appendBox(requests, CreativeObjectKind::Window, name,
            {{x - kWallHalfThickness, 4.15, minimumZ},
             {x + kWallHalfThickness, 5.25, maximumZ}});
  appendWallAlongZ(requests, name + " Lintel", x, minimumZ, maximumZ,
                   5.25, kWallTopMeters);
}

void appendOpenDoorFromXWall(
    std::vector<CreativeDocumentCreateRequest>& requests,
    std::string name,
    double jambX,
    double z,
    double openDirection) {
  const double minimumZ = std::min(z, z + openDirection * 1.8);
  const double maximumZ = std::max(z, z + openDirection * 1.8);
  appendBox(requests, CreativeObjectKind::Door, std::move(name),
            {{jambX - 0.1, kFloorTopMeters, minimumZ},
             {jambX + 0.1, 5.5, maximumZ}});
}

void appendOpenDoorFromZWall(
    std::vector<CreativeDocumentCreateRequest>& requests,
    std::string name,
    double x,
    double jambZ,
    double openDirection) {
  const double minimumX = std::min(x, x + openDirection * 1.8);
  const double maximumX = std::max(x, x + openDirection * 1.8);
  appendBox(requests, CreativeObjectKind::Door, std::move(name),
            {{minimumX, kFloorTopMeters, jambZ - 0.1},
             {maximumX, 5.5, jambZ + 0.1}});
}

std::int32_t ditchCenterX(std::int32_t z) noexcept {
  if (z < -15) {
    return -12;
  }
  if (z < 0) {
    return -10;
  }
  if (z <= 15) {
    return -8;
  }
  return -10;
}

std::uint16_t terrainHeight(std::int32_t x, std::int32_t z) noexcept {
  const std::int32_t distance = std::abs(x - ditchCenterX(z));
  if (distance <= 2) {
    return 1U;
  }
  if (distance <= 6) {
    return 2U;
  }
  return 3U;
}

std::vector<CreativeTerrainControlEdit> ditchHouseTerrainControls() {
  std::vector<CreativeTerrainControlEdit> edits;
  edits.reserve(169U);
  for (std::int32_t worldZ = -30; worldZ <= 30; worldZ += 5) {
    for (std::int32_t worldX = -30; worldX <= 30; worldX += 5) {
      edits.push_back({CreativeTerrainEditKind::Upsert,
                       {{worldX + 36, worldZ + 36},
                        terrainHeight(worldX, worldZ), 4U}});
    }
  }
  return edits;
}

std::vector<CreativeTerrainMaterialEdit> ditchHouseTerrainMaterials(
    const CreativeTerrainSurfacePlan& surface) {
  std::vector<CreativeTerrainMaterialEdit> edits;
  for (const CreativeTerrainColumn& column : surface.columns) {
    CreativeTerrainMaterial material = CreativeTerrainMaterial::Grass;
    if (column.heightCells == 1U) {
      material = CreativeTerrainMaterial::Sand;
    } else if (column.heightCells == 2U) {
      material = CreativeTerrainMaterial::Dirt;
    } else {
      continue;
    }
    edits.push_back({CreativeTerrainMaterialEditKind::Set, column.coord,
                     material});
  }
  return edits;
}

std::vector<CreativeDocumentCreateRequest> ditchHouseObjects() {
  std::vector<CreativeDocumentCreateRequest> requests;
  requests.reserve(72U);

  appendBox(requests, CreativeObjectKind::Floor, "Living Floor",
            {{6.0, kTerrainTopMeters, 0.0},
             {16.0, kFloorTopMeters, 10.0}});
  appendBox(requests, CreativeObjectKind::Floor, "Kitchen Floor",
            {{16.0, kTerrainTopMeters, 0.0},
             {26.0, kFloorTopMeters, 10.0}});
  appendBox(requests, CreativeObjectKind::Floor, "Workshop Floor",
            {{6.0, kTerrainTopMeters, -10.0},
             {16.0, kFloorTopMeters, 0.0}});
  appendBox(requests, CreativeObjectKind::Floor, "Bedroom Floor",
            {{16.0, kTerrainTopMeters, -10.0},
             {26.0, kFloorTopMeters, 0.0}});

  appendBox(requests, CreativeObjectKind::Room, "Living Room",
            {{6.0, kFloorTopMeters, 0.0}, {16.0, kWallTopMeters, 10.0}});
  appendBox(requests, CreativeObjectKind::Room, "Kitchen",
            {{16.0, kFloorTopMeters, 0.0}, {26.0, kWallTopMeters, 10.0}});
  appendBox(requests, CreativeObjectKind::Room, "Workshop",
            {{6.0, kFloorTopMeters, -10.0}, {16.0, kWallTopMeters, 0.0}});
  appendBox(requests, CreativeObjectKind::Room, "Bedroom",
            {{16.0, kFloorTopMeters, -10.0}, {26.0, kWallTopMeters, 0.0}});

  appendBox(requests, CreativeObjectKind::Roof, "Living Roof",
            {{6.0, kWallTopMeters, 0.0}, {16.0, 6.55, 10.0}});
  appendBox(requests, CreativeObjectKind::Roof, "Kitchen Roof",
            {{16.0, kWallTopMeters, 0.0}, {26.0, 6.55, 10.0}});
  appendBox(requests, CreativeObjectKind::Roof, "Workshop Roof",
            {{6.0, kWallTopMeters, -10.0}, {16.0, 6.55, 0.0}});
  appendBox(requests, CreativeObjectKind::Roof, "Bedroom Roof",
            {{16.0, kWallTopMeters, -10.0}, {26.0, 6.55, 0.0}});

  appendWallAlongX(requests, "North Wall West", 6.0, 10.0, -10.0);
  appendWallAlongX(requests, "North Wall Center", 12.0, 20.0, -10.0);
  appendWallAlongX(requests, "North Wall East", 22.0, 26.0, -10.0);
  appendWindowAlongX(requests, "Workshop North Window", 10.0, 12.0,
                     -10.0);
  appendWindowAlongX(requests, "Bedroom North Window", 20.0, 22.0,
                     -10.0);

  appendWallAlongX(requests, "South Wall West", 6.0, 10.0, 10.0);
  appendWallAlongX(requests, "South Wall Center", 12.0, 20.0, 10.0);
  appendWallAlongX(requests, "South Wall East", 22.0, 26.0, 10.0);
  appendOpenDoorFromXWall(requests, "Front Door Open", 10.1, 10.0, -1.0);
  appendWindowAlongX(requests, "Kitchen South Window", 20.0, 22.0, 10.0);

  appendWallAlongZ(requests, "West Wall North", 6.0, -10.0, -6.0);
  appendWallAlongZ(requests, "West Wall Center", 6.0, -4.0, 4.0);
  appendWallAlongZ(requests, "West Wall South", 6.0, 6.0, 10.0);
  appendWindowAlongZ(requests, "Workshop West Window", 6.0, -6.0, -4.0);
  appendWindowAlongZ(requests, "Living West Window", 6.0, 4.0, 6.0);

  appendWallAlongZ(requests, "East Wall North", 26.0, -10.0, -6.0);
  appendWallAlongZ(requests, "East Wall Center", 26.0, -4.0, 4.0);
  appendWallAlongZ(requests, "East Wall South", 26.0, 6.0, 10.0);
  appendWindowAlongZ(requests, "Bedroom East Window", 26.0, -6.0, -4.0);
  appendWindowAlongZ(requests, "Kitchen East Window", 26.0, 4.0, 6.0);

  appendWallAlongZ(requests, "Interior Long Wall North", 16.0, -10.0,
                   -6.0, kFloorTopMeters, kWallTopMeters,
                   kInteriorWallHalfThickness);
  appendWallAlongZ(requests, "Interior Long Wall Center", 16.0, -4.0,
                   4.0, kFloorTopMeters, kWallTopMeters,
                   kInteriorWallHalfThickness);
  appendWallAlongZ(requests, "Interior Long Wall South", 16.0, 6.0, 10.0,
                   kFloorTopMeters, kWallTopMeters,
                   kInteriorWallHalfThickness);
  appendOpenDoorFromZWall(requests, "North Interior Door Open", 16.0,
                          -5.9, -1.0);
  appendOpenDoorFromZWall(requests, "South Interior Door Open", 16.0,
                          4.1, 1.0);

  appendWallAlongX(requests, "Interior Cross Wall West", 6.0, 10.0, 0.0,
                   kFloorTopMeters, kWallTopMeters,
                   kInteriorWallHalfThickness);
  appendWallAlongX(requests, "Interior Cross Wall Center", 12.0, 20.0, 0.0,
                   kFloorTopMeters, kWallTopMeters,
                   kInteriorWallHalfThickness);
  appendWallAlongX(requests, "Interior Cross Wall East", 22.0, 26.0, 0.0,
                   kFloorTopMeters, kWallTopMeters,
                   kInteriorWallHalfThickness);
  appendOpenDoorFromXWall(requests, "West Interior Door Open", 10.1, 0.0,
                          1.0);
  appendOpenDoorFromXWall(requests, "East Interior Door Open", 20.1, 0.0,
                          -1.0);

  appendBox(requests, CreativeObjectKind::Furniture, "Living Sofa",
            {{8.0, kFloorTopMeters, 5.5}, {12.0, 4.25, 7.0}});
  appendBox(requests, CreativeObjectKind::Furniture, "Living Table",
            {{12.5, kFloorTopMeters, 4.0}, {14.5, 4.0, 6.0}});
  appendBox(requests, CreativeObjectKind::Furniture, "Kitchen Counter",
            {{17.0, kFloorTopMeters, 7.5}, {24.5, 4.25, 9.0}});
  appendBox(requests, CreativeObjectKind::Furniture, "Kitchen Table",
            {{19.0, kFloorTopMeters, 3.0}, {22.0, 4.15, 5.0}});
  appendBox(requests, CreativeObjectKind::Furniture, "Workshop Bench",
            {{8.0, kFloorTopMeters, -8.5}, {13.0, 4.3, -7.0}});
  appendBox(requests, CreativeObjectKind::Crate, "Workshop Crate",
            {{8.0, kFloorTopMeters, -3.0}, {9.5, 4.75, -1.5}});
  appendBox(requests, CreativeObjectKind::Barrel, "Workshop Barrel",
            {{12.5, kFloorTopMeters, -3.0}, {13.7, 4.45, -1.8}});
  appendBox(requests, CreativeObjectKind::Furniture, "Bedroom Bed",
            {{18.0, kFloorTopMeters, -8.5}, {22.0, 4.05, -5.5}});
  appendBox(requests, CreativeObjectKind::Furniture, "Bedroom Wardrobe",
            {{23.5, kFloorTopMeters, -8.5}, {25.0, 5.5, -6.0}});

  appendBox(requests, CreativeObjectKind::Bridge, "Ditch Footbridge",
            {{-16.0, 2.75, 13.0}, {-4.0, kTerrainTopMeters, 15.0}});
  appendBox(requests, CreativeObjectKind::Platform, "House Approach",
            {{-4.0, 2.9, 13.0}, {11.0, 3.15, 15.0}});
  appendBox(requests, CreativeObjectKind::Platform, "Front Porch",
            {{9.5, 3.0, 10.0}, {12.5, kFloorTopMeters, 15.0}});

  appendMarker(requests, CreativeObjectKind::SpawnPoint, "Player Arrival",
               {11.0, kFloorTopMeters, 13.5});
  appendMarker(requests, CreativeObjectKind::NpcSpawn, "House Occupant",
               {13.0, kFloorTopMeters, 7.5});

  appendBox(requests, CreativeObjectKind::Rock, "Ditch Rock North",
            {{-12.5, 1.0, -20.0}, {-11.0, 2.1, -18.5}});
  appendBox(requests, CreativeObjectKind::Rock, "Ditch Rock Mid",
            {{-9.0, 1.0, -4.0}, {-7.8, 1.8, -2.8}});
  appendBox(requests, CreativeObjectKind::Rock, "Ditch Rock South",
            {{-9.5, 1.0, 23.0}, {-7.5, 2.4, 25.0}});
  appendBox(requests, CreativeObjectKind::Rock, "House Yard Rock",
            {{29.0, kTerrainTopMeters, 6.0}, {30.5, 4.1, 7.5}},
            "boulder_01");

  return requests;
}

CreativeMapTemplateResult buildDitchHouseMap(CreativeDocumentId documentId) {
  CreativeMapTemplateResult result;
  result.requested = true;
  result.templateId = kDitchHouseMapTemplateId;
  if (documentId == kInvalidDocumentId) {
    setStatus(result, CreativeMapTemplateStatus::InvalidDocumentId,
              "creative_map_template_document_id_invalid");
    return result;
  }

  Facade facade;
  CreativeDocument document = CreativeDocument::create("Ditch House");
  if (!document.assignId(documentId) ||
      !document.setGridSettings({{-36.0, 0.0, -36.0}, 1.0,
                                 {72, 16, 72}}) ||
      !document.setWorldBounds({{-36.0, 0.0, -36.0},
                                {36.0, 16.0, 36.0}}) ||
      !facade.installDocument(std::move(document)).accepted) {
    setStatus(result, CreativeMapTemplateStatus::DocumentSetupFailed,
              "creative_map_template_document_setup_failed");
    return result;
  }

  const std::vector<CreativeTerrainControlEdit> terrainEdits =
      ditchHouseTerrainControls();
  const CreativeTerrainMutationReceipt terrain =
      facade.applyTerrainControlEdits(terrainEdits);
  if (!terrain.accepted || !terrain.changed) {
    setStatus(result, CreativeMapTemplateStatus::TerrainFailed,
              "creative_map_template_terrain_failed");
    return result;
  }

  const CreativeTerrainSurfacePlan surface =
      buildCreativeTerrainSurfacePlan(facade.document().terrainField());
  const std::vector<CreativeTerrainMaterialEdit> materialEdits =
      ditchHouseTerrainMaterials(surface);
  const CreativeTerrainMaterialMutationReceipt materials =
      facade.applyTerrainMaterialEdits(materialEdits);
  if (!surface.accepted || materialEdits.empty() || !materials.accepted ||
      !materials.changed) {
    setStatus(result, CreativeMapTemplateStatus::TerrainMaterialFailed,
              "creative_map_template_terrain_material_failed");
    return result;
  }

  const std::vector<CreativeDocumentCreateRequest> objects =
      ditchHouseObjects();
  const CreativeFacadeDocumentBatchCreateReceipt created =
      facade.createDocumentObjectsAtomically(objects);
  if (!created.accepted || !created.changed ||
      created.appliedCreateCount != objects.size()) {
    setStatus(result, CreativeMapTemplateStatus::ObjectBatchFailed,
              "creative_map_template_object_batch_failed");
    return result;
  }

  for (const CreativeObject& object : facade.document().objects()) {
    if (object.kind == CreativeObjectKind::Floor &&
        object.name == "Living Floor") {
      result.primaryFloorObjectId = object.id;
      break;
    }
  }
  result.document = facade.document();
  result.objectCount = result.document.objectCount();
  result.terrainControlCount = result.document.terrainField().controlCount();
  result.terrainMaterialOverrideCount =
      result.document.terrainMaterialField().overrideCount();
  if (result.primaryFloorObjectId == kInvalidObjectId) {
    setStatus(result, CreativeMapTemplateStatus::ObjectBatchFailed,
              "creative_map_template_primary_floor_missing");
    return result;
  }

  setStatus(result, CreativeMapTemplateStatus::Ready,
            "creative_map_template_ready", true);
  return result;
}

}  // namespace

bool isCreativeMapTemplateId(std::string_view templateId) noexcept {
  return templateId == kDitchHouseMapTemplateId;
}

std::string_view toString(CreativeMapTemplateStatus status) noexcept {
  switch (status) {
    case CreativeMapTemplateStatus::NotRequested:
      return "NotRequested";
    case CreativeMapTemplateStatus::UnknownTemplate:
      return "UnknownTemplate";
    case CreativeMapTemplateStatus::InvalidDocumentId:
      return "InvalidDocumentId";
    case CreativeMapTemplateStatus::DocumentSetupFailed:
      return "DocumentSetupFailed";
    case CreativeMapTemplateStatus::TerrainFailed:
      return "TerrainFailed";
    case CreativeMapTemplateStatus::TerrainMaterialFailed:
      return "TerrainMaterialFailed";
    case CreativeMapTemplateStatus::ObjectBatchFailed:
      return "ObjectBatchFailed";
    case CreativeMapTemplateStatus::Ready:
      return "Ready";
  }
  return "NotRequested";
}

CreativeMapTemplateResult buildCreativeMapTemplate(
    std::string_view templateId,
    CreativeDocumentId documentId) {
  if (templateId == kDitchHouseMapTemplateId) {
    return buildDitchHouseMap(documentId);
  }

  CreativeMapTemplateResult result;
  result.requested = true;
  result.templateId = templateId;
  setStatus(result, CreativeMapTemplateStatus::UnknownTemplate,
            "creative_map_template_unknown");
  return result;
}

}  // namespace iggy3d::creative
