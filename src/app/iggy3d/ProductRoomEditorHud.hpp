#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "app/iggy3d/ProductRoomEditingState.hpp"
#include "app/iggy3d/ProductRoomEditorCursor.hpp"

namespace iggy3d {

struct ProductRoomEditorHudLine {
  bool visible = false;
  std::string text;
};

struct ProductRoomEditorHud {
  bool visible = false;
  std::string status = "room_editor_hud_not_ready";
  std::string reasonCode = "room_editor_hud_not_ready";
  std::string toolName = "floor";
  std::string wallDirectionName = "up";
  std::int32_t gridX = 0;
  std::int32_t gridZ = 0;
  std::int32_t storyIndex = 0;
  std::string lastOperation = "none";
  bool lastOperationAccepted = false;
  std::string lastPrimitiveId = "none";
  std::array<ProductRoomEditorHudLine, 4> lines;
  std::uint64_t lineCount = 0;
};

struct ProductRoomEditorHudRequest {
  const ProductRoomEditingState& roomEditing;
  const ProductRoomEditorCursorState& cursor;
  bool gameplayActive = false;
  std::string_view lastOperation = "none";
  bool lastOperationAccepted = false;
  std::string_view lastPrimitiveId = "none";
};

ProductRoomEditorHud buildProductRoomEditorHud(
    const ProductRoomEditorHudRequest& request);

}  // namespace iggy3d
