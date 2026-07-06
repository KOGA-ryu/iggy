#include "app/iggy3d/ascii_room/Preview.hpp"

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
  request.sourceText = window.asciiRoomDraft.text;
  request.roomId = window.asciiRoomDraft.roomId;
  request.sourceName = window.asciiRoomDraft.sourceName;
  return request;
}

void recordProductAsciiRoomPreview(std::string_view sourceName,
                                   std::string_view roomId,
                                   const ProductAsciiRoomAuthoringResult& result,
                                   ProductAppWindowState& window) {
  window.asciiRoomPreview.status = result.status;
  window.asciiRoomPreview.reasonCode = result.reasonCode;
  window.asciiRoomPreview.failedStage = result.failedStage;
  window.asciiRoomPreview.roomId =
      roomId.empty() ? std::string{"none"} : std::string(roomId);
  window.asciiRoomPreview.sourceName =
      sourceName.empty() ? std::string{"none"} : std::string(sourceName);
  window.asciiRoomPreview.ready = result.ok;
  window.asciiRoomPreview.width = sizeReceiptValue(result.width);
  window.asciiRoomPreview.height = sizeReceiptValue(result.height);
  window.asciiRoomPreview.floorCount = sizeReceiptValue(result.floorCount);
  window.asciiRoomPreview.wallCount = sizeReceiptValue(result.wallCount);
  window.asciiRoomPreview.objectCount = sizeReceiptValue(result.objectCount);
  window.asciiRoomPreview.markerCount = sizeReceiptValue(result.markerCount);
  window.asciiRoomPreview.elevatedFloorCount =
      sizeReceiptValue(result.elevatedFloorCount);
  window.asciiRoomPreview.rampCount = sizeReceiptValue(result.rampCount);
  window.asciiRoomPreview.blockedSlopeCount =
      sizeReceiptValue(result.blockedSlopeCount);
  window.asciiRoomPreview.staticMeshCount =
      sizeReceiptValue(result.staticMeshCount);
  window.asciiRoomPreview.anchorCount = sizeReceiptValue(result.anchorCount);
  window.asciiRoomPreview.spatialSurfaceCount =
      sizeReceiptValue(result.spatialSurfaceCount);
  window.asciiRoomPreview.assetTextWritten = result.assetText.ok;
  window.asciiRoomPreview.assetTextBytes =
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
