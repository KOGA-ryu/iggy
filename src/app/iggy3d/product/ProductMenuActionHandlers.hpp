#pragma once

#include <optional>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/input/InputAction.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductPauseMenuActionContext {
  FrontendState& frontend;
  const ProductAppOptions& options;
  FrontendSettingsTab& settingsTab;
  std::optional<Session>& activeSession;
  ProductAppWindowState& window;
  bool& closeRequested;
};

struct ProductDevToolsMenuActionContext {
  FrontendState& frontend;
  ProductAppWindowState& window;
};

struct ProductMenuActionResult {
  bool handled = false;
  bool accepted = false;
};

ProductMenuActionResult applyProductPauseMenuAction(
    InputAction action,
    ProductPauseMenuActionContext& context);

ProductMenuActionResult applyProductDevOverlayMenuAction(
    InputAction action,
    ProductDevToolsMenuActionContext& context);

ProductMenuActionResult applyProductStarterDevToolsMenuAction(
    InputAction action,
    ProductDevToolsMenuActionContext& context);

}  // namespace iggy3d
