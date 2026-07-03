#include "app/iggy3d/creative/DocumentMutation.hpp"
#include "app/iggy3d/creative/DocumentWireframe.hpp"

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

cr::CreativeSpatialProjectionRequest makeProjectionRequest() {
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

}  // namespace

int main() {
  const bool ok = emptyDocumentReturnsStableEmptyReceipt() &&
                  genericRoomCreateProducesDescriptorBoxItem() &&
                  hiddenRoomProducesNoItemsButKeepsObjectCount() &&
                  visibilityToggleRemovesAndRestoresItem() &&
                  multipleVisibleObjectsPreserveDocumentOrder() &&
                  objectSpanUnknownObjectIsCountedButNotRendered() &&
                  invalidProjectionSettingsFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
