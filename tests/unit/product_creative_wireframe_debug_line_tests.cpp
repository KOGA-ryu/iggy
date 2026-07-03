#include "app/iggy3d/view/CreativeWireframeDebugLines.hpp"

#include "app/iggy3d/creative/Document.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool sameVec3(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameColor(iggy3d::ProductCreativeWireframeDebugLineColor lhs,
               iggy3d::ProductCreativeWireframeDebugLineColor rhs) {
  return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b &&
         lhs.a == rhs.a;
}

cr::CreativeSpatialProjectionRequest projectionRequest() {
  cr::CreativeSpatialProjectionRequest request;
  request.gridSize = {64, 64, 16};
  request.cellSize = 1.0;
  request.clampToGrid = true;
  request.includeAuthoringOnly = false;
  return request;
}

cr::CreativeObjectId createObject(cr::CreativeDocument& document,
                                  cr::CreativeObjectKind kind) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);
  return receipt.objectId;
}

cr::CreativeDocumentWireframeSegment segment(
    cr::CreativeObjectId objectId,
    cr::CreativeObjectKind objectKind,
    cr::CreativeDocumentWireframeStyle style,
    cr::CreativeDocumentWireframeSegmentKind segmentKind,
    cr::CreativeVec3 start,
    cr::CreativeVec3 end) {
  cr::CreativeDocumentWireframeSegment out;
  out.objectId = objectId;
  out.objectKind = objectKind;
  out.style = style;
  out.segmentKind = segmentKind;
  out.start = start;
  out.end = end;
  return out;
}

bool roomDefaultDocumentBuildsTwelveStructuralLines() {
  cr::CreativeDocument document;
  const cr::CreativeObjectId roomId =
      createObject(document, cr::CreativeObjectKind::Room);
  const cr::CreativeDocumentWireframeSegmentBuildResult segments =
      cr::buildCreativeDocumentWireframeSegments(document,
                                                projectionRequest());

  const iggy3d::ProductCreativeWireframeDebugLineBuildResult result =
      iggy3d::buildProductCreativeWireframeDebugLines(segments.segmentList);
  const auto expectedColor =
      iggy3d::productCreativeWireframeDebugLineColorForStyle(
          cr::CreativeDocumentWireframeStyle::Structural);

  return expect(roomId != cr::kInvalidObjectId, "room created") &&
         expect(result.receipt.requested, "room lines requested") &&
         expect(result.receipt.sourceAvailable, "room lines source") &&
         expect(result.receipt.segmentCount == 12U,
                "room lines segment count") &&
         expect(result.receipt.lineCount == 12U, "room line count") &&
         expect(result.receipt.skippedDegenerateCount == 0U,
                "room no skipped") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeWireframeDebugLineStatus::Built,
                "room lines built") &&
         expect(result.receipt.reasonCode ==
                    "product_creative_wireframe_debug_lines_built",
                "room lines reason") &&
         expect(result.lineList.lines.size() == 12U,
                "room lines vector count") &&
         expect(result.lineList.lines[0].objectId == roomId,
                "room first id") &&
         expect(result.lineList.lines[0].objectKind ==
                    cr::CreativeObjectKind::Room,
                "room first kind") &&
         expect(result.lineList.lines[0].style ==
                    cr::CreativeDocumentWireframeStyle::Structural,
                "room first style") &&
         expect(result.lineList.lines[0].segmentKind ==
                    cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                "room first segment kind") &&
         expect(sameColor(result.lineList.lines[0].color, expectedColor),
                "room structural color") &&
         expect(sameVec3(result.lineList.lines[0].start,
                         {0.0F, 0.0F, 0.0F}),
                "room first start") &&
         expect(sameVec3(result.lineList.lines[0].end,
                         {10.0F, 0.0F, 0.0F}),
                "room first end") &&
         expect(sameVec3(result.lineList.lines[11].start,
                         {0.0F, 0.0F, 10.0F}),
                "room last start") &&
         expect(sameVec3(result.lineList.lines[11].end,
                         {0.0F, 4.0F, 10.0F}),
                "room last end") &&
         expect(result.lineList.lines[0].thickness == 1.0F,
                "room default thickness");
}

bool roomAndCrateLineOrderPreservesSegmentOrder() {
  cr::CreativeDocument document;
  const cr::CreativeObjectId roomId =
      createObject(document, cr::CreativeObjectKind::Room);
  const cr::CreativeObjectId crateId =
      createObject(document, cr::CreativeObjectKind::Crate);
  const cr::CreativeDocumentWireframeSegmentBuildResult segments =
      cr::buildCreativeDocumentWireframeSegments(document,
                                                projectionRequest());

  const iggy3d::ProductCreativeWireframeDebugLineBuildResult result =
      iggy3d::buildProductCreativeWireframeDebugLines(segments.segmentList);

  return expect(result.receipt.lineCount == 24U, "order line count") &&
         expect(result.lineList.lines.size() == 24U,
                "order line vector count") &&
         expect(result.lineList.lines[0].objectId == roomId,
                "order first room") &&
         expect(result.lineList.lines[11].objectId == roomId,
                "order last room") &&
         expect(result.lineList.lines[12].objectId == crateId,
                "order first crate") &&
         expect(result.lineList.lines[23].objectId == crateId,
                "order last crate");
}

bool emptySegmentListReportsNoLines() {
  const cr::CreativeDocumentWireframeSegmentList segments;

  const iggy3d::ProductCreativeWireframeDebugLineBuildResult result =
      iggy3d::buildProductCreativeWireframeDebugLines(segments);

  return expect(result.receipt.requested, "empty requested") &&
         expect(result.receipt.sourceAvailable, "empty source") &&
         expect(result.receipt.segmentCount == 0U,
                "empty segment count") &&
         expect(result.receipt.lineCount == 0U, "empty line count") &&
         expect(result.lineList.lines.empty(), "empty no lines") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeWireframeDebugLineStatus::NoLines,
                "empty no-lines status") &&
         expect(result.receipt.reasonCode ==
                    "product_creative_wireframe_debug_lines_source_empty",
                "empty reason");
}

bool missingSourceFailsClosed() {
  iggy3d::ProductCreativeWireframeDebugLineBuildRequest request;
  request.segmentCount = 1U;
  request.sourceAvailable = false;

  const iggy3d::ProductCreativeWireframeDebugLineBuildResult result =
      iggy3d::buildProductCreativeWireframeDebugLines(request);

  return expect(result.receipt.requested, "missing requested") &&
         expect(!result.receipt.sourceAvailable, "missing source false") &&
         expect(result.receipt.segmentCount == 1U,
                "missing segment count copied") &&
         expect(result.receipt.lineCount == 0U, "missing no lines") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeWireframeDebugLineStatus::MissingSource,
                "missing status") &&
         expect(result.receipt.reasonCode ==
                    "product_creative_wireframe_debug_lines_source_missing",
                "missing reason");
}

bool unknownStyleNondegenerateSegmentEmitsFallbackLine() {
  cr::CreativeDocumentWireframeSegmentList segments;
  segments.segments.push_back(segment(
      42,
      cr::CreativeObjectKind::Unknown,
      cr::CreativeDocumentWireframeStyle::Unknown,
      cr::CreativeDocumentWireframeSegmentKind::Unknown,
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0}));

  const iggy3d::ProductCreativeWireframeDebugLineBuildResult result =
      iggy3d::buildProductCreativeWireframeDebugLines(segments);
  const auto fallback =
      iggy3d::productCreativeWireframeDebugLineColorForStyle(
          cr::CreativeDocumentWireframeStyle::Unknown);

  return expect(result.receipt.lineCount == 1U, "unknown line count") &&
         expect(result.receipt.skippedDegenerateCount == 0U,
                "unknown no skipped") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeWireframeDebugLineStatus::Built,
                "unknown built") &&
         expect(result.lineList.lines[0].objectId == 42U,
                "unknown object id") &&
         expect(result.lineList.lines[0].objectKind ==
                    cr::CreativeObjectKind::Unknown,
                "unknown object kind") &&
         expect(result.lineList.lines[0].segmentKind ==
                    cr::CreativeDocumentWireframeSegmentKind::Unknown,
                "unknown segment kind") &&
         expect(sameColor(result.lineList.lines[0].color, fallback),
                "unknown fallback color") &&
         expect(sameVec3(result.lineList.lines[0].start,
                         {1.0F, 2.0F, 3.0F}),
                "unknown start") &&
         expect(sameVec3(result.lineList.lines[0].end,
                         {4.0F, 5.0F, 6.0F}),
                "unknown end");
}

bool degenerateSegmentIsSkippedAndCounted() {
  cr::CreativeDocumentWireframeSegmentList segments;
  segments.segments.push_back(segment(
      57,
      cr::CreativeObjectKind::Room,
      cr::CreativeDocumentWireframeStyle::Structural,
      cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
      {2.0, 2.0, 2.0},
      {2.0, 2.0, 2.0}));

  const iggy3d::ProductCreativeWireframeDebugLineBuildResult result =
      iggy3d::buildProductCreativeWireframeDebugLines(segments);

  return expect(result.receipt.segmentCount == 1U,
                "degenerate segment count") &&
         expect(result.receipt.lineCount == 0U, "degenerate line count") &&
         expect(result.receipt.skippedDegenerateCount == 1U,
                "degenerate skipped count") &&
         expect(result.lineList.lines.empty(), "degenerate no lines") &&
         expect(result.receipt.status ==
                    iggy3d::ProductCreativeWireframeDebugLineStatus::NoLines,
                "degenerate no-lines status") &&
         expect(result.receipt.reasonCode ==
                    "product_creative_wireframe_debug_lines_none",
                "degenerate reason");
}

bool customThicknessIsCopied() {
  cr::CreativeDocumentWireframeSegment segments[] = {
      segment(7,
              cr::CreativeObjectKind::CameraRail,
              cr::CreativeDocumentWireframeStyle::Camera,
              cr::CreativeDocumentWireframeSegmentKind::Line,
              {0.0, 0.0, 0.0},
              {0.0, 1.0, 0.0}),
  };
  iggy3d::ProductCreativeWireframeDebugLineBuildRequest request;
  request.segments = segments;
  request.segmentCount = 1U;
  request.sourceAvailable = true;
  request.thickness = 2.5F;

  const iggy3d::ProductCreativeWireframeDebugLineBuildResult result =
      iggy3d::buildProductCreativeWireframeDebugLines(request);

  return expect(result.receipt.lineCount == 1U, "thickness line count") &&
         expect(result.lineList.lines[0].thickness == 2.5F,
                "thickness copied");
}

}  // namespace

int main() {
  const bool ok = roomDefaultDocumentBuildsTwelveStructuralLines() &&
                  roomAndCrateLineOrderPreservesSegmentOrder() &&
                  emptySegmentListReportsNoLines() &&
                  missingSourceFailsClosed() &&
                  unknownStyleNondegenerateSegmentEmitsFallbackLine() &&
                  degenerateSegmentIsSkippedAndCounted() &&
                  customThicknessIsCopied();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
