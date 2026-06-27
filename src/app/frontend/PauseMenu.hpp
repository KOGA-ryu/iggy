#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "app/frontend/FrontendRoute.hpp"
#include "app/frontend/FrontendState.hpp"

namespace iggy3d {

struct MenuRowModel {
  FrontendAction action = FrontendAction::None;
  bool enabled = true;
  std::string_view disabledReason = "none";
  std::string_view command = "none";
};

struct PauseMenuContext {
  bool pauseOpen = false;
  bool runtimeSessionAvailable = false;
  bool saveRootWritable = false;
  std::uint64_t compatibleSaveCount = 0;
  bool developerToolsEnabled = true;
  bool activeRoomEditable = false;
  bool roomEditingReady = false;
};

struct PauseMenuModel {
  std::vector<MenuRowModel> rows;
  FrontendAction selected = FrontendAction::Resume;
  bool selectedEnabled = true;
  std::string_view selectedDisabledReason = "none";
  std::string_view selectedCommand = "pause_resume";
  std::uint64_t enabledRowCount = 0;
};

PauseMenuModel buildPauseMenuModel(const PauseMenuContext& context,
                                   FrontendAction selected);
FrontendRouteResult routePauseAction(const PauseMenuModel& model,
                                     FrontendAction action);
std::string_view pauseCommandName(FrontendAction action);

}  // namespace iggy3d
