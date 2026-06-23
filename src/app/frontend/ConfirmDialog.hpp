#pragma once

#include <string_view>

#include "app/frontend/FrontendState.hpp"

namespace iggy3d {

struct ConfirmDialogModel {
  FrontendAction confirmAction = FrontendAction::None;
  FrontendAction cancelAction = FrontendAction::Back;
  std::string_view title = "Confirm";
  std::string_view message = "Confirm action";
  bool destructive = false;
};

ConfirmDialogModel buildExitConfirmDialog();
ConfirmDialogModel buildDeleteSaveConfirmDialog();

}  // namespace iggy3d
