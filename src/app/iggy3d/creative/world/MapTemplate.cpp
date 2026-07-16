#include "app/iggy3d/creative/world/MapTemplate.hpp"

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/recipes/BuildingRecipe.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

constexpr double kTerrainTopMeters = 3.0;
// The ditch-house intentionally uses a heavier slab than the descriptor
// default because it bridges the authored terrain shell.
constexpr double kDitchHouseFloorThicknessMeters = 0.25;
constexpr double kFloorTopMeters =
    kTerrainTopMeters + kDitchHouseFloorThicknessMeters;
constexpr double kWallTopMeters = 6.25;
// This authored lightweight cap intentionally overrides the one-meter default
// roof layer used by the general layout compiler.
constexpr double kDitchHouseRoofThicknessMeters = 0.30;
constexpr double kRoofTopMeters =
    kWallTopMeters + kDitchHouseRoofThicknessMeters;
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

[[nodiscard]] CreativeBuildingOpeningSpec ditchWindow(
    std::string stableKey,
    std::string name,
    double centerOffsetMeters) {
  return makeCreativeBuildingWindowOpening(
      std::move(stableKey), std::move(name), centerOffsetMeters, 2.0, 0.9,
      1.1);
}

[[nodiscard]] CreativeBuildingOpeningSpec ditchOpenDoor(
    std::string stableKey,
    std::string name,
    double centerOffsetMeters,
    CreativeBuildingOpeningPose pose) {
  CreativeBuildingOpeningSpec opening = makeCreativeBuildingDoorOpening(
      std::move(stableKey), std::move(name), centerOffsetMeters, 2.0, 3.0);
  opening.pose = pose;
  opening.insertHeightMeters = 2.25;
  opening.insertWidthMeters = 1.8;
  opening.insertThicknessMeters = 0.2;
  return opening;
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

  CreativeBuildingRecipeRequest building;
  building.stableKey = "ditch_house";
  building.name = "Ditch House Building";
  building.rootMode = CreativeBuildingRootMode::None;
  building.tags = {"map_template:ditch_house"};
  building.boxes = {
      {CreativeObjectKind::Floor,
       "floor.living",
       "Living Floor",
       {{6.0, kTerrainTopMeters, 0.0},
        {16.0, kFloorTopMeters, 10.0}}},
      {CreativeObjectKind::Floor,
       "floor.kitchen",
       "Kitchen Floor",
       {{16.0, kTerrainTopMeters, 0.0},
        {26.0, kFloorTopMeters, 10.0}}},
      {CreativeObjectKind::Floor,
       "floor.workshop",
       "Workshop Floor",
       {{6.0, kTerrainTopMeters, -10.0},
        {16.0, kFloorTopMeters, 0.0}}},
      {CreativeObjectKind::Floor,
       "floor.bedroom",
       "Bedroom Floor",
       {{16.0, kTerrainTopMeters, -10.0},
        {26.0, kFloorTopMeters, 0.0}}},
      {CreativeObjectKind::Room,
       "room.living",
       "Living Room",
       {{6.0, kFloorTopMeters, 0.0}, {16.0, kWallTopMeters, 10.0}}},
      {CreativeObjectKind::Room,
       "room.kitchen",
       "Kitchen",
       {{16.0, kFloorTopMeters, 0.0}, {26.0, kWallTopMeters, 10.0}}},
      {CreativeObjectKind::Room,
       "room.workshop",
       "Workshop",
       {{6.0, kFloorTopMeters, -10.0}, {16.0, kWallTopMeters, 0.0}}},
      {CreativeObjectKind::Room,
       "room.bedroom",
       "Bedroom",
       {{16.0, kFloorTopMeters, -10.0}, {26.0, kWallTopMeters, 0.0}}},
      {CreativeObjectKind::Roof,
       "roof.living",
       "Living Roof",
       {{6.0, kWallTopMeters, 0.0}, {16.0, kRoofTopMeters, 10.0}}},
      {CreativeObjectKind::Roof,
       "roof.kitchen",
       "Kitchen Roof",
       {{16.0, kWallTopMeters, 0.0}, {26.0, kRoofTopMeters, 10.0}}},
      {CreativeObjectKind::Roof,
       "roof.workshop",
       "Workshop Roof",
       {{6.0, kWallTopMeters, -10.0}, {16.0, kRoofTopMeters, 0.0}}},
      {CreativeObjectKind::Roof,
       "roof.bedroom",
       "Bedroom Roof",
       {{16.0, kWallTopMeters, -10.0}, {26.0, kRoofTopMeters, 0.0}}},
  };

  const double exteriorThickness = kWallHalfThickness * 2.0;
  const double interiorThickness = kInteriorWallHalfThickness * 2.0;
  const double wallHeight = kWallTopMeters - kFloorTopMeters;
  building.walls = {
      {"wall.north",
       "North Wall",
       {6.0, kFloorTopMeters, -10.0},
       {26.0, kFloorTopMeters, -10.0},
       wallHeight,
       exteriorThickness,
       {ditchWindow("window.workshop_north", "Workshop North Window", 5.0),
        ditchWindow("window.bedroom_north", "Bedroom North Window", 15.0)},
       {"North Wall West", "North Wall Center", "North Wall East"}},
      {"wall.south",
       "South Wall",
       {6.0, kFloorTopMeters, 10.0},
       {26.0, kFloorTopMeters, 10.0},
       wallHeight,
       exteriorThickness,
       {ditchOpenDoor("door.front", "Front Door Open", 5.0,
                      CreativeBuildingOpeningPose::
                          OpenFromStartNegativeNormal),
        ditchWindow("window.kitchen_south", "Kitchen South Window", 15.0)},
       {"South Wall West", "South Wall Center", "South Wall East"}},
      {"wall.west",
       "West Wall",
       {6.0, kFloorTopMeters, -10.0},
       {6.0, kFloorTopMeters, 10.0},
       wallHeight,
       exteriorThickness,
       {ditchWindow("window.workshop_west", "Workshop West Window", 5.0),
        ditchWindow("window.living_west", "Living West Window", 15.0)},
       {"West Wall North", "West Wall Center", "West Wall South"}},
      {"wall.east",
       "East Wall",
       {26.0, kFloorTopMeters, -10.0},
       {26.0, kFloorTopMeters, 10.0},
       wallHeight,
       exteriorThickness,
       {ditchWindow("window.bedroom_east", "Bedroom East Window", 5.0),
        ditchWindow("window.kitchen_east", "Kitchen East Window", 15.0)},
       {"East Wall North", "East Wall Center", "East Wall South"}},
      {"wall.interior_long",
       "Interior Long Wall",
       {16.0, kFloorTopMeters, -10.0},
       {16.0, kFloorTopMeters, 10.0},
       wallHeight,
       interiorThickness,
       {ditchOpenDoor("door.interior_north", "North Interior Door Open", 5.0,
                      CreativeBuildingOpeningPose::
                          OpenFromStartNegativeNormal),
        ditchOpenDoor("door.interior_south", "South Interior Door Open", 15.0,
                      CreativeBuildingOpeningPose::
                          OpenFromStartPositiveNormal)},
       {"Interior Long Wall North", "Interior Long Wall Center",
        "Interior Long Wall South"}},
      {"wall.interior_cross",
       "Interior Cross Wall",
       {6.0, kFloorTopMeters, 0.0},
       {26.0, kFloorTopMeters, 0.0},
       wallHeight,
       interiorThickness,
       {ditchOpenDoor("door.interior_west", "West Interior Door Open", 5.0,
                      CreativeBuildingOpeningPose::
                          OpenFromStartPositiveNormal),
        ditchOpenDoor("door.interior_east", "East Interior Door Open", 15.0,
                      CreativeBuildingOpeningPose::
                          OpenFromStartNegativeNormal)},
       {"Interior Cross Wall West", "Interior Cross Wall Center",
        "Interior Cross Wall East"}},
  };

  const CreativeBuildingRecipeResult buildingRecipe =
      buildCreativeBuildingRecipe(building);
  if (!buildingRecipe.receipt.accepted) {
    return {};
  }
  CreativeRecipeMaterializeResult materialized =
      materializeCreativeRecipe(buildingRecipe.plan, 1U);
  if (!materialized.receipt.accepted) {
    return {};
  }
  requests.insert(requests.end(),
                  std::make_move_iterator(materialized.createRequests.begin()),
                  std::make_move_iterator(materialized.createRequests.end()));

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
