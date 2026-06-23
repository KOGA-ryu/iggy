#include "app/iggy3d/AppShell.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>

#include "app/PackageRuntimeLookup.hpp"
#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/DefaultWorldTemplate.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/SaveBridge.hpp"
#include "content/PackageLoader.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "app/input/GamepadInput.hpp"
#include "app/input/KeyboardInput.hpp"
#include "app/input/MouseInput.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/RenderDiagnostics.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/targeting/ReachQuery.hpp"
#include "runtime/targeting/TargetQuery.hpp"
#include "runtime/world/WorldState.hpp"

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

std::string packageLoadStatusName(PackageLoadStatus status) {
  switch (status) {
    case PackageLoadStatus::Ok:
      return "ok";
    case PackageLoadStatus::MissingPackageFile:
      return "missing_package_file";
    case PackageLoadStatus::PackageReadFailed:
      return "package_read_failed";
    case PackageLoadStatus::ScenarioReadFailed:
      return "scenario_read_failed";
    case PackageLoadStatus::ParseError:
      return "parse_error";
    case PackageLoadStatus::UnsupportedKey:
      return "unsupported_key";
    case PackageLoadStatus::MissingRequiredKey:
      return "missing_required_key";
    case PackageLoadStatus::MissingScenarioId:
      return "missing_scenario_id";
    case PackageLoadStatus::InvalidNumber:
      return "invalid_number";
    case PackageLoadStatus::InvalidEnum:
      return "invalid_enum";
    case PackageLoadStatus::InvalidPath:
      return "invalid_path";
  }
  return "unknown";
}

std::string commandKindName(CommandKind kind) {
  switch (kind) {
    case CommandKind::Move:
      return "move";
    case CommandKind::Interact:
      return "interact";
    case CommandKind::Attack:
      return "attack";
    case CommandKind::Retry:
      return "retry";
    case CommandKind::Reset:
      return "reset";
    case CommandKind::None:
      return "none";
    default:
      break;
  }
  return "other";
}

std::string commandRejectionReasonName(CommandRejectionReason reason) {
  switch (reason) {
    case CommandRejectionReason::None:
      return "none";
    case CommandRejectionReason::InvalidTarget:
      return "invalid_target";
    case CommandRejectionReason::OutOfRange:
      return "out_of_range";
    case CommandRejectionReason::TargetNotReachable:
      return "target_not_reachable";
    case CommandRejectionReason::InvalidDamage:
      return "invalid_damage";
    case CommandRejectionReason::TargetDefeated:
      return "target_defeated";
    case CommandRejectionReason::FriendlyFireBlocked:
      return "friendly_fire_blocked";
    case CommandRejectionReason::InvalidActor:
      return "invalid_actor";
    default:
      break;
  }
  return "rejected";
}

std::string reachGateName(CommandRejectionReason reason) {
  if (reason == CommandRejectionReason::None) {
    return "pass";
  }
  if (reason == CommandRejectionReason::OutOfRange ||
      reason == CommandRejectionReason::TargetNotReachable) {
    return "fail";
  }
  return "not_attempted";
}

std::filesystem::path defaultProductPackagePath(const ProductAppOptions& options) {
  if (!options.devPackageOverride.empty()) {
    return options.devPackageOverride;
  }

  PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = PackageMode::BuildTreeVisual;
  lookupConfig.requireGraphicsRuntime = false;
  lookupConfig.requireShaderRoot = false;
  const PackageLookupResult lookup = resolvePackageRuntimeLookup(lookupConfig);
  if (lookup.outcome == RenderOutcome::Ok && !lookup.lookup.resourceRoot.empty()) {
    return lookup.lookup.resourceRoot / "demos" / "first_room" / "package.iggy3d.toml";
  }

  return std::filesystem::path{"fixtures"} / "demos" / "first_room" / "package.iggy3d.toml";
}

bool createProductSession(const ProductAppOptions& options,
                          std::optional<Session>& activeSession,
                          ProductAppWindowState& window) {
  const std::filesystem::path packagePath = defaultProductPackagePath(options);
  const PackageLoadResult package = loadPackage({packagePath.generic_string()});
  window.packageLoadStatus = packageLoadStatusName(package.status);
  if (package.status != PackageLoadStatus::Ok) {
    window.launchStatus = "package_load_failed";
    return false;
  }

  SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = package.scenario;
  create.config = package.scenario.config;

  Result<Session> session = Session::create(create);
  if (session.status != ResultStatus::Ok) {
    window.launchStatus = session.error.code.empty() ? "session_create_failed"
                                                     : session.error.code;
    return false;
  }

  activeSession = std::move(session.value);
  window.runtimeSessionCreated = true;
  window.gameplayActive = true;
  window.runtimeStateHash = activeSession->stateHash();
  window.launchStatus = "runtime_session_created";
  return true;
}

void launchProductNewWorld(const ProductAppOptions& options,
                           FrontendState& frontend,
                           std::optional<Session>& activeSession,
                           ProductAppWindowState& window) {
  window.launchAction = "create_and_enter";
  if (createProductSession(options, activeSession, window)) {
    enterFrontendGameplay(frontend, FrontendAction::CreateAndEnter);
    frontend.status = "gameplay_active";
  } else {
    frontend.status = "opening_menu_new_world_failed";
  }
}

EntityId productPlayerActor(const Session& session) {
  return session.state().players.actorForSlot(0);
}

const EntityState* productPlayerEntity(const Session& session) {
  const EntityId actor = productPlayerActor(session);
  return session.state().world.findById(actor);
}

TargetQueryResult queryProductGameplayTarget(const Session& session, CommandKind kind) {
  const EntityId actor = productPlayerActor(session);
  return queryTarget(TargetQueryRequest{&session.state().world, actor, false, {},
                                        kind, 0.0F, false, true});
}

void submitProductGameplayCommand(Session& session,
                                  ProductAppWindowState& window,
                                  CommandRecord command) {
  const EntityState* beforePlayer = productPlayerEntity(session);
  const Vec3 before = beforePlayer == nullptr ? Vec3{} : beforePlayer->transform.position;
  window.gameplayInputUsed = true;
  window.gameplayCommandSubmitted = true;
  window.gameplayCommandKind = commandKindName(command.kind);

  const SessionCommandResult submitted = session.submitCommand(command);
  window.gameplayCommandAccepted =
      submitted.command.admission == CommandAdmissionStatus::Accepted;
  window.gameplayLastRejection = commandRejectionReasonName(submitted.command.rejection);
  window.gameplayReachGate = reachGateName(submitted.command.rejection);
  window.gameplayCommandStatus =
      window.gameplayCommandAccepted ? "accepted" : "rejected";

  if (window.gameplayCommandAccepted) {
    const StatusResult tick = session.tick();
    window.gameplayTickAdvanced = tick.status == ResultStatus::Ok;
  }

  const EntityState* afterPlayer = productPlayerEntity(session);
  if (afterPlayer != nullptr && beforePlayer != nullptr) {
    window.playerPositionChanged =
        window.playerPositionChanged ||
        !nearlyEqual(before, afterPlayer->transform.position);
  }
  window.runtimeStateHash = session.stateHash();
}

void submitProductMove(Session& session,
                       ProductAppWindowState& window,
                       float moveX,
                       float moveY,
                       const char* source) {
  const EntityState* actor = productPlayerEntity(session);
  if (actor == nullptr) {
    window.gameplayCommandStatus = "missing_player";
    return;
  }
  if (moveX == 0.0F && moveY == 0.0F) {
    return;
  }
  const float magnitude = std::sqrt(moveX * moveX + moveY * moveY);
  const float scale = magnitude > 1.0F ? 1.0F / magnitude : 1.0F;
  constexpr float kStepMeters = 1.0F;
  Vec3 destination = actor->transform.position;
  destination.x += moveX * scale * kStepMeters;
  destination.z += moveY * scale * kStepMeters;

  CommandRecord command;
  command.playerSlot = 0;
  command.actor = actor->id;
  command.kind = CommandKind::Move;
  command.source = CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = destination;
  window.gameplayInputSource = source;
  submitProductGameplayCommand(session, window, command);
}

void submitProductTargetCommand(Session& session,
                                ProductAppWindowState& window,
                                CommandKind kind,
                                const char* source) {
  const EntityId actor = productPlayerActor(session);
  const TargetQueryResult target = queryProductGameplayTarget(session, kind);
  window.targetDiscovered = target.status == TargetQueryStatus::Found;
  if (!window.targetDiscovered) {
    window.gameplayInputUsed = true;
    window.gameplayInputSource = source;
    window.gameplayCommandKind = commandKindName(kind);
    window.gameplayCommandStatus = "no_target";
    window.gameplayReachGate = "not_attempted";
    return;
  }

  const ReachQueryResult reach =
      queryReach(ReachQueryRequest{&session.state().world, actor, target.target, false, {},
                                   session.state().config.interactionRangeMeters, true});
  const CommandRejectionReason reachReason = rejectionReasonForReach(reach);
  window.gameplayReachGate = reachGateName(reachReason);

  CommandRecord command;
  command.playerSlot = 0;
  command.actor = actor;
  command.kind = kind;
  command.source = CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = target.target;
  if (kind == CommandKind::Attack) {
    command.payload.attackDamage = 3;
  }
  window.gameplayInputSource = source;
  submitProductGameplayCommand(session, window, command);
  if (kind == CommandKind::Interact && window.gameplayCommandAccepted) {
    window.interactionExecuted = true;
  }
  if (kind == CommandKind::Attack && window.gameplayCommandAccepted) {
    window.attackExecuted = true;
  }
}

void applyProductGameplayActions(Session& session,
                                 const ActionState& actions,
                                 ProductAppWindowState& window,
                                 const char* source) {
  const float moveX = actionAxisValue(actions, InputAction::PlayerMoveX);
  const float moveY = actionAxisValue(actions, InputAction::PlayerMoveY);
  if (moveX != 0.0F || moveY != 0.0F) {
    submitProductMove(session, window, moveX, moveY, source);
  }
  if (actionWasPressed(actions, InputAction::PlayerInteract)) {
    submitProductTargetCommand(session, window, CommandKind::Interact, source);
  }
  if (actionWasPressed(actions, InputAction::PlayerAttack)) {
    submitProductTargetCommand(session, window, CommandKind::Attack, source);
  }
  if (actionWasPressed(actions, InputAction::PlayerRetryOrReset)) {
    const SessionResetResult reset = session.resetToBaseline();
    window.gameplayInputUsed = true;
    window.gameplayInputSource = source;
    window.gameplayCommandKind = "reset";
    window.gameplayCommandSubmitted = true;
    window.gameplayCommandAccepted = reset.reset;
    window.gameplayCommandStatus = reset.reset ? "accepted" : "rejected";
    window.runtimeStateHash = session.stateHash();
  }
}

void runScriptedProductGameplaySmoke(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window) {
  if (!activeSession.has_value()) {
    window.gameplayCommandStatus = "missing_session";
    return;
  }

  window.scriptedGameplaySmoke = true;
  ActionState actions;
  recordAction(actions, InputAction::PlayerMoveX, true, false, false, 1.0F);
  applyProductGameplayActions(*activeSession, actions, window, "scripted");
  clearActionState(actions);
  recordAction(actions, InputAction::PlayerMoveX, true, false, false, 1.0F);
  applyProductGameplayActions(*activeSession, actions, window, "scripted");
  clearActionState(actions);
  recordAction(actions, InputAction::PlayerMoveY, true, false, false, 1.0F);
  applyProductGameplayActions(*activeSession, actions, window, "scripted");
  clearActionState(actions);
  recordAction(actions, InputAction::PlayerAttack, true, true, false, 1.0F);
  applyProductGameplayActions(*activeSession, actions, window, "scripted");
}

void applyGameplayProjectionMetrics(ProductAppWindowState& window,
                                    const SceneProjectionResult* scene,
                                    const DebugProjectionResult* debug,
                                    bool viewVisible) {
  if (!window.gameplayActive || scene == nullptr) {
    window.gameplayViewVisible = false;
    window.sceneItemCount = 0;
    window.debugItemCount = 0;
    window.playerVisible = false;
    window.roomVisible = false;
    window.objectiveVisible = false;
    window.rendererMutatedRuntime = false;
    return;
  }

  window.gameplayViewVisible = viewVisible;
  window.sceneItemCount = static_cast<std::uint64_t>(scene->items.size());
  window.debugItemCount =
      debug == nullptr ? 0U : static_cast<std::uint64_t>(debug->items.size());
  window.playerVisible = scene->playerCount > 0;
  window.roomVisible = true;
  window.objectiveVisible =
      scene->pickupCount > 0 || scene->interactableCount > 0 || scene->markerCount > 0 ||
      scene->room.loaded;
  window.rendererMutatedRuntime = false;
}

void refreshGameplayProjectionMetrics(const std::optional<Session>& activeSession,
                                      ProductAppWindowState& window) {
  if (!window.gameplayActive || !activeSession.has_value()) {
    applyGameplayProjectionMetrics(window, nullptr, nullptr, false);
    return;
  }

  const SceneProjectionResult scene = buildSceneProjection(activeSession->state());
  const DebugProjectionResult debug = buildDebugProjection(activeSession->state());
  const bool viewWasVisible = window.gameplayViewVisible;
  window.runtimeStateHash = activeSession->stateHash();
  applyGameplayProjectionMetrics(window, &scene, &debug, viewWasVisible);
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

void applyOpeningMenuAction(FrontendState& frontend,
                            const ProductSaveBridgeResult& saves,
                            const ProductAppOptions& options,
                            FrontendSettingsTab& settingsTab,
                            std::optional<Session>& activeSession,
                            ProductAppWindowState& window,
                            InputAction action,
                            bool& closeRequested) {
  if (action == InputAction::SystemPause) {
    if (frontend.screen == FrontendScreen::Gameplay && window.gameplayActive) {
      openFrontendPause(frontend, FrontendAction::Resume);
      frontend.status = "pause_opened_from_gameplay";
      return;
    }
    if (frontend.screen == FrontendScreen::Pause) {
      closeFrontendOverlayToGameplay(frontend);
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
      closeFrontendOverlayToGameplay(frontend);
      return;
    }
    if (action != InputAction::MenuConfirm) {
      return;
    }
    if (frontend.selectedAction == FrontendAction::Resume) {
      closeFrontendOverlayToGameplay(frontend);
      return;
    }
    if (frontend.selectedAction == FrontendAction::Settings) {
      frontend.screen = FrontendScreen::Settings;
      frontend.childScreen = FrontendScreen::Pause;
      settingsTab = FrontendSettingsTab::Input;
      frontend.status = "pause_settings_selected";
      return;
    }
    if (frontend.selectedAction == FrontendAction::DevTools) {
      openFrontendDevOverlay(frontend, FrontendDevToolsCategory::Session);
      frontend.status = "pause_dev_tools_selected";
      return;
    }
    if (frontend.selectedAction == FrontendAction::ReturnToTitle) {
      window.gameplayActive = false;
      window.runtimeSessionCreated = false;
      frontend.screen = FrontendScreen::Starter;
      frontend.childScreen = FrontendScreen::Gameplay;
      frontend.selectedAction = FrontendAction::NewWorld;
      frontend.returnToTitleRequested = true;
      frontend.inputOwned = true;
      frontend.status = "returned_to_title";
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
        openFrontendPause(frontend, FrontendAction::Settings);
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

  if (frontend.selectedAction == FrontendAction::Continue && saves.slots.compatibleCount == 0) {
    frontend.status = "opening_menu_action_disabled";
    return;
  }
  if (frontend.selectedAction == FrontendAction::Exit) {
    frontend.status = "opening_menu_exit_requested";
    closeRequested = true;
    return;
  }
  if (frontend.selectedAction == FrontendAction::NewWorld) {
    launchProductNewWorld(options, frontend, activeSession, window);
    return;
  }
  if (frontend.selectedAction == FrontendAction::LoadSave) {
    frontend.childScreen = FrontendScreen::LoadSave;
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
    applyOpeningMenuAction(frontend, saves, options, settingsTab, activeSession, window,
                           routed.action, closeRequested);
  }
}

ProductAppWindowState runOpeningMenuWindow(const ProductAppOptions& options,
                                           const ProductWorldTemplate& world,
                                           FrontendState& frontend,
                                           std::optional<Session>& activeSession,
                                           ProductAppWindowState window,
                                           const ProductSaveBridgeResult& saves) {
  window.requested = options.windowMode == ProductWindowMode::Window;
  window.inputOwner =
      frontend.screen == FrontendScreen::Starter ? MenuOwner::Starter : MenuOwner::None;
  window.gameplayInputSuppressed = frontend.screen == FrontendScreen::Starter;
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

    routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession, actionState,
                          pollKeyboardMenuAction(keyboard), window, closeRequested);

    const InputAction gamepadAction = pollGamepadMenuAction(gamepad);
    if (gamepadAction != InputAction::None) {
      window.gamepadMenuSelectUsed = true;
      routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession, actionState,
                            gamepadAction, window, closeRequested);
    }

    const MouseClick click = pollMouseClick(mouse);
    if (click.clicked) {
      const OpeningMenuHitTestResult hit = openingMenuActionAt(frontend, click.x, click.y);
      if (hit.hit) {
        window.mouseMenuSelectUsed = true;
        if (hit.area == OpeningMenuHitArea::StarterAction) {
          frontend.selectedAction = hit.action;
          routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession, actionState,
                                mouseClickAction(click), window, closeRequested);
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
      applyProductGameplayActions(*activeSession, acceptedGameplayActions, window,
                                  "action_map");
    }

    SceneProjectionResult scene;
    DebugProjectionResult debug;
    const SceneProjectionResult* scenePtr = nullptr;
    const DebugProjectionResult* debugPtr = nullptr;
    if (window.gameplayActive && activeSession.has_value()) {
      scene = buildSceneProjection(activeSession->state());
      debug = buildDebugProjection(activeSession->state());
      scenePtr = &scene;
      debugPtr = &debug;
      window.runtimeStateHash = activeSession->stateHash();
    }

    if (window.drawable) {
      const OpeningMenuViewState view =
          drawOpeningMenuView(*renderer, options, world, frontend, settingsTab,
                              window.gameplayActive, window.runtimeStateHash, scenePtr,
                              debugPtr, saves);
      applyGameplayProjectionMetrics(window, scenePtr, debugPtr,
                                     window.gameplayActive && scenePtr != nullptr);
      window.menuTextDrawn = window.menuTextDrawn || view.textDrawn;
      window.selectedRowDrawn = window.selectedRowDrawn || view.selectedRowDrawn;
      window.menuRowCount = view.rowCount;
    } else {
      applyGameplayProjectionMetrics(window, scenePtr, debugPtr, false);
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
  const ProductWorldTemplate world =
      options.devPackageOverride.empty()
          ? defaultProductWorldTemplate()
          : devOverrideProductWorldTemplate(options.devPackageOverride.generic_string(),
                                            options.devScenario);
  const ProductSaveBridgeResult saves =
      scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  const FrontendSettings settings = productFrontendSettingsFromOptions(options);
  std::optional<Session> activeSession;
  ProductAppWindowState window;

  FrontendState frontend;
  completeFrontendBoot(frontend, true, true);
  frontend.screen = FrontendScreen::Starter;
  frontend.selectedAction = saves.slots.compatibleCount > 0 ? FrontendAction::Continue
                                                            : FrontendAction::NewWorld;
  frontend.status = "opening_menu_ready";
  frontend.inputOwned = true;

  if (options.autoNewWorld) {
    launchProductNewWorld(options, frontend, activeSession, window);
  }
  if (options.scriptedGameplaySmoke) {
    runScriptedProductGameplaySmoke(activeSession, window);
  }

  window = runOpeningMenuWindow(options, world, frontend, activeSession, window, saves);
  refreshGameplayProjectionMetrics(activeSession, window);

  if (options.printRenderReceipt) {
    std::cout << formatRenderReceipt(
        buildProductAppReceipt(options, world, frontend, settings, window, saves));
  }

  return window.requested && !window.created ? 77 : 0;
}

}  // namespace iggy3d
