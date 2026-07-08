#include "app/iggy3d/automation/AutomationControl.hpp"

#include <vector>

#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"

namespace iggy3d {

bool automationFailurePreservesLoaded(std::string_view status) {
  return status == "applied" || status == "loaded" ||
         status == "command_failed";
}

void applyProductAutomationControl(ProductAutomationControlContext& context) {
  std::vector<ProductAutomationCommand> commands;
  // branch-gate: BG-1014
  if (!readProductAutomationCommands(context.automationControlPath, context.window,
                                     commands)) {
    return;
  }
  for (const ProductAutomationCommand& command : commands) {
    // branch-gate: BG-1014
    if (!context.applyCommand(command)) {
      // branch-gate: BG-1014
      if (context.window.automationControl.lastKey == "none") {
        context.window.automationControl.lastKey = command.key;
      }
      // branch-gate: BG-1014
      if (context.window.automationControl.lastAction == "none") {
        // branch-gate: BG-1014
        context.window.automationControl.lastAction =
            command.value.empty() ? "none" : command.value;
      }
      context.window.automationControl.lastOwner = context.currentOwner();
      context.window.automationControl.lastResult = "failed";
      // branch-gate: BG-1014
      if (automationFailurePreservesLoaded(context.window.automationControl.status)) {
        context.window.automationControl.status = "command_failed";
      } else {
        context.window.automationControl.loaded = false;
      }
      return;
    }
  }
  // branch-gate: BG-1014
  if (context.window.automationControl.status == "loaded" && commands.empty()) {
    context.window.automationControl.lastResult = "none";
  }
  context.window.frontendShell.selectedSettingsTab = context.settingsTab;
}

}  // namespace iggy3d
