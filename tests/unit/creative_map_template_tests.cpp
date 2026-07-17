#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/world/MapTemplate.hpp"
#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"
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

  return expect(map.accepted, "ditch house template accepted") &&
         expect(map.status == cr::CreativeMapTemplateStatus::Ready,
                "ditch house template ready") &&
         expect(map.document.id() == 17U && map.document.name() == "Ditch House",
                "ditch house identity") &&
         expect(map.objectCount == 74U && map.document.objectCount() == 74U,
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
         expect(frontDoor != nullptr &&
                    std::fabs(frontDoor->bounds.max.x -
                              frontDoor->bounds.min.x - 0.2) <= 1.0e-9 &&
                    std::fabs(frontDoor->bounds.max.z -
                              frontDoor->bounds.min.z - 1.8) <= 1.0e-9,
                "front door is stored in its open pose") &&
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
  cr::CreativeRoomBakeRequest request;
  request.document = &map.document;
  request.roomId = "ditch_house";
  request.validateReachability = false;
  const cr::CreativeRoomBakeResult bake =
      cr::buildRoomAssetFromCreativeDocument(request);
  const bool boulderMeshPresent = std::any_of(
      bake.room.staticMeshes.begin(), bake.room.staticMeshes.end(),
      [](const iggy3d::RoomStaticMeshAsset& mesh) {
        return mesh.meshId == "asset:boulder_01";
      });
  const iggy3d::SceneProjectionResult projected =
      iggy3d::buildSceneProjection({}, &bake.room);
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
                "boulder mesh id survives scene projection");
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
      expect(opened.document.objectCount() == 74U &&
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
  const cr::CreativeTerrainHeightSample houseGround =
      cr::sampleCreativeTerrainHeight(map.document.terrainField(), {20, 28});

  const auto firstProvenance =
      cr::creativeWorldLayoutBuildingTemplateInstanceProvenance(
          map.worldLayout, 0U);
  const auto secondProvenance =
      cr::creativeWorldLayoutBuildingTemplateInstanceProvenance(
          map.worldLayout, 1U);
  const auto firstSync = cr::inspectCreativeWorldLayoutBuildingTemplateSync(
      map.worldLayout, 0U,
      map.buildingTemplates.empty() ? nullptr : &map.buildingTemplates[0]);
  const auto secondSync = cr::inspectCreativeWorldLayoutBuildingTemplateSync(
      map.worldLayout, 1U,
      map.buildingTemplates.empty() ? nullptr : &map.buildingTemplates[0]);

  return expect(map.accepted &&
                    map.status == cr::CreativeMapTemplateStatus::Ready,
                "builder estate template accepted") &&
         expect(map.document.id() == 31U &&
                    map.document.name() == "Builder Estate",
                "builder estate identity") &&
         expect(map.objectCount == 117U &&
                    map.terrainControlCount == 162U,
                "builder estate stable authored counts") &&
         expect(map.worldLayoutPresent &&
                    map.worldLayout.stableKey == "builder_estate_layout" &&
                    map.worldLayout.buildings.size() == 2U &&
                    map.worldLayout.rooms.size() == 8U &&
                    map.worldLayout.boxes.size() == 16U &&
                    map.worldLayout.openings.size() == 20U,
                "builder estate owns two four-room building symbols") &&
         expect(countKind(map.document, cr::CreativeObjectKind::Door) == 10U &&
                    countKind(map.document,
                              cr::CreativeObjectKind::Window) == 10U,
                "builder estate materializes every authored opening") &&
         expect(map.linkedBuildingInstanceCount == 2U &&
                    map.roomSymbolCount == 8U &&
                    map.openingSymbolCount == 20U &&
                    map.supplementalRecipeCount == 1U,
                "builder estate exposes reference-map semantic counts") &&
         expect(map.worldLayout.terrainProfiles.size() == 6U &&
                    map.worldLayout.terrainPaths.size() == 2U &&
                    map.worldLayout.terrainPathPoints.size() == 4U,
                "builder estate terrain stays in bounded semantic recipes") &&
         expect(firstProvenance.valid && secondProvenance.valid &&
                    firstProvenance.templateId ==
                        cr::kBuilderEstateHouseTemplateId &&
                    secondProvenance.templateId ==
                        cr::kBuilderEstateHouseTemplateId &&
                    firstProvenance.orientation ==
                        cr::CreativeWorldLayoutBuildingTemplateOrientation::
                            Identity &&
                    secondProvenance.orientation ==
                        cr::CreativeWorldLayoutBuildingTemplateOrientation::
                            RotateRight90,
                "builder estate preserves linked source and rotated pose") &&
         expect(firstSync.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                            Current &&
                    secondSync.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                            Current,
                "builder estate instances resolve against bundled source") &&
         expect(bridge != nullptr &&
                    cr::creativeRecipeObjectHasInstanceProvenance(
                        *bridge, cr::CreativeRecipeKind::ObjectLibrary,
                        "builder_estate_layout.objects",
                        cr::CreativeRecipeObjectRole::Source,
                        "bridge.ditch"),
                "builder estate bridge uses layout-owned object recipe") &&
         expect(westBoulder != nullptr &&
                    westBoulder->assetId == "boulder_01",
                "builder estate carries reusable asset reference") &&
         expect(map.worldLayout.objects.size() == 11U &&
                    map.worldLayout.objects[0].kind ==
                        cr::CreativeObjectKind::Bridge &&
                    map.worldLayout.objects[7].assetId == "boulder_01" &&
                    map.worldLayout.objects[9].mode ==
                        cr::CreativeObjectLibraryPlacementMode::Point,
                "builder estate exposes props and anchors as layout symbols") &&
         expect(map.document.terrainMaterialField().materialAt({20, 20}) ==
                        cr::CreativeTerrainMaterial::Sand &&
                    map.document.terrainMaterialField().materialAt({20, 44}) ==
                        cr::CreativeTerrainMaterial::Dirt,
                "builder estate ditch and road own semantic materials") &&
         expect(map.primaryFloorObjectId != cr::kInvalidObjectId,
                "builder estate exposes primary floor") &&
         expect(floor != nullptr && houseGround.present &&
                    houseGround.heightCells == 4U &&
                    floor->bounds.min.y == 4.0 && floor->bounds.max.y == 5.0,
                "builder estate floors sit exactly on authored terrain") &&
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
      expect(bake.receipt.bakedAnchorCount == 2U &&
                 bake.receipt.bakedStaticMeshCount > 0U &&
                 bake.receipt.bakedSpatialSurfaceCount > 0U,
             "builder estate bakes geometry collision and anchors") &&
      expect(saved.accepted && saved.worldLayoutPresent,
             "builder estate durable save includes world layout") &&
      expect(opened.accepted && opened.worldLayoutPresent &&
                 opened.document.objectCount() == 117U &&
                 opened.worldLayout.buildings.size() == 2U &&
                 opened.worldLayout.rooms.size() == 8U &&
                 opened.worldLayout.openings.size() == 20U &&
                 opened.worldLayout.objects.size() == 11U &&
                 opened.worldLayout.objects[7].assetId == "boulder_01" &&
                 opened.worldLayout.terrainProfiles.size() == 6U &&
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
         expect(decoded.layout.objects.size() == 11U && objectsExact,
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

  return expect(ids.size() == 1U &&
                    ids.front() == cr::kBuilderEstateHouseTemplateId,
                "built-in building template registry is explicit") &&
         expect(house.accepted &&
                    house.value.templateId ==
                        cr::kBuilderEstateHouseTemplateId &&
                    house.value.normalizedLayout.rooms.size() == 4U &&
                    house.value.normalizedLayout.openings.size() == 10U,
                "builder estate house source is reusable") &&
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
