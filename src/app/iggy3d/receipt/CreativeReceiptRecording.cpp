#include "app/iggy3d/ReceiptBuilder.hpp"

#include <charconv>
#include <string>
#include <string_view>
#include <utility>

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/gameplay/MovementProof.hpp"
#include "app/iggy3d/debug/MovementDebugHud.hpp"
#include "app/iggy3d/debug/DebugHudState.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/creative/CreativeUiCommandDiagnostics.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"
#include "app/iggy3d/creative/bridge/ViewportPickFrame.hpp"
#include "app/iggy3d/creative/bridge/WireframeFrame.hpp"
#include "app/iggy3d/ProductCreativeBakedRoomRefresh.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "app/iggy3d/receipt/ReceiptFields.hpp"

namespace iggy3d {
namespace {

ProductCreativeBakedRoomRefreshDiagnostics&
uiCommandBakedRoomRefreshFields(ProductAppWindowState& window) noexcept {
  return window.creativeAuthoring.creativeUiCommand.bakedRoomRefresh;
}

ProductCreativeBakedRoomRefreshDiagnostics&
autoBakedRoomRefreshFields(ProductAppWindowState& window) noexcept {
  return window.creativeAuthoring.creativeBakedRoomAutoRefresh;
}

void resetProductCreativeBakedRoomRefreshDiagnostics(
    ProductCreativeBakedRoomRefreshDiagnostics& fields) {
  fields = ProductCreativeBakedRoomRefreshDiagnostics{};
}

void copyProductCreativeBakedRoomRefreshDiagnostics(
    ProductCreativeBakedRoomRefreshDiagnostics& fields,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh) {
  fields.requested = true;
  fields.accepted = refresh.accepted;
  fields.clearedActiveRoom = refresh.clearedActiveRoom;
  fields.status = refresh.status;
  fields.reasonCode = refresh.reasonCode;
  fields.bakeMeasured = refresh.bakeMeasured;
  fields.bakeElapsedMicroseconds = refresh.bakeElapsedMicroseconds;
  fields.bakedDocumentRevision = refresh.bakedDocumentRevision;
  fields.staticMeshCount = refresh.staticMeshCount;
  fields.anchorCount = refresh.anchorCount;
  fields.spatialSurfaceCount = refresh.spatialSurfaceCount;
  fields.collisionReady = refresh.collisionReady;
  fields.collisionQuerySurfaceCount = refresh.collisionQuerySurfaceCount;
}

void resetProductCreativeUiCommandBakedRoomRefresh(
    ProductAppWindowState& window) {
  resetProductCreativeBakedRoomRefreshDiagnostics(
      uiCommandBakedRoomRefreshFields(window));
}

void resetProductCreativeBakedRoomAutoRefresh(ProductAppWindowState& window) {
  resetProductCreativeBakedRoomRefreshDiagnostics(
      autoBakedRoomRefreshFields(window));
}

std::string_view productUiThemeReceiptName(ProductUiThemeId theme) noexcept {
  switch (theme) {
    case ProductUiThemeId::System:
      return "system";
    case ProductUiThemeId::Journal:
      return "journal";
  }
  return "unknown";
}

std::string_view productUiHitSurfaceReceiptName(
    ProductUiHitSurface surface) noexcept {
  switch (surface) {
    case ProductUiHitSurface::None:
      return "none";
    case ProductUiHitSurface::StarterMenu:
      return "starter_menu";
    case ProductUiHitSurface::PauseMenu:
      return "pause_menu";
    case ProductUiHitSurface::CreativeOverlay:
      return "creative_overlay";
    case ProductUiHitSurface::Notebook:
      return "notebook";
  }
  return "unknown";
}

std::string_view uiHitKindReceiptName(UiHitKind kind) noexcept {
  switch (kind) {
    case UiHitKind::None:
      return "none";
    case UiHitKind::Button:
      return "button";
    case UiHitKind::Row:
      return "row";
    case UiHitKind::Slider:
      return "slider";
    case UiHitKind::Toggle:
      return "toggle";
    case UiHitKind::Viewport:
      return "viewport";
  }
  return "unknown";
}

}  // namespace

void recordProductCreativeUiProjection(
    ProductAppWindowState& window,
    const ProductCreativeUiProjectionReceipt& receipt) {
  CreativeAuthoringStore& authoring = window.creativeAuthoring;
  authoring.creativeUiProjection.requested = receipt.requested;
  authoring.creativeUiProjection.ready = receipt.ready;
  authoring.creativeUiProjection.partial = receipt.partial;
  authoring.creativeUiProjection.status = std::string(receipt.status);
  authoring.creativeUiProjection.reasonCode = std::string(receipt.reasonCode);
  authoring.creativeUiProjection.usedModel = receipt.usedModel;
  authoring.creativeUiProjection.usedFacade = receipt.usedFacade;
  authoring.creativeUiProjection.virtualWidth = receipt.virtualWidth;
  authoring.creativeUiProjection.virtualHeight = receipt.virtualHeight;
  authoring.creativeUiProjection.theme =
      std::string(productUiThemeReceiptName(receipt.theme));
  authoring.creativeUiProjection.panelCount = receipt.panelCount;
  authoring.creativeUiProjection.modelRowCount = receipt.modelRowCount;
  authoring.creativeUiProjection.primitiveCount = receipt.primitiveCount;
  authoring.creativeUiProjection.textCount = receipt.textCount;
  authoring.creativeUiProjection.rectCount = receipt.rectCount;
  authoring.creativeUiProjection.rowCount = receipt.rowCount;
  authoring.creativeUiProjection.disabledRowCount = receipt.disabledRowCount;
  authoring.creativeUiProjection.hitRegionCount = receipt.hitRegionCount;
}

void recordProductCreativeUiInputFrame(
    ProductAppWindowState& window,
    const ProductCreativeUiInputFrameReceipt& receipt) {
  CreativeAuthoringStore& authoring = window.creativeAuthoring;
  authoring.creativeUiInput.requested = receipt.requested;
  authoring.creativeUiInput.clickPresent = receipt.clickPresent;
  authoring.creativeUiInput.drawListAvailable = receipt.drawListAvailable;
  authoring.creativeUiInput.routed = receipt.routed;
  authoring.creativeUiInput.hit = receipt.hit;
  authoring.creativeUiInput.consumed = receipt.consumed;
  authoring.creativeUiInput.enabled = receipt.enabled;
  authoring.creativeUiInput.surface =
      std::string(productUiHitSurfaceReceiptName(receipt.surface));
  authoring.creativeUiInput.kind = std::string(uiHitKindReceiptName(receipt.kind));
  authoring.creativeUiInput.action = std::string(frontendActionName(receipt.action));
  authoring.creativeUiInput.layerIndex =
      static_cast<std::uint64_t>(receipt.layerIndex);
  authoring.creativeUiInput.regionIndex =
      static_cast<std::uint64_t>(receipt.regionIndex);
  authoring.creativeUiInput.semanticId =
      receipt.semanticId.empty() ? "none" : receipt.semanticId;
  authoring.creativeUiInput.status = receipt.status;
  authoring.creativeUiInput.reasonCode = receipt.reasonCode;

  if (receipt.clickPresent) {
    authoring.creativeUiLast.clickSeen = true;
    authoring.creativeUiLast.clickX = floatReceiptValue(receipt.clickX);
    authoring.creativeUiLast.clickY = floatReceiptValue(receipt.clickY);
    authoring.creativeUiLast.inputHit = receipt.hit;
    authoring.creativeUiLast.inputConsumed = receipt.consumed;
    authoring.creativeUiLast.inputStatus = receipt.status;
    authoring.creativeUiLast.inputSemanticId =
        receipt.semanticId.empty() ? "none" : receipt.semanticId;
  }
}

void recordProductCreativeUiDownstreamClick(
    ProductAppWindowState& window,
    const ProductCreativeUiDownstreamClickReceipt& receipt) {
  CreativeAuthoringStore& authoring = window.creativeAuthoring;
  authoring.creativeUiInput.downstreamClickRequested = receipt.requested;
  authoring.creativeUiInput.downstreamClickPresent = receipt.clickPresent;
  authoring.creativeUiInput.downstreamClickHigherPriority =
      receipt.higherPriorityUiConsumed;
  authoring.creativeUiInput.downstreamClickSuppressed = receipt.suppressed;
  authoring.creativeUiInput.downstreamClickStatus = receipt.status;
  authoring.creativeUiInput.downstreamClickReasonCode = receipt.reasonCode;
}

void recordProductCreativeUiCommandFrame(
    ProductAppWindowState& window,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  CreativeAuthoringStore& authoring = window.creativeAuthoring;
  authoring.creativeUiCommand =
      toProductCreativeUiCommandDiagnostics(receipt);
  resetProductCreativeUiCommandBakedRoomRefresh(window);

  const bool commandTouchedCreativeState =
      receipt.inputClickPresent ||
      receipt.commandKind != ProductCreativeUiCommandKind::None ||
      receipt.accepted || receipt.changed || receipt.create.document.requested ||
      receipt.mutation.requested ||
      (receipt.inputConsumed && !receipt.semanticId.empty());
  if (commandTouchedCreativeState) {
    authoring.creativeUiLast.commandKind =
        std::string(productCreativeUiCommandKindReceiptName(receipt.commandKind));
    authoring.creativeUiLast.commandStatus = receipt.status;
    authoring.creativeUiLast.commandCreateRequested =
        receipt.create.document.requested;
    authoring.creativeUiLast.commandCreateAccepted =
        receipt.create.document.accepted;
    authoring.creativeUiLast.commandCreateChanged =
        receipt.create.document.changed;
    authoring.creativeUiLast.commandCreateObjectId =
        receipt.create.document.objectId;
  }
}

void recordProductCreativeUiBakedRoomRefresh(
    ProductAppWindowState& window,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh) {
  copyProductCreativeBakedRoomRefreshDiagnostics(
      uiCommandBakedRoomRefreshFields(window), refresh);
}

void recordProductCreativeBakedRoomAutoRefresh(
    ProductAppWindowState& window,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh) {
  copyProductCreativeBakedRoomRefreshDiagnostics(
      autoBakedRoomRefreshFields(window), refresh);
}

void recordProductCreativeDocumentRevisionFrame(
    ProductAppWindowState& window,
    bool observed,
    std::uint64_t documentIdBefore,
    std::uint64_t revisionBefore,
    std::uint64_t documentIdAfter,
    std::uint64_t revisionAfter) {
  CreativeAuthoringStore& authoring = window.creativeAuthoring;
  resetProductCreativeBakedRoomAutoRefresh(window);
  authoring.creativeDocumentRevision.observed = observed;
  authoring.creativeDocumentChangedThisFrame = false;
  authoring.creativeDocumentRevision.documentId = observed ? documentIdAfter : 0U;
  authoring.creativeDocumentRevision.beforeFrame = observed ? revisionBefore : 0U;
  authoring.creativeDocumentRevision.afterFrame = observed ? revisionAfter : 0U;
  if (!observed) {
    return;
  }

  const bool documentReplaced = documentIdBefore != documentIdAfter;
  const bool revisionChanged = revisionBefore != revisionAfter;
  if (!documentReplaced && !revisionChanged) {
    return;
  }

  authoring.creativeDocumentChangedThisFrame = true;
  authoring.creativeBakedRoomStale = true;
  authoring.creativeBakedRoomStaleDocumentId = documentIdAfter;
  authoring.creativeBakedRoomStaleRevision = revisionAfter;
  authoring.creativeBakedRoomStaleStatus =
      documentReplaced ? "creative_baked_room_stale_document_replaced"
                       : "creative_baked_room_stale_document_changed";
  authoring.creativeBakedRoomStaleReasonCode =
      authoring.creativeBakedRoomStaleStatus;
}

void recordProductCreativeBakedRoomFresh(ProductAppWindowState& window,
                                         std::uint64_t documentId,
                                         std::uint64_t revision) {
  CreativeAuthoringStore& authoring = window.creativeAuthoring;
  authoring.creativeBakedRoomStale = false;
  authoring.creativeBakedRoomStaleDocumentId = documentId;
  authoring.creativeBakedRoomStaleRevision = revision;
  authoring.creativeBakedRoomStaleStatus = "creative_baked_room_fresh";
  authoring.creativeBakedRoomStaleReasonCode = "creative_baked_room_fresh";
}

void recordProductCreativeViewportPickFrame(
    ProductAppWindowState& window,
    const ProductCreativeViewportPickFrameReceipt& receipt) {
  CreativeAuthoringStore& authoring = window.creativeAuthoring;
  authoring.creativeViewportPickRequested = receipt.requested;
  authoring.creativeViewportPickActive = receipt.active;
  authoring.creativeViewportPickClickPresent = receipt.clickPresent;
  authoring.creativeViewportPickClickSuppressed =
      receipt.downstreamClickSuppressed;
  authoring.creativeViewportPickFacadeAvailable = receipt.facadeAvailable;
  authoring.creativeViewportPickSourceAvailable = receipt.sourceAvailable;
  authoring.creativeViewportPickProjected = receipt.projected;
  authoring.creativeViewportPickPicked = receipt.picked;
  authoring.creativeViewportPickObjectCount = receipt.objectCount;
  authoring.creativeViewportPickProjectionCellCount =
      receipt.projectionCellCount;
  authoring.creativeViewportPickStatus = receipt.status;
  authoring.creativeViewportPickReasonCode = receipt.reasonCode;
  authoring.creativeViewportPickPickStatus =
      std::string(creative::toString(receipt.pickStatus));
  authoring.creativeViewportPickMessage =
      receipt.pickMessage.empty() ? "none" : receipt.pickMessage;
  authoring.creativeViewportPickCoordX = receipt.coord.x;
  authoring.creativeViewportPickCoordY = receipt.coord.y;
  authoring.creativeViewportPickCoordZ = receipt.coord.z;
  authoring.creativeViewportPickGridIndex = receipt.gridIndex;
  authoring.creativeViewportPickObjectId = receipt.objectId;
  authoring.creativeViewportPickObjectKind =
      std::string(creative::toString(receipt.objectKind));
  authoring.creativeViewportPickOccupancyKind =
      std::string(creative::toString(receipt.occupancyKind));
  authoring.creativeViewportPickTarget = receipt.target.value;
  authoring.creativeViewportPickCellIndex =
      static_cast<std::uint64_t>(receipt.cellIndex);
}

void recordProductCreativeWireframeFrame(
    ProductAppWindowState& window,
    const ProductCreativeWireframeFrameReceipt& receipt) {
  CreativeAuthoringStore& authoring = window.creativeAuthoring;
  authoring.creativeWireframeRequested = receipt.requested;
  authoring.creativeWireframeActive = receipt.active;
  authoring.creativeWireframeFacadeAvailable = receipt.facadeAvailable;
  authoring.creativeWireframeDocumentAvailable = receipt.documentAvailable;
  authoring.creativeWireframeSourceAvailable = receipt.sourceAvailable;
  authoring.creativeWireframeObjectCount = receipt.objectCount;
  authoring.creativeWireframeVisibleObjectCount = receipt.visibleObjectCount;
  authoring.creativeWireframeItemCount = receipt.itemCount;
  authoring.creativeWireframeSegmentCount = receipt.segmentCount;
  authoring.creativeWireframeBoxItemCount = receipt.boxItemCount;
  authoring.creativeWireframeLineItemCount = receipt.lineItemCount;
  authoring.creativeWireframePointItemCount = receipt.pointItemCount;
  authoring.creativeWireframeSkippedDegenerateCount =
      receipt.skippedDegenerateCount;
  authoring.creativeWireframeStatus = receipt.status;
  authoring.creativeWireframeReasonCode = receipt.reasonCode;
  authoring.creativeWireframeWireframeStatus =
      std::string(creative::toString(receipt.wireframeStatus));
  authoring.creativeWireframeWireframeReasonCode =
      receipt.wireframeReasonCode.empty() ? "none"
                                          : receipt.wireframeReasonCode;
  authoring.creativeWireframeSegmentStatus =
      std::string(creative::toString(receipt.segmentStatus));
  authoring.creativeWireframeSegmentReasonCode =
      receipt.segmentReasonCode.empty() ? "none" : receipt.segmentReasonCode;
  authoring.creativeWireframeDebugLineRequested = receipt.debugLineRequested;
  authoring.creativeWireframeDebugLineSourceAvailable =
      receipt.debugLineSourceAvailable;
  authoring.creativeWireframeDebugLineInputSegmentCount =
      receipt.debugLineInputSegmentCount;
  authoring.creativeWireframeDebugLineCount = receipt.debugLineCount;
  authoring.creativeWireframeDebugLineSkippedDegenerateCount =
      receipt.debugLineSkippedDegenerateCount;
  authoring.creativeWireframeDebugLineStatus =
      std::string(toString(receipt.debugLineStatus));
  authoring.creativeWireframeDebugLineReasonCode =
      receipt.debugLineReasonCode.empty() ? "none" : receipt.debugLineReasonCode;
}

}  // namespace iggy3d
