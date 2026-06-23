#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "app/frontend/FrontendState.hpp"
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
  bool pauseMenuOpen = false;
  bool devToolsOpen = false;
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
};

FrontendReceiptFields receiptFieldsFromFrontendState(const FrontendState& state);
void appendFrontendReceiptFields(RenderReceipt& receipt,
                                 const FrontendReceiptFields& fields);

}  // namespace iggy3d
