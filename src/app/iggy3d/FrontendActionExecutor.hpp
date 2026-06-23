#pragma once

#include <string_view>

#include "app/frontend/FrontendState.hpp"

namespace iggy3d {

struct ProductFrontendActionResult {
  FrontendAction action = FrontendAction::None;
  bool launchRequested = false;
  bool returnToTitleRequested = false;
  bool quitRequested = false;
  std::string_view status = "frontend_action_not_requested";
};

ProductFrontendActionResult executeProductFrontendAction(FrontendState& state,
                                                         FrontendAction action);

}  // namespace iggy3d
