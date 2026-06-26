#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "content/authoring/EditableRoomDocument.hpp"

namespace iggy3d {

enum class ProductRoomEditorTool : std::uint8_t {
  Floor,
  Wall,
};

enum class ProductRoomEditorDirection : std::uint8_t {
  Up,
  Down,
  Left,
  Right,
};

struct ProductRoomEditorCursorState {
  std::int32_t gridX = 0;
  std::int32_t gridZ = 0;
  std::int32_t storyIndex = 0;
  float cellSizeMeters = 1.0F;
  ProductRoomEditorTool selectedTool = ProductRoomEditorTool::Floor;
  ProductRoomEditorDirection wallDirection = ProductRoomEditorDirection::Up;
};

struct ProductRoomEditorCursorResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  ProductRoomEditorCursorState state;
  std::optional<RoomEditCommand> command;
};

std::string_view productRoomEditorToolName(ProductRoomEditorTool tool);
std::string_view productRoomEditorDirectionName(ProductRoomEditorDirection direction);

ProductRoomEditorCursorResult moveProductRoomEditorCursor(
    ProductRoomEditorCursorState state,
    ProductRoomEditorDirection direction);

ProductRoomEditorCursorResult cycleProductRoomEditorTool(
    ProductRoomEditorCursorState state);

ProductRoomEditorCursorResult setProductRoomEditorTool(
    ProductRoomEditorCursorState state,
    ProductRoomEditorTool tool);

ProductRoomEditorCursorResult setProductRoomEditorWallDirection(
    ProductRoomEditorCursorState state,
    ProductRoomEditorDirection direction);

ProductRoomEditorCursorResult rotateProductRoomEditorWallDirectionClockwise(
    ProductRoomEditorCursorState state);

ProductRoomEditorCursorResult buildProductRoomEditorPlaceCommand(
    ProductRoomEditorCursorState state,
    const EditableRoomDocument* document);

}  // namespace iggy3d
