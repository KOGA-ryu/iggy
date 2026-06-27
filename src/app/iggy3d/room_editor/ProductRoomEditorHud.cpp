#include "app/iggy3d/room_editor/ProductRoomEditorHud.hpp"

#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace iggy3d {
namespace {

void appendHudLine(ProductRoomEditorHud& hud, std::string text) {
  // branch-gate: BG-1034
  if (hud.lineCount >= hud.lines.size()) {
    return;
  }
  ProductRoomEditorHudLine& line = hud.lines[hud.lineCount];
  line.visible = true;
  line.text = std::move(text);
  ++hud.lineCount;
}

std::string_view resultName(bool accepted) {
  constexpr std::array<std::string_view, 2> kResultNames = {
      "rejected",
      "accepted",
  };
  return kResultNames[accepted];
}

std::string signedNumber(std::int64_t value) {
  std::ostringstream out;
  out << std::showpos << value;
  return out.str();
}

}  // namespace

ProductRoomEditorHud buildProductRoomEditorHud(
    const ProductRoomEditorHudRequest& request) {
  ProductRoomEditorHud hud;
  hud.toolName = std::string(productRoomEditorToolName(request.cursor.selectedTool));
  hud.wallDirectionName =
      std::string(productRoomEditorDirectionName(request.cursor.wallDirection));
  hud.gridX = request.cursor.gridX;
  hud.gridZ = request.cursor.gridZ;
  hud.storyIndex = request.cursor.storyIndex;
  hud.lastOperation = std::string(request.lastOperation);
  hud.lastOperationAccepted = request.lastOperationAccepted;
  hud.lastPrimitiveId = std::string(request.lastPrimitiveId);
  // branch-gate: BG-1034
  if (request.placementPreview != nullptr) {
    hud.previewActive = true;
    hud.previewStatus = request.placementPreview->status;
    hud.previewCandidateId = request.placementPreview->primitiveId;
    hud.previewOptimizedDrawDelta =
        request.placementPreview->optimizedDrawDelta;
    hud.previewOptimizedTriangleDelta =
        request.placementPreview->optimizedTriangleDelta;
  }

  // branch-gate: BG-1034
  if (!request.gameplayActive || !request.roomEditing.ready) {
    return hud;
  }

  hud.visible = true;
  hud.status = "room_editor_hud_ready";
  hud.reasonCode = hud.status;
  appendHudLine(hud, "EDITOR TOOL " + hud.toolName);
  appendHudLine(hud, "GRID " + std::to_string(hud.gridX) + " " +
                         std::to_string(hud.gridZ) + " STORY " +
                         std::to_string(hud.storyIndex));
  // branch-gate: BG-1034
  if (request.cursor.selectedTool == ProductRoomEditorTool::Wall) {
    appendHudLine(hud, "WALL DIR " + hud.wallDirectionName);
  }
  // branch-gate: BG-1034
  if (hud.previewActive) {
    appendHudLine(hud, "PREVIEW " + hud.previewCandidateId + " " +
                           hud.previewStatus);
    appendHudLine(hud, "OPT DRAWS " +
                           signedNumber(hud.previewOptimizedDrawDelta) +
                           " TRIS " +
                           signedNumber(hud.previewOptimizedTriangleDelta));
  }
  appendHudLine(hud, "LAST " + hud.lastOperation + " " +
                         std::string(resultName(hud.lastOperationAccepted)));
  return hud;
}

}  // namespace iggy3d
