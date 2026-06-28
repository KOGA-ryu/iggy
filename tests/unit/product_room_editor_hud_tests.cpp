#include "app/iggy3d/room_editor/Presentation.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductRoomEditingState readyEditing() {
  iggy3d::ProductRoomEditingState editing;
  editing.ready = true;
  editing.status = "room_editing_ready";
  editing.reasonCode = "room_editing_ready";
  return editing;
}

iggy3d::ProductRoomEditorCursorState wallCursor() {
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.gridX = 1;
  cursor.gridZ = 0;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  cursor.wallDirection = iggy3d::ProductRoomEditorDirection::Up;
  return cursor;
}

bool notReadyHidesHudButCopiesFacts() {
  iggy3d::ProductRoomEditingState editing;
  const iggy3d::ProductRoomEditorCursorState cursor = wallCursor();
  iggy3d::ProductRoomEditorPlacementPreviewResult preview;
  preview.ok = true;
  preview.status = "room_editor_preview_ready";
  preview.reasonCode = preview.status;
  preview.primitiveId = "edit_wall_1";
  preview.before.optimizedDrawCount = 33;
  preview.after.optimizedDrawCount = 34;
  preview.before.optimizedTriangleCount = 276;
  preview.after.optimizedTriangleCount = 288;
  preview.optimizedDrawDelta = 1;
  preview.optimizedTriangleDelta = 12;

  const iggy3d::ProductRoomEditorHud hud = iggy3d::buildProductRoomEditorHud({
      editing,
      cursor,
      true,
      "editor.preview",
      true,
      "edit_wall_1",
      &preview,
  });

  return expect(!hud.visible, "not ready hud hidden") &&
         expect(hud.status == "room_editor_hud_not_ready",
                "not ready status") &&
         expect(hud.previewActive, "not ready copies preview active") &&
         expect(hud.previewCandidateId == "edit_wall_1",
                "not ready copies preview candidate") &&
         expect(hud.previewBeforeDrawCount == 33U,
                "not ready copies preview before draw") &&
         expect(hud.previewAfterDrawCount == 34U,
                "not ready copies preview after draw") &&
         expect(hud.lineCount == 0U, "not ready emits no lines");
}

bool readyHudWithoutPreviewShowsCursorAndLastOperation() {
  const iggy3d::ProductRoomEditingState editing = readyEditing();
  const iggy3d::ProductRoomEditorCursorState cursor = wallCursor();
  const iggy3d::ProductRoomEditorHud hud = iggy3d::buildProductRoomEditorHud({
      editing,
      cursor,
      true,
      "editor.place",
      true,
      "edit_wall_1",
      nullptr,
  });

  return expect(hud.visible, "ready hud visible") &&
         expect(hud.status == "room_editor_hud_ready", "ready hud status") &&
         expect(hud.toolName == "wall", "ready hud tool") &&
         expect(hud.wallDirectionName == "up", "ready hud wall direction") &&
         expect(hud.gridX == 1, "ready hud grid x") &&
         expect(hud.gridZ == 0, "ready hud grid z") &&
         expect(!hud.previewActive, "ready hud has no preview") &&
         expect(hud.lineCount == 4U, "ready hud line count") &&
         expect(hud.lines[0].text == "EDITOR TOOL wall",
                "ready hud tool line") &&
         expect(hud.lines[2].text == "WALL DIR up",
                "ready hud wall direction line") &&
         expect(hud.lines[3].text == "LAST editor.place accepted",
                "ready hud last operation line") &&
         expect(hud.lines[3].text.find("DRAW ") == std::string::npos,
                "ready hud omits draw impact without preview") &&
         expect(hud.lines[3].text.find("TRIS ") == std::string::npos,
                "ready hud omits triangle impact without preview");
}

bool readyHudShowsPlacementPreviewFacts() {
  const iggy3d::ProductRoomEditingState editing = readyEditing();
  const iggy3d::ProductRoomEditorCursorState cursor = wallCursor();
  iggy3d::ProductRoomEditorPlacementPreviewResult preview;
  preview.ok = true;
  preview.status = "room_editor_preview_ready";
  preview.reasonCode = preview.status;
  preview.primitiveId = "edit_wall_1";
  preview.before.optimizedDrawCount = 33;
  preview.after.optimizedDrawCount = 34;
  preview.before.optimizedTriangleCount = 276;
  preview.after.optimizedTriangleCount = 288;
  preview.optimizedDrawDelta = 1;
  preview.optimizedTriangleDelta = 12;

  const iggy3d::ProductRoomEditorHud hud = iggy3d::buildProductRoomEditorHud({
      editing,
      cursor,
      true,
      "editor.preview",
      true,
      "edit_wall_1",
      &preview,
  });

  return expect(hud.visible, "preview hud visible") &&
         expect(hud.previewActive, "preview hud active") &&
         expect(hud.previewStatus == "room_editor_preview_ready",
                "preview hud status") &&
         expect(hud.previewCandidateId == "edit_wall_1",
                "preview hud candidate") &&
         expect(hud.previewBeforeDrawCount == 33U,
                "preview hud before draw count") &&
         expect(hud.previewAfterDrawCount == 34U,
                "preview hud after draw count") &&
         expect(hud.previewBeforeTriangleCount == 276U,
                "preview hud before triangle count") &&
         expect(hud.previewAfterTriangleCount == 288U,
                "preview hud after triangle count") &&
         expect(hud.previewOptimizedDrawDelta == 1,
                "preview hud draw delta") &&
         expect(hud.previewOptimizedTriangleDelta == 12,
                "preview hud triangle delta") &&
         expect(hud.lineCount == 6U, "preview hud line count") &&
         expect(hud.lines[3].text ==
                    "PREVIEW edit_wall_1 room_editor_preview_ready",
                "preview hud preview line") &&
         expect(hud.lines[4].text == "DRAW 33 -> 34 (+1)",
                "preview hud draw impact line") &&
         expect(hud.lines[5].text == "TRIS 276 -> 288 (+12)",
                "preview hud triangle impact line");
}

bool objectToolHudShowsSelectedAssetAndPreviewImpact() {
  const iggy3d::ProductRoomEditingState editing = readyEditing();
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Object;
  cursor.selectedObjectAssetId = "wood_crate_proxy";
  cursor.gridX = 2;
  cursor.gridZ = -1;
  iggy3d::ProductRoomEditorPlacementPreviewResult preview;
  preview.ok = true;
  preview.status = "room_editor_preview_ready";
  preview.reasonCode = preview.status;
  preview.primitiveId = "edit_object_1";
  preview.tool = "object";
  preview.objectAssetId = "wood_crate_proxy";
  preview.before.optimizedDrawCount = 33;
  preview.after.optimizedDrawCount = 34;
  preview.before.optimizedTriangleCount = 276;
  preview.after.optimizedTriangleCount = 288;
  preview.optimizedDrawDelta = 1;
  preview.optimizedTriangleDelta = 12;

  const iggy3d::ProductRoomEditorHud hud = iggy3d::buildProductRoomEditorHud({
      editing,
      cursor,
      true,
      "editor.preview",
      true,
      "edit_object_1",
      &preview,
  });

  return expect(hud.visible, "object hud visible") &&
         expect(hud.toolName == "object", "object hud tool") &&
         expect(hud.lineCount == 6U, "object preview hud line count") &&
         expect(hud.lines[0].text == "EDITOR TOOL object",
                "object hud tool line") &&
         expect(hud.lines[2].text == "OBJECT wood_crate_proxy",
                "object hud asset line") &&
         expect(hud.lines[3].text ==
                    "PREVIEW edit_object_1 room_editor_preview_ready",
                "object hud preview line") &&
         expect(hud.lines[4].text == "DRAW 33 -> 34 (+1)",
                "object hud draw line") &&
         expect(hud.lines[5].text == "TRIS 276 -> 288 (+12)",
                "object hud triangle line");
}

}  // namespace

int main() {
  const bool ok = notReadyHidesHudButCopiesFacts() &&
                  readyHudWithoutPreviewShowsCursorAndLastOperation() &&
                  readyHudShowsPlacementPreviewFacts() &&
                  objectToolHudShowsSelectedAssetAndPreviewImpact();
  return ok ? 0 : 1;
}
