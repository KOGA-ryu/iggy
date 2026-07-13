#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/world/MapTemplate.hpp"
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

bool unknownAndInvalidTemplateRequestsFailClosed() {
  const cr::CreativeMapTemplateResult unknown =
      cr::buildCreativeMapTemplate("castle");
  const cr::CreativeMapTemplateResult invalidId =
      cr::buildCreativeMapTemplate(cr::kDitchHouseMapTemplateId,
                                   cr::kInvalidDocumentId);

  return expect(!cr::isCreativeMapTemplateId("castle") &&
                    cr::isCreativeMapTemplateId(
                        cr::kDitchHouseMapTemplateId),
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
                  unknownAndInvalidTemplateRequestsFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
