#include "app/iggy3d/ProductAsciiRoomAuthoring.hpp"

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
         expect(result.markerCount == 5U, "marker count") &&
         expect(result.elevatedFloorCount == 0U, "flat elevated count") &&
         expect(result.rampCount == 0U, "flat ramp count") &&
         expect(result.blockedSlopeCount == 0U, "flat blocked slope count") &&
         expect(result.staticMeshCount == 35U, "static mesh count") &&
         expect(result.anchorCount == 5U, "anchor count") &&
         expect(result.spatialSurfaceCount == 55U, "spatial surface count") &&
         expect(result.authoredRoom.authoredRoom.id == "training_room_product",
                "authored room id") &&
         expect(result.roomAsset.room.id == "training_room_product",
                "room asset id") &&
         expect(result.roomAsset.room.sourceFile ==
                    "inline/training_room.iggyroom.txt",
                "room source file") &&
         expect(parsed.ok, "asset text parses") &&
         expect(parsed.room.id == "training_room_product", "parsed room id") &&
         expect(parsed.room.staticMeshes.size() == 35U, "parsed mesh count") &&
         expect(parsed.room.anchors.size() == 5U, "parsed anchor count") &&
         expect(parsed.room.spatialSurfaces.size() == 55U,
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
         expect(result.staticMeshCount == 35U, "mesh count preserved") &&
         expect(result.anchorCount == 5U, "anchor count preserved") &&
         expect(result.spatialSurfaceCount == 55U, "surface count preserved") &&
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
                  forwardsGridValidationFailure() &&
                  forwardsAssetTextFailureAfterRoomBuild() &&
                  supportsSkippingAssetTextForLiveEditing();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
