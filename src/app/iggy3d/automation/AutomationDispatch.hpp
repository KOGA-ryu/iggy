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
namespace creative {
struct CreativeAppState;
}



class Session;
struct FrontendState;
struct ProductAppWindowState;

struct ProductAutomationGameplayContext {
  FrontendState& frontend;
  ProductAppWindowState& window;
  Session* activeSession = nullptr;
  std::function<MenuOwner()> currentOwner;
};

ProductAutomationExecutionResult applyProductGameplayAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationGameplayContext& context);

struct FrontendState;
struct ProductAppOptions;
struct ProductAppWindowState;
struct ProductSaveBridgeResult;

struct ProductAutomationSaveBrowserContext {
  FrontendState& frontend;
  ProductAppWindowState& window;
  const ProductAppOptions& options;
  ProductSaveBridgeResult& saves;
  std::function<MenuOwner()> currentOwner;
};

ProductAutomationExecutionResult applyProductSaveBrowserAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationSaveBrowserContext& context);

struct FrontendState;
struct ProductAppWindowState;

struct ProductAutomationSystemContext {
  FrontendState& frontend;
  ProductAppWindowState& window;
  std::function<MenuOwner()> currentOwner;
  std::function<bool(InputAction)> routeInput;
  std::function<void()> returnToTitle;
  std::function<void()> requestQuit;
};

ProductAutomationExecutionResult applyProductSystemAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationSystemContext& context);
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
  creative::CreativeAppState* creativeApp = nullptr;
};

bool applyProductAutomationCommand(
    const ProductAutomationCommand& command,
    ProductAutomationDispatchContext& context);

bool applyProductAutomationAppCommand(const ProductAutomationCommand& command,
                                      ProductAutomationAppContext context);

}  // namespace iggy3d
