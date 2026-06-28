#pragma once

#include <cstdint>
#include <string>

#include "content/authoring/EditableRoomDocument.hpp"
#include "projection/scene/SceneItem.hpp"

namespace iggy3d {

struct ProductActiveRoomState;
struct ProductAsciiRoomAuthoringRequest;
struct ProductRoomEditingOperationResult;
struct ProductRoomEditingStartResult;
struct ProductRoomEditingState;
struct ProductRoomEditorActionResult;
struct ProductRoomEditorCursorState;

enum class ProductRoomAuthoringInputSource : std::uint8_t {
  Ai,
  Hotkey,
  Mouse,
  Script,
};

struct ProductRoomAuthoringSnapshot {
  bool ready = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  EditableRoomDocument document;
  RoomAsset room;
  SceneRoomProjection roomProjection;
  std::uint64_t documentFloorCount = 0;
  std::uint64_t documentWallCount = 0;
  std::uint64_t documentObjectCount = 0;
  std::uint64_t roomStaticMeshCount = 0;
  std::uint64_t roomSpatialSurfaceCount = 0;
  std::uint64_t projectedMeshCount = 0;
  std::uint64_t projectedFloorMeshCount = 0;
  std::uint64_t projectedWallMeshCount = 0;
  std::uint64_t projectedPropMeshCount = 0;
  std::uint64_t undoDepth = 0;
  std::uint64_t redoDepth = 0;
};

struct ProductRoomAuthoringCommandResult {
  bool accepted = false;
  ProductRoomAuthoringInputSource inputSource = ProductRoomAuthoringInputSource::Script;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  RoomEditResult edit;
  ProductRoomAuthoringSnapshot snapshot;
};

const char* productRoomAuthoringInputSourceName(
    ProductRoomAuthoringInputSource source);

ProductRoomAuthoringSnapshot buildProductRoomAuthoringSnapshot(
    const EditableRoomSession& session);

class ProductRoomAuthoringController {
 public:
  ProductRoomAuthoringController() = default;
  explicit ProductRoomAuthoringController(EditableRoomDocument document);

  const ProductRoomAuthoringSnapshot& snapshot() const;
  ProductRoomAuthoringCommandResult submit(ProductRoomAuthoringInputSource source,
                                           const RoomEditCommand& command);
  ProductRoomAuthoringCommandResult undo(ProductRoomAuthoringInputSource source);
  ProductRoomAuthoringCommandResult redo(ProductRoomAuthoringInputSource source);

 private:
  EditableRoomSession session_;
  ProductRoomAuthoringSnapshot snapshot_;
};

struct ProductRoomAuthoringStartFromAsciiRequest {
  const ProductAsciiRoomAuthoringRequest& request;
};

struct ProductRoomAuthoringStartFromActiveRoomRequest {
  const ProductActiveRoomState& activeRoom;
};

struct ProductRoomAuthoringEditCommandRequest {
  ProductRoomEditingState& state;
  ProductRoomAuthoringInputSource source = ProductRoomAuthoringInputSource::Script;
  const RoomEditCommand& command;
};

struct ProductRoomAuthoringUndoRedoRequest {
  ProductRoomEditingState& state;
  ProductRoomAuthoringInputSource source = ProductRoomAuthoringInputSource::Script;
};

struct ProductRoomAuthoringCursorPlaceRequest {
  const ProductRoomEditingState& editing;
  const ProductRoomEditorCursorState& cursor;
  ProductRoomAuthoringInputSource source = ProductRoomAuthoringInputSource::Hotkey;
};

ProductRoomEditingStartResult startProductRoomAuthoringFromAsciiDraft(
    ProductRoomAuthoringStartFromAsciiRequest request);

ProductRoomEditingStartResult startProductRoomAuthoringFromActiveRoom(
    ProductRoomAuthoringStartFromActiveRoomRequest request);

ProductRoomEditingOperationResult applyProductRoomAuthoringEditCommand(
    ProductRoomAuthoringEditCommandRequest request);

ProductRoomEditingOperationResult undoProductRoomAuthoringEdit(
    ProductRoomAuthoringUndoRedoRequest request);

ProductRoomEditingOperationResult redoProductRoomAuthoringEdit(
    ProductRoomAuthoringUndoRedoRequest request);

ProductRoomEditorActionResult applyProductRoomAuthoringCursorPlace(
    ProductRoomAuthoringCursorPlaceRequest request);

}  // namespace iggy3d
