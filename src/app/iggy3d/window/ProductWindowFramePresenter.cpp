#include "app/iggy3d/window/ProductWindowFramePresenter.hpp"

#include "app/iggy3d/OpeningMenuView.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

namespace {

void applyProductWindowProjectionMetrics(
    ProductAppWindowState& window,
    const ProductGameplayProjectionFrame& projectionFrame,
    bool viewVisible) {
  applyGameplayProjectionMetrics(window,
                                 projectionFrame.scenePtr(),
                                 projectionFrame.debugPtr(),
                                 projectionFrame.drawListPtr(),
                                 projectionFrame.viewportFramePtr(),
                                 projectionFrame.renderBridgePtr(),
                                 viewVisible);
}

void presentProductVulkanFrame(ProductWindowFramePresenterRequest request) {
  // branch-gate: BG-1030
  if (request.projectionFrame.scenePtr() != nullptr &&
      request.projectionFrame.debugPtr() != nullptr &&
      request.projectionFrame.scene.room.loaded) {
    const SdlDrawableExtent drawableExtent = request.sdlWindow.drawableExtent();
    // branch-gate: BG-1030
    if (drawableExtent.width > 0U && drawableExtent.height > 0U) {
      const FrameInput renderFrame = makeProductVulkanFrame(
          request.projectionFrame.scene, request.projectionFrame.debug,
          request.window.framesPresented + 1U, drawableExtent.width,
          drawableExtent.height, request.window.viewport.cameraYawDegrees,
          request.window.viewport.cameraPitchDegrees);
      const RenderSubmitResult submit =
          request.renderer.vulkanRenderer.submitFrame(renderFrame);
      recordProductVulkanSubmit(request.window, submit);
    } else {
      request.window.productVulkanStatus = "frame_not_submitted";
      request.window.productVulkanReasonCode = "frame_not_drawable";
    }
  } else {
    request.window.productVulkanStatus = "waiting_for_gameplay_room";
    request.window.productVulkanReasonCode =
        "product_vulkan_waiting_for_gameplay_room";
    // branch-gate: BG-1072
    if (request.frontend.screen == FrontendScreen::Starter) {
      recordProductVulkanMenuUnsupported(request.window, "starter");
    }
  }
}

void presentProductSdlFrame(ProductWindowFramePresenterRequest request) {
  const OpeningMenuViewState view =
      drawOpeningMenuView(*request.renderer.sdlRenderer,
                          request.options,
                          request.world,
                          request.frontend,
                          request.settingsTab,
                          request.worldSetupDraft,
                          request.window.worldSetupDungeonDraftEditMode,
                          request.window.worldSetupDungeonDraftModified,
                          request.window.worldSetupDungeonDraftCursorRow,
                          request.window.worldSetupDungeonDraftCursorColumn,
                          request.window.gameplayActive,
                          request.window.runtimeStateHash,
                          request.projectionFrame.viewportFramePtr(),
                          &request.projectionFrame.feedback,
                          &request.projectionFrame.interactionModeHud,
                          &request.projectionFrame.topDownMapOverlay,
                          &request.projectionFrame.movementHud,
                          &request.projectionFrame.npcBehaviorHud,
                          &request.projectionFrame.roomEditorHud,
                          request.projectionFrame.sceneItemCount,
                          request.projectionFrame.debugPtr(),
                          request.window.viewport.cameraYawDegrees,
                          request.window.viewport.cameraPitchDegrees,
                          request.saves);
  request.window.viewport.cameraHeadingVisible =
      request.window.viewport.cameraHeadingVisible || view.cameraHeadingDrawn;
  request.window.menuTextDrawn = request.window.menuTextDrawn || view.textDrawn;
  request.window.selectedRowDrawn =
      request.window.selectedRowDrawn || view.selectedRowDrawn;
  request.window.menuRowCount = view.rowCount;
}

}  // namespace

void presentProductWindowFrame(ProductWindowFramePresenterRequest request) {
  // branch-gate: BG-1030
  if (request.window.drawable) {
    applyProductWindowProjectionMetrics(request.window,
                                        request.projectionFrame,
                                        request.projectionFrame.viewVisible);
    // branch-gate: BG-1030
    if (request.renderer.useVulkanRenderer) {
      presentProductVulkanFrame(request);
    } else {
      presentProductSdlFrame(request);
    }
  } else {
    applyProductWindowProjectionMetrics(request.window, request.projectionFrame, false);
  }
}

}  // namespace iggy3d
