#include "app/iggy3d/window/ProductWindowLoop.hpp"

#include "app/frontend/MenuInput.hpp"
#include "app/iggy3d/gameplay/ProductGameplayProjectionRefresh.hpp"
#include "app/iggy3d/window/ProductWindowFramePresenter.hpp"
#include "app/iggy3d/window/ProductWindowInputFrame.hpp"
#include "app/iggy3d/window/ProductWindowRendererLifecycle.hpp"
#include "app/iggy3d/product/ProductMenuInputRouter.hpp"

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

}  // namespace

ProductAppWindowState runProductWindowLoop(const ProductWindowLoopRequest& request) {
  ProductAppWindowState window = request.window;
  window.requested = request.options.windowMode == ProductWindowMode::Window;
  const bool useVulkanRenderer =
      productWindowRendererUsesVulkan(request.options.renderer);
  window.productVulkanRendererRequested = useVulkanRenderer;
  window.inputOwner = productInputOwnerFor(request.frontend, window);
  window.gameplayInputSuppressed =
      frontendBlocksGameplayInput(request.frontend) ||
      menuOwnerBlocksGameplay(window.inputOwner);
  // branch-gate: BG-1031
  if (!window.requested) {
    return window;
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
    return window;
  }

  ProductWindowRendererState renderer = createProductWindowRenderer(
      ProductWindowRendererRequest{request.options.renderer, &createInfo,
                                   &sdlWindow, &window});
  // branch-gate: BG-1031
  if (!renderer.ready) {
    return window;
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

    processProductWindowInputFrame(ProductWindowInputFrameContext{
        request.frontend, request.saves, request.options, settingsTab,
        request.activeSession, request.worldSetupDraft, window, request.settings,
        inputFrame, closeRequested, &sdlWindow});

    const ProductGameplayProjectionFrame projectionFrame =
        buildProductGameplayProjectionFrame(ProductGameplayProjectionFrameRequest{
            request.activeSession, window, request.settings.devToolsEnabled,
            request.settings.debugOverlayEnabled, request.options.renderer});

    presentProductWindowFrame(ProductWindowFramePresenterRequest{
        request.options, request.world, request.frontend, settingsTab,
        request.worldSetupDraft, window, request.saves, sdlWindow, renderer,
        projectionFrame});
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
  return window;
#else
  (void)request;
  window.sdlAvailable = false;
  window.created = false;
  window.drawable = false;
  window.openingMenuVisible = false;
  window.status = "sdl3_unavailable";
  return window;
#endif
}

}  // namespace iggy3d
