#include "app/iggy3d/EditableRoomToAuthoredRoom.hpp"

namespace iggy3d {
namespace {

SaveAuthoredRoomSemanticsRecord authoredSemanticsFrom(
    const EditableRoomSemantics& semantics) {
  SaveAuthoredRoomSemanticsRecord out;
  out.materialId = semantics.materialId;
  out.traversalTags = semantics.traversalTags;
  out.gameplayTags = semantics.gameplayTags;
  out.walkable = semantics.walkable;
  out.blocksActor = semantics.blocksActor;
  out.blocksProjectile = semantics.blocksProjectile;
  return out;
}

SaveAuthoredRoomFloorRecord authoredFloorFrom(const EditableRoomFloor& floor) {
  SaveAuthoredRoomFloorRecord out;
  out.id = floor.id;
  out.storyIndex = floor.storyIndex;
  out.centerMeters = floor.centerMeters;
  out.sizeMeters = floor.sizeMeters;
  out.semantics = authoredSemanticsFrom(floor.semantics);
  out.locked = floor.locked;
  out.hidden = floor.hidden;
  return out;
}

SaveAuthoredRoomWallRecord authoredWallFrom(const EditableRoomWall& wall) {
  SaveAuthoredRoomWallRecord out;
  out.id = wall.id;
  out.storyIndex = wall.storyIndex;
  out.startMeters = wall.startMeters;
  out.endMeters = wall.endMeters;
  out.bottomY = wall.bottomY;
  out.heightMeters = wall.heightMeters;
  out.thicknessMeters = wall.thicknessMeters;
  out.semantics = authoredSemanticsFrom(wall.semantics);
  out.locked = wall.locked;
  out.hidden = wall.hidden;
  return out;
}

}  // namespace

EditableRoomToAuthoredRoomResult buildAuthoredRoomFromEditableRoomDocument(
    const EditableRoomDocument& document) {
  EditableRoomToAuthoredRoomResult result;
  result.authoredRoom.present = true;
  result.authoredRoom.id = document.id.empty() ? "editable_room" : document.id;
  result.authoredRoom.version = document.version == 0U ? 1U : document.version;
  result.authoredRoom.source =
      document.source.empty() ? "iggy3d.editor" : document.source;
  result.authoredRoom.sourceFile =
      document.sourceFile.empty() ? "editable_room" : document.sourceFile;
  result.authoredRoom.sourceSubset =
      document.sourceSubset.empty() ? "authoring" : document.sourceSubset;

  result.authoredRoom.floors.reserve(document.floors.size());
  for (const EditableRoomFloor& floor : document.floors) {
    result.authoredRoom.floors.push_back(authoredFloorFrom(floor));
  }
  result.authoredRoom.walls.reserve(document.walls.size());
  for (const EditableRoomWall& wall : document.walls) {
    result.authoredRoom.walls.push_back(authoredWallFrom(wall));
  }

  result.floorCount = result.authoredRoom.floors.size();
  result.wallCount = result.authoredRoom.walls.size();
  result.markerCount = result.authoredRoom.markers.size();
  result.ok = true;
  result.status = "editable_room_authored_ready";
  result.reasonCode = "editable_room_authored_ready";
  return result;
}

}  // namespace iggy3d
