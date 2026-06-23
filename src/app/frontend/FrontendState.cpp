#include "app/frontend/FrontendState.hpp"

namespace iggy3d {

std::string_view frontendScreenName(FrontendScreen screen) {
  switch (screen) {
    case FrontendScreen::BootStatus:
      return "boot";
    case FrontendScreen::Starter:
      return "starter";
    case FrontendScreen::NewWorld:
      return "new_world";
    case FrontendScreen::LoadSave:
      return "load_save";
    case FrontendScreen::Settings:
      return "settings";
    case FrontendScreen::StarterDevTools:
      return "starter_dev_tools";
    case FrontendScreen::Gameplay:
      return "gameplay";
    case FrontendScreen::Pause:
      return "pause";
    case FrontendScreen::DevOverlay:
      return "dev_overlay";
    case FrontendScreen::ExitConfirm:
      return "exit_confirm";
    case FrontendScreen::DeleteConfirm:
      return "delete_confirm";
  }
  return "boot";
}

std::string_view frontendActionName(FrontendAction action) {
  switch (action) {
    case FrontendAction::None:
      return "none";
    case FrontendAction::Continue:
      return "continue";
    case FrontendAction::NewWorld:
      return "new_world";
    case FrontendAction::LoadSave:
      return "load_save";
    case FrontendAction::Settings:
      return "settings";
    case FrontendAction::DevTools:
      return "dev_tools";
    case FrontendAction::Exit:
      return "exit";
    case FrontendAction::CreateAndEnter:
      return "create_and_enter";
    case FrontendAction::Load:
      return "load";
    case FrontendAction::Delete:
      return "delete";
    case FrontendAction::Back:
      return "back";
    case FrontendAction::Apply:
      return "apply";
    case FrontendAction::RestoreDefaults:
      return "restore_defaults";
    case FrontendAction::Resume:
      return "resume";
    case FrontendAction::Save:
      return "save";
    case FrontendAction::SaveAndExit:
      return "save_and_exit";
    case FrontendAction::ReturnToTitle:
      return "return_to_title";
    case FrontendAction::ExitGame:
      return "exit_game";
  }
  return "none";
}

std::string_view frontendDevToolsCategoryName(FrontendDevToolsCategory category) {
  switch (category) {
    case FrontendDevToolsCategory::None:
      return "none";
    case FrontendDevToolsCategory::Session:
      return "session";
    case FrontendDevToolsCategory::Input:
      return "input";
    case FrontendDevToolsCategory::Player:
      return "player";
    case FrontendDevToolsCategory::Movement:
      return "movement";
    case FrontendDevToolsCategory::WorldEditor:
      return "world_editor";
    case FrontendDevToolsCategory::Collision:
      return "collision";
    case FrontendDevToolsCategory::Spells:
      return "spells";
    case FrontendDevToolsCategory::Camera:
      return "camera";
    case FrontendDevToolsCategory::Renderer:
      return "renderer";
    case FrontendDevToolsCategory::Performance:
      return "performance";
  }
  return "none";
}

const std::vector<FrontendAction>& starterActionOrder() {
  static const std::vector<FrontendAction> actions = {
      FrontendAction::Continue,
      FrontendAction::NewWorld,
      FrontendAction::LoadSave,
      FrontendAction::Delete,
      FrontendAction::Settings,
      FrontendAction::DevTools,
      FrontendAction::Exit,
  };
  return actions;
}

const std::vector<FrontendAction>& pauseActionOrder() {
  static const std::vector<FrontendAction> actions = {
      FrontendAction::Resume,
      FrontendAction::Save,
      FrontendAction::SaveAndExit,
      FrontendAction::LoadSave,
      FrontendAction::Settings,
      FrontendAction::DevTools,
      FrontendAction::ReturnToTitle,
      FrontendAction::ExitGame,
  };
  return actions;
}

const std::vector<FrontendDevToolsCategory>& devToolsCategoryOrder() {
  static const std::vector<FrontendDevToolsCategory> categories = {
      FrontendDevToolsCategory::Session,
      FrontendDevToolsCategory::Input,
      FrontendDevToolsCategory::Player,
      FrontendDevToolsCategory::Movement,
      FrontendDevToolsCategory::WorldEditor,
      FrontendDevToolsCategory::Collision,
      FrontendDevToolsCategory::Spells,
      FrontendDevToolsCategory::Camera,
      FrontendDevToolsCategory::Renderer,
      FrontendDevToolsCategory::Performance,
  };
  return categories;
}

void completeFrontendBoot(FrontendState& state, bool packageReady, bool saveScanComplete) {
  state.bootScanComplete = true;
  state.packageReady = packageReady;
  state.saveScanComplete = saveScanComplete;
  if (packageReady && saveScanComplete) {
    state.screen = FrontendScreen::Starter;
    state.selectedAction = FrontendAction::Continue;
    state.status = "starter_screen_ready";
    state.inputOwned = true;
  } else {
    state.screen = FrontendScreen::BootStatus;
    state.selectedAction = FrontendAction::None;
    state.status = "boot_failed";
    state.inputOwned = true;
  }
}

void enterFrontendGameplay(FrontendState& state, FrontendAction launchAction) {
  state.screen = FrontendScreen::Gameplay;
  state.childScreen = FrontendScreen::Gameplay;
  state.selectedAction = launchAction;
  state.launchRequested = true;
  state.pauseMenuOpen = false;
  state.devToolsOpen = false;
  state.inputOwned = false;
  if (launchAction == FrontendAction::Load || launchAction == FrontendAction::LoadSave ||
      launchAction == FrontendAction::Continue) {
    state.status = "frontend_launch_loaded_save";
  } else {
    state.status = "frontend_launch_new_world";
  }
}

void openFrontendPause(FrontendState& state, FrontendAction selectedAction) {
  state.screen = FrontendScreen::Pause;
  state.childScreen = FrontendScreen::Gameplay;
  state.selectedAction = selectedAction;
  state.pauseMenuOpen = true;
  state.devToolsOpen = false;
  state.inputOwned = true;
  state.status = "pause_menu_ready";
}

void openFrontendDevOverlay(FrontendState& state, FrontendDevToolsCategory category) {
  state.screen = FrontendScreen::DevOverlay;
  state.childScreen = FrontendScreen::Gameplay;
  state.selectedAction = FrontendAction::DevTools;
  state.devToolsCategory = category == FrontendDevToolsCategory::None
                               ? FrontendDevToolsCategory::Session
                               : category;
  state.pauseMenuOpen = false;
  state.devToolsOpen = true;
  state.inputOwned = true;
  state.status = "dev_overlay_ready";
}

void closeFrontendOverlayToGameplay(FrontendState& state) {
  state.screen = FrontendScreen::Gameplay;
  state.childScreen = FrontendScreen::Gameplay;
  state.pauseMenuOpen = false;
  state.devToolsOpen = false;
  state.inputOwned = false;
  state.status = "frontend_gameplay_active";
}

bool frontendBlocksGameplayInput(const FrontendState& state) {
  return state.inputOwned || state.screen == FrontendScreen::Starter ||
         state.screen == FrontendScreen::Pause || state.screen == FrontendScreen::DevOverlay ||
         state.screen == FrontendScreen::BootStatus;
}

}  // namespace iggy3d
