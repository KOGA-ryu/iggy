#include "app/iggy3d/ascii_room/Authoring.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

#include "content/assets/RoomAsset.hpp"

namespace {

constexpr std::string_view kTrainingRoom =
    "#######\n"
    "#P..N.#\n"
    "#.+.$.#\n"
    "#..E..#\n"
    "#######\n";

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductAsciiRoomAuthoringRequest trainingRoomRequest() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = std::string(kTrainingRoom);
  request.sourceName = "inline/training_room.iggyroom.txt";
  request.roomId = "training_room_product";
  return request;
}

bool buildsProductOwnedAuthoringResult() {
  const iggy3d::ProductAsciiRoomAuthoringResult result =
      iggy3d::buildProductAsciiRoomAuthoring(trainingRoomRequest());
  const iggy3d::RoomAssetParseResult parsed =
      iggy3d::parseRoomAssetText(result.assetText.text);

  return expect(result.ok, "result ok") &&
         expect(result.status == "product_ascii_room_ready", "result status") &&
         expect(result.reasonCode == "product_ascii_room_ready", "result reason") &&
         expect(result.failedStage == "none", "no failed stage") &&
         expect(result.source.status == "ascii_room_ok", "source status") &&
         expect(result.grid.ok, "grid ok") &&
         expect(result.authoredRoom.ok, "authored room ok") &&
         expect(result.roomAsset.ok, "room asset ok") &&
         expect(result.assetText.ok, "asset text ok") &&
         expect(result.width == 7U, "width") &&
         expect(result.height == 5U, "height") &&
         expect(result.floorCount == 15U, "floor count") &&
         expect(result.wallCount == 20U, "wall count") &&
         expect(result.objectCount == 0U, "object count") &&
         expect(result.markerCount == 5U, "marker count") &&
         expect(result.elevatedFloorCount == 0U, "flat elevated count") &&
         expect(result.rampCount == 0U, "flat ramp count") &&
         expect(result.blockedSlopeCount == 0U, "flat blocked slope count") &&
         expect(result.staticMeshCount == 36U, "static mesh count") &&
         expect(result.anchorCount == 5U, "anchor count") &&
         expect(result.spatialSurfaceCount == 56U, "spatial surface count") &&
         expect(result.authoredRoom.authoredRoom.id == "training_room_product",
                "authored room id") &&
         expect(result.roomAsset.room.id == "training_room_product",
                "room asset id") &&
         expect(result.roomAsset.room.sourceFile ==
                    "inline/training_room.iggyroom.txt",
                "room source file") &&
         expect(parsed.ok, "asset text parses") &&
         expect(parsed.room.id == "training_room_product", "parsed room id") &&
         expect(parsed.room.staticMeshes.size() == 36U, "parsed mesh count") &&
         expect(parsed.room.anchors.size() == 5U, "parsed anchor count") &&
         expect(parsed.room.spatialSurfaces.size() == 56U,
                "parsed spatial surface count");
}

bool carriesTerrainCountsToProductResult() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "######\n"
      "#P1>!#\n"
      "######\n";
  request.sourceName = "inline/terrain_room.iggyroom.txt";
  request.roomId = "terrain_room_product";

  const iggy3d::ProductAsciiRoomAuthoringResult result =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  const iggy3d::RoomAssetParseResult parsed =
      iggy3d::parseRoomAssetText(result.assetText.text);

  return expect(result.ok, "terrain product result ok") &&
         expect(result.floorCount == 4U, "terrain floor count") &&
         expect(result.wallCount == 14U, "terrain wall count") &&
         expect(result.objectCount == 0U, "terrain object count") &&
         expect(result.markerCount == 1U, "terrain marker count") &&
         expect(result.elevatedFloorCount == 1U, "terrain elevated count") &&
         expect(result.rampCount == 1U, "terrain ramp count") &&
         expect(result.blockedSlopeCount == 1U, "terrain blocked count") &&
         expect(result.staticMeshCount == 18U, "terrain static mesh count") &&
         expect(result.anchorCount == 1U, "terrain anchor count") &&
         expect(result.spatialSurfaceCount == 32U,
                "terrain spatial surface count") &&
         expect(parsed.ok, "terrain asset text parses") &&
         expect(parsed.room.spatialSurfaces.size() == 32U,
                "terrain parsed spatial surfaces");
}

bool carriesCrateObjectToProductResult() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "#####\n"
      "#PCE#\n"
      "#####\n";
  request.sourceName = "inline/crate_room.iggyroom.txt";
  request.roomId = "crate_room_product";

  const iggy3d::ProductAsciiRoomAuthoringResult result =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  const iggy3d::RoomAssetParseResult parsed =
      iggy3d::parseRoomAssetText(result.assetText.text);

  return expect(result.ok, "crate product result ok") &&
         expect(result.floorCount == 3U, "crate floor count") &&
         expect(result.wallCount == 12U, "crate wall count") &&
         expect(result.objectCount == 1U, "crate object count") &&
         expect(result.markerCount == 2U, "crate marker count") &&
         expect(result.staticMeshCount == 16U, "crate static mesh count") &&
         expect(result.anchorCount == 2U, "crate anchor count") &&
         expect(result.spatialSurfaceCount == 29U,
                "crate spatial surface count") &&
         expect(result.authoredRoom.authoredRoom.objects.size() == 1U,
                "crate authored object count") &&
         expect(result.authoredRoom.authoredRoom.objects[0].id ==
                    "object_crate_r1_c2",
                "crate authored object id") &&
         expect(result.roomAsset.room.staticMeshes[15].role == "prop",
                "crate room asset prop role") &&
         expect(parsed.ok, "crate asset text parses") &&
         expect(parsed.room.staticMeshes.size() == 16U,
                "crate parsed mesh count") &&
         expect(parsed.room.spatialSurfaces.size() == 29U,
                "crate parsed spatial surfaces");
}

bool scaleDirectiveChangesAuthoredTileSize() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "*5\n"
      "#####\n"
      "#P.E#\n"
      "#####\n";
  request.sourceName = "inline/scaled_room.iggyroom.txt";
  request.roomId = "scaled_room_product";

  const iggy3d::ProductAsciiRoomAuthoringResult result =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  const auto& floor = result.authoredRoom.authoredRoom.floors.front();

  return expect(result.ok, "scaled product result ok") &&
         expect(result.source.hasTileScaleDirective, "scaled directive present") &&
         expect(result.source.tileScaleMeters == 5.0F, "scaled directive value") &&
         expect(result.width == 5U, "scaled layout width") &&
         expect(result.height == 3U, "scaled layout height") &&
         expect(result.floorCount == 3U, "scaled floor count") &&
         expect(result.wallCount == 12U, "scaled wall count") &&
         expect(floor.id == "floor_r1_c1", "scaled floor id") &&
         expect(floor.sizeMeters.x == 5.0F, "scaled floor size x") &&
         expect(floor.sizeMeters.z == 5.0F, "scaled floor size z") &&
         expect(floor.centerMeters.x == -5.0F, "scaled floor center x") &&
         expect(floor.centerMeters.z == 0.0F, "scaled floor center z") &&
         expect(result.roomAsset.room.staticMeshes.front().sizeMeters.x == 5.0F,
                "scaled room mesh size x");
}

bool forwardsGridValidationFailure() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "...\n"
      "...\n";
  request.sourceName = "inline/no_spawn.iggyroom.txt";
  request.roomId = "no_spawn_room";

  const iggy3d::ProductAsciiRoomAuthoringResult result =
      iggy3d::buildProductAsciiRoomAuthoring(request);

  return expect(!result.ok, "result rejected") &&
         expect(result.failedStage == "grid", "failed at grid") &&
         expect(result.status == "ascii_room_missing_player_spawn", "status") &&
         expect(result.reasonCode == "ascii_room_missing_player_spawn", "reason") &&
         expect(result.source.status == "ascii_room_ok", "source ok") &&
         expect(!result.grid.ok, "grid rejected") &&
         expect(result.roomAsset.staticMeshCount == 0U, "no room meshes") &&
         expect(result.assetText.text.empty(), "no asset text") &&
         expect(!result.diagnostics.empty(), "diagnostic forwarded");
}

bool forwardsAssetTextFailureAfterRoomBuild() {
  iggy3d::ProductAsciiRoomAuthoringRequest request = trainingRoomRequest();
  request.assetTextConfig.feetToMeters = 0.0F;

  const iggy3d::ProductAsciiRoomAuthoringResult result =
      iggy3d::buildProductAsciiRoomAuthoring(request);

  return expect(!result.ok, "result rejected") &&
         expect(result.failedStage == "asset_text", "failed at asset text") &&
         expect(result.status == "ascii_room_asset_text_invalid_conversion",
                "status") &&
         expect(result.reasonCode == "ascii_room_asset_text_invalid_conversion",
                "reason") &&
         expect(result.roomAsset.ok, "room asset was built") &&
         expect(result.staticMeshCount == 36U, "mesh count preserved") &&
         expect(result.anchorCount == 5U, "anchor count preserved") &&
         expect(result.spatialSurfaceCount == 56U, "surface count preserved") &&
         expect(!result.assetText.ok, "asset text rejected");
}

bool supportsSkippingAssetTextForLiveEditing() {
  iggy3d::ProductAsciiRoomAuthoringRequest request = trainingRoomRequest();
  request.emitAssetText = false;

  const iggy3d::ProductAsciiRoomAuthoringResult result =
      iggy3d::buildProductAsciiRoomAuthoring(request);

  return expect(result.ok, "result ok") &&
         expect(result.status == "product_ascii_room_ready", "status") &&
         expect(result.assetText.status == "not_requested", "asset text not requested") &&
         expect(result.assetText.text.empty(), "asset text empty") &&
         expect(result.roomAsset.ok, "room asset ok") &&
         expect(result.roomAsset.room.id == "training_room_product", "room id");
}

}  // namespace

int main() {
  const bool ok = buildsProductOwnedAuthoringResult() &&
                  carriesTerrainCountsToProductResult() &&
                  carriesCrateObjectToProductResult() &&
                  scaleDirectiveChangesAuthoredTileSize() &&
                  forwardsGridValidationFailure() &&
                  forwardsAssetTextFailureAfterRoomBuild() &&
                  supportsSkippingAssetTextForLiveEditing();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
