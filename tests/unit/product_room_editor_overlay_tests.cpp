#include "app/iggy3d/ProductPrimitiveDrawList.hpp"
#include "app/iggy3d/ProductRoomEditorOverlay.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float a, float b) {
  return std::fabs(a - b) < 0.0001F;
}

bool hiddenWhenEditingNotReady() {
  const iggy3d::ProductRoomEditorCursorState cursor;
  const iggy3d::ProductRoomEditorOverlay overlay =
      iggy3d::buildProductRoomEditorOverlay(cursor, false);
  return expect(!overlay.visible, "not ready hidden") &&
         expect(overlay.status == "room_editor_overlay_not_ready",
                "not ready status") &&
         expect(overlay.itemCount == 0U, "not ready count");
}

bool floorCursorCenterUsesGridMath() {
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.gridX = 2;
  cursor.gridZ = -3;
  cursor.cellSizeMeters = 2.0F;
  const iggy3d::ProductRoomEditorOverlay overlay =
      iggy3d::buildProductRoomEditorOverlay(cursor, true);
  return expect(overlay.visible, "floor visible") &&
         expect(overlay.status == "room_editor_overlay_ready",
                "floor ready status") &&
         expect(overlay.itemCount == 1U, "floor count") &&
         expect(near(overlay.worldPosition.x, 4.0F), "floor x") &&
         expect(near(overlay.worldPosition.y, 0.08F), "floor y") &&
         expect(near(overlay.worldPosition.z, -6.0F), "floor z");
}

bool wallCursorUsesPlacementEdgeConvention() {
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.gridX = 3;
  cursor.gridZ = 4;
  cursor.cellSizeMeters = 2.0F;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  cursor.wallDirection = iggy3d::ProductRoomEditorDirection::Up;
  const iggy3d::ProductRoomEditorOverlay overlay =
      iggy3d::buildProductRoomEditorOverlay(cursor, true);
  return expect(overlay.visible, "wall visible") &&
         expect(near(overlay.wallStartMeters.x, 5.0F), "wall start x") &&
         expect(near(overlay.wallStartMeters.y, 0.0F), "wall start y") &&
         expect(near(overlay.wallStartMeters.z, 7.0F), "wall start z") &&
         expect(near(overlay.wallEndMeters.x, 7.0F), "wall end x") &&
         expect(near(overlay.wallEndMeters.y, 0.0F), "wall end y") &&
         expect(near(overlay.wallEndMeters.z, 7.0F), "wall end z") &&
         expect(near(overlay.worldPosition.x, 6.0F), "wall center x") &&
         expect(near(overlay.worldPosition.y, 0.15F), "wall center y") &&
         expect(near(overlay.worldPosition.z, 7.0F), "wall center z");
}

bool invalidCellSizeRejects() {
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.cellSizeMeters = 0.0F;
  const iggy3d::ProductRoomEditorOverlay overlay =
      iggy3d::buildProductRoomEditorOverlay(cursor, true);
  return expect(!overlay.visible, "invalid hidden") &&
         expect(overlay.status == "room_editor_overlay_invalid_cell_size",
                "invalid status") &&
         expect(overlay.itemCount == 0U, "invalid count");
}

bool drawListAppendIsDeterministic() {
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.gridX = 1;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  const iggy3d::ProductRoomEditorOverlay overlay =
      iggy3d::buildProductRoomEditorOverlay(cursor, true);
  const iggy3d::ProductPrimitiveDrawList list =
      iggy3d::buildProductPrimitiveDrawList(nullptr, nullptr, nullptr, nullptr,
                                            &overlay);
  return expect(list.itemCount == 1U, "overlay item count") &&
         expect(list.roomEditorCursorVisible, "overlay draw visible") &&
         expect(list.roomEditorCursorCount == 1U, "overlay draw count") &&
         expect(list.items.size() == 1U, "overlay item vector") &&
         expect(list.items[0].kind ==
                    iggy3d::ProductPrimitiveDrawKind::RoomEditorCursor,
                "overlay draw kind") &&
         expect(list.items[0].stableName == "room_editor_cursor",
                "overlay stable name") &&
         expect(!list.items[0].targetable, "overlay not targetable") &&
         expect(!list.items[0].interactable, "overlay not interactable") &&
         expect(near(list.items[0].worldPosition.x, 1.0F),
                "overlay draw x");
}

}  // namespace

int main() {
  const bool ok = hiddenWhenEditingNotReady() && floorCursorCenterUsesGridMath() &&
                  wallCursorUsesPlacementEdgeConvention() &&
                  invalidCellSizeRejects() && drawListAppendIsDeterministic();
  return ok ? 0 : 1;
}
