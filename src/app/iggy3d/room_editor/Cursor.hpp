#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "app/iggy3d/view/ViewportFraming.hpp"
#include "content/authoring/EditableRoomDocument.hpp"

namespace iggy3d {

enum class ProductRoomEditorTool : std::uint8_t {
  Floor,
  Wall,
  Object,
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
  std::string selectedObjectAssetId = "wood_crate_proxy";
};

struct ProductRoomEditorCursorResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  ProductRoomEditorCursorState state;
  std::optional<RoomEditCommand> command;
};

struct ProductRoomEditorMousePickRequest {
  bool roomEditingReady = false;
  ProductRoomEditorCursorState cursor;
  float screenX = 0.0F;
  float screenY = 0.0F;
  ProductViewportFrameConfig viewportConfig;
  Vec3 anchorWorld;
};

struct ProductRoomEditorMousePickResult {
  bool ok = false;
  std::string status = "room_editor_mouse_pick_not_requested";
  std::string reasonCode = "room_editor_mouse_pick_not_requested";
  ProductRoomEditorCursorState cursor;
  float worldX = 0.0F;
  float worldZ = 0.0F;
  std::int32_t gridX = 0;
  std::int32_t gridZ = 0;
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

ProductRoomEditorMousePickResult pickProductRoomEditorCursorFromScreen(
    const ProductRoomEditorMousePickRequest& request);

}  // namespace iggy3d
