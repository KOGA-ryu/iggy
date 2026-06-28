#include "app/iggy3d/ascii_room/Authoring.hpp"

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

AsciiRoomToRoomAssetConfig roomAssetConfigFor(
    const ProductAsciiRoomAuthoringRequest& request) {
  AsciiRoomToRoomAssetConfig config;
  config.roomId = resolvedRoomId(request);
  config.sourceName = resolvedSourceName(request);
  config.sourceSubset =
      request.sourceSubset.empty() ? "ascii_room_authoring" : request.sourceSubset;
  config.tileSizeMeters = request.tileSizeMeters;
  config.wallHeightMeters = request.wallHeightMeters;
  return config;
}

void copyDiagnosticState(const std::vector<AsciiRoomDiagnostic>& diagnostics,
                         ProductAsciiRoomAuthoringResult& result) {
  result.diagnostics = diagnostics;
}

void copyCounts(ProductAsciiRoomAuthoringResult& result) {
  result.width = result.source.width;
  result.height = result.source.height;
  result.floorCount = result.authoredRoom.floorCount;
  result.wallCount = result.authoredRoom.wallCount;
  result.objectCount = result.authoredRoom.objectCount;
  result.markerCount = result.authoredRoom.markerCount;
  result.elevatedFloorCount = result.authoredRoom.elevatedFloorCount;
  result.rampCount = result.authoredRoom.rampCount;
  result.blockedSlopeCount = result.authoredRoom.blockedSlopeCount;
  result.staticMeshCount = result.roomAsset.staticMeshCount;
  result.anchorCount = result.roomAsset.anchorCount;
  result.spatialSurfaceCount = result.roomAsset.spatialSurfaceCount;
}

ProductAsciiRoomAuthoringResult failedResult(ProductAsciiRoomAuthoringResult result,
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

ProductAsciiRoomAuthoringResult buildProductAsciiRoomAuthoring(
    const ProductAsciiRoomAuthoringRequest& request) {
  ProductAsciiRoomAuthoringResult result;

  const std::string sourceName = resolvedSourceName(request);
  result.source = parseAsciiRoomSource(request.sourceText, sourceName);
  if (result.source.status != "ascii_room_ok") {
    copyDiagnosticState(result.source.diagnostics, result);
    const std::string status = result.source.status;
    const std::string reason = result.source.reasonCode;
    return failedResult(std::move(result), "source", status, reason);
  }

  result.grid = buildAsciiRoomGrid(result.source);
  if (!result.grid.ok) {
    copyDiagnosticState(result.grid.diagnostics, result);
    const std::string status = result.grid.status;
    const std::string reason = result.grid.reasonCode;
    return failedResult(std::move(result), "grid", status, reason);
  }

  result.authoredRoom =
      compileAsciiRoomToAuthoredRoom(result.grid.grid, compileConfigFor(request));
  if (!result.authoredRoom.ok) {
    copyDiagnosticState(result.authoredRoom.diagnostics, result);
    const std::string status = result.authoredRoom.status;
    const std::string reason = result.authoredRoom.reasonCode;
    return failedResult(std::move(result), "authored_room", status, reason);
  }

  result.roomAsset =
      buildRoomAssetFromAsciiRoom(result.authoredRoom, roomAssetConfigFor(request));
  if (!result.roomAsset.ok) {
    copyDiagnosticState(result.roomAsset.diagnostics, result);
    const std::string status = result.roomAsset.status;
    const std::string reason = result.roomAsset.reasonCode;
    return failedResult(std::move(result), "room_asset", status, reason);
  }

  if (request.emitAssetText) {
    result.assetText =
        writeAsciiRoomAssetText(result.roomAsset.room, request.assetTextConfig);
    if (!result.assetText.ok) {
      const std::string status = result.assetText.status;
      const std::string reason = result.assetText.reasonCode;
      return failedResult(std::move(result), "asset_text", status, reason);
    }
  }

  result.ok = true;
  result.failedStage = "none";
  result.status = "product_ascii_room_ready";
  result.reasonCode = "product_ascii_room_ready";
  copyCounts(result);
  return result;
}

}  // namespace iggy3d
