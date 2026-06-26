#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/ProductRoomEditorCursor.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

struct ProductRoomEditorOverlay {
  bool visible = false;
  std::string status = "room_editor_overlay_not_ready";
  std::string reasonCode = "room_editor_overlay_not_ready";
  std::uint64_t itemCount = 0;
  ProductRoomEditorCursorState cursor;
  Vec3 worldPosition;
  Vec3 wallStartMeters;
  Vec3 wallEndMeters;
};

ProductRoomEditorOverlay buildProductRoomEditorOverlay(
    const ProductRoomEditorCursorState& cursor,
    bool roomEditingReady);

}  // namespace iggy3d
