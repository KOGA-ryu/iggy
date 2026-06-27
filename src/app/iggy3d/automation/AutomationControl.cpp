#include "app/iggy3d/automation/AutomationControl.hpp"

#include <vector>

#include "app/iggy3d/ReceiptBuilder.hpp"

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
      if (context.window.automationControlLastKey == "none") {
        context.window.automationControlLastKey = command.key;
      }
      // branch-gate: BG-1014
      if (context.window.automationControlLastAction == "none") {
        // branch-gate: BG-1014
        context.window.automationControlLastAction =
            command.value.empty() ? "none" : command.value;
      }
      context.window.automationControlLastOwner = context.currentOwner();
      context.window.automationControlLastResult = "failed";
      // branch-gate: BG-1014
      if (automationFailurePreservesLoaded(context.window.automationControlStatus)) {
        context.window.automationControlStatus = "command_failed";
      } else {
        context.window.automationControlLoaded = false;
      }
      return;
    }
  }
  // branch-gate: BG-1014
  if (context.window.automationControlStatus == "loaded" && commands.empty()) {
    context.window.automationControlLastResult = "none";
  }
  context.window.selectedSettingsTab = context.settingsTab;
  context.window.inputOwner = context.currentOwner();
  context.window.gameplayInputSuppressed =
      context.gameplaySuppressed(context.window.inputOwner);
}

}  // namespace iggy3d
