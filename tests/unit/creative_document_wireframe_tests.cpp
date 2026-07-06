#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/document/DocumentWireframe.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameSegment(const cr::CreativeDocumentWireframeSegment& segment,
                 cr::CreativeObjectId objectId,
                 cr::CreativeDocumentWireframeSegmentKind kind,
                 cr::CreativeVec3 start,
                 cr::CreativeVec3 end) {
  return segment.objectId == objectId && segment.segmentKind == kind &&
         sameVec3(segment.start, start) && sameVec3(segment.end, end);
}

cr::CreativeSpatialProjectionRequest makeProjectionRequest() {
  cr::CreativeSpatialProjectionRequest request;
  request.gridSize = {64, 64, 16};
  request.cellSize = 1.0;
  request.clampToGrid = true;
  request.includeAuthoringOnly = false;
  return request;
}

std::vector<cr::CreativePathPoint> authoredPathPoints() {
  return {
      cr::CreativePathPoint{{2.0, 0.0, 2.0}},
      cr::CreativePathPoint{{4.0, 0.0, 2.0}},
      cr::CreativePathPoint{{4.0, 0.0, 4.0}},
  };
}

cr::CreativeObjectId createObject(cr::CreativeDocument& document,
                                  cr::CreativeObjectKind kind) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);
  return receipt.objectId;
}

cr::CreativeObjectId createPathObject(cr::CreativeDocument& document) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::PatrolRoute;
  request.name = "Patrol Route";
  request.hasPathOverride = true;
  request.pathPoints = authoredPathPoints();
  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);
  return receipt.objectId;
}

bool emptyDocumentReturnsStableEmptyReceipt() {
  const cr::CreativeDocument document;
  const cr::CreativeDocumentWireframeBuildResult result =
      cr::buildCreativeDocumentWireframeList(document, makeProjectionRequest());
  const cr::CreativeDocumentWireframeReceipt& receipt = result.receipt;

  return expect(receipt.requested, "empty requested") &&
         expect(receipt.documentAvailable, "empty document available") &&
         expect(!receipt.sourceAvailable, "empty source unavailable") &&
         expect(!receipt.projectionValid, "empty projection not needed") &&
         expect(receipt.objectCount == 0U, "empty object count") &&
         expect(receipt.itemCount == 0U, "empty item count") &&
         expect(result.drawList.items.empty(), "empty draw list") &&
         expect(receipt.status ==
                    cr::CreativeDocumentWireframeStatus::EmptySource,
                "empty status") &&
         expect(receipt.reasonCode ==
                    "creative_document_wireframe_source_empty",
                "empty reason");
}

bool genericRoomCreateProducesDescriptorBoxItem() {
  cr::CreativeDocument document;
  const cr::CreativeObjectId roomId =
      createObject(document, cr::CreativeObjectKind::Room);
  const cr::CreativeDocumentWireframeBuildResult result =
      cr::buildCreativeDocumentWireframeList(document, makeProjectionRequest());
  const cr::CreativeDocumentWireframeReceipt& receipt = result.receipt;
  const cr::CreativeDocumentWireframeItem& item = result.drawList.items[0];

  return expect(roomId != cr::kInvalidObjectId, "room created") &&
         expect(receipt.status == cr::CreativeDocumentWireframeStatus::Built,
                "room status built") &&
         expect(receipt.reasonCode == "creative_document_wireframe_built",
                "room reason") &&
         expect(receipt.objectCount == 1U, "room object count") &&
         expect(receipt.visibleObjectCount == 1U, "room visible count") &&
         expect(receipt.projectableObjectCount == 1U,
                "room projectable count") &&
         expect(receipt.projectedObjectCount == 1U,
                "room projected count") &&
         expect(receipt.projectionCellCount == 400U,
                "room projected cell count") &&
         expect(receipt.itemCount == 1U, "room item count") &&
         expect(result.drawList.items.size() == 1U, "room list count") &&
         expect(item.itemKind == cr::CreativeDocumentWireframeItemKind::Box,
                "room item kind") &&
         expect(item.objectId == roomId, "room item id") &&
         expect(item.objectKind == cr::CreativeObjectKind::Room,
                "room item object kind") &&
         expect(item.visible, "room item visible") &&
         expect(item.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::BoxProjection,
                "room item projection profile") &&
         expect(item.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Structural,
                "room item occupancy") &&
         expect(item.style == cr::CreativeDocumentWireframeStyle::Structural,
                "room item style") &&
         expect(sameVec3(item.bounds.min, {0.0, 0.0, 0.0}),
                "room bounds min") &&
         expect(sameVec3(item.bounds.max, {10.0, 4.0, 10.0}),
                "room bounds max") &&
         expect(sameVec3(item.start, {0.0, 0.0, 0.0}),
                "room start") &&
         expect(sameVec3(item.end, {10.0, 4.0, 10.0}), "room end") &&
         expect(item.projectedCellCount == 400U, "room item cells");
}

bool hiddenRoomProducesNoItemsButKeepsObjectCount() {
  cr::CreativeDocument document;
  const cr::CreativeObjectId roomId =
      createObject(document, cr::CreativeObjectKind::Room);
  const cr::CreativeDocumentMutationReceipt mutation =
      cr::setDocumentObjectVisible(document, roomId, false);

  const cr::CreativeDocumentWireframeBuildResult result =
      cr::buildCreativeDocumentWireframeList(document, makeProjectionRequest());
  const cr::CreativeDocumentWireframeReceipt& receipt = result.receipt;

  return expect(mutation.status == cr::CreativeDocumentMutationStatus::Applied,
                "hidden mutation applied") &&
         expect(receipt.objectCount == 1U, "hidden object count") &&
         expect(receipt.visibleObjectCount == 0U, "hidden visible count") &&
         expect(receipt.hiddenObjectCount == 1U, "hidden count") &&
         expect(receipt.itemCount == 0U, "hidden item count") &&
         expect(result.drawList.items.empty(), "hidden draw list") &&
         expect(receipt.status ==
                    cr::CreativeDocumentWireframeStatus::NoVisibleItems,
                "hidden status") &&
         expect(receipt.reasonCode ==
                    "creative_document_wireframe_no_visible_items",
                "hidden reason");
}

bool visibilityToggleRemovesAndRestoresItem() {
  cr::CreativeDocument document;
  const cr::CreativeObjectId roomId =
      createObject(document, cr::CreativeObjectKind::Room);
  static_cast<void>(cr::setDocumentObjectVisible(document, roomId, false));
  const cr::CreativeDocumentWireframeBuildResult hidden =
      cr::buildCreativeDocumentWireframeList(document, makeProjectionRequest());

  const cr::CreativeDocumentMutationReceipt shownMutation =
      cr::setDocumentObjectVisible(document, roomId, true);
  const cr::CreativeDocumentWireframeBuildResult shown =
      cr::buildCreativeDocumentWireframeList(document, makeProjectionRequest());

  return expect(hidden.receipt.itemCount == 0U, "toggle hidden no item") &&
         expect(shownMutation.status ==
                    cr::CreativeDocumentMutationStatus::Applied,
                "toggle visible mutation applied") &&
         expect(shown.receipt.itemCount == 1U, "toggle visible item") &&
         expect(shown.drawList.items[0].objectId == roomId,
                "toggle visible item id") &&
         expect(shown.drawList.items[0].visible,
                "toggle visible item visible") &&
         expect(document.objectCount() == 1U, "toggle object retained");
}

bool multipleVisibleObjectsPreserveDocumentOrder() {
  cr::CreativeDocument document;
  const cr::CreativeObjectId roomId =
      createObject(document, cr::CreativeObjectKind::Room);
  const cr::CreativeObjectId crateId =
      createObject(document, cr::CreativeObjectKind::Crate);

  const cr::CreativeDocumentWireframeBuildResult result =
      cr::buildCreativeDocumentWireframeList(document, makeProjectionRequest());

  return expect(roomId != cr::kInvalidObjectId, "order room created") &&
         expect(crateId != cr::kInvalidObjectId, "order crate created") &&
         expect(result.receipt.status ==
                    cr::CreativeDocumentWireframeStatus::Built,
                "order built") &&
         expect(result.receipt.objectCount == 2U, "order object count") &&
         expect(result.receipt.itemCount == 2U, "order item count") &&
         expect(result.drawList.items.size() == 2U, "order list size") &&
         expect(result.drawList.items[0].objectId == roomId,
                "order first room") &&
         expect(result.drawList.items[0].objectKind ==
                    cr::CreativeObjectKind::Room,
                "order first kind") &&
         expect(result.drawList.items[1].objectId == crateId,
                "order second crate") &&
         expect(result.drawList.items[1].objectKind ==
                    cr::CreativeObjectKind::Crate,
                "order second kind") &&
         expect(result.drawList.items[1].itemKind ==
                    cr::CreativeDocumentWireframeItemKind::Box,
                "order crate box") &&
         expect(sameVec3(result.drawList.items[1].bounds.max,
                         {1.0, 1.0, 1.0}),
                "order crate default bounds");
}

bool patrolRoutePathEmitsOrderedGameplayLineSegments() {
  cr::CreativeDocument document;
  const cr::CreativeObjectId routeId = createPathObject(document);

  const cr::CreativeDocumentWireframeBuildResult list =
      cr::buildCreativeDocumentWireframeList(document, makeProjectionRequest());
  const cr::CreativeDocumentWireframeSegmentBuildResult segments =
      cr::buildCreativeDocumentWireframeSegments(document,
                                                makeProjectionRequest());
  const cr::CreativeDocumentWireframeItem& item = list.drawList.items[0];
  const std::vector<cr::CreativeDocumentWireframeSegment>& segmentList =
      segments.segmentList.segments;

  return expect(routeId != cr::kInvalidObjectId, "path route created") &&
         expect(list.receipt.status == cr::CreativeDocumentWireframeStatus::Built,
                "path list built") &&
         expect(list.receipt.objectCount == 1U, "path object count") &&
         expect(list.receipt.projectableObjectCount == 1U,
                "path projectable count") &&
         expect(list.receipt.projectedObjectCount == 1U,
                "path projected count") &&
         expect(list.receipt.itemCount == 1U, "path item count") &&
         expect(list.drawList.items.size() == 1U, "path item vector count") &&
         expect(item.itemKind == cr::CreativeDocumentWireframeItemKind::Line,
                "path item is line") &&
         expect(item.objectId == routeId, "path item id") &&
         expect(item.objectKind == cr::CreativeObjectKind::PatrolRoute,
                "path item object kind") &&
         expect(item.projectionProfile ==
                    cr::CreativeSpatialProjectionProfile::PathProjection,
                "path item projection") &&
         expect(item.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Gameplay,
                "path item occupancy") &&
         expect(item.style == cr::CreativeDocumentWireframeStyle::Gameplay,
                "path item style") &&
         expect(item.pathPoints.size() == 3U, "path item point count") &&
         expect(sameVec3(item.pathPoints[0], {2.0, 0.0, 2.0}),
                "path item point 0") &&
         expect(sameVec3(item.pathPoints[1], {4.0, 0.0, 2.0}),
                "path item point 1") &&
         expect(sameVec3(item.pathPoints[2], {4.0, 0.0, 4.0}),
                "path item point 2") &&
         expect(segments.receipt.itemCount == 1U, "path segment item count") &&
         expect(segments.receipt.boxItemCount == 0U, "path segment no boxes") &&
         expect(segments.receipt.lineItemCount == 1U, "path segment line item") &&
         expect(segments.receipt.pointItemCount == 0U, "path segment no points") &&
         expect(segments.receipt.segmentCount == 2U, "path segment count") &&
         expect(segments.receipt.skippedDegenerateCount == 0U,
                "path segment no skipped") &&
         expect(segments.receipt.status ==
                    cr::CreativeDocumentWireframeSegmentStatus::Built,
                "path segments built") &&
         expect(segmentList.size() == 2U, "path segment vector count") &&
         expect(sameSegment(segmentList[0],
                            routeId,
                            cr::CreativeDocumentWireframeSegmentKind::Line,
                            {2.0, 0.0, 2.0},
                            {4.0, 0.0, 2.0}),
                "path first segment") &&
         expect(segmentList[0].style == cr::CreativeDocumentWireframeStyle::Gameplay,
                "path first segment style") &&
         expect(sameSegment(segmentList[1],
                            routeId,
                            cr::CreativeDocumentWireframeSegmentKind::Line,
                            {4.0, 0.0, 2.0},
                            {4.0, 0.0, 4.0}),
                "path second segment") &&
         expect(segmentList[1].style == cr::CreativeDocumentWireframeStyle::Gameplay,
                "path second segment style");
}

bool objectSpanUnknownObjectIsCountedButNotRendered() {
  cr::CreativeObject unknown;
  unknown.id = 77;
  unknown.kind = cr::CreativeObjectKind::Unknown;
  unknown.visible = true;
  const std::vector<cr::CreativeObject> objects{unknown};

  const cr::CreativeDocumentWireframeBuildResult result =
      cr::buildCreativeObjectWireframeList(objects, makeProjectionRequest());
  const cr::CreativeDocumentWireframeReceipt& receipt = result.receipt;

  return expect(receipt.requested, "unknown requested") &&
         expect(!receipt.documentAvailable, "unknown no document") &&
         expect(receipt.sourceAvailable, "unknown source available") &&
         expect(receipt.projectionValid, "unknown projection valid") &&
         expect(receipt.objectCount == 1U, "unknown object count") &&
         expect(receipt.visibleObjectCount == 1U, "unknown visible count") &&
         expect(receipt.nonProjectableObjectCount == 1U,
                "unknown non-projectable count") &&
         expect(receipt.itemCount == 0U, "unknown item count") &&
         expect(result.drawList.items.empty(), "unknown no items") &&
         expect(receipt.status ==
                    cr::CreativeDocumentWireframeStatus::NoRenderableItems,
                "unknown status") &&
         expect(receipt.reasonCode ==
                    "creative_document_wireframe_no_render_items",
                "unknown reason");
}

bool invalidProjectionSettingsFailClosed() {
  cr::CreativeDocument document;
  static_cast<void>(createObject(document, cr::CreativeObjectKind::Room));
  cr::CreativeSpatialProjectionRequest request = makeProjectionRequest();
  request.gridSize = {0, 64, 16};

  const cr::CreativeDocumentWireframeBuildResult result =
      cr::buildCreativeDocumentWireframeList(document, request);

  return expect(result.receipt.requested, "invalid requested") &&
         expect(result.receipt.documentAvailable, "invalid document") &&
         expect(result.receipt.sourceAvailable, "invalid source") &&
         expect(!result.receipt.projectionValid, "invalid projection flag") &&
         expect(result.receipt.objectCount == 1U, "invalid object count") &&
         expect(result.receipt.itemCount == 0U, "invalid item count") &&
         expect(result.receipt.status ==
                    cr::CreativeDocumentWireframeStatus::InvalidProjection,
                "invalid status") &&
         expect(result.receipt.reasonCode ==
                    "creative_document_wireframe_invalid_projection",
                "invalid reason");
}

bool roomDefaultBoxEmitsTwelveStableEdges() {
  cr::CreativeDocument document;
  const cr::CreativeObjectId roomId =
      createObject(document, cr::CreativeObjectKind::Room);

  const cr::CreativeDocumentWireframeSegmentBuildResult result =
      cr::buildCreativeDocumentWireframeSegments(document,
                                                makeProjectionRequest());
  const std::vector<cr::CreativeDocumentWireframeSegment>& segments =
      result.segmentList.segments;

  return expect(roomId != cr::kInvalidObjectId, "segments room created") &&
         expect(result.receipt.requested, "segments requested") &&
         expect(result.receipt.sourceAvailable, "segments source") &&
         expect(result.receipt.itemCount == 1U, "segments item count") &&
         expect(result.receipt.boxItemCount == 1U, "segments box count") &&
         expect(result.receipt.lineItemCount == 0U, "segments line count") &&
         expect(result.receipt.pointItemCount == 0U, "segments point count") &&
         expect(result.receipt.skippedDegenerateCount == 0U,
                "segments no degenerates") &&
         expect(result.receipt.segmentCount == 12U, "segments count") &&
         expect(result.receipt.status ==
                    cr::CreativeDocumentWireframeSegmentStatus::Built,
                "segments built") &&
         expect(result.receipt.reasonCode ==
                    "creative_document_wireframe_segments_built",
                "segments reason") &&
         expect(segments.size() == 12U, "segments vector count") &&
         expect(sameSegment(segments[0],
                            roomId,
                            cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                            {0.0, 0.0, 0.0},
                            {10.0, 0.0, 0.0}),
                "bottom edge 0") &&
         expect(sameSegment(segments[1],
                            roomId,
                            cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                            {10.0, 0.0, 0.0},
                            {10.0, 0.0, 10.0}),
                "bottom edge 1") &&
         expect(sameSegment(segments[2],
                            roomId,
                            cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                            {10.0, 0.0, 10.0},
                            {0.0, 0.0, 10.0}),
                "bottom edge 2") &&
         expect(sameSegment(segments[3],
                            roomId,
                            cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                            {0.0, 0.0, 10.0},
                            {0.0, 0.0, 0.0}),
                "bottom edge 3") &&
         expect(sameSegment(segments[4],
                            roomId,
                            cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                            {0.0, 4.0, 0.0},
                            {10.0, 4.0, 0.0}),
                "top edge 0") &&
         expect(sameSegment(segments[5],
                            roomId,
                            cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                            {10.0, 4.0, 0.0},
                            {10.0, 4.0, 10.0}),
                "top edge 1") &&
         expect(sameSegment(segments[6],
                            roomId,
                            cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                            {10.0, 4.0, 10.0},
                            {0.0, 4.0, 10.0}),
                "top edge 2") &&
         expect(sameSegment(segments[7],
                            roomId,
                            cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                            {0.0, 4.0, 10.0},
                            {0.0, 4.0, 0.0}),
                "top edge 3") &&
         expect(sameSegment(segments[8],
                            roomId,
                            cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                            {0.0, 0.0, 0.0},
                            {0.0, 4.0, 0.0}),
                "vertical edge 0") &&
         expect(sameSegment(segments[9],
                            roomId,
                            cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                            {10.0, 0.0, 0.0},
                            {10.0, 4.0, 0.0}),
                "vertical edge 1") &&
         expect(sameSegment(segments[10],
                            roomId,
                            cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                            {10.0, 0.0, 10.0},
                            {10.0, 4.0, 10.0}),
                "vertical edge 2") &&
         expect(sameSegment(segments[11],
                            roomId,
                            cr::CreativeDocumentWireframeSegmentKind::BoxEdge,
                            {0.0, 0.0, 10.0},
                            {0.0, 4.0, 10.0}),
                "vertical edge 3");
}

bool roomAndCrateSegmentsPreserveItemOrder() {
  cr::CreativeDocument document;
  const cr::CreativeObjectId roomId =
      createObject(document, cr::CreativeObjectKind::Room);
  const cr::CreativeObjectId crateId =
      createObject(document, cr::CreativeObjectKind::Crate);

  const cr::CreativeDocumentWireframeSegmentBuildResult result =
      cr::buildCreativeDocumentWireframeSegments(document,
                                                makeProjectionRequest());
  const std::vector<cr::CreativeDocumentWireframeSegment>& segments =
      result.segmentList.segments;

  return expect(result.receipt.itemCount == 2U, "order segment item count") &&
         expect(result.receipt.boxItemCount == 2U, "order segment boxes") &&
         expect(result.receipt.segmentCount == 24U, "order segment count") &&
         expect(segments.size() == 24U, "order vector size") &&
         expect(segments[0].objectId == roomId, "order first room") &&
         expect(segments[11].objectId == roomId, "order last room") &&
         expect(segments[12].objectId == crateId, "order first crate") &&
         expect(segments[23].objectId == crateId, "order last crate");
}

bool hiddenRoomProducesNoSegmentsThroughConveniencePath() {
  cr::CreativeDocument document;
  const cr::CreativeObjectId roomId =
      createObject(document, cr::CreativeObjectKind::Room);
  static_cast<void>(cr::setDocumentObjectVisible(document, roomId, false));

  const cr::CreativeDocumentWireframeSegmentBuildResult result =
      cr::buildCreativeDocumentWireframeSegments(document,
                                                makeProjectionRequest());

  return expect(result.receipt.requested, "hidden segments requested") &&
         expect(result.receipt.sourceAvailable, "hidden segments source") &&
         expect(result.receipt.itemCount == 0U, "hidden segment items") &&
         expect(result.receipt.segmentCount == 0U, "hidden segment count") &&
         expect(result.segmentList.segments.empty(), "hidden no segments") &&
         expect(result.receipt.status ==
                    cr::CreativeDocumentWireframeSegmentStatus::EmptySource,
                "hidden segment status") &&
         expect(result.receipt.reasonCode ==
                    "creative_document_wireframe_segments_source_empty",
                "hidden segment reason");
}

bool lineItemEmitsOneSegment() {
  cr::CreativeDocumentWireframeItem item;
  item.itemKind = cr::CreativeDocumentWireframeItemKind::Line;
  item.objectId = 19;
  item.objectKind = cr::CreativeObjectKind::CameraRail;
  item.style = cr::CreativeDocumentWireframeStyle::Camera;
  item.start = {1.0, 2.0, 3.0};
  item.end = {4.0, 5.0, 6.0};

  cr::CreativeDocumentWireframeDrawList drawList;
  drawList.items.push_back(item);
  const cr::CreativeDocumentWireframeSegmentBuildResult result =
      cr::buildCreativeDocumentWireframeSegments(drawList);

  return expect(result.receipt.itemCount == 1U, "line item count") &&
         expect(result.receipt.lineItemCount == 1U, "line count") &&
         expect(result.receipt.segmentCount == 1U, "line segment count") &&
         expect(result.segmentList.segments.size() == 1U,
                "line segment vector") &&
         expect(sameSegment(result.segmentList.segments[0],
                            19,
                            cr::CreativeDocumentWireframeSegmentKind::Line,
                            {1.0, 2.0, 3.0},
                            {4.0, 5.0, 6.0}),
                "line segment endpoints") &&
         expect(result.segmentList.segments[0].objectKind ==
                    cr::CreativeObjectKind::CameraRail,
                "line object kind") &&
         expect(result.segmentList.segments[0].style ==
                    cr::CreativeDocumentWireframeStyle::Camera,
                "line style");
}

bool pointItemEmitsNoSegmentsWithTruthfulCounts() {
  cr::CreativeDocumentWireframeItem item;
  item.itemKind = cr::CreativeDocumentWireframeItemKind::Point;
  item.objectId = 23;
  item.objectKind = cr::CreativeObjectKind::SpawnPoint;
  item.style = cr::CreativeDocumentWireframeStyle::Navigation;
  item.start = {2.0, 3.0, 4.0};
  item.end = item.start;

  cr::CreativeDocumentWireframeDrawList drawList;
  drawList.items.push_back(item);
  const cr::CreativeDocumentWireframeSegmentBuildResult result =
      cr::buildCreativeDocumentWireframeSegments(drawList);

  return expect(result.receipt.itemCount == 1U, "point item count") &&
         expect(result.receipt.pointItemCount == 1U, "point count") &&
         expect(result.receipt.segmentCount == 0U, "point segment count") &&
         expect(result.receipt.skippedDegenerateCount == 0U,
                "point no degenerate") &&
         expect(result.segmentList.segments.empty(), "point no segments") &&
         expect(result.receipt.status ==
                    cr::CreativeDocumentWireframeSegmentStatus::NoSegments,
                "point status") &&
         expect(result.receipt.reasonCode ==
                    "creative_document_wireframe_segments_none",
                "point reason");
}

bool degenerateBoxIsSkippedWithoutSegments() {
  cr::CreativeDocumentWireframeItem item;
  item.itemKind = cr::CreativeDocumentWireframeItemKind::Box;
  item.objectId = 29;
  item.objectKind = cr::CreativeObjectKind::Crate;
  item.style = cr::CreativeDocumentWireframeStyle::Structural;
  item.bounds = {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}};

  cr::CreativeDocumentWireframeDrawList drawList;
  drawList.items.push_back(item);
  const cr::CreativeDocumentWireframeSegmentBuildResult result =
      cr::buildCreativeDocumentWireframeSegments(drawList);

  return expect(result.receipt.itemCount == 1U,
                "degenerate item count") &&
         expect(result.receipt.boxItemCount == 1U,
                "degenerate box count") &&
         expect(result.receipt.skippedDegenerateCount == 1U,
                "degenerate skipped count") &&
         expect(result.receipt.segmentCount == 0U,
                "degenerate no segments") &&
         expect(result.segmentList.segments.empty(),
                "degenerate segment vector") &&
         expect(result.receipt.status ==
                    cr::CreativeDocumentWireframeSegmentStatus::NoSegments,
                "degenerate status") &&
         expect(result.receipt.reasonCode ==
                    "creative_document_wireframe_segments_none",
                "degenerate reason");
}

}  // namespace

int main() {
  const bool ok = emptyDocumentReturnsStableEmptyReceipt() &&
                  genericRoomCreateProducesDescriptorBoxItem() &&
                  hiddenRoomProducesNoItemsButKeepsObjectCount() &&
                  visibilityToggleRemovesAndRestoresItem() &&
                  multipleVisibleObjectsPreserveDocumentOrder() &&
                  patrolRoutePathEmitsOrderedGameplayLineSegments() &&
                  objectSpanUnknownObjectIsCountedButNotRendered() &&
                  invalidProjectionSettingsFailClosed() &&
                  roomDefaultBoxEmitsTwelveStableEdges() &&
                  roomAndCrateSegmentsPreserveItemOrder() &&
                  hiddenRoomProducesNoSegmentsThroughConveniencePath() &&
                  lineItemEmitsOneSegment() &&
                  pointItemEmitsNoSegmentsWithTruthfulCounts() &&
                  degenerateBoxIsSkippedWithoutSegments();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
