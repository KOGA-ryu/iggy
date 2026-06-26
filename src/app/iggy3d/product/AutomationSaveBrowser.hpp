#pragma once

#include "app/iggy3d/product/Automation.hpp"

namespace iggy3d {

struct FrontendState;
struct ProductAppOptions;
struct ProductAppWindowState;
struct ProductSaveBridgeResult;

struct ProductAutomationSaveBrowserContext {
  FrontendState& frontend;
  ProductAppWindowState& window;
  const ProductAppOptions& options;
  const ProductSaveBridgeResult& saves;
  std::function<MenuOwner()> currentOwner;
};

ProductAutomationExecutionResult applyProductSaveBrowserAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationSaveBrowserContext& context);

}  // namespace iggy3d
