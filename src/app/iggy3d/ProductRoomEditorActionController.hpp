#pragma once

#include <string>

#include "app/iggy3d/ProductRoomEditorCursor.hpp"
#include "app/iggy3d/ProductRoomEditingState.hpp"
#include "app/input/ActionState.hpp"

namespace iggy3d {

struct ProductRoomEditorActionResult {
  bool ok = false;
  bool handled = false;
  std::string status = "room_editor_action_ignored";
  std::string reasonCode = "room_editor_action_ignored";
  std::string operation = "none";
  bool operationAccepted = false;
  std::string primitiveId = "none";
  ProductRoomEditingState editing;
  ProductRoomEditorCursorState cursor;
};

ProductRoomEditorActionResult applyProductRoomEditorAction(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    const ActionStateEntry& action,
    ProductRoomAuthoringInputSource inputSource =
        ProductRoomAuthoringInputSource::Hotkey);

ProductRoomEditorActionResult applyProductRoomEditorActions(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    const ActionState& actions,
    ProductRoomAuthoringInputSource inputSource =
        ProductRoomAuthoringInputSource::Hotkey);

ProductRoomEditorActionResult applyProductRoomEditorMousePick(
    const ProductRoomEditingState& editing,
    const ProductRoomEditorMousePickRequest& request,
    std::string operation = "room_editor.mouse_pick");

}  // namespace iggy3d
