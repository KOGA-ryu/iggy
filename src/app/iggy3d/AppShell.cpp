#include "app/iggy3d/AppShell.hpp"

#include <iostream>
#include <optional>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/ProductAppOperations.hpp"
#include "app/iggy3d/ProductCameraController.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/automation/AutomationControl.hpp"
#include "app/iggy3d/automation/AutomationDispatch.hpp"
#include "app/iggy3d/menu/ProductMenuInputRouter.hpp"
#include "app/iggy3d/ProductBuiltinDungeon.hpp"
#include "app/iggy3d/gameplay/ProductGameplayProjectionRefresh.hpp"
#include "app/iggy3d/gameplay/ProductGameplayTapeRunner.hpp"
#include "app/iggy3d/menu/ProductMenuTransitions.hpp"
#include "app/iggy3d/ProductScriptedGameplayDriver.hpp"
#include "app/iggy3d/window/Loop.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/input/ActionState.hpp"
#include "runtime/session/Session.hpp"

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

int runProductApp(int argc, char** argv) {
  const ProductAppOptionsParseResult parsed = parseProductAppOptions(argc, argv);
  if (parsed.status == ProductAppOptionStatus::Help) {
    std::cout << productAppHelpText();
    return 0;
  }
  if (parsed.status != ProductAppOptionStatus::Ok) {
    RenderReceipt receipt;
    appendReceiptField(receipt, "app", "iggy3d");
    appendReceiptField(receipt, "result", "fail");
    appendReceiptField(receipt, "reason_code", productAppOptionStatusReason(parsed.status));
    appendReceiptField(receipt, "option", parsed.option);
    std::cout << formatRenderReceipt(receipt);
    return 2;
  }

  const ProductAppOptions& options = parsed.options;
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  ProductSaveBridgeResult saves =
      scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  const FrontendSettings settings = productFrontendSettingsFromOptions(options);
  std::optional<Session> activeSession;
  // branch-gate: BG-1026
  WorldSetupDraft worldSetupDraft = options.devPackageOverride.empty()
                                        ? makeProductDefaultWorldSetupDraft()
                                        : makeDefaultWorldSetupDraft();
  ProductAppWindowState window;
  recordWorldSetupDraftState(worldSetupDraft, window);

  FrontendState frontend;
  initializeProductStarterTransition(frontend, window, saves.slots.compatibleCount > 0);

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
      options.automationControlPath, window, automationSettingsTab,
      [&frontend, &saves, &options, &automationSettingsTab, &activeSession,
       &worldSetupDraft, &window, &automationCloseRequested](
          const ProductAutomationCommand& command) {
        return applyProductAutomationAppCommand(
            command, ProductAutomationAppContext{
                         frontend, saves, options, automationSettingsTab,
                         activeSession, worldSetupDraft, window,
                         automationCloseRequested});
      },
      [&frontend, &window]() { return productInputOwnerFor(frontend, window); },
      [&frontend](MenuOwner owner) {
        return frontendBlocksGameplayInput(frontend) || menuOwnerBlocksGameplay(owner);
      },
  };
  applyProductAutomationControl(automationControlContext);
  if (!automationCloseRequested) {
    runProductGameplayTapeFromOptions(
        ProductGameplayTapeOptionsRunRequest{options, activeSession, window});
  }
  saves = scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  if (automationCloseRequested) {
    window.status = "automation_close_requested";
  }

  window = runProductWindowLoop(ProductWindowLoopRequest{
      options, world, frontend, activeSession, worldSetupDraft, window, settings, saves});
  refreshProductGameplayProjectionMetrics(
      ProductGameplayProjectionRefreshRequest{activeSession, window,
                                              settings.devToolsEnabled,
                                              settings.debugOverlayEnabled,
                                              options.renderer});

  if (options.printRenderReceipt) {
    std::cout << formatRenderReceipt(
        buildProductAppReceipt(options, world, frontend, settings, window, saves));
  }

  return window.requested && !window.created ? 77 : 0;
}

}  // namespace iggy3d
