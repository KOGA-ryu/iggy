#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/world/MapTemplate.hpp"
#include "app/iggy3d/creative/world/WorldLayoutArchitecture.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"
#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"
#include "content/assets/StaticMeshAsset.hpp"
#include "projection/scene/SceneProjection.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::uint64_t countKind(const cr::CreativeDocument& document,
                        cr::CreativeObjectKind kind) {
  return static_cast<std::uint64_t>(std::count_if(
      document.objects().begin(), document.objects().end(),
      [kind](const cr::CreativeObject& object) { return object.kind == kind; }));
}

const cr::CreativeObject* findNamed(const cr::CreativeDocument& document,
                                    std::string_view name) {
  const auto found = std::find_if(
      document.objects().begin(), document.objects().end(),
      [name](const cr::CreativeObject& object) { return object.name == name; });
  return found == document.objects().end() ? nullptr : &*found;
}

bool sameRect(cr::CreativeWorldLayoutRect lhs,
              cr::CreativeWorldLayoutRect rhs) {
  return lhs.minimum == rhs.minimum && lhs.maximum == rhs.maximum;
}

const cr::CreativeObject* findFirstKind(const cr::CreativeDocument& document,
                                        cr::CreativeObjectKind kind) {
  const auto found = std::find_if(
      document.objects().begin(), document.objects().end(),
      [kind](const cr::CreativeObject& object) { return object.kind == kind; });
  return found == document.objects().end() ? nullptr : &*found;
}

bool sameVec(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameAuthoredObject(const cr::CreativeObject& lhs,
                        const cr::CreativeObject& rhs) {
  return lhs.id == rhs.id && lhs.kind == rhs.kind && lhs.name == rhs.name &&
         lhs.assetId == rhs.assetId &&
         sameVec(lhs.transform.position, rhs.transform.position) &&
         sameVec(lhs.transform.rotationEulerRadians,
                 rhs.transform.rotationEulerRadians) &&
         sameVec(lhs.transform.scale, rhs.transform.scale) &&
         sameVec(lhs.bounds.min, rhs.bounds.min) &&
         sameVec(lhs.bounds.max, rhs.bounds.max) &&
         lhs.layerId == rhs.layerId && lhs.visible == rhs.visible &&
         lhs.locked == rhs.locked && lhs.tags == rhs.tags &&
         lhs.parentId == rhs.parentId &&
         lhs.attachmentSocket == rhs.attachmentSocket;
}

bool ditchHouseHasDeterministicAuthoredShape() {
  const cr::CreativeMapTemplateResult map =
      cr::buildCreativeMapTemplate(cr::kDitchHouseMapTemplateId, 17U);
  const cr::CreativeObject* frontDoor =
      findNamed(map.document, "Front Door Open");
  const cr::CreativeObject* yardRock =
      findNamed(map.document, "House Yard Rock");
  const cr::CreativeObject* livingFloor =
      findNamed(map.document, "Living Floor");
  const cr::CreativeObject* livingRoom =
      findNamed(map.document, "Living Room");
  const cr::CreativeObject* livingRoof =
      findNamed(map.document, "Living Roof");
  const cr::CreativeObject* northWallWest =
      findNamed(map.document, "North Wall West");
  const cr::CreativeWorldLayoutArchitecturalProfile residential =
      cr::defaultCreativeWorldLayoutArchitecturalProfile(
          cr::CreativeWorldLayoutArchitecturalProfileKind::Residential);
  const auto hasDitchHouseProfileTag = [](const cr::CreativeObject* object) {
    return object != nullptr &&
           std::find(object->tags.begin(), object->tags.end(),
                     "architecture_profile:ditch_house") != object->tags.end();
  };

  return expect(map.accepted, "ditch house template accepted") &&
         expect(map.status == cr::CreativeMapTemplateStatus::Ready,
                "ditch house template ready") &&
         expect(map.document.id() == 17U && map.document.name() == "Ditch House",
                "ditch house identity") &&
         expect(map.objectCount == 132U && map.document.objectCount() == 132U,
                "ditch house stable object count") &&
         expect(map.terrainControlCount == 169U,
                "ditch house stable terrain control count") &&
         expect(map.terrainMaterialOverrideCount > 0U,
                "ditch house terrain is materially authored") &&
         expect(countKind(map.document, cr::CreativeObjectKind::Room) == 4U,
                "ditch house has four room metadata volumes") &&
         expect(countKind(map.document, cr::CreativeObjectKind::Floor) == 4U,
                "ditch house has four room floors") &&
         expect(countKind(map.document, cr::CreativeObjectKind::Wall) == 32U,
                "ditch house has segmented wall ownership") &&
         expect(countKind(map.document, cr::CreativeObjectKind::Door) == 5U,
                "ditch house has five open door panels") &&
         expect(countKind(map.document, cr::CreativeObjectKind::Window) == 7U,
                "ditch house has seven window inserts") &&
         expect(countKind(map.document, cr::CreativeObjectKind::SpawnPoint) ==
                    1U &&
                    countKind(map.document, cr::CreativeObjectKind::NpcSpawn) ==
                        1U,
                "ditch house has player and npc anchors") &&
         expect(map.primaryFloorObjectId != cr::kInvalidObjectId,
                "ditch house primary floor exposed") &&
         expect(livingFloor != nullptr && livingRoom != nullptr &&
                    livingRoof != nullptr && northWallWest != nullptr &&
                    std::fabs((livingFloor->bounds.max.y -
                               livingFloor->bounds.min.y) -
                              5.0 *
                                  cr::defaultCreativeStructuralLayerThicknessMeters(
                                      cr::CreativeObjectKind::Floor)) <=
                        1.0e-9 &&
                    std::fabs((livingRoom->bounds.max.y -
                               livingRoom->bounds.min.y) -
                              residential.floorToFloorMeters) <= 1.0e-9 &&
                    std::fabs((livingRoof->bounds.max.y -
                               livingRoof->bounds.min.y) -
                              cr::defaultCreativeStructuralLayerThicknessMeters(
                                  cr::CreativeObjectKind::Roof)) <= 1.0e-9 &&
                    std::fabs((northWallWest->bounds.max.z -
                               northWallWest->bounds.min.z) -
                              cr::defaultCreativeWallGeometry()
                                  .thicknessMeters) <= 1.0e-9 &&
                    livingFloor->bounds.max.y == livingRoom->bounds.min.y &&
                    livingRoom->bounds.max.y == livingRoof->bounds.min.y &&
                    hasDitchHouseProfileTag(livingFloor) &&
                    hasDitchHouseProfileTag(northWallWest),
                "ditch house scale follows its named architectural profile") &&
         expect(frontDoor != nullptr &&
                    frontDoor->door.initialState ==
                        cr::CreativeDoorInitialState::Open &&
                    frontDoor->door.hingeSide ==
                        cr::CreativeDoorHingeSide::MinimumEdge &&
                    frontDoor->door.swingSide ==
                        cr::CreativeDoorSwingSide::NegativeNormal,
                "front door retains semantic open state and swing") &&
         expect(yardRock != nullptr && yardRock->assetId == "boulder_01",
                "yard rock references imported boulder asset");
}

bool ditchTerrainHasLowDryChannelAndRaisedHouseBank() {
  const cr::CreativeMapTemplateResult map =
      cr::buildCreativeMapTemplate(cr::kDitchHouseMapTemplateId);
  const cr::CreativeTerrainHeightSample channel =
      cr::sampleCreativeTerrainHeight(map.document.terrainField(), {26, 31});
  const cr::CreativeTerrainHeightSample houseBank =
      cr::sampleCreativeTerrainHeight(map.document.terrainField(), {51, 36});
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainSurfacePlan(map.document.terrainField());
  const cr::CreativeTerrainRenderPlan rendered =
      cr::buildCreativeTerrainRenderPlan(
          surface, map.document.terrainMaterialField(),
          map.document.gridSettings().origin,
          map.document.gridSettings().cellSizeMeters);

  return expect(map.accepted, "terrain setup accepted") &&
         expect(channel.present && channel.heightCells == 1U,
                "ditch channel is one cell high") &&
         expect(houseBank.present && houseBank.heightCells == 3U,
                "house bank is three cells high") &&
         expect(map.document.terrainMaterialField().materialAt({26, 31}) ==
                    cr::CreativeTerrainMaterial::Sand,
                "ditch bed is sand") &&
         expect(map.document.terrainMaterialField().materialAt({31, 31}) ==
                    cr::CreativeTerrainMaterial::Dirt,
                "ditch bank is dirt") &&
         expect(surface.accepted && !surface.columns.empty() &&
                    surface.columns.size() <=
                        cr::kCreativeTerrainRenderPatchCapacity,
                "terrain remains inside render capacity") &&
         expect(rendered.accepted &&
                    std::any_of(
                        rendered.patches.begin(), rendered.patches.end(),
                        [](const cr::CreativeTerrainSurfacePatch& patch) {
                          return patch.center.x >= 15.0 &&
                                 patch.center.x <= 16.0 &&
                                 patch.center.z >= 0.0 &&
                                 patch.center.z <= 1.0 &&
                                 patch.center.y == 3.0;
                        }),
                "raised terrain renders beneath the house in world space");
}

bool ditchHouseBakesToRuntimeGeometryAndAnchors() {
  const cr::CreativeMapTemplateResult map =
      cr::buildCreativeMapTemplate(cr::kDitchHouseMapTemplateId);
  const cr::CreativeObject* livingRoof =
      findNamed(map.document, "Living Roof");
  const cr::CreativeObject* frontDoor =
      findNamed(map.document, "Front Door Open");
  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  cr::CreativeRoomBakeRequest request;
  request.document = &map.document;
  request.roomId = "ditch_house";
  request.validateReachability = false;
  request.staticMeshAssetCatalog = &catalog;
  const cr::CreativeRoomBakeResult bake =
      cr::buildRoomAssetFromCreativeDocument(request);
  const bool boulderMeshPresent = std::any_of(
      bake.room.staticMeshes.begin(), bake.room.staticMeshes.end(),
      [](const iggy3d::RoomStaticMeshAsset& mesh) {
        return mesh.meshId == "asset:boulder_01";
      });
  const iggy3d::SceneProjectionResult projected =
      iggy3d::buildSceneProjection({}, &bake.room);
  const auto stableMeshId = [](const cr::CreativeObject* object) {
    return object == nullptr
               ? std::string{}
               : "creative_object_" + std::to_string(object->id);
  };
  const std::string roofMeshId = stableMeshId(livingRoof);
  const std::string doorMeshId = stableMeshId(frontDoor);
  const auto findBakedMesh = [&](std::string_view id) {
    return std::find_if(
        bake.room.staticMeshes.begin(), bake.room.staticMeshes.end(),
        [id](const iggy3d::RoomStaticMeshAsset& mesh) { return mesh.id == id; });
  };
  const auto findProjectedMesh = [&](std::string_view id) {
    return std::find_if(
        projected.room.meshes.begin(), projected.room.meshes.end(),
        [id](const iggy3d::SceneRoomMeshItem& mesh) { return mesh.id == id; });
  };
  const auto bakedRoof = findBakedMesh(roofMeshId);
  const auto bakedDoor = findBakedMesh(doorMeshId);
  const auto projectedRoof = findProjectedMesh(roofMeshId);
  const auto projectedDoor = findProjectedMesh(doorMeshId);
  const bool boulderProjectionPresent = std::any_of(
      projected.room.meshes.begin(), projected.room.meshes.end(),
      [](const iggy3d::SceneRoomMeshItem& mesh) {
        return mesh.meshId == "asset:boulder_01";
      });

  return expect(map.accepted, "bake setup accepted") &&
         expect(bake.receipt.accepted &&
                    bake.receipt.status == cr::CreativeRoomBakeStatus::Baked,
                "ditch house room bake accepted") &&
         expect(bake.receipt.skippedRoomMetadataCount == 4U,
                "room volumes remain metadata") &&
         expect(bake.receipt.bakedAnchorCount == 2U,
                "player and npc anchors bake") &&
         expect(bake.receipt.bakedStaticMeshCount > 0U &&
                    bake.receipt.bakedSpatialSurfaceCount > 0U,
                "house and terrain bake to geometry and collision") &&
         expect(boulderMeshPresent,
                "boulder asset reference survives room bake") &&
         expect(boulderProjectionPresent,
                "boulder mesh id survives scene projection") &&
         expect(livingRoof != nullptr && frontDoor != nullptr &&
                    bakedRoof != bake.room.staticMeshes.end() &&
                    bakedDoor != bake.room.staticMeshes.end() &&
                    bakedRoof->semanticRole == "Roof" &&
                    bakedDoor->semanticRole == "Door" &&
                    !bakedRoof->materialId.empty() &&
                    !bakedDoor->materialId.empty(),
                "room bake preserves exact architectural meaning and material") &&
         expect(projectedRoof != projected.room.meshes.end() &&
                    projectedDoor != projected.room.meshes.end() &&
                    projected.room.openingVisible &&
                    projectedRoof->semanticRole == bakedRoof->semanticRole &&
                    projectedDoor->semanticRole == bakedDoor->semanticRole &&
                    projectedRoof->materialId == bakedRoof->materialId &&
                    projectedDoor->materialId == bakedDoor->materialId,
                "scene projection preserves architectural meaning and material");
}

bool ditchHouseSurvivesDurableSaveRoundTrip() {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      "iggy3d_creative_map_template_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);

  cr::CreativeMapTemplateResult map =
      cr::buildCreativeMapTemplate(cr::kDitchHouseMapTemplateId, 23U);
  iggy3d::CreativeWorldSaveRequest save;
  save.saveRoot = root;
  save.saveId = "ditch_house";
  save.document = &map.document;
  save.worldTitle = "Ditch House";
  save.saveTitle = "Ditch House";
  const iggy3d::CreativeWorldSaveResult saved =
      iggy3d::saveCreativeWorld(save);
  const iggy3d::CreativeWorldOpenResult opened =
      iggy3d::openCreativeWorld({root, "ditch_house"});
  const cr::CreativeObject* reopenedRock =
      findNamed(opened.document, "House Yard Rock");

  const bool ok =
      expect(map.accepted, "save setup accepted") &&
      expect(saved.accepted && saved.saved,
             "ditch house durable save accepted") &&
      expect(opened.accepted && opened.document.id() == 23U,
             "ditch house durable save reopened") &&
      expect(opened.document.objectCount() == 132U &&
                 opened.document.terrainField().controlCount() == 169U &&
                 opened.document.terrainMaterialField().overrideCount() ==
                     map.terrainMaterialOverrideCount,
             "ditch house authored content survives round trip") &&
      expect(reopenedRock != nullptr && reopenedRock->assetId == "boulder_01",
             "boulder asset reference survives durable save");
  std::filesystem::remove_all(root, error);
  return ok;
}

bool builderEstateIsDeterministicLinkedSemanticMap() {
  const cr::CreativeMapTemplateResult map =
      cr::buildCreativeMapTemplate(cr::kBuilderEstateMapTemplateId, 31U);
  const cr::CreativeMapTemplateResult repeated =
      cr::buildCreativeMapTemplate(cr::kBuilderEstateMapTemplateId, 31U);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(map.worldLayout);
  const cr::CreativeWorldLayoutEncodeResult repeatedEncoded =
      cr::encodeCreativeWorldLayout(repeated.worldLayout);
  const cr::CreativeObject* bridge = findNamed(map.document, "Ditch Bridge");
  const cr::CreativeObject* westBoulder =
      findNamed(map.document, "Ditch West Boulder");
  const cr::CreativeObject* floor =
      findFirstKind(map.document, cr::CreativeObjectKind::Floor);
  const cr::CreativeObject* roof =
      findFirstKind(map.document, cr::CreativeObjectKind::Roof);
  const cr::CreativeTerrainHeightSample houseGround =
      cr::sampleCreativeTerrainHeight(map.document.terrainField(), {20, 36});
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(map.worldLayout);
  const cr::CreativeWorldLayoutBuildingDimensions dimensions =
      cr::measureCreativeWorldLayoutBuildingDimensions(
          map.document.gridSettings(), map.worldLayout, 0U);
  const std::size_t facadeCount =
      expanded.accepted
          ? static_cast<std::size_t>(std::count_if(
                expanded.expanded.walls.begin(), expanded.expanded.walls.end(),
                [](const cr::CreativeWorldLayoutWall& wall) {
                  return wall.baseLayer == 4.0 && wall.heightCells == 10U;
                }))
          : 0U;
  const std::size_t partitionCount =
      expanded.accepted
          ? static_cast<std::size_t>(std::count_if(
                expanded.expanded.walls.begin(), expanded.expanded.walls.end(),
                [](const cr::CreativeWorldLayoutWall& wall) {
                  return wall.heightCells == 5U;
                }))
          : 0U;
  bool repeatedLevelFootprints = map.worldLayout.rooms.size() == 8U;
  for (std::size_t roomIndex = 0U;
       repeatedLevelFootprints && roomIndex < 4U; ++roomIndex) {
    repeatedLevelFootprints =
        sameRect(map.worldLayout.rooms[roomIndex].footprint,
                 map.worldLayout.rooms[roomIndex + 4U].footprint);
  }

  const auto firstProvenance =
      cr::creativeWorldLayoutBuildingTemplateInstanceProvenance(
          map.worldLayout, 0U);
  const auto firstSync = cr::inspectCreativeWorldLayoutBuildingTemplateSync(
      map.worldLayout, 0U,
      map.buildingTemplates.empty() ? nullptr : &map.buildingTemplates[0]);
  const auto firstBlockoutSync =
      cr::inspectCreativeWorldLayoutBuildingBlockoutSync(map.worldLayout, 0U);

  if (!map.accepted) {
    std::cerr << "Builder Estate generation rejected: " << map.reasonCode
              << '\n';
  }
  if (map.objectCount != 211U || map.terrainControlCount != 111U ||
      map.document.terrainOperationStack().operations.size() != 2U) {
    std::cerr << "Builder Estate counts: objects=" << map.objectCount
              << " terrain-controls=" << map.terrainControlCount
              << " terrain-operations="
              << map.document.terrainOperationStack().operations.size()
              << " terrain-height-cells="
              << map.document.terrainHeightField().cellCount() << '\n';
  }
  return expect(map.accepted &&
                    map.status == cr::CreativeMapTemplateStatus::Ready,
                "builder estate template accepted") &&
         expect(map.document.id() == 31U &&
                    map.document.name() == "Builder Estate",
                "builder estate identity") &&
         expect(map.objectCount == 211U &&
                    map.terrainControlCount == 111U &&
                    map.document.terrainOperationStack().operations.size() ==
                        2U &&
                    map.document.terrainHeightField().cellCount() >
                        map.terrainControlCount,
                "builder estate stable authored terrain sources") &&
         expect(map.worldLayoutPresent &&
                    map.worldLayout.stableKey == "builder_estate_layout" &&
                    map.worldLayout.buildings.size() == 1U &&
                    map.worldLayout.levels.size() == 2U &&
                    map.worldLayout.rooms.size() == 8U &&
                    map.worldLayout.boxes.empty() &&
                    map.worldLayout.openings.size() == 22U &&
                    map.worldLayout.verticalConnectors.size() == 1U,
                "builder estate owns one connected two-storey building") &&
         expect(expanded.accepted && expanded.expanded.walls.size() == 8U &&
                    facadeCount == 4U && partitionCount == 4U &&
                    map.worldLayout.levels[0].floorTopLayer == 4.0 &&
                    map.worldLayout.levels[1].floorTopLayer == 9.0 &&
                    map.worldLayout.levels[0].wallHeightCells == 5U &&
                    map.worldLayout.levels[1].wallHeightCells == 5U &&
                    sameRect(map.worldLayout.buildings[0].rootFootprint,
                             {{16, 28}, {64, 76}}) &&
                    repeatedLevelFootprints,
                "builder estate owns four continuous facades and equal level shells") &&
         expect(countKind(map.document, cr::CreativeObjectKind::Door) == 7U &&
                    countKind(map.document,
                              cr::CreativeObjectKind::Window) == 15U,
                "builder estate materializes every authored opening") &&
         expect(map.linkedBuildingInstanceCount == 1U &&
                    map.roomSymbolCount == 8U &&
                    map.openingSymbolCount == 22U &&
                    map.supplementalRecipeCount == 1U,
                "builder estate exposes reference-map semantic counts") &&
         expect(map.worldLayout.terrainProfiles.size() == 9U &&
                    map.worldLayout.terrainPaths.size() == 2U &&
                    map.worldLayout.terrainPaths[0].recipe.points.size() == 2U &&
                    map.worldLayout.terrainPaths[1].recipe.points.size() == 2U,
                "builder estate terrain stays in bounded semantic recipes") &&
         expect(firstProvenance.valid &&
                    firstProvenance.templateId ==
                        cr::kBuilderEstateHouseTemplateId &&
                    firstProvenance.orientation ==
                        cr::CreativeWorldLayoutBuildingTemplateOrientation::
                            Identity,
                "builder estate preserves its linked source and pose") &&
         expect(firstSync.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                            Current,
                "builder estate resolves against its bundled source") &&
         expect(firstBlockoutSync.accepted &&
                    firstBlockoutSync.state ==
                        cr::CreativeWorldLayoutBuildingBlockoutSyncState::
                            Unlinked,
                "template instance does not retain competing blockout ownership") &&
         expect(bridge != nullptr &&
                    cr::creativeRecipeObjectHasInstanceProvenance(
                        *bridge, cr::CreativeRecipeKind::ObjectLibrary,
                        "builder_estate_layout.objects.bridge.ditch",
                        cr::CreativeRecipeObjectRole::Source,
                        "bridge.ditch"),
                "builder estate bridge uses layout-owned object recipe") &&
         expect(westBoulder != nullptr &&
                    westBoulder->assetId == "boulder_01",
                "builder estate carries reusable asset reference") &&
         expect(map.worldLayout.objects.size() == 9U &&
                    map.worldLayout.objects[0].kind ==
                        cr::CreativeObjectKind::Bridge &&
                    map.worldLayout.objects[5].assetId == "boulder_01" &&
                    map.worldLayout.objects[7].mode ==
                        cr::CreativeObjectLibraryPlacementMode::Point,
                "builder estate exposes props and anchors as layout symbols") &&
         expect(map.document.terrainMaterialField().materialAt({20, 20}) ==
                        cr::CreativeTerrainMaterial::Sand &&
                    map.document.terrainMaterialField().materialAt({20, 78}) ==
                        cr::CreativeTerrainMaterial::Dirt,
                "builder estate ditch and road own semantic materials") &&
         expect(map.primaryFloorObjectId != cr::kInvalidObjectId,
                "builder estate exposes primary floor") &&
         expect(floor != nullptr && houseGround.present &&
                    houseGround.heightCells == 4U &&
                    floor->bounds.max.y == 4.0 &&
                    std::fabs((floor->bounds.max.y - floor->bounds.min.y) -
                              6.0 *
                                  cr::defaultCreativeStructuralLayerThicknessMeters(
                                      cr::CreativeObjectKind::Floor)) <=
                        1.0e-9,
                "builder estate floor top meets terrain with exact slab thickness") &&
         expect(roof != nullptr && roof->bounds.min.y == 14.0 &&
                    roof->bounds.max.y == 14.25 &&
                    sameVec(roof->transform.scale, {1.0, 1.0, 1.0}),
                "builder estate roof rises from its upper-storey support plane") &&
         expect(dimensions.accepted && dimensions.occupiedLevelCount == 2U &&
                    dimensions.uniformFloorToFloor &&
                    dimensions.uniformWallHeight &&
                    dimensions.footprintWidthMeters == 48.0 &&
                    dimensions.footprintDepthMeters == 48.0 &&
                    dimensions.minimumFloorToFloorMeters == 5.0 &&
                    dimensions.exteriorFacadeHeightMeters == 10.0 &&
                    std::fabs(dimensions.totalHeightMeters - 10.55) <= 1.0e-9,
                "builder estate publishes its exact architectural scale") &&
         expect(encoded.accepted && repeatedEncoded.accepted &&
                    encoded.encodedText == repeatedEncoded.encodedText &&
                    repeated.objectCount == map.objectCount &&
                    repeated.terrainControlCount == map.terrainControlCount,
                "builder estate generation is deterministic");
}

bool builderEstateBakesAndSurvivesWorldLayoutRoundTrip() {
  cr::CreativeMapTemplateResult map =
      cr::buildCreativeMapTemplate(cr::kBuilderEstateMapTemplateId, 32U);
  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &map.document;
  bakeRequest.roomId = "builder_estate";
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult bake =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);

  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      "iggy3d_builder_estate_map_template_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  iggy3d::CreativeWorldSaveRequest save;
  save.saveRoot = root;
  save.saveId = "builder_estate";
  save.document = &map.document;
  save.worldTitle = "Builder Estate";
  save.saveTitle = "Builder Estate";
  save.worldLayout = &map.worldLayout;
  const iggy3d::CreativeWorldSaveResult saved =
      iggy3d::saveCreativeWorld(save);
  const iggy3d::CreativeWorldOpenResult opened =
      iggy3d::openCreativeWorld({root, "builder_estate"});

  const bool ok =
      expect(map.accepted && bake.receipt.accepted,
             "builder estate room bake accepted") &&
      expect(bake.receipt.bakedAnchorCount == 4U &&
                 bake.receipt.bakedStaticMeshCount > 0U &&
                 bake.receipt.bakedSpatialSurfaceCount > 0U,
             "builder estate bakes geometry collision and anchors") &&
      expect(saved.accepted && saved.worldLayoutPresent,
             "builder estate durable save includes world layout") &&
      expect(opened.accepted && opened.worldLayoutPresent &&
                 opened.document.objectCount() == 211U &&
                 opened.worldLayout.buildings.size() == 1U &&
                 opened.worldLayout.levels.size() == 2U &&
                 opened.worldLayout.rooms.size() == 8U &&
                 opened.worldLayout.openings.size() == 22U &&
                 opened.worldLayout.verticalConnectors.size() == 1U &&
                 opened.worldLayout.objects.size() == 9U &&
                 opened.worldLayout.objects[5].assetId == "boulder_01" &&
                 opened.worldLayout.terrainProfiles.size() == 9U &&
                 std::all_of(opened.worldLayout.terrainProfiles.begin(),
                             opened.worldLayout.terrainProfiles.end(),
                             [](const auto& profile) {
                               return profile.kind ==
                                      cr::CreativeTerrainRecipeKind::Plateau;
                             }),
             "builder estate document and symbols reopen together");
  std::filesystem::remove_all(root, error);
  return ok;
}

bool builderEstateEncodedLayoutRegeneratesExactThreeDimensionalOutput() {
  const cr::CreativeMapTemplateResult source =
      cr::buildCreativeMapTemplate(cr::kBuilderEstateMapTemplateId, 33U);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(source.worldLayout);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(encoded.encodedText);

  cr::CreativeDocument blank = cr::CreativeDocument::create("Builder Estate");
  const bool blankReady = source.accepted && encoded.accepted &&
                          decoded.accepted && blank.assignId(33U) &&
                          blank.setGridSettings(source.document.gridSettings()) &&
                          blank.setWorldBounds(source.document.worldBounds());
  cr::Facade facade;
  const cr::CreativeFacadeDocumentInstallReceipt installed =
      blankReady ? facade.installDocument(std::move(blank))
                 : cr::CreativeFacadeDocumentInstallReceipt{};
  const cr::CreativeWorldLayoutCompileResult compiled =
      installed.accepted
          ? cr::buildCreativeWorldLayoutPlan(facade.document(), decoded.layout)
          : cr::CreativeWorldLayoutCompileResult{};
  const cr::CreativeWorldLayoutApplyReceipt applied =
      compiled.receipt.accepted
          ? cr::applyCreativeWorldLayoutPlan(facade, compiled.plan)
          : cr::CreativeWorldLayoutApplyReceipt{};

  const cr::CreativeDocument& regenerated = facade.document();
  const bool objectsExact =
      applied.accepted &&
      source.document.objects().size() == regenerated.objects().size() &&
      std::equal(source.document.objects().begin(),
                 source.document.objects().end(),
                 regenerated.objects().begin(), sameAuthoredObject);
  const bool terrainExact =
      applied.accepted &&
      std::equal(source.document.terrainField().controls().begin(),
                 source.document.terrainField().controls().end(),
                 regenerated.terrainField().controls().begin(),
                 regenerated.terrainField().controls().end()) &&
      std::equal(source.document.terrainMaterialField().overrides().begin(),
                 source.document.terrainMaterialField().overrides().end(),
                 regenerated.terrainMaterialField().overrides().begin(),
                 regenerated.terrainMaterialField().overrides().end());

  return expect(blankReady && installed.accepted && compiled.receipt.accepted &&
                    applied.accepted && applied.changed,
                "encoded builder estate layout regenerates from a blank document") &&
         expect(decoded.layout.objects.size() == 9U && objectsExact,
                "regenerated builder estate object graph is exact") &&
         expect(terrainExact,
                "regenerated builder estate terrain and materials are exact");
}

bool builtInHouseTemplateRegistryIsExplicit() {
  const std::span<const std::string_view> ids =
      cr::creativeBuiltInBuildingTemplateIds();
  const cr::CreativeWorldLayoutBuildingTemplateResult house =
      cr::buildCreativeBuiltInBuildingTemplate(
          cr::kBuilderEstateHouseTemplateId);
  const cr::CreativeWorldLayoutBuildingTemplateResult unknown =
      cr::buildCreativeBuiltInBuildingTemplate("unknown.house");
  const cr::CreativeWorldLayoutBuildingBlockoutSyncReceipt blockoutSync =
      house.accepted
          ? cr::inspectCreativeWorldLayoutBuildingBlockoutSync(
                house.value.normalizedLayout, 0U)
          : cr::CreativeWorldLayoutBuildingBlockoutSyncReceipt{};

  return expect(ids.size() == 1U &&
                    ids.front() == cr::kBuilderEstateHouseTemplateId,
                "built-in building template registry is explicit") &&
         expect(house.accepted &&
                    house.value.templateId ==
                        cr::kBuilderEstateHouseTemplateId &&
                    house.value.normalizedLayout.levels.size() == 2U &&
                    house.value.normalizedLayout.rooms.size() == 8U &&
                    house.value.normalizedLayout.openings.size() == 22U &&
                    house.value.normalizedLayout.verticalConnectors.size() ==
                        1U &&
                    blockoutSync.accepted &&
                    blockoutSync.state ==
                        cr::CreativeWorldLayoutBuildingBlockoutSyncState::
                            Unlinked,
                "builder estate house is reusable under template ownership") &&
         expect(!unknown.accepted &&
                    unknown.status ==
                        cr::CreativeWorldLayoutBuildingTemplateStatus::
                            InvalidRequest,
                "unknown built-in building template rejects");
}

bool unknownAndInvalidTemplateRequestsFailClosed() {
  const cr::CreativeMapTemplateResult unknown =
      cr::buildCreativeMapTemplate("castle");
  const cr::CreativeMapTemplateResult invalidId =
      cr::buildCreativeMapTemplate(cr::kDitchHouseMapTemplateId,
                                   cr::kInvalidDocumentId);

  return expect(!cr::isCreativeMapTemplateId("castle") &&
                    cr::isCreativeMapTemplateId(
                        cr::kDitchHouseMapTemplateId) &&
                    cr::isCreativeMapTemplateId(
                        cr::kBuilderEstateMapTemplateId),
                "template id registry is explicit") &&
         expect(!unknown.accepted &&
                    unknown.status ==
                        cr::CreativeMapTemplateStatus::UnknownTemplate,
                "unknown template rejected") &&
         expect(!invalidId.accepted &&
                    invalidId.status ==
                        cr::CreativeMapTemplateStatus::InvalidDocumentId,
                "invalid document id rejected");
}

}  // namespace

int main() {
  const bool ok = ditchHouseHasDeterministicAuthoredShape() &&
                  ditchTerrainHasLowDryChannelAndRaisedHouseBank() &&
                  ditchHouseBakesToRuntimeGeometryAndAnchors() &&
                  ditchHouseSurvivesDurableSaveRoundTrip() &&
                  builderEstateIsDeterministicLinkedSemanticMap() &&
                  builderEstateBakesAndSurvivesWorldLayoutRoundTrip() &&
                  builderEstateEncodedLayoutRegeneratesExactThreeDimensionalOutput() &&
                  builtInHouseTemplateRegistryIsExplicit() &&
                  unknownAndInvalidTemplateRequestsFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
