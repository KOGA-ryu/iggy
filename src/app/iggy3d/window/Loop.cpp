#include "app/iggy3d/window/Loop.hpp"

#include <chrono>
#include <string_view>
#include <utility>

#include "app/frontend/MenuInput.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"
#include "app/iggy3d/window/FramePresenter.hpp"
#include "app/iggy3d/creative/bridge/WindowCoordinateSpace.hpp"
#include "app/iggy3d/creative/bridge/UiWindowFrame.hpp"
#include "app/iggy3d/creative/bridge/WireframeFrame.hpp"
#include "app/iggy3d/window/InputFrame.hpp"
#include "app/iggy3d/window/MouseCapturePolicy.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <thread>

#include "app/platform/SdlWindow.hpp"
#endif

namespace iggy3d {

namespace {

const char* productWindowTitle(const FrontendState& frontend,
                               const ProductAppWindowState& window,
                               const creative::CreativeAppState* creativeApp) {
  // The title must agree with what is actually DRAWN. The opening menu draws
  // while frontend.screen == Starter (the FramePresenter menu gate). The title,
  // historically, keyed only on window.gameplay.gameplayActive + the creative predicate
  // (window-side state). Those two sources can disagree if a build is only
  // partially rebuilt (window.gameplay.gameplayActive flips but the frontend.screen
  // transition is not linked in) — which surfaces as the confusing "iggy3d -
  // Creative" title over a still-visible starter menu. Gate the title on the
  // SAME frontend surface the menu gate reads first, so they can never
  // contradict.
  // branch-gate: BG-1031
  if (frontend.screen == FrontendScreen::Starter) {
    return "iggy3d - Opening Menu";
  }
  // branch-gate: BG-1031
  if (window.gameplay.gameplayActive) {
    // TV1-H (TL-5): the window title is creative-aware — the creative document
    // editor runs over the gameplay backdrop but is its own surface.
    if (productCreativeDocumentEditorActiveForSource(window, creativeApp)) {
      return "iggy3d - Creative";
    }
    return "iggy3d - Gameplay";
  }
  return "iggy3d - Opening Menu";
}

void recordNoWindowMouseCapturePolicy(const FrontendState& frontend,
                                      ProductAppWindowState& window,
                                      const creative::CreativeAppState*
                                          creativeApp) {
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  const ProductMouseCapturePolicy policy = buildProductMouseCapturePolicy({
      window.gameplay.gameplayActive,
      window.inputDevice.interactionMode,
      surface.inputOwner,
      surface.gameplayInputSuppressed,
      true,
      false,
      productCreativeDocumentEditorActiveForSource(window, creativeApp),
      window.creativeAuthoring.creativeNavigateActive,
  });
  window.inputDevice.mouseCapture.requested = policy.requested;
  window.inputDevice.mouseCapture.active = false;
  window.inputDevice.mouseCapture.status = policy.status;
  window.inputDevice.mouseCapture.reasonCode = policy.reasonCode;
  window.inputDevice.mouseCapture.mode = policy.mode;
  window.inputDevice.mouseCapture.inputOwner = policy.inputOwner;
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

std::uint64_t elapsedMicroseconds(
    std::chrono::steady_clock::time_point started) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - started)
          .count());
}

void recordFirstStartupMeasurement(
    bool& measured,
    std::uint64_t& microseconds,
    std::string& status,
    std::chrono::steady_clock::time_point started,
    std::string_view statusValue) {
  if (measured) {
    return;
  }
  measured = true;
  microseconds = elapsedMicroseconds(started);
  status = std::string{statusValue};
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
  window.presentPath.productVulkanRenderer.requested = useVulkanRenderer;
  (void)syncProductWindowInputOwnerFromActiveSurface(request.frontend, window);
  // branch-gate: BG-1031
  if (!window.requested) {
    recordNoWindowMouseCapturePolicy(request.frontend, window,
                                     request.creativeApp);
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
  window.frontendShell.openingMenuVisible =
      window.created && request.frontend.screen == FrontendScreen::Starter;
  // branch-gate: BG-1031
  if (!window.created) {
    window.frontendShell.status = "window_create_failed";
    return ProductWindowLoopResult{std::move(window), std::move(saves)};
  }

  const auto rendererStarted = std::chrono::steady_clock::now();
  ProductWindowRendererState renderer = createProductWindowRenderer(
      ProductWindowRendererRequest{request.options.renderer, &createInfo,
                                   &sdlWindow, &window});
  if (useVulkanRenderer) {
    window.frontendShell.startup.vulkanRendererInitMeasured = true;
    window.frontendShell.startup.vulkanRendererInitMicroseconds =
        elapsedMicroseconds(rendererStarted);
    window.frontendShell.startup.vulkanRendererInitStatus =
        renderer.ready ? "startup_vulkan_renderer_init_ready"
                       : window.presentPath.productVulkanReasonCode;
  }
  // branch-gate: BG-1031
  if (!renderer.ready) {
    return ProductWindowLoopResult{std::move(window), std::move(saves)};
  }

  sdlWindow.setTitle(
      productWindowTitle(request.frontend, window, request.creativeApp));
  const auto start = std::chrono::steady_clock::now();
  ProductWindowInputFrameState inputFrame;
  initializeProductWindowInputFrameState(inputFrame, window);
  bool closeRequested = false;
  FrontendSettingsTab settingsTab = FrontendSettingsTab::Input;
  while (sdlWindow.isOpen()) {
    sdlWindow.pollEvents();
    ++window.frontendShell.eventPollCount;
    window.drawable = sdlWindow.isDrawable();
    sdlWindow.setTitle(
        productWindowTitle(request.frontend, window, request.creativeApp));

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
    const auto creativeUiStarted = std::chrono::steady_clock::now();
    const ProductCreativeUiFrame creativeUiFrame = buildProductCreativeUiWindowFrame(
        ProductCreativeUiWindowFrameRequest{&window,
                                            request.creativeApp,
                                            drawableExtent.width,
                                            drawableExtent.height,
                                            createInfo.width,
                                            createInfo.height,
                                            ProductUiThemeId::System,
                                            eventState.windowWidth,
                                            eventState.windowHeight});
    if (creativeUiFrame.receipt.active) {
      recordFirstStartupMeasurement(
          window.frontendShell.startup.creativeUiFirstFrameMeasured,
          window.frontendShell.startup.creativeUiFirstFrameMicroseconds,
          window.frontendShell.startup.creativeUiFirstFrameStatus,
          creativeUiStarted,
          creativeUiFrame.receipt.status);
    }
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
                window.gameplay.gameplayActive && request.activeSession.has_value()});
    const ProductUiDrawList* creativeUiInputDrawList =
        creativeUiInputAvailability.inputAvailable ? creativeUiDrawList
                                                   : nullptr;

    processProductWindowInputFrame(ProductWindowInputFrameContext{
        request.frontend, saves, request.options, settingsTab,
        request.activeSession, request.worldSetupDraft, window, request.settings,
        inputFrame, closeRequested, &sdlWindow, request.creativeApp,
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

    const auto wireframeStarted = std::chrono::steady_clock::now();
    const ProductCreativeWireframeFrameBuildResult wireframeFrame =
        buildProductCreativeWireframeFrame(ProductCreativeWireframeFrameRequest{
            &window,
            request.creativeApp,
            creativeWireframeProjectionRequest()});
    if (wireframeFrame.receipt.active) {
      recordFirstStartupMeasurement(
          window.frontendShell.startup.creativeWireframeFirstFrameMeasured,
          window.frontendShell.startup.creativeWireframeFirstFrameMicroseconds,
          window.frontendShell.startup.creativeWireframeFirstFrameStatus,
          wireframeStarted,
          wireframeFrame.receipt.status);
    }
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
            request.frontend, request.creativeApp});

    presentProductWindowFrame(ProductWindowFramePresenterRequest{
        request.options, request.world, request.frontend, settingsTab,
        request.worldSetupDraft, window, saves, sdlWindow, renderer,
        projectionFrame, creativeUiDrawList, creativeWireframeDebugLines,
        request.creativeApp});
    ++window.frontendShell.framesPresented;

    // branch-gate: BG-1031
    if (request.options.frames > 0 &&
        window.frontendShell.framesPresented >= request.options.frames) {
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
  window.frontendShell.selectedSettingsTab = settingsTab;
  finalizeProductWindowRendererStatus(renderer, window);
  return ProductWindowLoopResult{std::move(window), std::move(saves)};
#else
  (void)request;
  window.sdlAvailable = false;
  window.created = false;
  window.drawable = false;
  window.frontendShell.openingMenuVisible = false;
  window.frontendShell.status = "sdl3_unavailable";
  // BLIND on the box (this #else is preprocessed out with system SDL3 ON). Mirrors the SDL
  // returns exactly: `saves == request.saves` here (unmutated), returned with the window.
  return ProductWindowLoopResult{std::move(window), std::move(saves)};
#endif
}

}  // namespace iggy3d
