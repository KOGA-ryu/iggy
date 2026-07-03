#include "app/iggy3d/window/Loop.hpp"

#include <utility>

#include "app/frontend/MenuInput.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"
#include "app/iggy3d/window/FramePresenter.hpp"
#include "app/iggy3d/window/CreativeWindowCoordinateSpace.hpp"
#include "app/iggy3d/window/CreativeUiWindowFrame.hpp"
#include "app/iggy3d/window/CreativeWireframeFrame.hpp"
#include "app/iggy3d/window/InputFrame.hpp"
#include "app/iggy3d/window/MouseCapturePolicy.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <chrono>
#include <thread>

#include "app/platform/SdlWindow.hpp"
#endif

namespace iggy3d {

namespace {

const char* productWindowTitle(const ProductAppWindowState& window) {
  // branch-gate: BG-1031
  if (window.gameplayActive) {
    return "iggy3d - Gameplay";
  }
  return "iggy3d - Opening Menu";
}

void recordNoWindowMouseCapturePolicy(const FrontendState& frontend,
                                      ProductAppWindowState& window) {
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  const ProductMouseCapturePolicy policy = buildProductMouseCapturePolicy({
      window.gameplayActive,
      window.interactionMode,
      surface.inputOwner,
      surface.gameplayInputSuppressed,
      true,
      false,
  });
  window.mouseCaptureRequested = policy.requested;
  window.mouseCaptureActive = false;
  window.mouseCaptureStatus = policy.status;
  window.mouseCaptureReasonCode = policy.reasonCode;
  window.mouseCaptureMode = policy.mode;
  window.mouseCaptureInputOwner = policy.inputOwner;
}

creative::CreativeSpatialProjectionRequest
creativeViewportPickProjectionRequest() noexcept {
  creative::CreativeSpatialProjectionRequest request;
  request.gridSize = {64, 64, 8};
  request.cellSize = 1.0;
  request.clampToGrid = true;
  request.includeAuthoringOnly = false;
  return request;
}

creative::CreativeSpatialProjectionRequest
creativeWireframeProjectionRequest() noexcept {
  creative::CreativeSpatialProjectionRequest request;
  request.gridSize = {64, 64, 16};
  request.cellSize = 1.0;
  request.clampToGrid = true;
  request.includeAuthoringOnly = false;
  return request;
}

}  // namespace

ProductWindowLoopResult runProductWindowLoop(const ProductWindowLoopRequest& request) {
  ProductAppWindowState window = request.window;
  // Mutable in-loop catalog. `request.saves` is the snapshot the loop starts from; we copy it
  // (like `window` above) so in-window mutations re-scan into it: a live soft-delete
  // (Operations.cpp) and a live new-world (menu/ActionHandlers.cpp) both refresh this copy. It
  // is RETURNED with the window so Continue / the Load list / the receipt all report the true
  // end-of-session catalog without an app restart. The request itself stays a read-only input.
  ProductSaveBridgeResult saves = request.saves;
  window.requested = request.options.windowMode == ProductWindowMode::Window;
  const bool useVulkanRenderer =
      productWindowRendererUsesVulkan(request.options.renderer);
  window.productVulkanRendererRequested = useVulkanRenderer;
  (void)syncProductWindowInputOwnerFromActiveSurface(request.frontend, window);
  // branch-gate: BG-1031
  if (!window.requested) {
    recordNoWindowMouseCapturePolicy(request.frontend, window);
    return ProductWindowLoopResult{std::move(window), std::move(saves)};
  }

#if defined(IGGY3D_HAS_SDL3)
  window.sdlAvailable = true;

  SdlWindowCreateInfo createInfo;
  createInfo.title = "iggy3d - Opening Menu";
  createInfo.width = 1280;
  createInfo.height = 720;
  createInfo.resizable = true;
  createInfo.highDpi = true;
  createInfo.vulkan = useVulkanRenderer;

  SdlWindow sdlWindow(createInfo);
  window.created = sdlWindow.nativeWindow() != nullptr;
  window.drawable = sdlWindow.isDrawable();
  window.openingMenuVisible =
      window.created && request.frontend.screen == FrontendScreen::Starter;
  // branch-gate: BG-1031
  if (!window.created) {
    window.status = "window_create_failed";
    return ProductWindowLoopResult{std::move(window), std::move(saves)};
  }

  ProductWindowRendererState renderer = createProductWindowRenderer(
      ProductWindowRendererRequest{request.options.renderer, &createInfo,
                                   &sdlWindow, &window});
  // branch-gate: BG-1031
  if (!renderer.ready) {
    return ProductWindowLoopResult{std::move(window), std::move(saves)};
  }

  sdlWindow.setTitle(productWindowTitle(window));
  const auto start = std::chrono::steady_clock::now();
  ProductWindowInputFrameState inputFrame;
  initializeProductWindowInputFrameState(inputFrame, window);
  bool closeRequested = false;
  FrontendSettingsTab settingsTab = FrontendSettingsTab::Input;
  while (sdlWindow.isOpen()) {
    sdlWindow.pollEvents();
    ++window.eventPollCount;
    window.drawable = sdlWindow.isDrawable();
    sdlWindow.setTitle(productWindowTitle(window));

    const SdlDrawableExtent drawableExtent = sdlWindow.drawableExtent();
    const SdlWindowEventState& eventState = sdlWindow.eventState();
    const ProductCreativeWindowCoordinateSpace creativeCoordinateSpace =
        resolveProductCreativeWindowCoordinateSpace(
            ProductCreativeWindowCoordinateSpaceRequest{
                eventState.windowWidth,
                eventState.windowHeight,
                drawableExtent.width,
                drawableExtent.height,
                createInfo.width,
                createInfo.height,
                1280,
                720});
    const ProductCreativeUiFrame creativeUiFrame = buildProductCreativeUiWindowFrame(
        ProductCreativeUiWindowFrameRequest{&window,
                                            request.creativeFacade,
                                            drawableExtent.width,
                                            drawableExtent.height,
                                            createInfo.width,
                                            createInfo.height,
                                            ProductUiThemeId::System,
                                            eventState.windowWidth,
                                            eventState.windowHeight});
    const ProductUiDrawList* creativeUiDrawList =
        creativeUiFrame.projection.drawList.ready
            ? &creativeUiFrame.projection.drawList
            : nullptr;
    const ProductCreativeUiOverlayInputAvailability creativeUiInputAvailability =
        resolveProductCreativeUiOverlayInputAvailability(
            ProductCreativeUiOverlayInputAvailabilityRequest{
                creativeUiDrawList,
                renderer.useVulkanRenderer,
                window.drawable,
                drawableExtent.width,
                drawableExtent.height,
                window.gameplayActive && request.activeSession.has_value()});
    const ProductUiDrawList* creativeUiInputDrawList =
        creativeUiInputAvailability.inputAvailable ? creativeUiDrawList
                                                   : nullptr;

    processProductWindowInputFrame(ProductWindowInputFrameContext{
        request.frontend, saves, request.options, settingsTab,
        request.activeSession, request.worldSetupDraft, window, request.settings,
        inputFrame, closeRequested, &sdlWindow, request.creativeFacade,
        creativeUiInputDrawList,
        creative::CreativeViewportPickViewport{
            0.0F,
            0.0F,
            static_cast<float>(creativeCoordinateSpace.virtualWidth),
            static_cast<float>(creativeCoordinateSpace.virtualHeight),
        },
        creativeViewportPickProjectionRequest(),
        0,
        creative::CreativeViewportPickDepthMode::HighestZFirst,
        {}});

    const ProductCreativeWireframeFrameBuildResult wireframeFrame =
        buildProductCreativeWireframeFrame(ProductCreativeWireframeFrameRequest{
            &window,
            request.creativeFacade,
            creativeWireframeProjectionRequest()});
    recordProductCreativeWireframeFrame(window, wireframeFrame.receipt);
    const ProductCreativeWireframeDebugLineList* creativeWireframeDebugLines =
        wireframeFrame.receipt.active &&
                wireframeFrame.receipt.debugLineSourceAvailable
            ? &wireframeFrame.debugLineList
            : nullptr;

    const ProductGameplayProjectionFrame projectionFrame =
        buildProductGameplayProjectionFrame(ProductGameplayProjectionFrameRequest{
            request.activeSession, window, request.settings.devToolsEnabled,
            request.settings.debugOverlayEnabled, request.options.renderer,
            request.frontend});

    presentProductWindowFrame(ProductWindowFramePresenterRequest{
        request.options, request.world, request.frontend, settingsTab,
        request.worldSetupDraft, window, saves, sdlWindow, renderer,
        projectionFrame, creativeUiDrawList, creativeWireframeDebugLines});
    ++window.framesPresented;

    // branch-gate: BG-1031
    if (request.options.frames > 0 &&
        window.framesPresented >= request.options.frames) {
      break;
    }
    // branch-gate: BG-1031
    if (closeRequested) {
      break;
    }
    // branch-gate: BG-1031
    if (request.options.holdSeconds > 0) {
      const auto elapsed = std::chrono::steady_clock::now() - start;
      // branch-gate: BG-1031
      if (elapsed >= std::chrono::seconds(request.options.holdSeconds)) {
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  shutdownProductWindowInputFrameState(inputFrame, &sdlWindow, &window);
  shutdownProductWindowRenderer(renderer);
  window.selectedSettingsTab = settingsTab;
  finalizeProductWindowRendererStatus(renderer, window);
  return ProductWindowLoopResult{std::move(window), std::move(saves)};
#else
  (void)request;
  window.sdlAvailable = false;
  window.created = false;
  window.drawable = false;
  window.openingMenuVisible = false;
  window.status = "sdl3_unavailable";
  // BLIND on the box (this #else is preprocessed out with system SDL3 ON). Mirrors the SDL
  // returns exactly: `saves == request.saves` here (unmutated), returned with the window.
  return ProductWindowLoopResult{std::move(window), std::move(saves)};
#endif
}

}  // namespace iggy3d
