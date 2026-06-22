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

struct EditableRoomDocument {
  std::string id = "editable_room";
  std::uint32_t version = 1;
  std::string source = "iggy3d.editor";
  std::string sourceFile = "editable_room";
  std::string sourceSubset = "authoring";
  std::vector<EditableRoomFloor> floors;
  std::vector<EditableRoomWall> walls;
};

enum class RoomEditCommandKind : std::uint8_t {
  AddFloor,
  DeleteFloor,
  SetFloorSemantics,
  AddWall,
  DeleteWall,
  SetWallSemantics,
};

struct RoomEditCommand {
  RoomEditCommandKind kind = RoomEditCommandKind::AddFloor;
  EditableRoomFloor floor;
  EditableRoomWall wall;
  std::string targetId;
  EditableRoomSemantics semantics;
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
RoomEditCommand addWallCommand(EditableRoomWall wall);
RoomEditCommand deleteWallCommand(std::string id);
RoomEditCommand setWallSemanticsCommand(std::string id, EditableRoomSemantics semantics);

const EditableRoomFloor* findEditableFloor(const EditableRoomDocument& document,
                                           const std::string& id);
const EditableRoomWall* findEditableWall(const EditableRoomDocument& document,
                                         const std::string& id);

std::vector<std::string> runtimeIdsForEditableFloor(const EditableRoomFloor& floor);
std::vector<std::string> runtimeIdsForEditableWall(const EditableRoomWall& wall);

RoomEditResult applyRoomEditCommand(EditableRoomDocument& document,
                                    const RoomEditCommand& command);
RoomBakeResult bakeEditableRoomDocument(const EditableRoomDocument& document);
const char* roomEditStatusName(RoomEditStatus status);
const char* roomEditCommandKindName(RoomEditCommandKind kind);

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
