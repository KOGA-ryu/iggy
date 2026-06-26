#pragma once

#include <functional>

#include "app/iggy3d/product/Automation.hpp"
#include "app/input/InputRouter.hpp"

namespace iggy3d {

class Session;
struct ProductAppOptions;
struct ProductSaveBridgeResult;

struct ProductAutomationDispatchContext {
  FrontendState& frontend;
  const ProductSaveBridgeResult& saves;
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

bool applyProductAutomationCommand(
    const ProductAutomationCommand& command,
    ProductAutomationDispatchContext& context);

}  // namespace iggy3d
