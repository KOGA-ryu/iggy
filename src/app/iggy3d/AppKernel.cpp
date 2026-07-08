#include "app/iggy3d/AppKernel.hpp"

#include <iostream>
#include <utility>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/automation/AutomationControl.hpp"
#include "app/iggy3d/automation/AutomationDispatch.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"
#include "app/iggy3d/gameplay/ScriptedDriver.hpp"
#include "app/iggy3d/gameplay/TapeRunner.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/view/CameraController.hpp"
#include "app/iggy3d/window/Loop.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/iggy3d/world/ProductNewWorldLaunch.hpp"
#include "app/iggy3d/world/ProductWorldTemplateOperations.hpp"
#include "app/input/ActionState.hpp"

namespace iggy3d {

namespace {

FrontendInputBackend settingsInputBackendFromOptions(ProductInputBackend backend) {
  switch (backend) {
    case ProductInputBackend::Keyboard:
      return FrontendInputBackend::Keyboard;
    case ProductInputBackend::Gamepad:
      return FrontendInputBackend::Gamepad;
    case ProductInputBackend::Auto:
      return FrontendInputBackend::Auto;
  }
  return FrontendInputBackend::Auto;
}

FrontendRendererChoice settingsRendererFromOptions(ProductRendererRequest renderer) {
  switch (renderer) {
    case ProductRendererRequest::Null:
      return FrontendRendererChoice::Null;
    case ProductRendererRequest::Vulkan:
      return FrontendRendererChoice::Vulkan;
  }
  return FrontendRendererChoice::Null;
}

FrontendWindowMode settingsWindowModeFromOptions(ProductWindowMode mode) {
  switch (mode) {
    case ProductWindowMode::NoWindow:
      return FrontendWindowMode::NoWindow;
    case ProductWindowMode::Window:
      return FrontendWindowMode::Window;
  }
  return FrontendWindowMode::NoWindow;
}

FrontendSettings productFrontendSettingsFromOptions(const ProductAppOptions& options) {
  FrontendSettings settings = defaultFrontendSettings();
  settings.inputBackend = settingsInputBackendFromOptions(options.inputBackend);
  settings.renderer = settingsRendererFromOptions(options.renderer);
  settings.windowMode = settingsWindowModeFromOptions(options.windowMode);
  settings.cameraMode = FrontendCameraMode::FirstPerson;
  settings.devToolsEnabled = true;
  settings.debugOverlayEnabled = options.debugOverlay;
  return settings;
}

}  // namespace

int AppKernel::run(const ProductAppOptions& options) {
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  saves = scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  settings = productFrontendSettingsFromOptions(options);
  creativeApp.facade.reset();
  // branch-gate: BG-1026
  worldSetupDraft = options.devPackageOverride.empty()
                        ? makeProductDefaultWorldSetupDraft()
                        : makeDefaultWorldSetupDraft();
  recordWorldSetupDraftState(worldSetupDraft, window);

  initializeProductStarterTransition(frontend, window,
                                     saves.slots.compatibleCount > 0);

  if (options.autoNewWorld) {
    launchProductNewWorld(options, worldSetupDraft, frontend, activeSession, window);
  }
  if (options.scriptedGameplaySmoke) {
    runScriptedProductGameplaySmoke(activeSession, window);
    ActionState scriptedLook;
    recordAction(scriptedLook, InputAction::PlayerLookX, true, false, false, 1.0F);
    recordAction(scriptedLook, InputAction::PlayerLookY, true, false, false, 0.5F);
    applyProductCameraActions(scriptedLook, window.viewport, settings, "scripted");
  }

  FrontendSettingsTab automationSettingsTab = FrontendSettingsTab::None;
  bool automationCloseRequested = false;
  ProductAutomationControlContext automationControlContext{
      options.automationControlPath, frontend, window, automationSettingsTab,
      [this, &options, &automationSettingsTab, &automationCloseRequested](
          const ProductAutomationCommand& command) {
        return applyProductAutomationAppCommand(
            command, ProductAutomationAppContext{
                         frontend, saves, options, settings,
                         automationSettingsTab, activeSession, worldSetupDraft,
                         window, automationCloseRequested, &creativeApp});
      },
      [this]() { return productInputOwnerFor(frontend, window); },
  };
  applyProductAutomationControl(automationControlContext);
  if (!automationCloseRequested) {
    runProductGameplayTapeFromOptions(
        ProductGameplayTapeOptionsRunRequest{options, activeSession, window});
  }
  saves = scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  if (automationCloseRequested) {
    window.frontendShell.status = "automation_close_requested";
  }

  // The loop returns the true end-of-session catalog (in-window soft-delete / new-world fold into
  // it); consume it as the single source of truth for the receipt -- no exit-time re-scan.
  ProductWindowLoopResult loopResult = runProductWindowLoop(ProductWindowLoopRequest{
      options, world, frontend, activeSession, worldSetupDraft, window, settings,
      saves, &creativeApp});
  window = std::move(loopResult.window);
  saves = std::move(loopResult.saves);
  refreshProductGameplayProjectionMetrics(
      ProductGameplayProjectionRefreshRequest{activeSession, window,
                                              settings.devToolsEnabled,
                                              settings.debugOverlayEnabled,
                                              options.renderer, frontend,
                                              &creativeApp});

  if (options.printRenderReceipt) {
    std::cout << formatRenderReceipt(
        buildProductAppReceipt(options,
                               world,
                               frontend,
                               settings,
                               window,
                               saves,
                               creativeApp.identity,
                               activeSession.has_value()
                                   ? activeSession->stateHash()
                                   : 0U));
  }

  return window.requested && !window.created ? 77 : 0;
}

}  // namespace iggy3d
