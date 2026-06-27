#pragma once

#include "app/iggy3d/automation/Automation.hpp"

namespace iggy3d {

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

}  // namespace iggy3d
