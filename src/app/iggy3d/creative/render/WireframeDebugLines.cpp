#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d {
namespace {

void setStatus(ProductCreativeWireframeDebugLineReceipt& receipt,
               ProductCreativeWireframeDebugLineStatus status,
               std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.message = reasonCode;
  receipt.reasonCode = reasonCode;
}

}  // namespace

std::string_view toString(
    ProductCreativeWireframeDebugLineStatus status) noexcept {
  switch (status) {
    case ProductCreativeWireframeDebugLineStatus::Unknown:
      return "Unknown";
    case ProductCreativeWireframeDebugLineStatus::MissingSource:
      return "MissingSource";
    case ProductCreativeWireframeDebugLineStatus::NoLines:
      return "NoLines";
    case ProductCreativeWireframeDebugLineStatus::Built:
      return "Built";
  }

  return "Unknown";
}

ProductCreativeWireframeDebugLineColor
productCreativeWireframeDebugLineColorForStyle(
    creative::CreativeDocumentWireframeStyle style) noexcept {
  switch (style) {
    case creative::CreativeDocumentWireframeStyle::Structural:
      return {0.42F, 0.78F, 0.86F, 1.0F};
    case creative::CreativeDocumentWireframeStyle::Collision:
      return {0.93F, 0.46F, 0.34F, 1.0F};
    case creative::CreativeDocumentWireframeStyle::Navigation:
      return {0.49F, 0.79F, 0.69F, 1.0F};
    case creative::CreativeDocumentWireframeStyle::Trigger:
      return {0.95F, 0.72F, 0.36F, 1.0F};
    case creative::CreativeDocumentWireframeStyle::Gameplay:
      return {0.80F, 0.50F, 0.87F, 1.0F};
    case creative::CreativeDocumentWireframeStyle::Light:
      return {0.96F, 0.84F, 0.38F, 1.0F};
    case creative::CreativeDocumentWireframeStyle::Audio:
      return {0.54F, 0.64F, 0.92F, 1.0F};
    case creative::CreativeDocumentWireframeStyle::Camera:
      return {0.70F, 0.60F, 0.93F, 1.0F};
    case creative::CreativeDocumentWireframeStyle::Testing:
      return {0.74F, 0.76F, 0.78F, 1.0F};
    case creative::CreativeDocumentWireframeStyle::Authoring:
      return {0.68F, 0.74F, 0.70F, 1.0F};
    case creative::CreativeDocumentWireframeStyle::Unknown:
      break;
  }

  return {0.86F, 0.88F, 0.82F, 1.0F};
}

ProductCreativeWireframeDebugLineBuildResult
buildProductCreativeWireframeDebugLines(
    const ProductCreativeWireframeDebugLineBuildRequest& request) {
  ProductCreativeWireframeDebugLineBuildResult result;
  ProductCreativeWireframeDebugLineReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.sourceAvailable = request.sourceAvailable &&
                            (request.segments != nullptr ||
                             request.segmentCount == 0U);
  receipt.segmentCount = request.segmentCount;

  if (!receipt.sourceAvailable) {
    setStatus(receipt,
              ProductCreativeWireframeDebugLineStatus::MissingSource,
              "product_creative_wireframe_debug_lines_source_missing");
    return result;
  }

  if (request.segmentCount == 0U) {
    setStatus(receipt,
              ProductCreativeWireframeDebugLineStatus::NoLines,
              "product_creative_wireframe_debug_lines_source_empty");
    return result;
  }

  result.lineList.lines.reserve(request.segmentCount);
  const std::span<const creative::CreativeDocumentWireframeSegment> segments(
      request.segments,
      request.segmentCount);
  for (const creative::CreativeDocumentWireframeSegment& segment : segments) {
    if (creative::creativeVec3ExactlyEqual(segment.start, segment.end)) {
      ++receipt.skippedDegenerateCount;
      continue;
    }

    const creative::CreativeCoreVec3Conversion start =
        creative::creativeVec3ToCoreChecked(segment.start);
    const creative::CreativeCoreVec3Conversion end =
        creative::creativeVec3ToCoreChecked(segment.end);
    if (!start.converted || !end.converted) {
      ++receipt.skippedInvalidCount;
      continue;
    }

    ProductCreativeWireframeDebugLine line;
    line.start = start.value;
    line.end = end.value;
    line.color = productCreativeWireframeDebugLineColorForStyle(segment.style);
    line.objectId = segment.objectId;
    line.objectKind = segment.objectKind;
    line.style = segment.style;
    line.segmentKind = segment.segmentKind;
    line.thickness = request.thickness;
    result.lineList.lines.push_back(line);
  }

  receipt.lineCount = result.lineList.lines.size();
  if (receipt.lineCount > 0U) {
    setStatus(receipt,
              ProductCreativeWireframeDebugLineStatus::Built,
              "product_creative_wireframe_debug_lines_built");
    return result;
  }

  setStatus(receipt,
            ProductCreativeWireframeDebugLineStatus::NoLines,
            "product_creative_wireframe_debug_lines_none");
  return result;
}

ProductCreativeWireframeDebugLineBuildResult
buildProductCreativeWireframeDebugLines(
    std::span<const creative::CreativeDocumentWireframeSegment> segments) {
  return buildProductCreativeWireframeDebugLines(
      ProductCreativeWireframeDebugLineBuildRequest{segments.data(),
                                                    segments.size(),
                                                    true,
                                                    1.0F});
}

ProductCreativeWireframeDebugLineBuildResult
buildProductCreativeWireframeDebugLines(
    const creative::CreativeDocumentWireframeSegmentList& segmentList) {
  return buildProductCreativeWireframeDebugLines(std::span{
      segmentList.segments.data(),
      segmentList.segments.size()});
}

}  // namespace iggy3d
