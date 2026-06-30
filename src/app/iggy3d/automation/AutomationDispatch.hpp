#pragma once

#include <functional>
#include <optional>

#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/automation/Automation.hpp"
#include "app/input/InputRouter.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

class Session;
struct ProductAppOptions;
struct ProductSaveBridgeResult;

struct ProductAutomationDispatchContext {
  FrontendState& frontend;
  ProductSaveBridgeResult& saves;
  const ProductAppOptions& options;
  FrontendSettingsTab& settingsTab;
  Session* activeSession = nullptr;
  WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState& window;
  std::function<MenuOwner()> currentOwner;
  std::function<bool(InputAction)> routeInput;
  std::function<bool()> activateAsciiRoom;
  std::function<InputRoutingResult(InputAction)> routeEditorInput;
  std::function<void()> returnToTitle;
  std::function<void()> requestQuit;
};

struct ProductAutomationAppContext {
  FrontendState& frontend;
  ProductSaveBridgeResult& saves;
  const ProductAppOptions& options;
  FrontendSettings& settings;
  FrontendSettingsTab& settingsTab;
  std::optional<Session>& activeSession;
  WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState& window;
  bool& closeRequested;
};

bool applyProductAutomationCommand(
    const ProductAutomationCommand& command,
    ProductAutomationDispatchContext& context);

bool applyProductAutomationAppCommand(const ProductAutomationCommand& command,
                                      ProductAutomationAppContext context);

}  // namespace iggy3d
