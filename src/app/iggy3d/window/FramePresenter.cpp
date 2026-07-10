#include "app/iggy3d/window/FramePresenter.hpp"

#include <chrono>
#include <string>

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/menu/DrawList.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/menu/PauseUi.hpp"
#include "app/iggy3d/view/OpeningMenuView.hpp"

namespace iggy3d {

namespace {

std::uint64_t elapsedMicroseconds(
    std::chrono::steady_clock::time_point started) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - started)
          .count());
}

void recordFirstVulkanSubmitMeasurement(
    ProductAppWindowState& window,
    std::chrono::steady_clock::time_point started,
    const RenderSubmitResult& submit) {
  if (window.frontendShell.startup.vulkanFirstSubmitMeasured) {
    return;
  }
  window.frontendShell.startup.vulkanFirstSubmitMeasured = true;
  window.frontendShell.startup.vulkanFirstSubmitMicroseconds = elapsedMicroseconds(started);
  window.frontendShell.startup.vulkanFirstSubmitStatus = std::string{submit.reason.code};
}

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
      const bool creativeEditorOverlayActive =
          productCreativeDocumentEditorActiveForSource(request.window,
                                                       request.creativeApp) &&
          request.creativeUiDrawList != nullptr &&
          request.creativeUiDrawList->ready;
      ProductVulkanGameplayFrame renderFrame = buildProductVulkanGameplayFrame(
          request.projectionFrame, request.window.frontendShell.framesPresented + 1U,
          drawableExtent.width, drawableExtent.height,
          request.window.viewport.cameraYawDegrees,
          request.window.viewport.cameraPitchDegrees,
          request.window.gameplay.gameplayMovement.tuning,
          request.window.gameplay.gameplayMovement.tuningSelectedField,
          request.window.gameplay.gameplayMovement.tuningVisible,
          request.frontend.screen == FrontendScreen::DevOverlay &&
              frontendDevToolsOpen(request.frontend),
          request.frontend.devToolsCategory,
          creativeEditorOverlayActive);
      const ProductCreativeWireframeDebugRenderFrame creativeDebugFrame =
          buildProductCreativeWireframeDebugRenderFrame(
              request.creativeWireframeDebugLineList);
      renderFrame.creativeWireframeDebugLines = creativeDebugFrame.lines;
      renderFrame.frame.creativeWireframeDebug = creativeDebugFrame.frame;
      renderFrame.frame.creativeWireframeDebug.lines =
          renderFrame.creativeWireframeDebugLines.data();
      // Creative UI is a transient one-frame overlay over gameplay. It is appended
      // before pause UI so the pause journal remains topmost when both exist.
      if (request.creativeUiDrawList != nullptr) {
        appendCreativeUiOverlay(renderFrame,
                                *request.creativeUiDrawList,
                                request.window.frontendShell.framesPresented + 1U,
                                drawableExtent.width,
                                drawableExtent.height);
      }
      // In-game pause opens the character's journal: overlay the Journal-themed
      // pause menu onto the frozen scene before the single submit. The Vulkan
      // path renders no pause menu otherwise. Enablement mirrors makePauseRow.
      // branch-gate: BG-1030
      if (frontendPauseMenuOpen(request.frontend)) {
        PauseMenuContext pauseContext;
        pauseContext.pauseOpen = true;
        pauseContext.runtimeSessionAvailable = request.window.gameplay.gameplayActive;
        pauseContext.saveRootWritable = !request.options.saveRoot.empty();
        pauseContext.compatibleSaveCount = request.saves.slots.compatibleCount;
        pauseContext.developerToolsEnabled = true;
        pauseContext.activeRoomEditable = request.window.creativeAuthoring.roomEditing.ready;
        pauseContext.roomEditingReady = request.window.creativeAuthoring.roomEditing.ready;
        const PauseMenuModel pauseModel =
            buildPauseMenuModel(pauseContext, request.frontend.selectedAction);
        ProductPauseUiRequest pauseUiRequest;
        pauseUiRequest.model = &pauseModel;
        const ProductUiDrawList pauseUi =
            buildProductPauseUiDrawList(pauseUiRequest);
        appendPauseMenuOverlay(renderFrame, pauseUi,
                               request.window.frontendShell.framesPresented + 1U,
                               drawableExtent.width, drawableExtent.height);
      }
      const auto submitStarted = std::chrono::steady_clock::now();
      const RenderSubmitResult submit =
          request.renderer.vulkanRenderer.submitFrame(
              refreshProductVulkanGameplayFrameInput(renderFrame));
      recordFirstVulkanSubmitMeasurement(request.window,
                                         submitStarted,
                                         submit);
      recordProductVulkanSubmit(request.window, submit);
    } else {
      request.window.presentPath.productVulkanStatus = "frame_not_submitted";
      request.window.presentPath.productVulkanReasonCode = "frame_not_drawable";
    }
  } else {
    request.window.presentPath.productVulkanStatus = "waiting_for_gameplay_room";
    request.window.presentPath.productVulkanReasonCode =
        "product_vulkan_waiting_for_gameplay_room";
    // branch-gate: BG-1072
    if (request.frontend.screen == FrontendScreen::Starter) {
      const ProductUiDrawListRequest uiRequest =
          buildProductStarterUiDrawListRequest(
              request.frontend,
              request.saves,
              request.worldSetupDraft,
              request.settingsTab,
              {request.window.creativeAuthoring.worldSetup.dungeonDraftEditMode,
               request.window.creativeAuthoring.worldSetup.dungeonDraftModified,
               request.window.creativeAuthoring.worldSetup.dungeonDraftCursorRow,
               request.window.creativeAuthoring.worldSetup.dungeonDraftCursorColumn,
               request.window.creativeAuthoring.worldSetup.dungeonDraftSelectedGlyph,
               request.window.creativeAuthoring.worldSetup.dungeonDraftLastGlyph,
               request.window.saveSession.selectedProductSave.id,
               request.window.saveSession.saveDelete.candidateId});
      const ProductUiDrawList menuUi = buildProductStarterUiDrawList(uiRequest);
      recordProductVulkanMenuUiDrawList(request.window, "starter", menuUi);
      const SdlDrawableExtent drawableExtent = request.sdlWindow.drawableExtent();
      // branch-gate: BG-1072
      if (drawableExtent.width > 0U && drawableExtent.height > 0U && menuUi.ready) {
        ProductVulkanMenuFrame menuFrame = buildProductVulkanStarterMenuFrame(
            {&menuUi,
             request.window.frontendShell.framesPresented + 1U,
             drawableExtent.width,
             drawableExtent.height});
        // branch-gate: BG-1072
        if (menuFrame.ready) {
          const auto submitStarted = std::chrono::steady_clock::now();
          const RenderSubmitResult submit = request.renderer.vulkanRenderer.submitFrame(
              refreshProductVulkanMenuFrameInput(menuFrame));
          recordFirstVulkanSubmitMeasurement(request.window,
                                             submitStarted,
                                             submit);
          recordProductVulkanSubmit(request.window, submit);
        } else {
          request.window.presentPath.productVulkanStatus = "frame_not_submitted";
          request.window.presentPath.productVulkanReasonCode = menuFrame.reasonCode;
        }
      }
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
	                          request.window.gameplay.gameplayMovement.tuning,
	                          request.window.gameplay.gameplayMovement.tuningSelectedField,
	                          request.window.gameplay.gameplayMovement.tuningVisible,
	                          request.worldSetupDraft,
                          request.window.creativeAuthoring.worldSetup.dungeonDraftEditMode,
                          request.window.creativeAuthoring.worldSetup.dungeonDraftModified,
                          request.window.creativeAuthoring.worldSetup.dungeonDraftCursorRow,
                          request.window.creativeAuthoring.worldSetup.dungeonDraftCursorColumn,
                          request.window.creativeAuthoring.worldSetup.dungeonDraftSelectedGlyph,
                          request.window.creativeAuthoring.worldSetup.dungeonDraftLastGlyph,
                          request.window.gameplay.gameplayActive,
                          request.projectionFrame.runtimeStateHash,
                          request.projectionFrame.viewportFramePtr(),
                          &request.projectionFrame.feedback,
                          &request.projectionFrame.interactionModeHud,
                          &request.projectionFrame.topDownMapOverlay,
                          &request.projectionFrame.movementHud,
                          &request.projectionFrame.npcBehaviorHud,
                          &request.projectionFrame.physicsHud,
                          &request.projectionFrame.positionHud,
                          &request.projectionFrame.roomEditorHud,
                          request.projectionFrame.sceneItemCount,
                          request.projectionFrame.debugPtr(),
                          request.window.viewport.cameraYawDegrees,
                          request.window.viewport.cameraPitchDegrees,
                          request.saves,
                          request.window.saveSession.saveDelete.candidateId);
  request.window.viewport.cameraHeadingVisible =
      request.window.viewport.cameraHeadingVisible || view.cameraHeadingDrawn;
  request.window.frontendShell.menuTextDrawn = request.window.frontendShell.menuTextDrawn || view.textDrawn;
  request.window.frontendShell.selectedRowDrawn =
      request.window.frontendShell.selectedRowDrawn || view.selectedRowDrawn;
  request.window.frontendShell.menuRowCount = view.rowCount;
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
