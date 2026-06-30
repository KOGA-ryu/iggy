#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "app/frontend/FrontendRoute.hpp"
#include "app/frontend/FrontendState.hpp"

namespace iggy3d {

struct StarterScreenModel {
  std::vector<FrontendAction> actions;
  FrontendAction selected = FrontendAction::Continue;
  FrontendAction disabled = FrontendAction::None;
  bool continueEnabled = false;
  bool selectedEnabled = false;
  std::string_view selectedDisabledReason = "no_compatible_save";
  std::string_view selectedCommand = "starter_continue";
  bool headerVisible = true;
  bool actionListVisible = true;
  bool detailPanelVisible = true;
  bool statusStripVisible = true;
  std::uint64_t compatibleSaveCount = 0;
};

StarterScreenModel buildStarterScreenModel(std::uint64_t compatibleSaveCount,
                                           FrontendAction selected);
FrontendRouteResult routeStarterAction(const StarterScreenModel& model,
                                       FrontendAction action);
FrontendRouteResult routeStarterBackFromChild(FrontendScreen childScreen);
std::string_view starterActionLabel(FrontendAction action);
std::string_view starterActionCommand(FrontendAction action);
std::string_view starterActionDisabledReason(FrontendAction action,
                                             std::uint64_t compatibleSaveCount);
bool starterActionEnabled(FrontendAction action, std::uint64_t compatibleSaveCount);

}  // namespace iggy3d
