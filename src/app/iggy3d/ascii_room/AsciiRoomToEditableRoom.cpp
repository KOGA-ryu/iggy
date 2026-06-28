#include "app/iggy3d/ascii_room/AsciiRoomToEditableRoom.hpp"

#include <utility>

namespace iggy3d {
namespace {

EditableRoomSemantics editableSemanticsFrom(
    const SaveAuthoredRoomSemanticsRecord& semantics) {
  EditableRoomSemantics out;
  out.materialId = semantics.materialId;
  out.traversalTags = semantics.traversalTags;
  out.gameplayTags = semantics.gameplayTags;
  out.walkable = semantics.walkable;
  out.blocksActor = semantics.blocksActor;
  out.blocksProjectile = semantics.blocksProjectile;
  return out;
}

EditableRoomFloor editableFloorFrom(const SaveAuthoredRoomFloorRecord& floor) {
  EditableRoomFloor out;
  out.id = floor.id;
  out.storyIndex = floor.storyIndex;
  out.centerMeters = floor.centerMeters;
  out.sizeMeters = floor.sizeMeters;
  out.semantics = editableSemanticsFrom(floor.semantics);
  out.locked = floor.locked;
  out.hidden = floor.hidden;
  return out;
}

EditableRoomWall editableWallFrom(const SaveAuthoredRoomWallRecord& wall) {
  EditableRoomWall out;
  out.id = wall.id;
  out.storyIndex = wall.storyIndex;
  out.startMeters = wall.startMeters;
  out.endMeters = wall.endMeters;
  out.bottomY = wall.bottomY;
  out.heightMeters = wall.heightMeters;
  out.thicknessMeters = wall.thicknessMeters;
  out.semantics = editableSemanticsFrom(wall.semantics);
  out.locked = wall.locked;
  out.hidden = wall.hidden;
  return out;
}

EditableRoomObject editableObjectFrom(const SaveAuthoredRoomObjectRecord& object) {
  EditableRoomObject out;
  out.id = object.id;
  out.assetId = object.assetId;
  out.storyIndex = object.storyIndex;
  out.positionMeters = object.positionMeters;
  out.sizeMeters = object.sizeMeters;
  out.yawDegrees = object.yawDegrees;
  out.blocksActor = object.semantics.blocksActor;
  out.blocksProjectile = object.semantics.blocksProjectile;
  out.locked = object.locked;
  out.hidden = object.hidden;
  return out;
}

void copyAuthoredMetadata(const SaveAuthoredRoomSection& authored,
                          EditableRoomDocument& document) {
  document.id = authored.id.empty() ? "ascii_room" : authored.id;
  document.version = authored.version;
  document.source = authored.source.empty() ? "iggy3d.ascii_room" : authored.source;
  document.sourceFile = authored.sourceFile.empty() ? "ascii_room" : authored.sourceFile;
  document.sourceSubset =
      authored.sourceSubset.empty() ? "ascii_room_authoring" : authored.sourceSubset;
}

}  // namespace

EditableRoomDocument buildEditableRoomDocumentFromAuthoredRoom(
    const SaveAuthoredRoomSection& authoredRoom) {
  EditableRoomDocument document;
  copyAuthoredMetadata(authoredRoom, document);
  document.floors.reserve(authoredRoom.floors.size());
  for (const SaveAuthoredRoomFloorRecord& floor : authoredRoom.floors) {
    document.floors.push_back(editableFloorFrom(floor));
  }
  document.walls.reserve(authoredRoom.walls.size());
  for (const SaveAuthoredRoomWallRecord& wall : authoredRoom.walls) {
    document.walls.push_back(editableWallFrom(wall));
  }
  document.objects.reserve(authoredRoom.objects.size());
  for (const SaveAuthoredRoomObjectRecord& object : authoredRoom.objects) {
    document.objects.push_back(editableObjectFrom(object));
  }
  return document;
}

AsciiRoomToEditableRoomResult buildEditableRoomFromAsciiRoom(
    const AsciiRoomGrid& grid,
    const AsciiRoomCompileConfig& config) {
  AsciiRoomToEditableRoomResult result;
  result.authoredRoom = compileAsciiRoomToAuthoredRoom(grid, config);
  result.floorCount = result.authoredRoom.floorCount;
  result.wallCount = result.authoredRoom.wallCount;
  result.objectCount = result.authoredRoom.objectCount;
  result.markerCount = result.authoredRoom.markerCount;

  if (!result.authoredRoom.ok) {
    result.ok = false;
    result.status = result.authoredRoom.status;
    result.reasonCode = result.authoredRoom.reasonCode;
    result.diagnostics = result.authoredRoom.diagnostics;
    return result;
  }

  result.document =
      buildEditableRoomDocumentFromAuthoredRoom(result.authoredRoom.authoredRoom);

  result.ok = true;
  result.status = "ascii_room_editable_ready";
  result.reasonCode = "ascii_room_editable_ready";
  return result;
}

}  // namespace iggy3d
