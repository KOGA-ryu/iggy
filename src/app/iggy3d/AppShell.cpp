#include "app/iggy3d/AppShell.hpp"

#include <iostream>
#include <optional>
#include <string>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/ProductAppOperations.hpp"
#include "app/iggy3d/ProductCameraController.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductAsciiRoomActivation.hpp"
#include "app/iggy3d/product/AutomationControl.hpp"
#include "app/iggy3d/product/AutomationDispatch.hpp"
#include "app/iggy3d/product/AutomationRoomEditing.hpp"
#include "app/iggy3d/product/ProductMenuInputRouter.hpp"
#include "app/iggy3d/ProductBuiltinDungeon.hpp"
#include "app/iggy3d/ProductGameplayProjectionRefresh.hpp"
#include "app/iggy3d/ProductGameplayTape.hpp"
#include "app/iggy3d/ProductGameplayTapeRunner.hpp"
#include "app/iggy3d/ProductMenuTransitions.hpp"
#include "app/iggy3d/ProductScriptedGameplayDriver.hpp"
#include "app/iggy3d/ProductWindowLoop.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/SaveBridge.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
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
  settings.debugOverlayEnabled = true;
  return settings;
}

std::string failedTapeStepReceiptValue(std::uint64_t stepIndex) {
  return stepIndex == 0U ? "none" : std::to_string(stepIndex);
}

void recordProductGameplayTapeParse(const ProductGameplayTapeParseResult& parsed,
                                    ProductAppWindowState& window) {
  window.gameplayTapeLoaded = parsed.ok;
  window.gameplayTapeStatus = parsed.status;
  window.gameplayTapeReasonCode = parsed.reasonCode;
  window.gameplayTapeLineCount = parsed.lineCount;
  window.gameplayTapeStepCount =
      static_cast<std::uint64_t>(parsed.tape.steps.size());
  window.gameplayTapeFailedStep = failedTapeStepReceiptValue(parsed.failedLine);
  window.gameplayTapeFailedSourceLine = parsed.failedLine;
  window.gameplayTapeFailedAction = "none";
  window.gameplayTapeFailedTarget = parsed.failedToken;
  window.gameplayTapeFailedRejection = "none";
}

void recordProductGameplayTapeRun(const ProductGameplayTapeRunResult& run,
                                  ProductAppWindowState& window) {
  window.gameplayTapeStatus = run.status;
  window.gameplayTapeReasonCode = run.reasonCode;
  window.gameplayTapeStepCount = run.stepCount;
  window.gameplayTapeExecutedStepCount = run.executedStepCount;
  window.gameplayTapeExpectedRejectedStepCount = run.expectedRejectedStepCount;
  window.gameplayTapeExpectedBlockedStepCount = run.expectedBlockedStepCount;
  window.gameplayTapeFailedStep = failedTapeStepReceiptValue(run.failedStepIndex);
  window.gameplayTapeFailedSourceLine = run.failedSourceLine;
  window.gameplayTapeFailedAction = run.failedAction;
  window.gameplayTapeFailedTarget = run.failedTarget;
  window.gameplayTapeFailedRejection = run.failedRejection;
  window.gameplayTapeFailedMovementBlock = run.failedMovementBlock;
  window.gameplayTapeLastAction = run.lastAction;
  window.gameplayTapeLastTarget = run.lastTarget;
  window.gameplayTapeLastMovementBlock = run.lastMovementBlock;
  window.gameplayTapeKeyCollected = run.keyCollected;
  window.gameplayTapeSecretDoorOpened = run.secretDoorOpened;
  window.gameplayTapeTreasureCollected = run.treasureCollected;
  window.gameplayTapeNpcTargetable = run.npcTargetable;
  window.gameplayTapeNpcDefeated = run.npcDefeated;
  window.gameplayTapeExitObjectiveComplete = run.exitObjectiveComplete;
  window.gameplayTapeLoopComplete = run.loopComplete;
  window.gameplayTapeAiCommandLogged = run.aiCommandLogged;
  window.gameplayTapeAiAttackLogged = run.aiAttackLogged;
  window.gameplayTapeAiWaitLogged = run.aiWaitLogged;
  window.gameplayTapeAiPlayerDamaged = run.aiPlayerDamaged;
  window.gameplayTapeAiPlayerHpBefore = run.aiPlayerHpBefore;
  window.gameplayTapeAiPlayerHpAfter = run.aiPlayerHpAfter;
  window.gameplayTapeAiActorId = run.aiActorId;
  window.gameplayTapeAiTargetId = run.aiTargetId;
  window.gameplayTapeAiBehavior = run.aiBehavior;
  window.gameplayTapeAiIntent = run.aiIntent;
  window.sessionOutcome = run.sessionOutcome;
  window.runtimeStateHash = run.runtimeStateHash;
  if (!run.ok) {
    window.status = "gameplay_tape_failed";
  }
}

void runProductGameplayTapeFromOptions(const ProductAppOptions& options,
                                       std::optional<Session>& activeSession,
                                       ProductAppWindowState& window) {
  if (options.gameplayTapePath.empty()) {
    return;
  }

  window.gameplayTapeRequested = true;
  window.gameplayTapePath = options.gameplayTapePath.generic_string();
  const ProductGameplayTapeParseResult parsed =
      loadProductGameplayTapeFile(options.gameplayTapePath);
  recordProductGameplayTapeParse(parsed, window);
  if (!parsed.ok) {
    window.status = "gameplay_tape_parse_failed";
    return;
  }

  const ProductGameplayTapeRunResult run = runProductGameplayTape(
      ProductGameplayTapeRunRequest{activeSession.has_value() ? &*activeSession : nullptr,
                                    &parsed.tape,
                                    productActiveRoomCollisionSurfaces(
                                        window.activeRoomCollision),
                                    &window.activeRoom,
                                    &window.activeRoomCollision});
  recordProductGameplayTapeRun(run, window);
}

bool routeAutomationInput(FrontendState& frontend,
                          const ProductSaveBridgeResult& saves,
                          const ProductAppOptions& options,
                          FrontendSettingsTab& settingsTab,
                          std::optional<Session>& activeSession,
                          WorldSetupDraft& worldSetupDraft,
                          ProductAppWindowState& window,
                          InputAction action,
                          bool& closeRequested) {
  ActionState actionState;
  ProductOpeningMenuInputContext menuContext{
      frontend, saves, options, settingsTab, activeSession, worldSetupDraft,
      window, closeRequested};
  routeProductOpeningMenuInput(action, actionState, menuContext);
  window.automationControlLastOwner = productInputOwnerFor(frontend, window);
  return window.lastInputAccepted || action == InputAction::None;
}

bool applyProductAutomationCommand(const ProductAutomationCommand& command,
                                   FrontendState& frontend,
                                   const ProductSaveBridgeResult& saves,
                                   const ProductAppOptions& options,
                                   FrontendSettingsTab& settingsTab,
                                   std::optional<Session>& activeSession,
                                   WorldSetupDraft& worldSetupDraft,
                                   ProductAppWindowState& window,
                                   bool& closeRequested) {
  ProductAutomationDispatchContext dispatchContext{
      frontend, saves, options, settingsTab,
      // branch-gate: BG-1016
      activeSession.has_value() ? &*activeSession : nullptr,
      worldSetupDraft, window,
      [&frontend, &window]() { return productInputOwnerFor(frontend, window); },
      [&frontend, &saves, &options, &settingsTab, &activeSession, &worldSetupDraft,
       &window, &closeRequested](InputAction action) {
        return routeAutomationInput(frontend, saves, options, settingsTab,
                                    activeSession, worldSetupDraft, window, action,
                                    closeRequested);
      },
      [&activeSession, &window, &frontend]() {
        const ProductAsciiRoomActivationResult activated =
            activateProductAsciiRoomPreview(activeSession, window);
        // branch-gate: BG-1016
        if (activated.ok) {
          enterProductGameplayTransition(frontend, window,
                                         FrontendAction::CreateAndEnter);
        }
        return activated.ok;
      },
      [&window](InputAction action) {
        InputRoutingContext routingContext;
        routingContext.owners.editor = window.roomEditing.ready;
        routingContext.owners.gameplay = true;
        return routeInputAction(routingContext, action);
      },
      [&frontend, &window, &activeSession]() {
        returnProductToTitleTransition(frontend, window);
        activeSession.reset();
      },
      [&closeRequested]() { closeRequested = true; },
  };
  return applyProductAutomationCommand(command, dispatchContext);
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
        return applyProductAutomationCommand(command, frontend, saves, options,
                                             automationSettingsTab, activeSession,
                                             worldSetupDraft, window,
                                             automationCloseRequested);
      },
      [&frontend, &window]() { return productInputOwnerFor(frontend, window); },
      [&frontend](MenuOwner owner) {
        return frontendBlocksGameplayInput(frontend) || menuOwnerBlocksGameplay(owner);
      },
  };
  applyProductAutomationControl(automationControlContext);
  if (!automationCloseRequested) {
    runProductGameplayTapeFromOptions(options, activeSession, window);
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
                                              settings.debugOverlayEnabled});

  if (options.printRenderReceipt) {
    std::cout << formatRenderReceipt(
        buildProductAppReceipt(options, world, frontend, settings, window, saves));
  }

  return window.requested && !window.created ? 77 : 0;
}

}  // namespace iggy3d
