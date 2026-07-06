#include "app/iggy3d/creative/document/DocumentWireframe.hpp"

#include <algorithm>
#include <span>
#include <string_view>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool isValidWireframeProjectionRequest(
    const CreativeSpatialProjectionRequest& request) noexcept {
  return isValidGridSize(request.gridSize) && request.cellSize > 0.0;
}

void setReceiptStatus(CreativeDocumentWireframeReceipt& receipt,
                      CreativeDocumentWireframeStatus status,
                      std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.message = reasonCode;
  receipt.reasonCode = reasonCode;
}

void setSegmentReceiptStatus(CreativeDocumentWireframeSegmentReceipt& receipt,
                             CreativeDocumentWireframeSegmentStatus status,
                             std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.message = reasonCode;
  receipt.reasonCode = reasonCode;
}

[[nodiscard]] bool isNonProjectableObject(
    const CreativeObject& object,
    CreativeSpatialProjectionProfile profile) noexcept {
  return object.id == kInvalidObjectId ||
         object.kind == CreativeObjectKind::Unknown ||
         profile == CreativeSpatialProjectionProfile::Unknown ||
         profile == CreativeSpatialProjectionProfile::NoProjection;
}

[[nodiscard]] CreativeDocumentWireframeItem makeWireframeItem(
    const CreativeObject& object,
    const CreativeSpatialProjectionReceipt& projectionReceipt) {
  CreativeDocumentWireframeItem item;
  item.itemKind = wireframeItemKindForProjection(projectionReceipt.profile);
  item.objectId = object.id;
  item.objectKind = object.kind;
  item.visible = object.visible;
  item.projectionProfile = projectionReceipt.profile;
  item.occupancyKind = projectionReceipt.occupancyKind;
  item.style = wireframeStyleForOccupancy(projectionReceipt.occupancyKind);
  item.projectedBounds = projectionReceipt.projectedBounds;
  item.projectedCellCount = projectionReceipt.cells.size();

  switch (item.itemKind) {
    case CreativeDocumentWireframeItemKind::Box:
      item.bounds = object.bounds;
      item.start = object.bounds.min;
      item.end = object.bounds.max;
      break;
    case CreativeDocumentWireframeItemKind::Point:
      item.bounds = CreativeBounds{object.transform.position,
                                   object.transform.position};
      item.start = object.transform.position;
      item.end = object.transform.position;
      break;
    case CreativeDocumentWireframeItemKind::Line:
      if (projectionReceipt.profile ==
              CreativeSpatialProjectionProfile::PathProjection ||
          projectionReceipt.profile ==
              CreativeSpatialProjectionProfile::LinkProjection) {
        item.pathPoints.reserve(object.pathPoints.size());
        for (const CreativePathPoint& point : object.pathPoints) {
          item.pathPoints.push_back(point.position);
        }
        if (!item.pathPoints.empty()) {
          item.start = item.pathPoints.front();
          item.end = item.pathPoints.back();
          item.bounds = CreativeBounds{item.start, item.start};
          for (const CreativeVec3 point : item.pathPoints) {
            item.bounds.min.x = std::min(item.bounds.min.x, point.x);
            item.bounds.min.y = std::min(item.bounds.min.y, point.y);
            item.bounds.min.z = std::min(item.bounds.min.z, point.z);
            item.bounds.max.x = std::max(item.bounds.max.x, point.x);
            item.bounds.max.y = std::max(item.bounds.max.y, point.y);
            item.bounds.max.z = std::max(item.bounds.max.z, point.z);
          }
        }
      } else {
        item.bounds = object.bounds;
        item.start = object.bounds.min;
        item.end = object.bounds.max;
      }
      break;
    case CreativeDocumentWireframeItemKind::Unknown:
      break;
  }

  return item;
}

[[nodiscard]] bool samePoint(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] bool isDegenerateBounds(CreativeBounds bounds) noexcept {
  return bounds.min.x >= bounds.max.x || bounds.min.y >= bounds.max.y ||
         bounds.min.z >= bounds.max.z;
}

void appendSegment(CreativeDocumentWireframeSegmentList& list,
                   const CreativeDocumentWireframeItem& item,
                   CreativeDocumentWireframeSegmentKind segmentKind,
                   CreativeVec3 start,
                   CreativeVec3 end) {
  CreativeDocumentWireframeSegment segment;
  segment.objectId = item.objectId;
  segment.objectKind = item.objectKind;
  segment.style = item.style;
  segment.segmentKind = segmentKind;
  segment.start = start;
  segment.end = end;
  list.segments.push_back(segment);
}

void appendBoxSegments(CreativeDocumentWireframeSegmentList& list,
                       const CreativeDocumentWireframeItem& item) {
  const CreativeVec3 min = item.bounds.min;
  const CreativeVec3 max = item.bounds.max;

  const CreativeVec3 bottomFrontLeft{min.x, min.y, min.z};
  const CreativeVec3 bottomFrontRight{max.x, min.y, min.z};
  const CreativeVec3 bottomBackRight{max.x, min.y, max.z};
  const CreativeVec3 bottomBackLeft{min.x, min.y, max.z};

  const CreativeVec3 topFrontLeft{min.x, max.y, min.z};
  const CreativeVec3 topFrontRight{max.x, max.y, min.z};
  const CreativeVec3 topBackRight{max.x, max.y, max.z};
  const CreativeVec3 topBackLeft{min.x, max.y, max.z};

  appendSegment(list,
                item,
                CreativeDocumentWireframeSegmentKind::BoxEdge,
                bottomFrontLeft,
                bottomFrontRight);
  appendSegment(list,
                item,
                CreativeDocumentWireframeSegmentKind::BoxEdge,
                bottomFrontRight,
                bottomBackRight);
  appendSegment(list,
                item,
                CreativeDocumentWireframeSegmentKind::BoxEdge,
                bottomBackRight,
                bottomBackLeft);
  appendSegment(list,
                item,
                CreativeDocumentWireframeSegmentKind::BoxEdge,
                bottomBackLeft,
                bottomFrontLeft);

  appendSegment(list,
                item,
                CreativeDocumentWireframeSegmentKind::BoxEdge,
                topFrontLeft,
                topFrontRight);
  appendSegment(list,
                item,
                CreativeDocumentWireframeSegmentKind::BoxEdge,
                topFrontRight,
                topBackRight);
  appendSegment(list,
                item,
                CreativeDocumentWireframeSegmentKind::BoxEdge,
                topBackRight,
                topBackLeft);
  appendSegment(list,
                item,
                CreativeDocumentWireframeSegmentKind::BoxEdge,
                topBackLeft,
                topFrontLeft);

  appendSegment(list,
                item,
                CreativeDocumentWireframeSegmentKind::BoxEdge,
                bottomFrontLeft,
                topFrontLeft);
  appendSegment(list,
                item,
                CreativeDocumentWireframeSegmentKind::BoxEdge,
                bottomFrontRight,
                topFrontRight);
  appendSegment(list,
                item,
                CreativeDocumentWireframeSegmentKind::BoxEdge,
                bottomBackRight,
                topBackRight);
  appendSegment(list,
                item,
                CreativeDocumentWireframeSegmentKind::BoxEdge,
                bottomBackLeft,
                topBackLeft);
}

}  // namespace

std::string_view toString(CreativeDocumentWireframeStatus status) noexcept {
  switch (status) {
    case CreativeDocumentWireframeStatus::Unknown:
      return "Unknown";
    case CreativeDocumentWireframeStatus::MissingSource:
      return "MissingSource";
    case CreativeDocumentWireframeStatus::EmptySource:
      return "EmptySource";
    case CreativeDocumentWireframeStatus::InvalidProjection:
      return "InvalidProjection";
    case CreativeDocumentWireframeStatus::NoVisibleItems:
      return "NoVisibleItems";
    case CreativeDocumentWireframeStatus::NoRenderableItems:
      return "NoRenderableItems";
    case CreativeDocumentWireframeStatus::Built:
      return "Built";
  }

  return "Unknown";
}

std::string_view toString(
    CreativeDocumentWireframeItemKind itemKind) noexcept {
  switch (itemKind) {
    case CreativeDocumentWireframeItemKind::Unknown:
      return "Unknown";
    case CreativeDocumentWireframeItemKind::Box:
      return "Box";
    case CreativeDocumentWireframeItemKind::Point:
      return "Point";
    case CreativeDocumentWireframeItemKind::Line:
      return "Line";
  }

  return "Unknown";
}

std::string_view toString(CreativeDocumentWireframeStyle style) noexcept {
  switch (style) {
    case CreativeDocumentWireframeStyle::Unknown:
      return "Unknown";
    case CreativeDocumentWireframeStyle::Structural:
      return "Structural";
    case CreativeDocumentWireframeStyle::Collision:
      return "Collision";
    case CreativeDocumentWireframeStyle::Navigation:
      return "Navigation";
    case CreativeDocumentWireframeStyle::Trigger:
      return "Trigger";
    case CreativeDocumentWireframeStyle::Gameplay:
      return "Gameplay";
    case CreativeDocumentWireframeStyle::Light:
      return "Light";
    case CreativeDocumentWireframeStyle::Audio:
      return "Audio";
    case CreativeDocumentWireframeStyle::Camera:
      return "Camera";
    case CreativeDocumentWireframeStyle::Testing:
      return "Testing";
    case CreativeDocumentWireframeStyle::Authoring:
      return "Authoring";
  }

  return "Unknown";
}

std::string_view toString(
    CreativeDocumentWireframeSegmentStatus status) noexcept {
  switch (status) {
    case CreativeDocumentWireframeSegmentStatus::Unknown:
      return "Unknown";
    case CreativeDocumentWireframeSegmentStatus::MissingSource:
      return "MissingSource";
    case CreativeDocumentWireframeSegmentStatus::EmptySource:
      return "EmptySource";
    case CreativeDocumentWireframeSegmentStatus::Built:
      return "Built";
    case CreativeDocumentWireframeSegmentStatus::NoSegments:
      return "NoSegments";
  }

  return "Unknown";
}

std::string_view toString(
    CreativeDocumentWireframeSegmentKind segmentKind) noexcept {
  switch (segmentKind) {
    case CreativeDocumentWireframeSegmentKind::Unknown:
      return "Unknown";
    case CreativeDocumentWireframeSegmentKind::BoxEdge:
      return "BoxEdge";
    case CreativeDocumentWireframeSegmentKind::Line:
      return "Line";
  }

  return "Unknown";
}

CreativeDocumentWireframeStyle wireframeStyleForOccupancy(
    CreativeSpatialOccupancyKind occupancyKind) noexcept {
  switch (occupancyKind) {
    case CreativeSpatialOccupancyKind::Unknown:
      return CreativeDocumentWireframeStyle::Unknown;
    case CreativeSpatialOccupancyKind::Structural:
      return CreativeDocumentWireframeStyle::Structural;
    case CreativeSpatialOccupancyKind::Collision:
      return CreativeDocumentWireframeStyle::Collision;
    case CreativeSpatialOccupancyKind::Navigation:
      return CreativeDocumentWireframeStyle::Navigation;
    case CreativeSpatialOccupancyKind::Trigger:
      return CreativeDocumentWireframeStyle::Trigger;
    case CreativeSpatialOccupancyKind::Gameplay:
      return CreativeDocumentWireframeStyle::Gameplay;
    case CreativeSpatialOccupancyKind::Light:
      return CreativeDocumentWireframeStyle::Light;
    case CreativeSpatialOccupancyKind::Audio:
      return CreativeDocumentWireframeStyle::Audio;
    case CreativeSpatialOccupancyKind::Camera:
      return CreativeDocumentWireframeStyle::Camera;
    case CreativeSpatialOccupancyKind::Testing:
      return CreativeDocumentWireframeStyle::Testing;
    case CreativeSpatialOccupancyKind::Authoring:
      return CreativeDocumentWireframeStyle::Authoring;
  }

  return CreativeDocumentWireframeStyle::Unknown;
}

CreativeDocumentWireframeItemKind wireframeItemKindForProjection(
    CreativeSpatialProjectionProfile profile) noexcept {
  switch (profile) {
    case CreativeSpatialProjectionProfile::BoxProjection:
    case CreativeSpatialProjectionProfile::VolumeProjection:
      return CreativeDocumentWireframeItemKind::Box;
    case CreativeSpatialProjectionProfile::PointProjection:
      return CreativeDocumentWireframeItemKind::Point;
    case CreativeSpatialProjectionProfile::LineProjection:
    case CreativeSpatialProjectionProfile::PathProjection:
    case CreativeSpatialProjectionProfile::LinkProjection:
      return CreativeDocumentWireframeItemKind::Line;
    case CreativeSpatialProjectionProfile::Unknown:
    case CreativeSpatialProjectionProfile::NoProjection:
      return CreativeDocumentWireframeItemKind::Unknown;
  }

  return CreativeDocumentWireframeItemKind::Unknown;
}

CreativeDocumentWireframeBuildResult buildCreativeDocumentWireframeList(
    const CreativeDocumentWireframeBuildRequest& request) {
  CreativeDocumentWireframeBuildResult result;
  CreativeDocumentWireframeReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.documentAvailable = request.documentAvailable;
  receipt.objectCount = request.objectCount;
  receipt.sourceAvailable = request.objects != nullptr &&
                            request.objectCount > 0U;

  if (request.objects == nullptr && request.objectCount > 0U) {
    setReceiptStatus(receipt,
                     CreativeDocumentWireframeStatus::MissingSource,
                     "creative_document_wireframe_source_missing");
    return result;
  }

  if (request.objectCount == 0U) {
    setReceiptStatus(receipt,
                     CreativeDocumentWireframeStatus::EmptySource,
                     "creative_document_wireframe_source_empty");
    return result;
  }

  if (!isValidWireframeProjectionRequest(request.projectionRequest)) {
    setReceiptStatus(receipt,
                     CreativeDocumentWireframeStatus::InvalidProjection,
                     "creative_document_wireframe_invalid_projection");
    return result;
  }
  receipt.projectionValid = true;

  const std::span<const CreativeObject> objects(request.objects,
                                               request.objectCount);
  result.drawList.items.reserve(objects.size());
  for (const CreativeObject& object : objects) {
    const CreativeSpatialProjectionProfile profile =
        projectionProfileForObject(object.kind);
    if (!object.visible) {
      ++receipt.hiddenObjectCount;
      if (isNonProjectableObject(object, profile)) {
        ++receipt.nonProjectableObjectCount;
      }
      continue;
    }

    ++receipt.visibleObjectCount;
    if (isNonProjectableObject(object, profile)) {
      ++receipt.nonProjectableObjectCount;
      continue;
    }
    ++receipt.projectableObjectCount;

    const CreativeSpatialProjectionReceipt projectionReceipt =
        projectObjectToGrid(object, request.projectionRequest);
    if (projectionReceipt.status !=
            CreativeSpatialProjectionStatus::Projected ||
        projectionReceipt.cells.empty()) {
      if (projectionReceipt.status ==
              CreativeSpatialProjectionStatus::InvalidObject ||
          projectionReceipt.status ==
              CreativeSpatialProjectionStatus::NoProjection) {
        ++receipt.nonProjectableObjectCount;
      }
      continue;
    }

    ++receipt.projectedObjectCount;
    receipt.projectionCellCount += projectionReceipt.cells.size();
    const CreativeDocumentWireframeItemKind itemKind =
        wireframeItemKindForProjection(projectionReceipt.profile);
    if (itemKind == CreativeDocumentWireframeItemKind::Unknown) {
      ++receipt.nonProjectableObjectCount;
      continue;
    }
    result.drawList.items.push_back(makeWireframeItem(object,
                                                      projectionReceipt));
  }

  receipt.itemCount = result.drawList.items.size();
  if (!result.drawList.items.empty()) {
    setReceiptStatus(receipt,
                     CreativeDocumentWireframeStatus::Built,
                     "creative_document_wireframe_built");
  } else if (receipt.visibleObjectCount == 0U) {
    setReceiptStatus(receipt,
                     CreativeDocumentWireframeStatus::NoVisibleItems,
                     "creative_document_wireframe_no_visible_items");
  } else {
    setReceiptStatus(receipt,
                     CreativeDocumentWireframeStatus::NoRenderableItems,
                     "creative_document_wireframe_no_render_items");
  }

  return result;
}

CreativeDocumentWireframeBuildResult buildCreativeObjectWireframeList(
    std::span<const CreativeObject> objects,
    const CreativeSpatialProjectionRequest& projectionRequest) {
  return buildCreativeDocumentWireframeList(
      CreativeDocumentWireframeBuildRequest{objects.data(),
                                            objects.size(),
                                            projectionRequest,
                                            false});
}

CreativeDocumentWireframeBuildResult buildCreativeDocumentWireframeList(
    const CreativeDocument& document,
    const CreativeSpatialProjectionRequest& projectionRequest) {
  const std::span<const CreativeObject> objects = document.objects();
  return buildCreativeDocumentWireframeList(
      CreativeDocumentWireframeBuildRequest{objects.data(),
                                            objects.size(),
                                            projectionRequest,
                                            true});
}

CreativeDocumentWireframeSegmentBuildResult buildCreativeDocumentWireframeSegments(
    const CreativeDocumentWireframeSegmentBuildRequest& request) {
  CreativeDocumentWireframeSegmentBuildResult result;
  CreativeDocumentWireframeSegmentReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.sourceAvailable = request.sourceAvailable &&
                            (request.items != nullptr ||
                             request.itemCount == 0U);
  receipt.itemCount = request.itemCount;

  if (!receipt.sourceAvailable) {
    setSegmentReceiptStatus(
        receipt,
        CreativeDocumentWireframeSegmentStatus::MissingSource,
        "creative_document_wireframe_segments_source_missing");
    return result;
  }

  if (request.itemCount == 0U) {
    setSegmentReceiptStatus(
        receipt,
        CreativeDocumentWireframeSegmentStatus::EmptySource,
        "creative_document_wireframe_segments_source_empty");
    return result;
  }

  result.segmentList.segments.reserve(request.itemCount * 12U);
  const std::span<const CreativeDocumentWireframeItem> items(request.items,
                                                            request.itemCount);
  for (const CreativeDocumentWireframeItem& item : items) {
    switch (item.itemKind) {
      case CreativeDocumentWireframeItemKind::Box:
        ++receipt.boxItemCount;
        if (isDegenerateBounds(item.bounds)) {
          ++receipt.skippedDegenerateCount;
          break;
        }
        appendBoxSegments(result.segmentList, item);
        break;
      case CreativeDocumentWireframeItemKind::Line:
        ++receipt.lineItemCount;
        if (item.pathPoints.size() >= 2U) {
          for (std::size_t index = 0; index < item.pathPoints.size() - 1U;
               ++index) {
            const CreativeVec3 start = item.pathPoints[index];
            const CreativeVec3 end = item.pathPoints[index + 1U];
            if (samePoint(start, end)) {
              ++receipt.skippedDegenerateCount;
              continue;
            }
            appendSegment(result.segmentList,
                          item,
                          CreativeDocumentWireframeSegmentKind::Line,
                          start,
                          end);
          }
          break;
        }
        if (samePoint(item.start, item.end)) {
          ++receipt.skippedDegenerateCount;
          break;
        }
        appendSegment(result.segmentList,
                      item,
                      CreativeDocumentWireframeSegmentKind::Line,
                      item.start,
                      item.end);
        break;
      case CreativeDocumentWireframeItemKind::Point:
        ++receipt.pointItemCount;
        break;
      case CreativeDocumentWireframeItemKind::Unknown:
        break;
    }
  }

  receipt.segmentCount = result.segmentList.segments.size();
  if (receipt.segmentCount > 0U) {
    setSegmentReceiptStatus(receipt,
                            CreativeDocumentWireframeSegmentStatus::Built,
                            "creative_document_wireframe_segments_built");
  } else {
    setSegmentReceiptStatus(receipt,
                            CreativeDocumentWireframeSegmentStatus::NoSegments,
                            "creative_document_wireframe_segments_none");
  }

  return result;
}

CreativeDocumentWireframeSegmentBuildResult buildCreativeDocumentWireframeSegments(
    const CreativeDocumentWireframeDrawList& drawList) {
  return buildCreativeDocumentWireframeSegments(
      CreativeDocumentWireframeSegmentBuildRequest{drawList.items.data(),
                                                   drawList.items.size(),
                                                   true});
}

CreativeDocumentWireframeSegmentBuildResult buildCreativeDocumentWireframeSegments(
    const CreativeDocument& document,
    const CreativeSpatialProjectionRequest& projectionRequest) {
  const CreativeDocumentWireframeBuildResult wireframe =
      buildCreativeDocumentWireframeList(document, projectionRequest);
  return buildCreativeDocumentWireframeSegments(wireframe.drawList);
}

}  // namespace iggy3d::creative
