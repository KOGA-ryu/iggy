#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace iggy3d {

enum class FrontendScreen : std::uint8_t {
  BootStatus,
  Starter,
  NewWorld,
  LoadSave,
  Settings,
  StarterDevTools,
  Gameplay,
  Pause,
  DevOverlay,
  ExitConfirm,
  DeleteConfirm,
};

enum class FrontendAction : std::uint8_t {
  None,
  Continue,
  NewWorld,
  LoadSave,
  Settings,
  DevTools,
  Exit,
  CreateAndEnter,
  Load,
  Delete,
  Back,
  Apply,
  RestoreDefaults,
  Resume,
  EditRoom,
  LeaveEditor,
  Save,
  SaveAndExit,
  ReturnToTitle,
  ExitGame,
};

enum class FrontendDevToolsCategory : std::uint8_t {
  None,
  Session,
  Input,
  Player,
  Movement,
  WorldEditor,
  Collision,
  Spells,
  Camera,
  Renderer,
  Performance,
};

enum class FrontendSaveBrowserMode : std::uint8_t {
  Load,
  Delete,
};

struct FrontendState {
  FrontendScreen screen = FrontendScreen::BootStatus;
  FrontendScreen childScreen = FrontendScreen::Gameplay;
  FrontendAction selectedAction = FrontendAction::None;
  FrontendAction disabledAction = FrontendAction::None;
  FrontendSaveBrowserMode saveBrowserMode = FrontendSaveBrowserMode::Load;
  FrontendDevToolsCategory devToolsCategory = FrontendDevToolsCategory::None;
  bool bootScanComplete = false;
  bool packageReady = false;
  bool saveScanComplete = false;
  bool launchRequested = false;
  bool returnToTitleRequested = false;
  bool inputOwned = true;
  std::string_view status = "frontend_boot_pending";
};

std::string_view frontendScreenName(FrontendScreen screen);
std::string_view frontendActionName(FrontendAction action);
std::string_view frontendDevToolsCategoryName(FrontendDevToolsCategory category);
std::string_view frontendSaveBrowserModeName(FrontendSaveBrowserMode mode);

const std::vector<FrontendAction>& starterActionOrder();
const std::vector<FrontendAction>& pauseActionOrder();
const std::vector<FrontendDevToolsCategory>& devToolsCategoryOrder();

bool frontendPauseMenuOpen(const FrontendState& state);
bool frontendDevToolsOpen(const FrontendState& state);
void completeFrontendBoot(FrontendState& state, bool packageReady, bool saveScanComplete);
void enterFrontendGameplay(FrontendState& state, FrontendAction launchAction);
void openFrontendPause(FrontendState& state, FrontendAction selectedAction);
void openFrontendDevOverlay(FrontendState& state, FrontendDevToolsCategory category);
void closeFrontendOverlayToGameplay(FrontendState& state);
bool frontendBlocksGameplayInput(const FrontendState& state);

}  // namespace iggy3d
