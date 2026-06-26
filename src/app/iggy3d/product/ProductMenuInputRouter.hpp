#pragma once

#include <optional>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputAction.hpp"
#include "app/input/InputRouter.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductSaveBridgeResult;
struct WorldSetupDraft;

struct ProductOpeningMenuInputContext {
  FrontendState& frontend;
  const ProductSaveBridgeResult& saves;
  const ProductAppOptions& options;
  FrontendSettingsTab& settingsTab;
  std::optional<Session>& activeSession;
  WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState& window;
  bool& closeRequested;
};

MenuOwner productInputOwnerFor(const FrontendState& frontend,
                               const ProductAppWindowState& window);

void applyProductOpeningMenuAction(InputAction action,
                                   ProductOpeningMenuInputContext context);

void routeProductOpeningMenuInput(InputAction inputAction,
                                  ActionState& actionState,
                                  ProductOpeningMenuInputContext context);

}  // namespace iggy3d
