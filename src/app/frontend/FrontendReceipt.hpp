#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "render/RenderDiagnostics.hpp"

namespace iggy3d {

struct FrontendReceiptFields {
  FrontendScreen screen = FrontendScreen::BootStatus;
  FrontendAction selectedAction = FrontendAction::None;
  std::string status = "frontend_boot_pending";
  bool launchRequested = false;
  bool returnToTitleRequested = false;
  bool inputOwned = true;
  FrontendScreen childScreen = FrontendScreen::Gameplay;
  FrontendAction disabledAction = FrontendAction::None;
  bool pauseOpen = false;
  bool devToolsVisible = false;
  FrontendDevToolsCategory devToolsCategory = FrontendDevToolsCategory::None;
  std::string settingsInputBackend = "keyboard";
  std::string settingsLookSensitivity = "1.000";
  bool settingsInvertLook = false;
  std::string selectedSaveId = "none";
  std::string selectedPackageId = "none";
  std::string selectedScenarioId = "none";
  std::filesystem::path saveRoot;
  std::uint64_t saveCount = 0;
  std::uint64_t compatibleSaveCount = 0;
  std::uint64_t corruptSaveCount = 0;
  std::string selectedSaveCompatible = "unavailable";
  std::string selectedSavePackageId = "none";
  std::string selectedSaveScenarioId = "none";
  std::uint64_t selectedSaveTick = 0;
  std::string selectedSaveHash = "none";
  std::uint64_t selectedSaveAuthoredFloorCount = 0;
  std::uint64_t selectedSaveAuthoredWallCount = 0;
  bool starterHeaderVisible = false;
  bool starterActionListVisible = false;
  bool starterDetailPanelVisible = false;
  bool starterStatusStripVisible = false;
  std::uint64_t starterRowCount = 0;
  bool starterSelectedEnabled = false;
  std::string starterSelectedDisabledReason = "no_compatible_save";
  std::string menuOwner = "none";
  bool gameplayInputSuppressed = false;
  FrontendAction pauseSelectedAction = FrontendAction::None;
  bool pauseSelectedEnabled = true;
  std::string pauseSelectedDisabledReason = "none";
  std::string pauseActionCommand = "none";
  bool pauseActionExecuted = false;
  std::string pauseActionStatus = "not_requested";
  std::string pauseParentScreen = "none";
  std::uint64_t pauseRowCount = 0;
  std::uint64_t pauseEnabledRowCount = 0;
  bool settingsOpen = false;
  std::string settingsParent = "none";
  FrontendSettingsTab settingsTab = FrontendSettingsTab::None;
  std::string settingsSelectedRow = "none";
  bool settingsSelectedEnabled = true;
  std::string settingsSelectedDisabledReason = "none";
  std::string settingsPersistence = "runtime_only";
  bool settingsApplyRequested = false;
  bool settingsRestoreDefaultsRequested = false;
  bool settingsBackRequested = false;
  std::string settingsRenderer = "unavailable";
  std::string settingsWindowMode = "unavailable";
  std::string settingsControllerLookSensitivity = "1.000";
  bool settingsAudioAvailable = false;
  bool settingsAccessibilityHighContrast = false;
  bool settingsAccessibilityReducedMotion = false;
  bool settingsDeveloperToolsEnabled = false;
  std::string devToolsParent = "none";
  bool devToolsInputBlocking = false;
  bool devToolsReadoutVisible = false;
  std::string devToolsSelectedAction = "none";
  bool devToolsSelectedEnabled = true;
  std::string devToolsSelectedDisabledReason = "none";
  std::string devToolsCommandStatus = "read_only";
  std::uint64_t devToolsRuntimeReadoutCount = 0;
  std::string gamepadOptionsOpens = "pause";
  bool gamepadCreateOptionsQuit = true;
  std::uint64_t windowLaunchCount = 0;
};

FrontendReceiptFields receiptFieldsFromFrontendState(const FrontendState& state);
void appendFrontendReceiptFields(RenderReceipt& receipt,
                                 const FrontendReceiptFields& fields);

}  // namespace iggy3d
