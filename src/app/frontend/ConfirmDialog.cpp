#include "app/frontend/ConfirmDialog.hpp"

namespace iggy3d {

ConfirmDialogModel buildExitConfirmDialog() {
  ConfirmDialogModel model;
  model.confirmAction = FrontendAction::ExitGame;
  model.title = "Exit Game";
  model.message = "Exit without changing the current save.";
  return model;
}

ConfirmDialogModel buildDeleteSaveConfirmDialog() {
  ConfirmDialogModel model;
  model.confirmAction = FrontendAction::Delete;
  model.title = "Delete Map";
  model.message = "Delete the selected map.";
  model.destructive = true;
  return model;
}

}  // namespace iggy3d
