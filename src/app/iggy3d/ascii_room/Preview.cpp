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
  request.sourceText = window.creativeAuthoring.asciiRoomDraft.text;
  request.roomId = window.creativeAuthoring.asciiRoomDraft.roomId;
  request.sourceName = window.creativeAuthoring.asciiRoomDraft.sourceName;
  return request;
}

void recordProductAsciiRoomPreview(std::string_view sourceName,
                                   std::string_view roomId,
                                   const ProductAsciiRoomAuthoringResult& result,
                                   ProductAppWindowState& window) {
  window.creativeAuthoring.asciiRoomPreview.status = result.status;
  window.creativeAuthoring.asciiRoomPreview.reasonCode = result.reasonCode;
  window.creativeAuthoring.asciiRoomPreview.failedStage = result.failedStage;
  window.creativeAuthoring.asciiRoomPreview.roomId =
      roomId.empty() ? std::string{"none"} : std::string(roomId);
  window.creativeAuthoring.asciiRoomPreview.sourceName =
      sourceName.empty() ? std::string{"none"} : std::string(sourceName);
  window.creativeAuthoring.asciiRoomPreview.ready = result.ok;
  window.creativeAuthoring.asciiRoomPreview.width = sizeReceiptValue(result.width);
  window.creativeAuthoring.asciiRoomPreview.height = sizeReceiptValue(result.height);
  window.creativeAuthoring.asciiRoomPreview.floorCount = sizeReceiptValue(result.floorCount);
  window.creativeAuthoring.asciiRoomPreview.wallCount = sizeReceiptValue(result.wallCount);
  window.creativeAuthoring.asciiRoomPreview.objectCount = sizeReceiptValue(result.objectCount);
  window.creativeAuthoring.asciiRoomPreview.markerCount = sizeReceiptValue(result.markerCount);
  window.creativeAuthoring.asciiRoomPreview.elevatedFloorCount =
      sizeReceiptValue(result.elevatedFloorCount);
  window.creativeAuthoring.asciiRoomPreview.rampCount = sizeReceiptValue(result.rampCount);
  window.creativeAuthoring.asciiRoomPreview.blockedSlopeCount =
      sizeReceiptValue(result.blockedSlopeCount);
  window.creativeAuthoring.asciiRoomPreview.staticMeshCount =
      sizeReceiptValue(result.staticMeshCount);
  window.creativeAuthoring.asciiRoomPreview.anchorCount = sizeReceiptValue(result.anchorCount);
  window.creativeAuthoring.asciiRoomPreview.spatialSurfaceCount =
      sizeReceiptValue(result.spatialSurfaceCount);
  window.creativeAuthoring.asciiRoomPreview.assetTextWritten = result.assetText.ok;
  window.creativeAuthoring.asciiRoomPreview.assetTextBytes =
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
