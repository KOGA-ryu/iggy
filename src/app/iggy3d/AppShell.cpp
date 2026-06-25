#include "app/iggy3d/AppShell.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/ProductAppOperations.hpp"
#include "app/iggy3d/ProductCameraController.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductAsciiRoomActivation.hpp"
#include "app/iggy3d/ProductAsciiRoomPreview.hpp"
#include "app/iggy3d/ProductGameplayController.hpp"
#include "app/iggy3d/ProductGameplayFeedback.hpp"
#include "app/iggy3d/ProductGameplayTape.hpp"
#include "app/iggy3d/ProductGameplayTapeRunner.hpp"
#include "app/iggy3d/ProductMenuTransitions.hpp"
#include "app/iggy3d/ProductMovementDebugHud.hpp"
#include "app/iggy3d/ProductNpcBehaviorDebugHud.hpp"
#include "app/iggy3d/ProductPrimitiveDrawList.hpp"
#include "app/iggy3d/ProductRenderBridge.hpp"
#include "app/iggy3d/ProductScriptedGameplayDriver.hpp"
#include "app/iggy3d/ProductViewportFraming.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/SaveBridge.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "app/input/GamepadInput.hpp"
#include "app/input/KeyboardInput.hpp"
#include "app/input/MouseInput.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/RenderDiagnostics.hpp"
#include "runtime/ai/NpcBehaviorDebugSnapshot.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/session/Session.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <chrono>
#include <thread>

#include <SDL3/SDL.h>

#include "app/iggy3d/OpeningMenuView.hpp"
#include "app/platform/SdlWindow.hpp"
#endif

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

DebugProjectionResult buildProductDebugProjectionWithNpcBehavior(
    const SessionState& state) {
  DebugProjectionResult debug = buildDebugProjection(state);
  const NpcBehaviorProfileCatalog catalog =
      makeBuiltInNpcBehaviorProfileCatalog();
  const NpcBehaviorDebugSnapshot snapshot = buildNpcBehaviorDebugSnapshot(
      {true, &state.world, &state.ai, &state.combat, &catalog,
       state.clock.tickIndex, 128});
  appendNpcBehaviorDebugSnapshot(debug, snapshot);
  return debug;
}

bool npcBehaviorHudHasUnresolvedProfile(const ProductNpcBehaviorDebugHud& hud) {
  for (const ProductNpcBehaviorDebugHudLine& line : hud.lines) {
    if (line.text.find("unresolved=") != std::string::npos) {
      return true;
    }
  }
  return false;
}

void copyNpcBehaviorDebugHud(ProductAppWindowState& window,
                             const ProductNpcBehaviorDebugHud& hud) {
  window.npcBehaviorDebugHudVisible = hud.visible;
  window.npcBehaviorDebugHudDebugAvailable = hud.debugAvailable;
  window.npcBehaviorDebugHudLineCount = static_cast<std::uint64_t>(hud.lineCount);
  window.npcBehaviorDebugHudStatus = hud.status;
  window.npcBehaviorDebugHudReasonCode = hud.reasonCode;
  window.npcBehaviorDebugHudHasUnresolvedProfile =
      npcBehaviorHudHasUnresolvedProfile(hud);
}

void applyGameplayProjectionMetrics(ProductAppWindowState& window,
                                    const SceneProjectionResult* scene,
                                    const DebugProjectionResult* debug,
                                    const ProductPrimitiveDrawList* drawList,
                                    const ProductViewportFrame* frame,
                                    const ProductRenderBridgeFrame* bridge,
                                    bool viewVisible) {
  if (!window.gameplayActive || scene == nullptr) {
    window.viewport.gameplayViewVisible = false;
    window.viewport.productDrawGridVisible = false;
    window.viewport.productDrawPlayerVisible = false;
    window.viewport.productDrawRoomVisible = false;
    window.viewport.productDrawObjectiveVisible = false;
    window.viewport.productDrawTargetIndicatorVisible = false;
    window.viewport.productDrawDoorVisible = false;
    window.viewport.productDrawOpenDoorVisible = false;
    window.viewport.productDrawClosedDoorVisible = false;
    window.viewport.productDrawItemCount = 0;
    window.viewport.productDrawDebugMarkerCount = 0;
    window.viewport.productDrawDoorCount = 0;
    window.viewport.productDrawOpenDoorCount = 0;
    window.viewport.productDrawClosedDoorCount = 0;
    window.viewport.productDrawRoomGeometryCount = 0;
    window.viewport.productDrawFloorTileCount = 0;
    window.viewport.productDrawElevatedFloorTileCount = 0;
    window.viewport.productDrawRampTileCount = 0;
    window.viewport.productDrawBlockedSlopeTileCount = 0;
    window.viewport.productDrawWallTileCount = 0;
    window.viewport.productViewProjection = "primitive_first_person";
    window.viewport.productViewYawApplied = false;
    window.viewport.productViewPitchApplied = false;
    window.viewport.productViewPlayerAnchorFound = false;
    window.viewport.productRenderBridgeReady = false;
    window.viewport.productViewFrameReady = false;
    window.viewport.productViewFrameItemCount = 0;
    window.viewport.productViewFrameOnScreenItemCount = 0;
    window.viewport.productViewFrameTargetItemCount = 0;
    window.viewport.productFeedbackBridgeReady = false;
    window.viewport.productFeedbackBridgeLineCount = 0;
    window.sceneItemCount = 0;
    window.debugItemCount = 0;
    window.playerVisible = false;
    window.roomVisible = false;
    window.objectiveVisible = false;
    window.rendererMutatedRuntime = false;
    return;
  }

  window.viewport.gameplayViewVisible = viewVisible;
  window.sceneItemCount = static_cast<std::uint64_t>(scene->items.size());
  window.debugItemCount =
      debug == nullptr ? 0U : static_cast<std::uint64_t>(debug->items.size());
  window.playerVisible = scene->playerCount > 0;
  window.roomVisible = true;
  window.objectiveVisible =
      scene->pickupCount > 0 || scene->interactableCount > 0 || scene->markerCount > 0 ||
      scene->room.loaded;
  window.rendererMutatedRuntime = false;
  if (drawList != nullptr) {
    window.viewport.productDrawGridVisible = drawList->gridVisible;
    window.viewport.productDrawPlayerVisible = drawList->playerVisible;
    window.viewport.productDrawRoomVisible = drawList->roomVisible;
    window.viewport.productDrawObjectiveVisible = drawList->objectiveVisible;
    window.viewport.productDrawTargetIndicatorVisible =
        drawList->playerFocusIndicatorVisible;
    window.viewport.productDrawItemCount = drawList->itemCount;
    window.viewport.productDrawDebugMarkerCount = drawList->debugMarkerCount;
    window.viewport.productDrawDoorVisible = drawList->doorVisible;
    window.viewport.productDrawOpenDoorVisible = drawList->openDoorVisible;
    window.viewport.productDrawClosedDoorVisible = drawList->closedDoorVisible;
    window.viewport.productDrawDoorCount = drawList->doorMarkerCount;
    window.viewport.productDrawOpenDoorCount = drawList->openDoorMarkerCount;
    window.viewport.productDrawClosedDoorCount = drawList->closedDoorMarkerCount;
    window.viewport.productDrawRoomGeometryCount = drawList->roomGeometryCount;
    window.viewport.productDrawFloorTileCount = drawList->floorTileCount;
    window.viewport.productDrawElevatedFloorTileCount =
        drawList->elevatedFloorTileCount;
    window.viewport.productDrawRampTileCount = drawList->rampTileCount;
    window.viewport.productDrawBlockedSlopeTileCount =
        drawList->blockedSlopeTileCount;
    window.viewport.productDrawWallTileCount = drawList->wallTileCount;
  }
  if (frame != nullptr) {
    window.viewport.productViewProjection = frame->projectionMode;
    window.viewport.productViewYawApplied = frame->yawApplied;
    window.viewport.productViewPitchApplied = frame->pitchApplied;
    window.viewport.productViewPlayerAnchorFound = frame->playerAnchorFound;
  }
  if (bridge != nullptr) {
    window.viewport.productRenderBridgeReady = bridge->ready;
    window.viewport.productViewFrameReady = bridge->viewFrameReady;
    window.viewport.productViewFrameItemCount = bridge->frameItemCount;
    window.viewport.productViewFrameOnScreenItemCount = bridge->onScreenItemCount;
    window.viewport.productViewFrameTargetItemCount = bridge->targetItemCount;
    window.viewport.productFeedbackBridgeReady = bridge->feedbackReady;
    window.viewport.productFeedbackBridgeLineCount = bridge->feedbackLineCount;
  }
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

void refreshGameplayProjectionMetrics(const std::optional<Session>& activeSession,
                                      ProductAppWindowState& window,
                                      bool developerToolsEnabled,
                                      bool debugOverlayEnabled) {
  if (!window.gameplayActive || !activeSession.has_value()) {
    window.sessionOutcome = "None";
    copyNpcBehaviorDebugHud(window,
                            buildProductNpcBehaviorDebugHud(nullptr,
                                                            window.gameplayActive,
                                                            developerToolsEnabled,
                                                            debugOverlayEnabled));
    applyGameplayProjectionMetrics(window, nullptr, nullptr, nullptr, nullptr, nullptr,
                                   false);
    return;
  }

  const SceneProjectionResult scene = buildSceneProjection(activeSession->state());
  const DebugProjectionResult debug =
      buildProductDebugProjectionWithNpcBehavior(activeSession->state());
  copyNpcBehaviorDebugHud(window,
                          buildProductNpcBehaviorDebugHud(&debug,
                                                          window.gameplayActive,
                                                          developerToolsEnabled,
                                                          debugOverlayEnabled));
  const RoomAsset* activeRoom =
      window.activeRoom.loaded ? &window.activeRoom.room : nullptr;
  const ProductPrimitiveDrawList drawList =
      buildProductPrimitiveDrawList(&scene, &debug, activeRoom,
                                    &window.activeRoomCollision);
  const ProductViewportFrame frame = buildProductViewportFrame(
      drawList, ProductViewportFrameConfig{window.viewport.cameraYawDegrees,
                                           window.viewport.cameraPitchDegrees});
  const ProductGameplayFeedback feedback = buildProductGameplayFeedback(window);
  const ProductRenderBridgeFrame bridge =
      buildProductRenderBridgeFrame(&drawList, &frame, &feedback);
  window.runtimeStateHash = activeSession->stateHash();
  window.sessionOutcome =
      std::string(productGameplayTapeSessionOutcomeName(activeSession->state().outcome));
  applyGameplayProjectionMetrics(window, &scene, &debug, &drawList, &frame, &bridge,
                                 true);
}

FrontendAction nextStarterSelection(FrontendAction current, InputAction action) {
  const std::array<FrontendAction, 6> actions = {
      FrontendAction::Continue,
      FrontendAction::NewWorld,
      FrontendAction::LoadSave,
      FrontendAction::Settings,
      FrontendAction::DevTools,
      FrontendAction::Exit,
  };

  std::size_t index = 0;
  for (std::size_t i = 0; i < actions.size(); ++i) {
    if (actions[i] == current) {
      index = i;
      break;
    }
  }

  if (action == InputAction::MenuUp) {
    index = index == 0 ? actions.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % actions.size();
  }
  return actions[index];
}

FrontendDevToolsCategory nextDevToolsSelection(FrontendDevToolsCategory current,
                                               InputAction action) {
  const auto& categories = devToolsCategoryOrder();
  std::size_t index = 0;
  for (std::size_t i = 0; i < categories.size(); ++i) {
    if (categories[i] == current) {
      index = i;
      break;
    }
  }
  if (action == InputAction::MenuUp) {
    index = index == 0 ? categories.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % categories.size();
  }
  return categories[index];
}

FrontendSettingsTab nextSettingsSelection(FrontendSettingsTab current, InputAction action) {
  const auto& tabs = settingsTabOrder();
  std::size_t index = 0;
  for (std::size_t i = 0; i < tabs.size(); ++i) {
    if (tabs[i] == current) {
      index = i;
      break;
    }
  }
  if (action == InputAction::MenuUp) {
    index = index == 0 ? tabs.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % tabs.size();
  }
  return tabs[index];
}

FrontendAction nextPauseSelection(FrontendAction current, InputAction action) {
  const auto& actions = pauseActionOrder();
  std::size_t index = 0;
  for (std::size_t i = 0; i < actions.size(); ++i) {
    if (actions[i] == current) {
      index = i;
      break;
    }
  }
  if (action == InputAction::MenuUp) {
    index = index == 0 ? actions.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % actions.size();
  }
  return actions[index];
}

MenuOwner productInputOwnerFor(const FrontendState& frontend,
                               const ProductAppWindowState& window) {
  if (frontend.screen == FrontendScreen::Settings ||
      frontend.childScreen == FrontendScreen::Settings) {
    return MenuOwner::Settings;
  }
  if (frontend.screen == FrontendScreen::DevOverlay ||
      frontend.childScreen == FrontendScreen::StarterDevTools) {
    return MenuOwner::DevTools;
  }
  if (frontend.screen == FrontendScreen::Starter) {
    return MenuOwner::Starter;
  }
  if (frontend.screen == FrontendScreen::Pause) {
    return MenuOwner::Pause;
  }
  if (frontend.screen == FrontendScreen::Gameplay && window.gameplayActive) {
    return MenuOwner::Gameplay;
  }
  return MenuOwner::None;
}

void recordWorldSetupDraftState(const WorldSetupDraft& draft,
                                ProductAppWindowState& window) {
  window.worldSetupTitle = draft.worldName;
  window.worldSetupAsciiRoomEnabled = draft.asciiRoomEnabled;
  window.worldSetupAsciiRoomTextPresent = !draft.asciiRoomText.empty();
  window.worldSetupAsciiRoomId =
      draft.asciiRoomId.empty() ? "none" : draft.asciiRoomId;
  window.worldSetupAsciiRoomSourceName =
      draft.asciiRoomSourceName.empty() ? "none" : draft.asciiRoomSourceName;
}

void applyOpeningMenuAction(FrontendState& frontend,
                            const ProductSaveBridgeResult& saves,
                            const ProductAppOptions& options,
                            FrontendSettingsTab& settingsTab,
                            std::optional<Session>& activeSession,
                            WorldSetupDraft& worldSetupDraft,
                            ProductAppWindowState& window,
                            InputAction action,
                            bool& closeRequested) {
  if (action == InputAction::SystemPause) {
    if (frontend.screen == FrontendScreen::Gameplay && window.gameplayActive) {
      openProductPauseTransition(frontend, window, FrontendAction::Resume);
      frontend.status = "pause_opened_from_gameplay";
      return;
    }
    if (frontend.screen == FrontendScreen::Pause) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (frontend.screen == FrontendScreen::DevOverlay) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (frontend.screen == FrontendScreen::Settings &&
        frontend.childScreen == FrontendScreen::Pause) {
      openProductPauseTransition(frontend, window, FrontendAction::Settings);
      return;
    }
    frontend.status = "opening_menu_pause_back_requested";
    closeRequested = true;
    return;
  }

  if (frontend.screen == FrontendScreen::Pause) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      frontend.selectedAction = nextPauseSelection(frontend.selectedAction, action);
      frontend.status = "pause_menu_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (action != InputAction::MenuConfirm) {
      return;
    }
    if (frontend.selectedAction == FrontendAction::Resume) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (frontend.selectedAction == FrontendAction::Settings) {
      openProductPauseSettingsTransition(frontend, window, settingsTab);
      return;
    }
    if (frontend.selectedAction == FrontendAction::DevTools) {
      openProductPauseDevToolsTransition(frontend, window,
                                         FrontendDevToolsCategory::Session);
      return;
    }
    if (frontend.selectedAction == FrontendAction::Save) {
      const ProductSaveWriteResult written =
          writeProductCurrentSessionSave(options, activeSession, "pause_save", window);
      frontend.status = written.ok ? "pause_save_written" : "pause_save_failed";
      window.launchStatus = written.ok ? "pause_save_written" : written.reasonCode;
      return;
    }
    if (frontend.selectedAction == FrontendAction::SaveAndExit) {
      const ProductSaveWriteResult written = writeProductCurrentSessionSave(
          options, activeSession, "pause_save_and_exit", window);
      frontend.status = written.ok ? "pause_save_and_exit_written"
                                   : "pause_save_and_exit_failed";
      window.launchStatus =
          written.ok ? "pause_save_and_exit_written" : written.reasonCode;
      if (written.ok) {
        returnProductToTitleTransition(frontend, window);
        activeSession.reset();
      }
      return;
    }
    if (frontend.selectedAction == FrontendAction::ReturnToTitle) {
      returnProductToTitleTransition(frontend, window);
      activeSession.reset();
      return;
    }
    if (frontend.selectedAction == FrontendAction::ExitGame) {
      frontend.status = "pause_exit_game_requested";
      closeRequested = true;
      return;
    }
    frontend.status = "pause_action_selected";
    return;
  }

  if (frontend.screen == FrontendScreen::DevOverlay) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      frontend.devToolsCategory = nextDevToolsSelection(frontend.devToolsCategory, action);
      frontend.status = "dev_overlay_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (action == InputAction::MenuConfirm) {
      frontend.status = "dev_overlay_category_selected";
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::StarterDevTools) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      frontend.devToolsCategory = nextDevToolsSelection(frontend.devToolsCategory, action);
      frontend.status = "dev_tools_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      frontend.childScreen = FrontendScreen::Gameplay;
      frontend.devToolsOpen = false;
      frontend.status = "dev_tools_closed";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      frontend.status = "dev_tools_category_selected";
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::Settings) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      settingsTab = nextSettingsSelection(settingsTab, action);
      frontend.status = "settings_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      if (frontend.screen == FrontendScreen::Settings &&
          frontend.childScreen == FrontendScreen::Pause) {
        openProductPauseTransition(frontend, window, FrontendAction::Settings);
      } else {
        frontend.childScreen = FrontendScreen::Gameplay;
      }
      frontend.status = "settings_closed";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      frontend.status = "settings_tab_selected";
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::DeleteConfirm &&
      window.saveDeleteConfirmationOpen) {
    if (action == InputAction::MenuBack) {
      cancelProductSaveDeleteConfirmation(window, frontend);
      return;
    }
    if (action == InputAction::MenuConfirm) {
      executeProductSaveSoftDelete(options, window, frontend);
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::NewWorld) {
    recordWorldSetupDraftState(worldSetupDraft, window);
    if (action == InputAction::MenuBack) {
      frontend.childScreen = FrontendScreen::Gameplay;
      frontend.selectedAction = FrontendAction::NewWorld;
      frontend.status = "new_world_closed";
      window.worldSetupStatus = "world_setup_back";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      launchProductNewWorld(options, worldSetupDraft, frontend, activeSession, window);
      return;
    }
    frontend.status = "new_world_input_ignored";
    return;
  }

  if (frontend.childScreen == FrontendScreen::LoadSave) {
    if (action == InputAction::MenuBack) {
      frontend.childScreen = FrontendScreen::Gameplay;
      frontend.status = "load_save_closed";
      return;
    }
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      moveSelectedProductSaveSlot(saves.slots, action, window);
      frontend.status = "load_save_selection_changed";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      if (frontend.selectedAction == FrontendAction::Delete) {
        openProductSaveDeleteConfirmation(saves.slots, window, frontend);
      } else {
        launchProductLoadSaveSelection(options,
                                       productWorldTemplateFromOptions(options),
                                       saves,
                                       frontend,
                                       activeSession,
                                       window);
      }
      return;
    }
  }

  if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
    frontend.selectedAction = nextStarterSelection(frontend.selectedAction, action);
    frontend.status = "opening_menu_selection_changed";
    return;
  }
  if (action == InputAction::MenuBack) {
    frontend.status = "opening_menu_back_requested";
    closeRequested = true;
    return;
  }
  if (action != InputAction::MenuConfirm) {
    return;
  }

  if (frontend.selectedAction == FrontendAction::Continue) {
    if (saves.slots.compatibleCount == 0) {
      frontend.status = "opening_menu_action_disabled";
      return;
    }
    launchProductContinueSave(options, productWorldTemplateFromOptions(options), saves,
                              frontend, activeSession, window);
    return;
  }
  if (frontend.selectedAction == FrontendAction::Exit) {
    frontend.status = "opening_menu_exit_requested";
    closeRequested = true;
    return;
  }
  if (frontend.selectedAction == FrontendAction::NewWorld) {
    frontend.childScreen = FrontendScreen::NewWorld;
    frontend.selectedAction = FrontendAction::CreateAndEnter;
    frontend.status = "opening_menu_new_world_selected";
    recordWorldSetupDraftState(worldSetupDraft, window);
    window.worldSetupStatus = "world_setup_open";
    return;
  }
  if (frontend.selectedAction == FrontendAction::LoadSave) {
    frontend.childScreen = FrontendScreen::LoadSave;
    initializeSelectedProductSaveSlot(saves.slots, window);
    frontend.status = "opening_menu_load_save_selected";
    return;
  }
  if (frontend.selectedAction == FrontendAction::Settings) {
    frontend.childScreen = FrontendScreen::Settings;
    settingsTab = FrontendSettingsTab::Input;
    frontend.status = "opening_menu_settings_selected";
    return;
  }
  if (frontend.selectedAction == FrontendAction::DevTools) {
    frontend.childScreen = FrontendScreen::StarterDevTools;
    frontend.devToolsOpen = true;
    frontend.devToolsCategory = FrontendDevToolsCategory::Session;
    frontend.status = "opening_menu_dev_tools_selected";
    return;
  }
  frontend.status = "opening_menu_action_selected";
}

void routeOpeningMenuInput(FrontendState& frontend,
                           const ProductSaveBridgeResult& saves,
                           const ProductAppOptions& options,
                           FrontendSettingsTab& settingsTab,
                           std::optional<Session>& activeSession,
                           WorldSetupDraft& worldSetupDraft,
                           ActionState& actionState,
                           InputAction inputAction,
                           ProductAppWindowState& window,
                           bool& closeRequested) {
  if (inputAction == InputAction::None) {
    return;
  }

  if (frontend.screen == FrontendScreen::Gameplay &&
      inputAction == InputAction::MenuBack) {
    inputAction = InputAction::SystemPause;
  }

  recordAction(actionState, inputAction, true, true, false, 1.0F);

  InputRoutingContext routingContext;
  routingContext.owners.starter = frontend.screen == FrontendScreen::Starter;
  routingContext.owners.pause = frontend.screen == FrontendScreen::Pause;
  routingContext.owners.settings = frontend.screen == FrontendScreen::Settings ||
                                   frontend.childScreen == FrontendScreen::Settings;
  routingContext.owners.devTools = frontend.screen == FrontendScreen::DevOverlay ||
                                   frontend.childScreen == FrontendScreen::StarterDevTools;
  routingContext.owners.gameplay = frontend.screen == FrontendScreen::Gameplay &&
                                   window.gameplayActive;
  const InputRoutingResult routed = routeInputAction(routingContext, inputAction);
  window.inputOwner = routed.owner;
  window.lastInputAction = routed.action;
  window.lastInputAccepted = routed.accepted;
  window.gameplayInputSuppressed = routed.gameplaySuppressed;
  if (routed.accepted) {
    applyOpeningMenuAction(frontend, saves, options, settingsTab, activeSession,
                           worldSetupDraft, window, routed.action, closeRequested);
  }
}

struct ProductAutomationCommand {
  std::string key;
  std::string value;
};

bool parseAutomationBool(std::string_view value, bool& out) {
  if (value == "true" || value == "1" || value == "yes") {
    out = true;
    return true;
  }
  if (value == "false" || value == "0" || value == "no") {
    out = false;
    return true;
  }
  return false;
}

bool parseAutomationFloat(std::string_view value, float& out) {
  if (value.empty()) {
    return false;
  }
  const auto [ptr, error] =
      std::from_chars(value.data(), value.data() + value.size(), out);
  return error == std::errc{} && ptr == value.data() + value.size() &&
         std::isfinite(out);
}

bool parseAutomationInputAction(std::string_view value, InputAction& out) {
  if (value == "up") {
    out = InputAction::MenuUp;
  } else if (value == "down") {
    out = InputAction::MenuDown;
  } else if (value == "left") {
    out = InputAction::MenuLeft;
  } else if (value == "right") {
    out = InputAction::MenuRight;
  } else if (value == "confirm") {
    out = InputAction::MenuConfirm;
  } else if (value == "back") {
    out = InputAction::MenuBack;
  } else if (value == "next_tab") {
    out = InputAction::MenuNextTab;
  } else if (value == "previous_tab") {
    out = InputAction::MenuPreviousTab;
  } else if (value == "none") {
    out = InputAction::None;
  } else {
    return false;
  }
  return true;
}

bool parseAutomationFrontendAction(std::string_view value, FrontendAction& out) {
  if (value == "continue") {
    out = FrontendAction::Continue;
  } else if (value == "new_world") {
    out = FrontendAction::NewWorld;
  } else if (value == "load_save") {
    out = FrontendAction::LoadSave;
  } else if (value == "settings") {
    out = FrontendAction::Settings;
  } else if (value == "dev_tools") {
    out = FrontendAction::DevTools;
  } else if (value == "exit") {
    out = FrontendAction::Exit;
  } else if (value == "resume") {
    out = FrontendAction::Resume;
  } else if (value == "save") {
    out = FrontendAction::Save;
  } else if (value == "save_and_exit") {
    out = FrontendAction::SaveAndExit;
  } else if (value == "return_to_title") {
    out = FrontendAction::ReturnToTitle;
  } else if (value == "exit_game") {
    out = FrontendAction::ExitGame;
  } else {
    return false;
  }
  return true;
}

bool parseAutomationSettingsTab(std::string_view value, FrontendSettingsTab& out) {
  if (value == "input") {
    out = FrontendSettingsTab::Input;
  } else if (value == "controls") {
    out = FrontendSettingsTab::Controls;
  } else if (value == "camera") {
    out = FrontendSettingsTab::Camera;
  } else if (value == "gameplay") {
    out = FrontendSettingsTab::Gameplay;
  } else if (value == "video_display") {
    out = FrontendSettingsTab::VideoDisplay;
  } else if (value == "audio") {
    out = FrontendSettingsTab::Audio;
  } else if (value == "accessibility") {
    out = FrontendSettingsTab::Accessibility;
  } else if (value == "developer") {
    out = FrontendSettingsTab::Developer;
  } else {
    return false;
  }
  return true;
}

bool parseAutomationDevToolsCategory(std::string_view value,
                                     FrontendDevToolsCategory& out) {
  if (value == "session") {
    out = FrontendDevToolsCategory::Session;
  } else if (value == "input") {
    out = FrontendDevToolsCategory::Input;
  } else if (value == "player") {
    out = FrontendDevToolsCategory::Player;
  } else if (value == "movement") {
    out = FrontendDevToolsCategory::Movement;
  } else if (value == "world_editor") {
    out = FrontendDevToolsCategory::WorldEditor;
  } else if (value == "collision") {
    out = FrontendDevToolsCategory::Collision;
  } else if (value == "spells") {
    out = FrontendDevToolsCategory::Spells;
  } else if (value == "camera") {
    out = FrontendDevToolsCategory::Camera;
  } else if (value == "renderer") {
    out = FrontendDevToolsCategory::Renderer;
  } else if (value == "performance") {
    out = FrontendDevToolsCategory::Performance;
  } else {
    return false;
  }
  return true;
}

bool parseAutomationOwner(std::string_view value, MenuOwner& out) {
  if (value == "starter") {
    out = MenuOwner::Starter;
  } else if (value == "pause") {
    out = MenuOwner::Pause;
  } else if (value == "settings") {
    out = MenuOwner::Settings;
  } else if (value == "dev_tools") {
    out = MenuOwner::DevTools;
  } else {
    return false;
  }
  return true;
}

bool hasAutomationKey(const std::vector<ProductAutomationCommand>& commands,
                      const std::string& key) {
  for (const ProductAutomationCommand& command : commands) {
    if (command.key == key) {
      return true;
    }
  }
  return false;
}

bool readProductAutomationCommands(const std::filesystem::path& path,
                                   ProductAppWindowState& window,
                                   std::vector<ProductAutomationCommand>& commands) {
  window.automationControlRequested = !path.empty();
  window.automationControlPath = path.empty() ? "" : path.generic_string();
  if (path.empty()) {
    return false;
  }
  window.automationControlScope = "frontend_menu";
  std::ifstream input(path);
  if (!input) {
    window.automationControlStatus = "read_failed";
    window.automationControlLastResult = "failed";
    return false;
  }

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    ++window.automationControlLineCount;
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos || equals == 0U) {
      window.automationControlStatus = "parse_error";
      window.automationControlLastKey = "none";
      window.automationControlLastResult = "failed";
      return false;
    }
    ProductAutomationCommand command{line.substr(0, equals), line.substr(equals + 1U)};
    if (hasAutomationKey(commands, command.key)) {
      window.automationControlStatus = "duplicate_key";
      window.automationControlLastKey = command.key;
      window.automationControlLastResult = "failed";
      return false;
    }
    commands.push_back(std::move(command));
  }
  window.automationControlLoaded = true;
  window.automationControlStatus = "loaded";
  return true;
}

void markAutomationApplied(ProductAppWindowState& window,
                           const ProductAutomationCommand& command,
                           std::string_view action,
                           MenuOwner owner,
                           std::string_view result) {
  window.automationControlLastKey = command.key;
  window.automationControlLastAction = std::string(action);
  window.automationControlLastOwner = owner;
  window.automationControlLastResult = std::string(result);
  if (result == "applied") {
    ++window.automationControlAppliedCount;
    window.automationControlStatus = "applied";
  }
}

bool automationFailurePreservesLoaded(std::string_view status) {
  return status == "applied" || status == "loaded";
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
  routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession,
                        worldSetupDraft, actionState, action, window, closeRequested);
  window.automationControlLastOwner = productInputOwnerFor(frontend, window);
  return window.lastInputAccepted || action == InputAction::None;
}

bool applyAutomationGameplayAxis(InputAction action,
                                 float value,
                                 FrontendState& frontend,
                                 std::optional<Session>& activeSession,
                                 ProductAppWindowState& window) {
  if (frontend.screen != FrontendScreen::Gameplay || !window.gameplayActive ||
      !activeSession.has_value()) {
    window.automationControlStatus = "owner_unavailable";
    return false;
  }

  ActionState actions;
  recordAction(actions, action, true, false, false, value);

  InputRoutingContext routingContext;
  routingContext.owners.gameplay = true;
  const InputRoutingResult routed = routeInputAction(routingContext, action);
  window.inputOwner = routed.owner;
  window.lastInputAction = routed.action;
  window.lastInputAccepted = routed.accepted;
  window.gameplayInputSuppressed = routed.gameplaySuppressed;
  if (!routed.accepted) {
    return false;
  }

  applyProductGameplayActions(
      *activeSession, actions, window, "automation",
      productActiveRoomCollisionSurfaces(window.activeRoomCollision));
  return window.gameplayCommandSubmitted && window.gameplayCommandAccepted &&
         window.gameplayTickAdvanced;
}

bool applyAutomationGameplayButton(InputAction action,
                                   FrontendState& frontend,
                                   std::optional<Session>& activeSession,
                                   ProductAppWindowState& window) {
  if (frontend.screen != FrontendScreen::Gameplay || !window.gameplayActive ||
      !activeSession.has_value()) {
    window.automationControlStatus = "owner_unavailable";
    return false;
  }

  ActionState actions;
  recordAction(actions, action, true, true, false, 1.0F);

  InputRoutingContext routingContext;
  routingContext.owners.gameplay = true;
  const InputRoutingResult routed = routeInputAction(routingContext, action);
  window.inputOwner = routed.owner;
  window.lastInputAction = routed.action;
  window.lastInputAccepted = routed.accepted;
  window.gameplayInputSuppressed = routed.gameplaySuppressed;
  if (!routed.accepted) {
    return false;
  }

  applyProductGameplayActions(
      *activeSession, actions, window, "automation",
      productActiveRoomCollisionSurfaces(window.activeRoomCollision));
  return window.gameplayCommandSubmitted && window.gameplayCommandAccepted &&
         window.gameplayTickAdvanced;
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
  const std::string_view key{command.key};
  const std::string_view value{command.value};
  bool boolValue = false;
  InputAction inputAction = InputAction::None;

  if (key == "automation.owner") {
    MenuOwner expectedOwner = MenuOwner::None;
    if (!parseAutomationOwner(value, expectedOwner)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    const MenuOwner currentOwner = productInputOwnerFor(frontend, window);
    if (currentOwner != expectedOwner) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, menuOwnerName(expectedOwner), currentOwner,
                            "failed");
      return false;
    }
    markAutomationApplied(window, command, menuOwnerName(expectedOwner), currentOwner,
                          "applied");
    return true;
  }

  if (key == "menu.input") {
    if (!parseAutomationInputAction(value, inputAction)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            inputAction, closeRequested);
    markAutomationApplied(window, command, inputActionName(inputAction),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  const bool menuBoolKey =
      key == "menu.up" || key == "menu.down" || key == "menu.left" ||
      key == "menu.right" || key == "menu.confirm" || key == "menu.back" ||
      key == "menu.next_tab" || key == "menu.previous_tab";
  if (menuBoolKey) {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "none", productInputOwnerFor(frontend, window),
                            "ignored");
      return true;
    }
    if (key == "menu.up") {
      inputAction = InputAction::MenuUp;
    } else if (key == "menu.down") {
      inputAction = InputAction::MenuDown;
    } else if (key == "menu.left") {
      inputAction = InputAction::MenuLeft;
    } else if (key == "menu.right") {
      inputAction = InputAction::MenuRight;
    } else if (key == "menu.confirm") {
      inputAction = InputAction::MenuConfirm;
    } else if (key == "menu.back") {
      inputAction = InputAction::MenuBack;
    } else if (key == "menu.next_tab") {
      inputAction = InputAction::MenuNextTab;
    } else {
      inputAction = InputAction::MenuPreviousTab;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            inputAction, closeRequested);
    markAutomationApplied(window, command, inputActionName(inputAction),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (key == "frontend.select" || key == "pause.select") {
    FrontendAction action = FrontendAction::None;
    if (!parseAutomationFrontendAction(value, action)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    frontend.selectedAction = action;
    markAutomationApplied(window, command, frontendActionName(action),
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.title" || key == "world_setup.title") {
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.title",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    worldSetupDraft.worldName = std::string(value);
    recordWorldSetupDraftState(worldSetupDraft, window);
    window.worldSetupStatus = "world_setup_title_updated";
    markAutomationApplied(window, command, "world.title",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.ascii_room_text" ||
      key == "world_setup.ascii_room_text") {
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.ascii_room_text",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    worldSetupDraft.asciiRoomEnabled = true;
    worldSetupDraft.asciiRoomText =
        decodeProductAsciiRoomAutomationText(value);
    recordWorldSetupDraftState(worldSetupDraft, window);
    window.worldSetupStatus = "world_setup_ascii_room_text_updated";
    markAutomationApplied(window, command, "world.ascii_room_text",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.ascii_room_id" ||
      key == "world_setup.ascii_room_id") {
    if (frontend.childScreen != FrontendScreen::NewWorld || value.empty()) {
      window.automationControlStatus =
          value.empty() ? "invalid_value" : "owner_unavailable";
      markAutomationApplied(window, command, "world.ascii_room_id",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    worldSetupDraft.asciiRoomEnabled = true;
    worldSetupDraft.asciiRoomId = std::string(value);
    recordWorldSetupDraftState(worldSetupDraft, window);
    window.worldSetupStatus = "world_setup_ascii_room_id_updated";
    markAutomationApplied(window, command, "world.ascii_room_id",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.ascii_room_source_name" ||
      key == "world_setup.ascii_room_source_name") {
    if (frontend.childScreen != FrontendScreen::NewWorld || value.empty()) {
      window.automationControlStatus =
          value.empty() ? "invalid_value" : "owner_unavailable";
      markAutomationApplied(window, command, "world.ascii_room_source_name",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    worldSetupDraft.asciiRoomEnabled = true;
    worldSetupDraft.asciiRoomSourceName = std::string(value);
    recordWorldSetupDraftState(worldSetupDraft, window);
    window.worldSetupStatus = "world_setup_ascii_room_source_name_updated";
    markAutomationApplied(window, command, "world.ascii_room_source_name",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.create" || key == "world_setup.create") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "world.create",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.create",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            InputAction::MenuConfirm, closeRequested);
    markAutomationApplied(window, command, inputActionName(InputAction::MenuConfirm),
                          window.automationControlLastOwner,
                          routed ? "applied" : "failed");
    return routed;
  }

  if (key == "ascii_room.text" || key == "frontend.ascii_room_text") {
    window.asciiRoomDraftText = decodeProductAsciiRoomAutomationText(value);
    window.asciiRoomPreviewStatus = "ascii_room_text_updated";
    window.asciiRoomPreviewReasonCode = "ascii_room_text_updated";
    window.asciiRoomPreviewFailedStage = "not_started";
    window.asciiRoomPreviewReady = false;
    markAutomationApplied(window, command, "ascii_room.text",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "ascii_room.room_id" || key == "frontend.ascii_room_id") {
    if (value.empty()) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    window.asciiRoomDraftRoomId = std::string(value);
    markAutomationApplied(window, command, "ascii_room.room_id",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "ascii_room.source_name" ||
      key == "frontend.ascii_room_source_name") {
    if (value.empty()) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    window.asciiRoomDraftSourceName = std::string(value);
    markAutomationApplied(window, command, "ascii_room.source_name",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "ascii_room.build" || key == "frontend.ascii_room_build") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "ascii_room.build",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }

    const bool previewBuilt = buildProductAsciiRoomPreview(window);
    markAutomationApplied(window, command, "ascii_room.build",
                          productInputOwnerFor(frontend, window),
                          previewBuilt ? "applied" : "failed");
    return previewBuilt;
  }

  if (key == "ascii_room.activate" || key == "frontend.ascii_room_activate") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "ascii_room.activate",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }

    const ProductAsciiRoomActivationResult activated =
        activateProductAsciiRoomPreview(activeSession, window);
    if (activated.ok) {
      enterProductGameplayTransition(frontend, window,
                                     FrontendAction::CreateAndEnter);
    }
    markAutomationApplied(window, command, "ascii_room.activate",
                          productInputOwnerFor(frontend, window),
                          activated.ok ? "applied" : "failed");
    return activated.ok;
  }

  if (key == "game.move_x" || key == "game.move_y" ||
      key == "frontend.game_move_x" || key == "frontend.game_move_y") {
    float axisValue = 0.0F;
    if (!parseAutomationFloat(value, axisValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    const InputAction action =
        key == "game.move_x" || key == "frontend.game_move_x"
            ? InputAction::PlayerMoveX
            : InputAction::PlayerMoveY;
    const bool moved = applyAutomationGameplayAxis(action, axisValue, frontend,
                                                   activeSession, window);
    markAutomationApplied(window,
                          command,
                          inputActionName(action),
                          productInputOwnerFor(frontend, window),
                          moved ? "applied" : "failed");
    return moved;
  }

  if (key == "game.attack" || key == "frontend.game_attack" ||
      key == "game.interact" || key == "frontend.game_interact") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    const InputAction action =
        key == "game.attack" || key == "frontend.game_attack"
            ? InputAction::PlayerAttack
            : InputAction::PlayerInteract;
    if (!boolValue) {
      markAutomationApplied(window, command, inputActionName(action),
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    const bool executed =
        applyAutomationGameplayButton(action, frontend, activeSession, window);
    markAutomationApplied(window,
                          command,
                          inputActionName(action),
                          productInputOwnerFor(frontend, window),
                          executed ? "applied" : "failed");
    return executed;
  }

  if (key == "save.select" || key == "frontend.save_select") {
    if (frontend.childScreen != FrontendScreen::LoadSave) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.select",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const bool selected = selectProductSaveSlotById(saves.slots, value, window);
    markAutomationApplied(window,
                          command,
                          window.selectedProductSaveId,
                          productInputOwnerFor(frontend, window),
                          selected ? "applied" : "ignored");
    return selected;
  }

  if (key == "save.delete" || key == "frontend.save_delete") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "save.delete",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (frontend.childScreen != FrontendScreen::LoadSave) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.delete",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    openProductSaveDeleteConfirmation(saves.slots, window, frontend);
    markAutomationApplied(window, command, "save.delete",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "save.show_deleted" || key == "frontend.show_deleted_saves") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "save.show_deleted",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (frontend.childScreen != FrontendScreen::LoadSave) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.show_deleted",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    openDeletedProductSaveBrowser(options, window, frontend);
    markAutomationApplied(window, command, "save.show_deleted",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "save.deleted_select" || key == "frontend.deleted_save_select") {
    if (frontend.childScreen != FrontendScreen::LoadSave ||
        !window.deletedSaveBrowserOpen) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.deleted_select",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const ProductSaveBridgeResult deletedSaves =
        scanDeletedProductSavesForOptions(options);
    recordDeletedProductSaveSlots(deletedSaves, window);
    const bool selected =
        selectDeletedProductSaveSlotById(deletedSaves.slots, value, window);
    markAutomationApplied(window,
                          command,
                          window.deletedSelectedSaveId,
                          productInputOwnerFor(frontend, window),
                          selected ? "applied" : "ignored");
    return selected;
  }

  if (key == "save.recover" || key == "frontend.save_recover") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "save.recover",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (frontend.childScreen != FrontendScreen::LoadSave ||
        !window.deletedSaveBrowserOpen) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.recover",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    executeProductSaveRecover(options, window, frontend);
    markAutomationApplied(window, command, "save.recover",
                          productInputOwnerFor(frontend, window),
                          window.saveRecoverExecuted ? "applied" : "failed");
    return window.saveRecoverExecuted;
  }

  if (key == "settings.tab") {
    if (!parseAutomationSettingsTab(value, settingsTab)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    window.selectedSettingsTab = settingsTab;
    markAutomationApplied(window, command, frontendSettingsTabName(settingsTab),
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "dev_tools.category") {
    FrontendDevToolsCategory category = FrontendDevToolsCategory::None;
    if (!parseAutomationDevToolsCategory(value, category)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    frontend.devToolsCategory = category;
    markAutomationApplied(window, command, frontendDevToolsCategoryName(category),
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "frontend.execute" || key == "pause.execute" ||
      key == "dev_tools.execute") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "none", productInputOwnerFor(frontend, window),
                            "ignored");
      return true;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            InputAction::MenuConfirm, closeRequested);
    markAutomationApplied(window, command, inputActionName(InputAction::MenuConfirm),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (key == "settings.apply" || key == "settings.restore_defaults") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (boolValue) {
      frontend.status = key == "settings.apply" ? "settings_applied"
                                                : "settings_defaults_restored";
    }
    markAutomationApplied(window, command, key == "settings.apply" ? "settings.apply"
                                                                   : "settings.restore_defaults",
                          productInputOwnerFor(frontend, window),
                          boolValue ? "applied" : "ignored");
    return true;
  }

  if (key == "settings.back" || key == "system.back") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "none", productInputOwnerFor(frontend, window),
                            "ignored");
      return true;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            InputAction::MenuBack, closeRequested);
    markAutomationApplied(window, command, inputActionName(InputAction::MenuBack),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (key == "frontend.return_to_title") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (boolValue) {
      returnProductToTitleTransition(frontend, window);
      activeSession.reset();
    }
    markAutomationApplied(window, command, "frontend.return_to_title",
                          productInputOwnerFor(frontend, window),
                          boolValue ? "applied" : "ignored");
    return true;
  }

  if (key == "system.pause") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "none", productInputOwnerFor(frontend, window),
                            "ignored");
      return true;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            InputAction::SystemPause, closeRequested);
    markAutomationApplied(window, command, inputActionName(InputAction::SystemPause),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (key == "system.quit") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (boolValue) {
      closeRequested = true;
    }
    markAutomationApplied(window, command, "system.quit",
                          productInputOwnerFor(frontend, window),
                          boolValue ? "applied" : "ignored");
    return true;
  }

  window.automationControlStatus = "unknown_key";
  return false;
}

void applyProductAutomationControl(const ProductAppOptions& options,
                                   FrontendState& frontend,
                                   const ProductSaveBridgeResult& saves,
                                   FrontendSettingsTab& settingsTab,
                                   std::optional<Session>& activeSession,
                                   WorldSetupDraft& worldSetupDraft,
                                   ProductAppWindowState& window,
                                   bool& closeRequested) {
  std::vector<ProductAutomationCommand> commands;
  if (!readProductAutomationCommands(options.automationControlPath, window, commands)) {
    return;
  }
  for (const ProductAutomationCommand& command : commands) {
    if (!applyProductAutomationCommand(command, frontend, saves, options, settingsTab,
                                       activeSession, worldSetupDraft, window,
                                       closeRequested)) {
      if (window.automationControlLastKey == "none") {
        window.automationControlLastKey = command.key;
      }
      if (window.automationControlLastAction == "none") {
        window.automationControlLastAction = command.value.empty() ? "none" : command.value;
      }
      window.automationControlLastOwner = productInputOwnerFor(frontend, window);
      window.automationControlLastResult = "failed";
      if (automationFailurePreservesLoaded(window.automationControlStatus)) {
        window.automationControlStatus = "command_failed";
      } else {
        window.automationControlLoaded = false;
      }
      return;
    }
  }
  if (window.automationControlStatus == "loaded" && commands.empty()) {
    window.automationControlLastResult = "none";
  }
  window.selectedSettingsTab = settingsTab;
  window.inputOwner = productInputOwnerFor(frontend, window);
  window.gameplayInputSuppressed = frontendBlocksGameplayInput(frontend);
}

ProductAppWindowState runOpeningMenuWindow(const ProductAppOptions& options,
                                           const ProductWorldTemplate& world,
                                           FrontendState& frontend,
                                           std::optional<Session>& activeSession,
                                           WorldSetupDraft& worldSetupDraft,
                                           ProductAppWindowState window,
                                           const FrontendSettings& settings,
                                           const ProductSaveBridgeResult& saves) {
  window.requested = options.windowMode == ProductWindowMode::Window;
  window.inputOwner = productInputOwnerFor(frontend, window);
  window.gameplayInputSuppressed = frontendBlocksGameplayInput(frontend);
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
  createInfo.vulkan = false;

  SdlWindow sdlWindow(createInfo);
  window.created = sdlWindow.nativeWindow() != nullptr;
  window.drawable = sdlWindow.isDrawable();
  window.openingMenuVisible = window.created && frontend.screen == FrontendScreen::Starter;
  if (!window.created) {
    window.status = "window_create_failed";
    return window;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(sdlWindow.nativeWindow(), nullptr);
  if (renderer == nullptr) {
    window.status = "renderer_create_failed";
    return window;
  }

  sdlWindow.setTitle(window.gameplayActive ? "iggy3d - Gameplay" : "iggy3d - Opening Menu");
  const auto start = std::chrono::steady_clock::now();
  KeyboardInputState keyboard;
  MouseInputState mouse;
  GamepadMenuState gamepad;
  initializeGamepadMenuState(gamepad);
  window.gamepadAvailable = gamepad.gamepadAvailable;
  window.gamepadName = gamepad.gamepadName;
  window.gamepadMapping = gamepad.gamepadAvailable ? "sdl_gamepad" : "unavailable";
  bool closeRequested = false;
  FrontendSettingsTab settingsTab = FrontendSettingsTab::Input;
  while (sdlWindow.isOpen()) {
    ActionState actionState;
    sdlWindow.pollEvents();
    ++window.eventPollCount;
    window.drawable = sdlWindow.isDrawable();
    sdlWindow.setTitle(window.gameplayActive ? "iggy3d - Gameplay" : "iggy3d - Opening Menu");

    routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession,
                          worldSetupDraft, actionState, pollKeyboardMenuAction(keyboard),
                          window, closeRequested);

    const InputAction gamepadAction = pollGamepadMenuAction(gamepad);
    if (gamepadAction != InputAction::None) {
      window.gamepadMenuSelectUsed = true;
      routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession,
                            worldSetupDraft, actionState, gamepadAction, window,
                            closeRequested);
    }

    const MouseClick click = pollMouseClick(mouse);
    if (click.clicked) {
      const OpeningMenuHitTestResult hit = openingMenuActionAt(frontend, click.x, click.y);
      if (hit.hit) {
        window.mouseMenuSelectUsed = true;
        if (hit.area == OpeningMenuHitArea::StarterAction) {
          frontend.selectedAction = hit.action;
          routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession,
                                worldSetupDraft, actionState, mouseClickAction(click),
                                window, closeRequested);
        } else if (hit.area == OpeningMenuHitArea::DevToolsCategory) {
          frontend.devToolsCategory = hit.devToolsCategory;
          frontend.status = "dev_tools_category_selected";
        } else if (hit.area == OpeningMenuHitArea::SettingsTab) {
          settingsTab = hit.settingsTab;
          frontend.status = "settings_tab_selected";
        }
      }
    }

    if (window.gameplayActive && activeSession.has_value() &&
        !frontendBlocksGameplayInput(frontend)) {
      ActionState gameplayActions;
      pollKeyboardGameplayActions(keyboard, gameplayActions);
      pollGamepadGameplayActions(gamepad, gameplayActions);
      pollMouseGameplayActions(mouse, gameplayActions);

      ActionState acceptedGameplayActions;
      InputRoutingContext routingContext;
      routingContext.owners.gameplay = true;
      for (const ActionStateEntry& entry : gameplayActions.entries) {
        const InputRoutingResult routed = routeInputAction(routingContext, entry.action);
        window.inputOwner = routed.owner;
        window.lastInputAction = routed.action;
        window.lastInputAccepted = routed.accepted;
        window.gameplayInputSuppressed = routed.gameplaySuppressed;
        if (routed.accepted) {
          recordAction(acceptedGameplayActions, entry.action, entry.down, entry.pressed,
                       entry.released, entry.value);
        }
      }
      applyProductCameraActions(acceptedGameplayActions, window.viewport, settings,
                                "action_map");
      const SpatialSurfaceSet* collisionSurfaces =
          productActiveRoomCollisionSurfaces(window.activeRoomCollision);
      applyProductGameplayActions(*activeSession, acceptedGameplayActions, window,
                                  "action_map", collisionSurfaces);
    }

    SceneProjectionResult scene;
    DebugProjectionResult debug;
    ProductPrimitiveDrawList drawList;
    ProductViewportFrame frame;
    std::size_t sceneItemCount = 0;
    const SceneProjectionResult* scenePtr = nullptr;
    const DebugProjectionResult* debugPtr = nullptr;
    const ProductPrimitiveDrawList* drawListPtr = nullptr;
    const ProductViewportFrame* framePtr = nullptr;
    const ProductRenderBridgeFrame* bridgePtr = nullptr;
    ProductGameplayFeedback feedback = buildProductGameplayFeedback(window);
    ProductMovementDebugHud movementHud = buildProductMovementDebugHud(
        window, settings.devToolsEnabled, settings.debugOverlayEnabled);
    ProductNpcBehaviorDebugHud npcBehaviorHud = buildProductNpcBehaviorDebugHud(
        nullptr, window.gameplayActive, settings.devToolsEnabled,
        settings.debugOverlayEnabled);
    copyNpcBehaviorDebugHud(window, npcBehaviorHud);
    ProductRenderBridgeFrame bridge;
    if (window.gameplayActive && activeSession.has_value()) {
      scene = buildSceneProjection(activeSession->state());
      debug = buildProductDebugProjectionWithNpcBehavior(activeSession->state());
      const RoomAsset* activeRoom =
          window.activeRoom.loaded ? &window.activeRoom.room : nullptr;
      drawList = buildProductPrimitiveDrawList(&scene, &debug, activeRoom,
                                               &window.activeRoomCollision);
      frame = buildProductViewportFrame(
          drawList, ProductViewportFrameConfig{window.viewport.cameraYawDegrees,
                                               window.viewport.cameraPitchDegrees});
      scenePtr = &scene;
      debugPtr = &debug;
      drawListPtr = &drawList;
      framePtr = &frame;
      sceneItemCount = scene.items.size();
      window.runtimeStateHash = activeSession->stateHash();
      feedback = buildProductGameplayFeedback(window);
      movementHud = buildProductMovementDebugHud(window, settings.devToolsEnabled,
                                                 settings.debugOverlayEnabled);
      npcBehaviorHud = buildProductNpcBehaviorDebugHud(&debug,
                                                       window.gameplayActive,
                                                       settings.devToolsEnabled,
                                                       settings.debugOverlayEnabled);
      copyNpcBehaviorDebugHud(window, npcBehaviorHud);
      bridge = buildProductRenderBridgeFrame(&drawList, &frame, &feedback);
      bridgePtr = &bridge;
    }

    if (window.drawable) {
      const OpeningMenuViewState view =
          drawOpeningMenuView(*renderer, options, world, frontend, settingsTab,
                              window.gameplayActive, window.runtimeStateHash, framePtr,
                              &feedback, &movementHud, &npcBehaviorHud,
                              sceneItemCount, debugPtr,
                              window.viewport.cameraYawDegrees,
                              window.viewport.cameraPitchDegrees, saves);
      applyGameplayProjectionMetrics(window, scenePtr, debugPtr, drawListPtr, framePtr,
                                     bridgePtr,
                                     window.gameplayActive && scenePtr != nullptr);
      window.viewport.cameraHeadingVisible =
          window.viewport.cameraHeadingVisible || view.cameraHeadingDrawn;
      window.menuTextDrawn = window.menuTextDrawn || view.textDrawn;
      window.selectedRowDrawn = window.selectedRowDrawn || view.selectedRowDrawn;
      window.menuRowCount = view.rowCount;
    } else {
      applyGameplayProjectionMetrics(window, scenePtr, debugPtr, drawListPtr, framePtr,
                                     bridgePtr, false);
    }
    ++window.framesPresented;

    if (options.frames > 0 && window.framesPresented >= options.frames) {
      break;
    }
    if (closeRequested) {
      break;
    }
    if (options.holdSeconds > 0) {
      const auto elapsed = std::chrono::steady_clock::now() - start;
      if (elapsed >= std::chrono::seconds(options.holdSeconds)) {
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  shutdownGamepadMenuState(gamepad);
  SDL_DestroyRenderer(renderer);
  window.selectedSettingsTab = settingsTab;
  window.status = window.menuTextDrawn ? "opening_menu_text_ready"
                                       : "opening_menu_window_ready";
  return window;
#else
  (void)world;
  (void)frontend;
  (void)worldSetupDraft;
  (void)saves;
  window.sdlAvailable = false;
  window.created = false;
  window.drawable = false;
  window.openingMenuVisible = false;
  window.status = "sdl3_unavailable";
  return window;
#endif
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
  WorldSetupDraft worldSetupDraft = makeDefaultWorldSetupDraft();
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
  applyProductAutomationControl(options, frontend, saves, automationSettingsTab,
                                activeSession, worldSetupDraft, window,
                                automationCloseRequested);
  if (!automationCloseRequested) {
    runProductGameplayTapeFromOptions(options, activeSession, window);
  }
  saves = scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  if (automationCloseRequested) {
    window.status = "automation_close_requested";
  }

  window =
      runOpeningMenuWindow(options, world, frontend, activeSession, worldSetupDraft,
                           window, settings, saves);
  refreshGameplayProjectionMetrics(activeSession, window, settings.devToolsEnabled,
                                   settings.debugOverlayEnabled);

  if (options.printRenderReceipt) {
    std::cout << formatRenderReceipt(
        buildProductAppReceipt(options, world, frontend, settings, window, saves));
  }

  return window.requested && !window.created ? 77 : 0;
}

}  // namespace iggy3d
