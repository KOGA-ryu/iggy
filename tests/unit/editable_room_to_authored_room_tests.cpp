#include "app/iggy3d/ascii_room/AsciiRoomGrid.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomSource.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomToEditableRoom.hpp"
#include "app/iggy3d/EditableRoomToAuthoredRoom.hpp"
#include "app/iggy3d/gameplay/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "runtime/save/SaveCodec.hpp"
#include "runtime/save/SaveLoad.hpp"
#include "runtime/session/Session.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::AsciiRoomCompileConfig compileConfig() {
  iggy3d::AsciiRoomCompileConfig config;
  config.roomId = "editable_save_room";
  config.sourceName = "unit/editable_save_room.iggyroom.txt";
  config.tileSizeMeters = 1.0F;
  config.floorThicknessMeters = 0.10F;
  config.wallHeightMeters = 2.50F;
  config.wallThicknessMeters = 1.0F;
  config.centerOnOrigin = true;
  return config;
}

iggy3d::EditableRoomDocument editableDocumentFromAscii() {
  const iggy3d::AsciiRoomSource source = iggy3d::parseAsciiRoomSource(
      "###\n"
      "#P#\n"
      "###\n",
      "unit/editable_save_room.iggyroom.txt");
  const iggy3d::AsciiRoomGridBuildResult grid = iggy3d::buildAsciiRoomGrid(source);
  const iggy3d::AsciiRoomToEditableRoomResult editable =
      iggy3d::buildEditableRoomFromAsciiRoom(grid.grid, compileConfig());
  return editable.document;
}

iggy3d::EditableRoomFloor editedFloor() {
  iggy3d::EditableRoomFloor floor;
  floor.id = "authored_floor_1";
  floor.storyIndex = 2;
  floor.centerMeters = {2.0F, 0.25F, 3.0F};
  floor.sizeMeters = {4.0F, 0.50F, 5.0F};
  floor.semantics = iggy3d::defaultFloorSemantics("stone_floor");
  floor.semantics.gameplayTags = {"safe_zone"};
  floor.locked = true;
  floor.hidden = false;
  return floor;
}

iggy3d::EditableRoomWall editedWall() {
  iggy3d::EditableRoomWall wall;
  wall.id = "authored_wall_1";
  wall.storyIndex = 3;
  wall.startMeters = {-1.0F, 0.0F, 1.0F};
  wall.endMeters = {1.0F, 0.0F, 1.0F};
  wall.bottomY = 0.25F;
  wall.heightMeters = 1.5F;
  wall.thicknessMeters = 0.2F;
  wall.semantics = iggy3d::defaultWallSemantics("stone_wall");
  wall.semantics.traversalTags = {"clamber"};
  wall.semantics.gameplayTags = {"training"};
  wall.locked = false;
  wall.hidden = true;
  return wall;
}

bool convertsEditableDocumentToAuthoredRoomSection() {
  iggy3d::EditableRoomDocument document = editableDocumentFromAscii();
  document.floors.clear();
  document.walls.clear();
  document.source = "iggy3d.editor";
  document.sourceSubset = "editable_room_authoring";
  document.floors.push_back(editedFloor());
  document.walls.push_back(editedWall());

  const iggy3d::EditableRoomToAuthoredRoomResult result =
      iggy3d::buildAuthoredRoomFromEditableRoomDocument(document);

  return expect(result.ok, "authored conversion ok") &&
         expect(result.status == "editable_room_authored_ready",
                "authored conversion status") &&
         expect(result.reasonCode == "editable_room_authored_ready",
                "authored conversion reason") &&
         expect(result.authoredRoom.present, "authored room present") &&
         expect(result.authoredRoom.id == "editable_save_room",
                "authored room id") &&
         expect(result.authoredRoom.source == "iggy3d.editor",
                "authored source") &&
         expect(result.authoredRoom.sourceFile ==
                    "unit/editable_save_room.iggyroom.txt",
                "authored source file") &&
         expect(result.authoredRoom.sourceSubset == "editable_room_authoring",
                "authored source subset") &&
         expect(result.floorCount == 1U, "authored floor count") &&
         expect(result.wallCount == 1U, "authored wall count") &&
         expect(result.markerCount == 0U, "authored marker count") &&
         expect(result.authoredRoom.floors[0].id == "authored_floor_1",
                "authored floor id") &&
         expect(result.authoredRoom.floors[0].storyIndex == 2,
                "authored floor story") &&
         expect(result.authoredRoom.floors[0].centerMeters.y == 0.25F,
                "authored floor center") &&
         expect(result.authoredRoom.floors[0].sizeMeters.z == 5.0F,
                "authored floor size") &&
         expect(result.authoredRoom.floors[0].semantics.materialId ==
                    "stone_floor",
                "authored floor material") &&
         expect(result.authoredRoom.floors[0].semantics.walkable,
                "authored floor walkable") &&
         expect(result.authoredRoom.floors[0].semantics.gameplayTags[0] ==
                    "safe_zone",
                "authored floor gameplay tag") &&
         expect(result.authoredRoom.floors[0].locked,
                "authored floor locked") &&
         expect(!result.authoredRoom.floors[0].hidden,
                "authored floor visible") &&
         expect(result.authoredRoom.walls[0].id == "authored_wall_1",
                "authored wall id") &&
         expect(result.authoredRoom.walls[0].storyIndex == 3,
                "authored wall story") &&
         expect(result.authoredRoom.walls[0].bottomY == 0.25F,
                "authored wall bottom") &&
         expect(result.authoredRoom.walls[0].heightMeters == 1.5F,
                "authored wall height") &&
         expect(result.authoredRoom.walls[0].thicknessMeters == 0.2F,
                "authored wall thickness") &&
         expect(result.authoredRoom.walls[0].semantics.blocksActor,
                "authored wall actor blocker") &&
         expect(result.authoredRoom.walls[0].semantics.blocksProjectile,
                "authored wall projectile blocker") &&
         expect(result.authoredRoom.walls[0].semantics.traversalTags[0] ==
                    "clamber",
                "authored wall traversal tag") &&
         expect(result.authoredRoom.walls[0].semantics.gameplayTags[0] ==
                    "training",
                "authored wall gameplay tag") &&
         expect(!result.authoredRoom.walls[0].locked,
                "authored wall unlocked") &&
         expect(result.authoredRoom.walls[0].hidden,
                "authored wall hidden");
}

bool convertedAuthoredRoomSurvivesSaveCodecAndActiveRoomRebuild() {
  iggy3d::EditableRoomDocument document = editableDocumentFromAscii();
  document.floors.push_back(editedFloor());
  document.walls.push_back(editedWall());
  const iggy3d::EditableRoomToAuthoredRoomResult authored =
      iggy3d::buildAuthoredRoomFromEditableRoomDocument(document);

  iggy3d::Session session = iggy3d::Session::create(
      iggy3d::SessionCreateRequest{}).value;
  iggy3d::SaveStateResult saved = iggy3d::saveSessionState(session.state());
  saved.envelope.authoredRoom = authored.authoredRoom;
  const iggy3d::SaveEncodeResult encoded =
      iggy3d::encodeSaveEnvelope(saved.envelope);
  const iggy3d::SaveDecodeResult decoded =
      iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::ProductActiveRoomState active =
      iggy3d::buildProductActiveRoomFromSavedAuthoredRoom(
          decoded.envelope.authoredRoom);
  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);

  return expect(authored.ok, "authored ok before codec") &&
         expect(encoded.status == iggy3d::SaveCodecStatus::Ok,
                "save encode ok") &&
         expect(encoded.encodedText.find("authoredRoom.present=true\n") !=
                    std::string::npos,
                "authored present encoded") &&
         expect(encoded.encodedText.find(
                    "authoredRoom.floor.1.id=authored_floor_1\n") !=
                    std::string::npos,
                "edited floor encoded") &&
         expect(encoded.encodedText.find(
                    "authoredRoom.wall.8.id=authored_wall_1\n") !=
                    std::string::npos,
                "edited wall encoded") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "save decode ok") &&
         expect(decoded.envelope.authoredRoom.present,
                "decoded authored present") &&
         expect(decoded.envelope.authoredRoom.floors.size() == 2U,
                "decoded floor count") &&
         expect(decoded.envelope.authoredRoom.walls.size() == 9U,
                "decoded wall count") &&
         expect(active.loaded, "active room from decoded authored loaded") &&
         expect(active.hasAuthoredRoom, "active room authored present") &&
         expect(active.authoredFloorCount == 2U,
                "active authored floor count") &&
         expect(active.authoredWallCount == 9U,
                "active authored wall count") &&
         expect(active.staticMeshCount == 11U, "active static mesh count") &&
         expect(active.spatialSurfaceCount == 20U,
                "active surface count") &&
         expect(collision.ready, "decoded active collision ready") &&
         expect(collision.querySurfaceCount == 20U,
                "decoded collision surface count");
}

}  // namespace

int main() {
  const bool ok = convertsEditableDocumentToAuthoredRoomSection() &&
                  convertedAuthoredRoomSurvivesSaveCodecAndActiveRoomRebuild();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
