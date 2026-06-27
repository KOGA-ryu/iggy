#include "app/iggy3d/ascii_room/Editing.hpp"

#include <string>
#include <utility>

namespace iggy3d {
namespace {

std::string resolvedRoomId(const ProductAsciiRoomAuthoringRequest& request) {
  return request.roomId.empty() ? "ascii_room" : request.roomId;
}

std::string resolvedSourceName(const ProductAsciiRoomAuthoringRequest& request) {
  return request.sourceName.empty() ? "inline_ascii_room" : request.sourceName;
}

AsciiRoomCompileConfig compileConfigFor(
    const ProductAsciiRoomAuthoringRequest& request) {
  AsciiRoomCompileConfig config;
  config.tileSizeMeters = request.tileSizeMeters;
  config.floorThicknessMeters = request.floorThicknessMeters;
  config.wallHeightMeters = request.wallHeightMeters;
  config.wallThicknessMeters = request.wallThicknessMeters;
  config.markerYMeters = request.markerYMeters;
  config.centerOnOrigin = request.centerOnOrigin;
  config.storyIndex = request.storyIndex;
  config.roomId = resolvedRoomId(request);
  config.sourceName = resolvedSourceName(request);
  return config;
}

void copyCounts(ProductAsciiRoomEditingResult& result) {
  result.width = result.source.width;
  result.height = result.source.height;
  result.floorCount = result.editableRoom.floorCount;
  result.wallCount = result.editableRoom.wallCount;
  result.markerCount = result.editableRoom.markerCount;
  result.documentFloorCount = result.snapshot.documentFloorCount;
  result.documentWallCount = result.snapshot.documentWallCount;
  result.projectedMeshCount = result.snapshot.projectedMeshCount;
  result.projectedFloorMeshCount = result.snapshot.projectedFloorMeshCount;
  result.projectedWallMeshCount = result.snapshot.projectedWallMeshCount;
}

ProductAsciiRoomEditingResult failedResult(ProductAsciiRoomEditingResult result,
                                           std::string stage,
                                           std::string status,
                                           std::string reason) {
  result.ok = false;
  result.failedStage = std::move(stage);
  result.status = std::move(status);
  result.reasonCode = std::move(reason);
  copyCounts(result);
  return result;
}

}  // namespace

ProductAsciiRoomEditingResult buildProductAsciiRoomEditing(
    const ProductAsciiRoomAuthoringRequest& request) {
  ProductAsciiRoomEditingResult result;

  const std::string sourceName = resolvedSourceName(request);
  result.source = parseAsciiRoomSource(request.sourceText, sourceName);
  if (result.source.status != "ascii_room_ok") {
    result.diagnostics = result.source.diagnostics;
    const std::string status = result.source.status;
    const std::string reason = result.source.reasonCode;
    return failedResult(std::move(result),
                        "source",
                        status,
                        reason);
  }

  result.grid = buildAsciiRoomGrid(result.source);
  if (!result.grid.ok) {
    result.diagnostics = result.grid.diagnostics;
    const std::string status = result.grid.status;
    const std::string reason = result.grid.reasonCode;
    return failedResult(std::move(result),
                        "grid",
                        status,
                        reason);
  }

  result.editableRoom =
      buildEditableRoomFromAsciiRoom(result.grid.grid, compileConfigFor(request));
  if (!result.editableRoom.ok) {
    result.diagnostics = result.editableRoom.diagnostics;
    const std::string status = result.editableRoom.status;
    const std::string reason = result.editableRoom.reasonCode;
    return failedResult(std::move(result),
                        "editable_room",
                        status,
                        reason);
  }

  result.controller = ProductRoomAuthoringController(result.editableRoom.document);
  result.snapshot = result.controller.snapshot();
  if (!result.snapshot.ready) {
    const std::string status = result.snapshot.status;
    const std::string reason = result.snapshot.reasonCode;
    return failedResult(std::move(result),
                        "room_authoring_controller",
                        status,
                        reason);
  }

  result.ok = true;
  result.status = "product_ascii_room_editing_ready";
  result.reasonCode = "product_ascii_room_editing_ready";
  result.failedStage = "none";
  copyCounts(result);
  return result;
}

}  // namespace iggy3d
