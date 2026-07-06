#include "app/iggy3d/ReceiptBuilder.hpp"

#include <charconv>
#include <string>
#include <string_view>
#include <utility>

#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/gameplay/MovementProof.hpp"
#include "app/iggy3d/debug/MovementDebugHud.hpp"
#include "app/iggy3d/debug/NpcBehaviorDebugHud.hpp"
#include "app/iggy3d/debug/PhysicsDebugHud.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"
#include "app/iggy3d/creative/bridge/ViewportPickFrame.hpp"
#include "app/iggy3d/creative/bridge/WireframeFrame.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "app/iggy3d/receipt/ReceiptFields.hpp"

namespace iggy3d {
namespace {

void setPhysicsMovementPlannerProof(ProductAppWindowState& window,
                                    std::string status,
                                    bool requested,
                                    bool used) {
  window.physicsMovementPlanner.requested = requested;
  window.physicsMovementPlanner.used = used;
  window.physicsMovementPlanner.status = std::move(status);
  window.physicsMovementPlanner.reasonCode = window.physicsMovementPlanner.status;
}

std::string_view creativeToolReceiptName(creative::Tool tool) noexcept;

ProductCreativeBakedRoomRefreshDiagnostics&
uiCommandBakedRoomRefreshFields(ProductAppWindowState& window) noexcept {
  return window.creativeUiCommand.bakedRoomRefresh;
}

ProductCreativeBakedRoomRefreshDiagnostics&
autoBakedRoomRefreshFields(ProductAppWindowState& window) noexcept {
  return window.creativeBakedRoomAutoRefresh;
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

void copyProductCreativeUiCommandMutationDiagnostics(
    ProductCreativeUiCommandMutationDiagnostics& fields,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  fields.requested = receipt.mutationRequested;
  fields.accepted = receipt.mutationAccepted;
  fields.changed = receipt.mutationChanged;
  fields.status = std::string(creative::toString(receipt.mutationStatus));
  fields.documentStatus =
      std::string(creative::toString(receipt.documentMutationStatus));
  fields.kind = std::string(creative::toString(receipt.mutationKind));
  fields.target = receipt.mutationTarget.value;
  fields.objectId = receipt.mutationObjectId;
  fields.objectKind = std::string(creative::toString(receipt.mutationObjectKind));
  fields.visibleBefore = receipt.visibleBefore;
  fields.visibleAfter = receipt.visibleAfter;
  fields.lockedBefore = receipt.lockedBefore;
  fields.lockedAfter = receipt.lockedAfter;
  fields.revisionBefore = receipt.revisionBefore;
  fields.revisionAfter = receipt.revisionAfter;
  fields.message =
      receipt.mutationMessage.empty() ? "none" : receipt.mutationMessage;
}

void copyProductCreativeUiCommandCreateDiagnostics(
    ProductCreativeUiCommandCreateDiagnostics& fields,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  fields.requested = receipt.createRequested;
  fields.accepted = receipt.createAccepted;
  fields.changed = receipt.createChanged;
  fields.status = std::string(creative::toString(receipt.createStatus));
  fields.objectId = receipt.createObjectId;
  fields.objectKind = std::string(creative::toString(receipt.createObjectKind));
  fields.objectName =
      receipt.createObjectName.empty() ? "none" : receipt.createObjectName;
  fields.revisionBefore = receipt.createRevisionBefore;
  fields.revisionAfter = receipt.createRevisionAfter;
  fields.dirtyFlags = receipt.createDirtyFlags;
  fields.message = receipt.createMessage.empty() ? "none" : receipt.createMessage;
  fields.reasonCode =
      receipt.createReasonCode.empty() ? "none" : receipt.createReasonCode;
}

void copyProductCreativeUiCommandDeleteDiagnostics(
    ProductCreativeUiCommandDeleteDiagnostics& fields,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  fields.requested = receipt.deleteRequested;
  fields.accepted = receipt.deleteAccepted;
  fields.changed = receipt.deleteChanged;
  fields.removed = receipt.deleteRemoved;
  fields.objectId = receipt.deleteObjectId;
  fields.objectKind = std::string(creative::toString(receipt.deleteObjectKind));
  fields.objectName =
      receipt.deleteObjectName.empty() ? "none" : receipt.deleteObjectName;
  fields.revisionBefore = receipt.deleteRevisionBefore;
  fields.revisionAfter = receipt.deleteRevisionAfter;
  fields.dirtyFlags = receipt.deleteDirtyFlags;
  fields.status = receipt.deleteStatus.empty() ? "Unknown" : receipt.deleteStatus;
  fields.message = receipt.deleteMessage.empty() ? "none" : receipt.deleteMessage;
  fields.reasonCode =
      receipt.deleteReasonCode.empty() ? "none" : receipt.deleteReasonCode;
}

void copyProductCreativeUiCommandUndoDiagnostics(
    ProductCreativeUiCommandUndoDiagnostics& fields,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  fields.requested = receipt.undoRequested;
  fields.accepted = receipt.undoAccepted;
  fields.changed = receipt.undoChanged;
  fields.hadSnapshot = receipt.undoHadSnapshot;
  fields.documentId = receipt.undoDocumentId;
  fields.revisionBefore = receipt.undoRevisionBefore;
  fields.revisionAfter = receipt.undoRevisionAfter;
  fields.objectCountBefore = receipt.undoObjectCountBefore;
  fields.objectCountAfter = receipt.undoObjectCountAfter;
  fields.depthBefore = receipt.undoDepthBefore;
  fields.depthAfter = receipt.undoDepthAfter;
  fields.status =
      receipt.undoStatus.empty() ? "creative_undo_not_requested"
                                 : receipt.undoStatus;
  fields.message = receipt.undoMessage.empty() ? "none" : receipt.undoMessage;
  fields.reasonCode =
      receipt.undoReasonCode.empty() ? "none" : receipt.undoReasonCode;
}

void copyProductCreativeUiCommandRoomShellDiagnostics(
    ProductCreativeUiCommandRoomShellDiagnostics& fields,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  fields.requested = receipt.shellRequested;
  fields.accepted = receipt.shellAccepted;
  fields.changed = receipt.shellChanged;
  fields.roomObjectId = receipt.shellRoomObjectId;
  fields.generatedObjectCount = receipt.shellGeneratedObjectCount;
  fields.removedObjectCount = receipt.shellRemovedObjectCount;
  fields.floorCount = receipt.shellFloorCount;
  fields.wallCount = receipt.shellWallCount;
  fields.revisionBefore = receipt.shellRevisionBefore;
  fields.revisionAfter = receipt.shellRevisionAfter;
  fields.status =
      receipt.shellStatus.empty() ? "creative_room_shell_not_requested"
                                  : receipt.shellStatus;
  fields.reasonCode =
      receipt.shellReasonCode.empty() ? "none" : receipt.shellReasonCode;
  fields.message = receipt.shellMessage.empty() ? "none" : receipt.shellMessage;
}

void copyProductCreativeUiCommandDiagnostics(
    ProductCreativeUiCommandDiagnostics& fields,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  fields.requested = receipt.requested;
  fields.facadeAvailable = receipt.facadeAvailable;
  fields.inputConsumed = receipt.inputConsumed;
  fields.inputEnabled = receipt.inputEnabled;
  fields.accepted = receipt.accepted;
  fields.changed = receipt.changed;
  fields.kind =
      std::string(productCreativeUiCommandKindReceiptName(receipt.commandKind));
  fields.tool =
      receipt.commandKind == ProductCreativeUiCommandKind::SetActiveTool
          ? std::string(creativeToolReceiptName(receipt.commandTool))
          : std::string("none");
  fields.objectKind = std::string(creative::toString(receipt.commandObjectKind));
  fields.toolBefore = std::string(creativeToolReceiptName(receipt.toolBefore));
  fields.toolAfter = std::string(creativeToolReceiptName(receipt.toolAfter));
  fields.semanticId = receipt.semanticId.empty() ? "none" : receipt.semanticId;
  fields.status = receipt.status;
  fields.reasonCode = receipt.reasonCode;
  copyProductCreativeUiCommandMutationDiagnostics(fields.mutation, receipt);
  copyProductCreativeUiCommandCreateDiagnostics(fields.create, receipt);
  copyProductCreativeUiCommandDeleteDiagnostics(fields.deleteObject, receipt);
  copyProductCreativeUiCommandUndoDiagnostics(fields.undo, receipt);
  copyProductCreativeUiCommandRoomShellDiagnostics(fields.shell, receipt);
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

std::string_view creativeToolReceiptName(creative::Tool tool) noexcept {
  switch (tool) {
    case creative::Tool::Select:
      return "Select";
    case creative::Tool::Move:
      return "Move";
    case creative::Tool::Measure:
      return "Measure";
    case creative::Tool::Navigate:
      return "Navigate";
  }
  return "Unknown";
}

}  // namespace

void recordProductPhysicsMovementPlannerTickProof(
    ProductAppWindowState& window,
    bool requested,
    bool collisionSurfacesAvailable,
    bool movementPhysicsStatsAvailable) {
  // branch-gate: BG-1114
  if (!requested) {
    setPhysicsMovementPlannerProof(
        window, "physics_movement_planner_disabled", false, false);
    return;
  }
  // branch-gate: BG-1114
  if (!collisionSurfacesAvailable) {
    setPhysicsMovementPlannerProof(
        window, "physics_movement_planner_no_collision_surfaces", true, false);
    return;
  }
  // branch-gate: BG-1114
  if (movementPhysicsStatsAvailable) {
    setPhysicsMovementPlannerProof(
        window, "physics_movement_planner_used", true, true);
    return;
  }
  setPhysicsMovementPlannerProof(
      window, "physics_movement_planner_not_used", true, false);
}

void recordProductCreativeUiProjection(
    ProductAppWindowState& window,
    const ProductCreativeUiProjectionReceipt& receipt) {
  window.creativeUiProjection.requested = receipt.requested;
  window.creativeUiProjection.ready = receipt.ready;
  window.creativeUiProjection.partial = receipt.partial;
  window.creativeUiProjection.status = std::string(receipt.status);
  window.creativeUiProjection.reasonCode = std::string(receipt.reasonCode);
  window.creativeUiProjection.usedModel = receipt.usedModel;
  window.creativeUiProjection.usedFacade = receipt.usedFacade;
  window.creativeUiProjection.virtualWidth = receipt.virtualWidth;
  window.creativeUiProjection.virtualHeight = receipt.virtualHeight;
  window.creativeUiProjection.theme =
      std::string(productUiThemeReceiptName(receipt.theme));
  window.creativeUiProjection.panelCount = receipt.panelCount;
  window.creativeUiProjection.modelRowCount = receipt.modelRowCount;
  window.creativeUiProjection.primitiveCount = receipt.primitiveCount;
  window.creativeUiProjection.textCount = receipt.textCount;
  window.creativeUiProjection.rectCount = receipt.rectCount;
  window.creativeUiProjection.rowCount = receipt.rowCount;
  window.creativeUiProjection.disabledRowCount = receipt.disabledRowCount;
  window.creativeUiProjection.hitRegionCount = receipt.hitRegionCount;
}

void recordProductCreativeUiInputFrame(
    ProductAppWindowState& window,
    const ProductCreativeUiInputFrameReceipt& receipt) {
  window.creativeUiInput.requested = receipt.requested;
  window.creativeUiInput.clickPresent = receipt.clickPresent;
  window.creativeUiInput.drawListAvailable = receipt.drawListAvailable;
  window.creativeUiInput.routed = receipt.routed;
  window.creativeUiInput.hit = receipt.hit;
  window.creativeUiInput.consumed = receipt.consumed;
  window.creativeUiInput.enabled = receipt.enabled;
  window.creativeUiInput.surface =
      std::string(productUiHitSurfaceReceiptName(receipt.surface));
  window.creativeUiInput.kind = std::string(uiHitKindReceiptName(receipt.kind));
  window.creativeUiInput.action = std::string(frontendActionName(receipt.action));
  window.creativeUiInput.layerIndex =
      static_cast<std::uint64_t>(receipt.layerIndex);
  window.creativeUiInput.regionIndex =
      static_cast<std::uint64_t>(receipt.regionIndex);
  window.creativeUiInput.semanticId =
      receipt.semanticId.empty() ? "none" : receipt.semanticId;
  window.creativeUiInput.status = receipt.status;
  window.creativeUiInput.reasonCode = receipt.reasonCode;

  if (receipt.clickPresent) {
    window.creativeUiLast.clickSeen = true;
    window.creativeUiLast.clickX = floatReceiptValue(receipt.clickX);
    window.creativeUiLast.clickY = floatReceiptValue(receipt.clickY);
    window.creativeUiLast.inputHit = receipt.hit;
    window.creativeUiLast.inputConsumed = receipt.consumed;
    window.creativeUiLast.inputStatus = receipt.status;
    window.creativeUiLast.inputSemanticId =
        receipt.semanticId.empty() ? "none" : receipt.semanticId;
  }
}

void recordProductCreativeUiDownstreamClick(
    ProductAppWindowState& window,
    const ProductCreativeUiDownstreamClickReceipt& receipt) {
  window.creativeUiInput.downstreamClickRequested = receipt.requested;
  window.creativeUiInput.downstreamClickPresent = receipt.clickPresent;
  window.creativeUiInput.downstreamClickHigherPriority =
      receipt.higherPriorityUiConsumed;
  window.creativeUiInput.downstreamClickSuppressed = receipt.suppressed;
  window.creativeUiInput.downstreamClickStatus = receipt.status;
  window.creativeUiInput.downstreamClickReasonCode = receipt.reasonCode;
}

void recordProductCreativeUiCommandFrame(
    ProductAppWindowState& window,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  copyProductCreativeUiCommandDiagnostics(window.creativeUiCommand, receipt);
  resetProductCreativeUiCommandBakedRoomRefresh(window);

  const bool commandTouchedCreativeState =
      receipt.inputClickPresent ||
      receipt.commandKind != ProductCreativeUiCommandKind::None ||
      receipt.accepted || receipt.changed || receipt.createRequested ||
      receipt.mutationRequested ||
      (receipt.inputConsumed && !receipt.semanticId.empty());
  if (commandTouchedCreativeState) {
    window.creativeUiLast.commandKind =
        std::string(productCreativeUiCommandKindReceiptName(receipt.commandKind));
    window.creativeUiLast.commandStatus = receipt.status;
    window.creativeUiLast.commandCreateRequested = receipt.createRequested;
    window.creativeUiLast.commandCreateAccepted = receipt.createAccepted;
    window.creativeUiLast.commandCreateChanged = receipt.createChanged;
    window.creativeUiLast.commandCreateObjectId = receipt.createObjectId;
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
  resetProductCreativeBakedRoomAutoRefresh(window);
  window.creativeDocumentRevision.observed = observed;
  window.creativeDocumentChangedThisFrame = false;
  window.creativeDocumentRevision.documentId = observed ? documentIdAfter : 0U;
  window.creativeDocumentRevision.beforeFrame = observed ? revisionBefore : 0U;
  window.creativeDocumentRevision.afterFrame = observed ? revisionAfter : 0U;
  if (!observed) {
    return;
  }

  const bool documentReplaced = documentIdBefore != documentIdAfter;
  const bool revisionChanged = revisionBefore != revisionAfter;
  if (!documentReplaced && !revisionChanged) {
    return;
  }

  window.creativeDocumentChangedThisFrame = true;
  window.creativeBakedRoomStale = true;
  window.creativeBakedRoomStaleDocumentId = documentIdAfter;
  window.creativeBakedRoomStaleRevision = revisionAfter;
  window.creativeBakedRoomStaleStatus =
      documentReplaced ? "creative_baked_room_stale_document_replaced"
                       : "creative_baked_room_stale_document_changed";
  window.creativeBakedRoomStaleReasonCode =
      window.creativeBakedRoomStaleStatus;
}

void recordProductCreativeBakedRoomFresh(ProductAppWindowState& window,
                                         std::uint64_t documentId,
                                         std::uint64_t revision) {
  window.creativeBakedRoomStale = false;
  window.creativeBakedRoomStaleDocumentId = documentId;
  window.creativeBakedRoomStaleRevision = revision;
  window.creativeBakedRoomStaleStatus = "creative_baked_room_fresh";
  window.creativeBakedRoomStaleReasonCode = "creative_baked_room_fresh";
}

void recordProductCreativeViewportPickFrame(
    ProductAppWindowState& window,
    const ProductCreativeViewportPickFrameReceipt& receipt) {
  window.creativeViewportPickRequested = receipt.requested;
  window.creativeViewportPickActive = receipt.active;
  window.creativeViewportPickClickPresent = receipt.clickPresent;
  window.creativeViewportPickClickSuppressed =
      receipt.downstreamClickSuppressed;
  window.creativeViewportPickFacadeAvailable = receipt.facadeAvailable;
  window.creativeViewportPickSourceAvailable = receipt.sourceAvailable;
  window.creativeViewportPickProjected = receipt.projected;
  window.creativeViewportPickPicked = receipt.picked;
  window.creativeViewportPickObjectCount = receipt.objectCount;
  window.creativeViewportPickProjectionCellCount =
      receipt.projectionCellCount;
  window.creativeViewportPickStatus = receipt.status;
  window.creativeViewportPickReasonCode = receipt.reasonCode;
  window.creativeViewportPickPickStatus =
      std::string(creative::toString(receipt.pickStatus));
  window.creativeViewportPickMessage =
      receipt.pickMessage.empty() ? "none" : receipt.pickMessage;
  window.creativeViewportPickCoordX = receipt.coord.x;
  window.creativeViewportPickCoordY = receipt.coord.y;
  window.creativeViewportPickCoordZ = receipt.coord.z;
  window.creativeViewportPickGridIndex = receipt.gridIndex;
  window.creativeViewportPickObjectId = receipt.objectId;
  window.creativeViewportPickObjectKind =
      std::string(creative::toString(receipt.objectKind));
  window.creativeViewportPickOccupancyKind =
      std::string(creative::toString(receipt.occupancyKind));
  window.creativeViewportPickTarget = receipt.target.value;
  window.creativeViewportPickCellIndex =
      static_cast<std::uint64_t>(receipt.cellIndex);
}

void recordProductCreativeWireframeFrame(
    ProductAppWindowState& window,
    const ProductCreativeWireframeFrameReceipt& receipt) {
  window.creativeWireframeRequested = receipt.requested;
  window.creativeWireframeActive = receipt.active;
  window.creativeWireframeFacadeAvailable = receipt.facadeAvailable;
  window.creativeWireframeDocumentAvailable = receipt.documentAvailable;
  window.creativeWireframeSourceAvailable = receipt.sourceAvailable;
  window.creativeWireframeObjectCount = receipt.objectCount;
  window.creativeWireframeVisibleObjectCount = receipt.visibleObjectCount;
  window.creativeWireframeItemCount = receipt.itemCount;
  window.creativeWireframeSegmentCount = receipt.segmentCount;
  window.creativeWireframeBoxItemCount = receipt.boxItemCount;
  window.creativeWireframeLineItemCount = receipt.lineItemCount;
  window.creativeWireframePointItemCount = receipt.pointItemCount;
  window.creativeWireframeSkippedDegenerateCount =
      receipt.skippedDegenerateCount;
  window.creativeWireframeStatus = receipt.status;
  window.creativeWireframeReasonCode = receipt.reasonCode;
  window.creativeWireframeWireframeStatus =
      std::string(creative::toString(receipt.wireframeStatus));
  window.creativeWireframeWireframeReasonCode =
      receipt.wireframeReasonCode.empty() ? "none"
                                          : receipt.wireframeReasonCode;
  window.creativeWireframeSegmentStatus =
      std::string(creative::toString(receipt.segmentStatus));
  window.creativeWireframeSegmentReasonCode =
      receipt.segmentReasonCode.empty() ? "none" : receipt.segmentReasonCode;
  window.creativeWireframeDebugLineRequested = receipt.debugLineRequested;
  window.creativeWireframeDebugLineSourceAvailable =
      receipt.debugLineSourceAvailable;
  window.creativeWireframeDebugLineInputSegmentCount =
      receipt.debugLineInputSegmentCount;
  window.creativeWireframeDebugLineCount = receipt.debugLineCount;
  window.creativeWireframeDebugLineSkippedDegenerateCount =
      receipt.debugLineSkippedDegenerateCount;
  window.creativeWireframeDebugLineStatus =
      std::string(toString(receipt.debugLineStatus));
  window.creativeWireframeDebugLineReasonCode =
      receipt.debugLineReasonCode.empty() ? "none" : receipt.debugLineReasonCode;
}

RenderReceipt buildProductAppReceipt(const ProductAppOptions& options,
                                     const ProductWorldTemplate& world,
                                     const FrontendState& frontend,
                                     const FrontendSettings& settings,
                                     const ProductAppWindowState& window,
                                     const ProductSaveBridgeResult& saves) {
  RenderReceipt receipt;
  const ProductActiveSurfaceFrame activeSurface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  const ProductCreativeSurfaceKind creativeSurface =
      productCreativeSurfaceKindForWindow(frontend, window);
  const bool mapMakerLive = productMapMakerLiveForWindow(frontend, window);
  const GameplayFeedback feedback = buildGameplayFeedback(window);
  const ProductMovementProofPacket movementProof =
      buildProductMovementProofPacket(window);
  const ProductVulkanGameplayReadiness vulkanGameplayReadiness =
      evaluateProductVulkanGameplayReadiness(window);
  const MovementDebugHud movementHud =
      buildMovementDebugHud(movementProof,
                            window.gameplayActive,
                            settings.devToolsEnabled,
                            settings.debugOverlayEnabled);
  const NpcBehaviorDebugHud npcBehaviorHud{
      window.npcBehaviorDebugHud.visible,
      settings.devToolsEnabled,
      settings.debugOverlayEnabled,
      window.npcBehaviorDebugHud.debugAvailable,
      static_cast<std::size_t>(window.npcBehaviorDebugHud.lineCount),
      window.npcBehaviorDebugHud.status,
      window.npcBehaviorDebugHud.reasonCode,
      {}};
  const PhysicsDebugHud physicsHud{
      window.physicsDebugHud.visible,
      settings.devToolsEnabled,
      settings.debugOverlayEnabled,
      window.physicsDebugHud.debugAvailable,
      window.physicsDebugHud.lineCount,
      window.physicsDebugHud.status,
      window.physicsDebugHud.reasonCode,
      window.physicsDebugHud.hasWarnings,
      {}};
  appendProductFrontendSettingsWindowFields(receipt, options, frontend, settings, window, creativeSurface, mapMakerLive);
  appendProductStartupWorldBuildoutFields(receipt, frontend, window, saves);
  appendProductSaveStateFields(receipt, frontend, window);
  appendProductGameplayRuntimeMovementFields(receipt, window, movementProof);
  appendProductDebugHudFields(receipt, window, movementHud, npcBehaviorHud, physicsHud);
  appendProductGameplaySceneStateFields(receipt, window);
  appendProductFeedbackSurfaceAutomationVulkanFields(receipt, window, feedback, activeSurface, vulkanGameplayReadiness);
  appendProductCreativeUiFields(receipt, window);
  appendProductCreativePickWireframeFields(receipt, window, vulkanGameplayReadiness);
  appendProductTailFields(receipt, options, world, window, saves);
  const bool windowFailed = window.requested && !window.created;
  appendReceiptField(receipt, "result", windowFailed ? "skip" : "pass");
  appendReceiptField(receipt, "reason_code",
                     windowFailed ? "sdl3_unavailable"
                                  : (window.gameplayActive ? "product_gameplay_ready"
                                                           : "opening_menu_ready"));
  return receipt;
}

}  // namespace iggy3d
