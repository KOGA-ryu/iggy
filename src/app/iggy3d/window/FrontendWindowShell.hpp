#pragma once

#include <cstdint>
#include <string>

#include "app/frontend/SettingsMenu.hpp"

namespace iggy3d {

struct FrontendWindowShell {
  bool openingMenuVisible = false;
  bool menuTextDrawn = false;
  bool selectedRowDrawn = false;
  bool mouseMenuSelectUsed = false;
  bool gamepadMenuSelectUsed = false;
  FrontendSettingsTab selectedSettingsTab = FrontendSettingsTab::None;
  std::string launchAction = "none";
  std::string launchStatus = "not_requested";
  std::string packageLoadStatus = "not_requested";
  std::uint64_t framesPresented = 0;
  std::uint64_t eventPollCount = 0;
  std::uint64_t menuRowCount = 0;
  std::string status = "window_not_requested";
};

}  // namespace iggy3d
