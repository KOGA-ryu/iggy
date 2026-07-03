#include "app/iggy3d/creative/DocumentWireframe.hpp"

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
      item.bounds = object.bounds;
      item.start = object.bounds.min;
      item.end = object.bounds.max;
      break;
    case CreativeDocumentWireframeItemKind::Unknown:
      break;
  }

  return item;
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

}  // namespace iggy3d::creative
