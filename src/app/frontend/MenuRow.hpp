#pragma once

#include <string_view>

#include "app/frontend/FrontendState.hpp"

namespace iggy3d {

struct MenuRow {
  FrontendAction action = FrontendAction::None;
  std::string_view label = "None";
  std::string_view command = "none";
  bool enabled = true;
  std::string_view disabledReason = "none";
};

}  // namespace iggy3d
