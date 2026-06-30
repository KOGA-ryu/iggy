#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "content/assets/RoomAsset.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

struct EditableRoomSemantics {
  std::string materialId;
  std::vector<std::string> traversalTags;
  std::vector<std::string> gameplayTags;
  bool walkable = false;
  bool blocksActor = false;
  bool blocksProjectile = false;
};

struct EditableRoomFloor {
  std::string id;
  std::int32_t storyIndex = 0;
  Vec3 centerMeters;
  Vec3 sizeMeters = {1.0F, 0.10F, 1.0F};
  EditableRoomSemantics semantics;
  bool locked = false;
  bool hidden = false;
};

struct EditableRoomWall {
  std::string id;
  std::int32_t storyIndex = 0;
  Vec3 startMeters;
  Vec3 endMeters;
  float bottomY = 0.0F;
  float heightMeters = 2.0F;
  float thicknessMeters = 0.20F;
  EditableRoomSemantics semantics;
  bool locked = false;
  bool hidden = false;
};

struct EditableRoomObject {
  std::string id;
  std::string assetId = "wood_crate_proxy";
  std::int32_t storyIndex = 0;
  Vec3 positionMeters;
  Vec3 sizeMeters = {0.8F, 0.8F, 0.8F};
  float yawDegrees = 0.0F;
  bool blocksActor = true;
  bool blocksProjectile = true;
  bool locked = false;
  bool hidden = false;
};

struct EditableRoomDocument {
  std::string id = "editable_room";
  std::uint32_t version = 1;
  std::string source = "iggy3d.editor";
  std::string sourceFile = "editable_room";
  std::string sourceSubset = "authoring";
  std::vector<EditableRoomFloor> floors;
  std::vector<EditableRoomWall> walls;
  std::vector<EditableRoomObject> objects;
};

enum class RoomEditCommandKind : std::uint8_t {
  AddFloor,
  DeleteFloor,
  SetFloorSemantics,
  MoveFloor,
  ResizeFloor,
  AddWall,
  DeleteWall,
  SetWallSemantics,
  MoveWall,
  StretchWall,
  RotateWall90,
  SetWallHeight,
  SetWallThickness,
  AddObject,
  DeleteObject,
  MoveObject,
};

struct RoomEditCommand {
  RoomEditCommandKind kind = RoomEditCommandKind::AddFloor;
  EditableRoomFloor floor;
  EditableRoomWall wall;
  EditableRoomObject object;
  std::string targetId;
  EditableRoomSemantics semantics;
  Vec3 deltaMeters;
  Vec3 positionMeters;
  Vec3 sizeMeters;
  Vec3 startMeters;
  Vec3 endMeters;
  float scalarMeters = 0.0F;
  bool useStart = false;
  bool rotateLeft = false;
};

enum class RoomEditStatus : std::uint8_t {
  Applied,
  UndoApplied,
  RedoApplied,
  InvalidCommand,
  DuplicateId,
  MissingPrimitive,
  LockedPrimitive,
  InvalidPrimitive,
  InvalidGeometry,
  InvalidSemantics,
  NothingToUndo,
  NothingToRedo,
};

struct RoomEditResult {
  RoomEditStatus status = RoomEditStatus::InvalidCommand;
  const char* reasonCode = "room_edit_invalid_command";
  std::string primitiveId;
  std::vector<std::string> affectedRuntimeIds;
};

struct RoomBakeResult {
  bool ok = false;
  const char* reasonCode = "room_bake_failed";
  RoomAsset room;
};

EditableRoomSemantics defaultFloorSemantics(std::string materialId = "debug_floor");
EditableRoomSemantics defaultWallSemantics(std::string materialId = "debug_wall");

RoomEditCommand addFloorCommand(EditableRoomFloor floor);
RoomEditCommand deleteFloorCommand(std::string id);
RoomEditCommand setFloorSemanticsCommand(std::string id, EditableRoomSemantics semantics);
RoomEditCommand moveFloorCommand(std::string id, Vec3 deltaMeters);
RoomEditCommand resizeFloorCommand(std::string id, Vec3 sizeMeters);
RoomEditCommand addWallCommand(EditableRoomWall wall);
RoomEditCommand deleteWallCommand(std::string id);
RoomEditCommand setWallSemanticsCommand(std::string id, EditableRoomSemantics semantics);
RoomEditCommand moveWallCommand(std::string id, Vec3 deltaMeters);
RoomEditCommand stretchWallStartCommand(std::string id, Vec3 startMeters);
RoomEditCommand stretchWallEndCommand(std::string id, Vec3 endMeters);
RoomEditCommand rotateWall90Command(std::string id, bool left);
RoomEditCommand setWallHeightCommand(std::string id, float heightMeters);
RoomEditCommand setWallThicknessCommand(std::string id, float thicknessMeters);
RoomEditCommand addObjectCommand(EditableRoomObject object);
RoomEditCommand deleteObjectCommand(std::string id);
RoomEditCommand moveObjectCommand(std::string id, Vec3 deltaMeters);
RoomEditCommand setObjectPositionCommand(std::string id, Vec3 positionMeters);

const EditableRoomFloor* findEditableFloor(const EditableRoomDocument& document,
                                           const std::string& id);
const EditableRoomWall* findEditableWall(const EditableRoomDocument& document,
                                         const std::string& id);
const EditableRoomObject* findEditableObject(const EditableRoomDocument& document,
                                             const std::string& id);

std::vector<std::string> runtimeIdsForEditableFloor(const EditableRoomFloor& floor);
std::vector<std::string> runtimeIdsForEditableWall(const EditableRoomWall& wall);
std::vector<std::string> runtimeIdsForEditableObject(const EditableRoomObject& object);
std::uint64_t nextEditableFloorIndex(const EditableRoomDocument& document);
std::uint64_t nextEditableWallIndex(const EditableRoomDocument& document);
std::uint64_t nextEditableObjectIndex(const EditableRoomDocument& document);

RoomEditResult applyRoomEditCommand(EditableRoomDocument& document,
                                    const RoomEditCommand& command);
RoomBakeResult bakeEditableRoomDocument(const EditableRoomDocument& document);
const char* roomEditStatusName(RoomEditStatus status);

class EditableRoomSession {
 public:
  EditableRoomSession() = default;
  explicit EditableRoomSession(EditableRoomDocument document);

  const EditableRoomDocument& document() const;
  RoomEditResult submit(const RoomEditCommand& command);
  RoomEditResult undo();
  RoomEditResult redo();
  std::size_t undoDepth() const;
  std::size_t redoDepth() const;

 private:
  struct HistoryEntry {
    RoomEditCommand command;
    EditableRoomDocument before;
    EditableRoomDocument after;
    RoomEditResult result;
  };

  EditableRoomDocument document_;
  std::vector<HistoryEntry> undoStack_;
  std::vector<HistoryEntry> redoStack_;
};

}  // namespace iggy3d
