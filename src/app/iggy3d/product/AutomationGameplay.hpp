#pragma once

#include "app/iggy3d/product/Automation.hpp"

namespace iggy3d {

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

}  // namespace iggy3d
