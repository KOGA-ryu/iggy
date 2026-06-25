#include "app/iggy3d/ProductAsciiRoomPreview.hpp"

#include <cstdint>
#include <string>

namespace iggy3d {
namespace {

std::uint64_t sizeReceiptValue(std::size_t value) {
  return static_cast<std::uint64_t>(value);
}

}  // namespace

std::string decodeProductAsciiRoomAutomationText(std::string_view value) {
  std::string decoded;
  decoded.reserve(value.size());
  for (std::size_t index = 0; index < value.size(); ++index) {
    if (value[index] != '\\' || index + 1U >= value.size()) {
      decoded.push_back(value[index]);
      continue;
    }

    const char escaped = value[++index];
    if (escaped == 'n') {
      decoded.push_back('\n');
    } else if (escaped == 't') {
      decoded.push_back('\t');
    } else if (escaped == '\\') {
      decoded.push_back('\\');
    } else {
      decoded.push_back('\\');
      decoded.push_back(escaped);
    }
  }
  return decoded;
}

ProductAsciiRoomAuthoringRequest productAsciiRoomAuthoringRequestFromDraft(
    const ProductAppWindowState& window) {
  ProductAsciiRoomAuthoringRequest request;
  request.sourceText = window.asciiRoomDraftText;
  request.roomId = window.asciiRoomDraftRoomId;
  request.sourceName = window.asciiRoomDraftSourceName;
  return request;
}

void recordProductAsciiRoomPreview(std::string_view sourceName,
                                   std::string_view roomId,
                                   const ProductAsciiRoomAuthoringResult& result,
                                   ProductAppWindowState& window) {
  window.asciiRoomPreviewStatus = result.status;
  window.asciiRoomPreviewReasonCode = result.reasonCode;
  window.asciiRoomPreviewFailedStage = result.failedStage;
  window.asciiRoomPreviewRoomId =
      roomId.empty() ? std::string{"none"} : std::string(roomId);
  window.asciiRoomPreviewSourceName =
      sourceName.empty() ? std::string{"none"} : std::string(sourceName);
  window.asciiRoomPreviewReady = result.ok;
  window.asciiRoomPreviewWidth = sizeReceiptValue(result.width);
  window.asciiRoomPreviewHeight = sizeReceiptValue(result.height);
  window.asciiRoomPreviewFloorCount = sizeReceiptValue(result.floorCount);
  window.asciiRoomPreviewWallCount = sizeReceiptValue(result.wallCount);
  window.asciiRoomPreviewMarkerCount = sizeReceiptValue(result.markerCount);
  window.asciiRoomPreviewStaticMeshCount =
      sizeReceiptValue(result.staticMeshCount);
  window.asciiRoomPreviewAnchorCount = sizeReceiptValue(result.anchorCount);
  window.asciiRoomPreviewSpatialSurfaceCount =
      sizeReceiptValue(result.spatialSurfaceCount);
  window.asciiRoomPreviewAssetTextWritten = result.assetText.ok;
  window.asciiRoomPreviewAssetTextBytes =
      sizeReceiptValue(result.assetText.text.size());
}

ProductAsciiRoomAuthoringResult buildProductAsciiRoomPreviewResult(
    ProductAppWindowState& window) {
  const ProductAsciiRoomAuthoringRequest request =
      productAsciiRoomAuthoringRequestFromDraft(window);
  const ProductAsciiRoomAuthoringResult result =
      buildProductAsciiRoomAuthoring(request);
  recordProductAsciiRoomPreview(request.sourceName, request.roomId, result, window);
  return result;
}

bool buildProductAsciiRoomPreview(ProductAppWindowState& window) {
  const ProductAsciiRoomAuthoringResult result =
      buildProductAsciiRoomPreviewResult(window);
  return result.ok;
}

}  // namespace iggy3d
