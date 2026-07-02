#pragma once

#include <optional>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/input/InputAction.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductSaveBridgeResult;
struct WorldSetupDraft;

struct ProductPauseMenuActionContext {
  FrontendState& frontend;
  const ProductAppOptions& options;
  FrontendSettingsTab& settingsTab;
  std::optional<Session>& activeSession;
  ProductAppWindowState& window;
  bool& closeRequested;
  FrontendSettings& settings;
};

struct ProductDevToolsMenuActionContext {
  FrontendState& frontend;
  ProductAppWindowState& window;
};

struct ProductSettingsMenuActionContext {
  FrontendState& frontend;
  FrontendSettingsTab& settingsTab;
  ProductAppWindowState& window;
};

struct ProductDeleteConfirmMenuActionContext {
  FrontendState& frontend;
  const ProductAppOptions& options;
  ProductSaveBridgeResult& saves;
  ProductAppWindowState& window;
};

struct ProductLoadSaveMenuActionContext {
  FrontendState& frontend;
  const ProductAppOptions& options;
  ProductSaveBridgeResult& saves;
  std::optional<Session>& activeSession;
  ProductAppWindowState& window;
};

struct ProductNewWorldMenuActionContext {
  FrontendState& frontend;
  const ProductAppOptions& options;
  ProductSaveBridgeResult& saves;
  std::optional<Session>& activeSession;
  WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState& window;
};

struct ProductStarterMenuActionContext {
  FrontendState& frontend;
  const ProductAppOptions& options;
  ProductSaveBridgeResult& saves;
  FrontendSettingsTab& settingsTab;
  std::optional<Session>& activeSession;
  WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState& window;
  bool& closeRequested;
};

struct ProductSystemPauseMenuActionContext {
  FrontendState& frontend;
  ProductAppWindowState& window;
  bool& closeRequested;
  FrontendSettings* settings = nullptr;
};

struct ProductGameplayMapMakerToggleActionContext {
  FrontendState& frontend;
  ProductAppWindowState& window;
};

struct ProductMenuActionResult {
  bool handled = false;
  bool accepted = false;
};

ProductMenuActionResult applyProductPauseMenuAction(
    InputAction action,
    ProductPauseMenuActionContext context);

ProductMenuActionResult applyProductDevOverlayMenuAction(
    InputAction action,
    ProductDevToolsMenuActionContext context);

ProductMenuActionResult applyProductStarterDevToolsMenuAction(
    InputAction action,
    ProductDevToolsMenuActionContext context);

ProductMenuActionResult applyProductSettingsMenuAction(
    InputAction action,
    ProductSettingsMenuActionContext context);

ProductMenuActionResult applyProductDeleteConfirmMenuAction(
    InputAction action,
    ProductDeleteConfirmMenuActionContext context);

ProductMenuActionResult applyProductLoadSaveMenuAction(
    InputAction action,
    ProductLoadSaveMenuActionContext context);

ProductMenuActionResult applyProductNewWorldMenuAction(
    InputAction action,
    ProductNewWorldMenuActionContext context);

ProductMenuActionResult applyProductStarterMenuAction(
    InputAction action,
    ProductStarterMenuActionContext context);

ProductMenuActionResult applyProductSystemPauseMenuAction(
    InputAction action,
    ProductSystemPauseMenuActionContext context);

ProductMenuActionResult applyProductGameplayMapMakerToggleAction(
    InputAction action,
    ProductGameplayMapMakerToggleActionContext context);

}  // namespace iggy3d
