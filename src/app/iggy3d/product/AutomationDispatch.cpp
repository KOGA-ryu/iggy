#include "app/iggy3d/product/AutomationDispatch.hpp"

#include "app/iggy3d/product/AutomationGameplay.hpp"
#include "app/iggy3d/product/AutomationRoomEditing.hpp"
#include "app/iggy3d/product/AutomationSaveBrowser.hpp"
#include "app/iggy3d/product/AutomationSystem.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"

namespace iggy3d {

namespace {

bool returnIfHandled(const ProductAutomationExecutionResult& execution,
                     bool& accepted) {
  // branch-gate: BG-1015
  if (execution.handled) {
    accepted = execution.accepted;
    return true;
  }
  return false;
}

}  // namespace

bool applyProductAutomationCommand(
    const ProductAutomationCommand& command,
    ProductAutomationDispatchContext& context) {
  const std::string_view key{command.key};
  static const ProductAutomationCommandRegistry automationRegistry =
      makeProductAutomationCommandRegistry();
  const std::string_view canonicalKey =
      productAutomationCanonicalKey(automationRegistry, key);
  const ProductAutomationCommandDispatchResult automationDispatch =
      resolveProductAutomationCommandDispatch({&automationRegistry, key});
  const ProductAutomationCommandDispatchSpec& automationSpec =
      automationDispatch.spec;

  bool accepted = false;

  ProductAutomationExecutionContext commonExecutionContext{
      context.frontend,
      context.settingsTab,
      context.window,
      context.currentOwner,
      context.routeInput,
  };
  // branch-gate: BG-1015
  if (returnIfHandled(applyProductCommonAutomationCommand(command, automationSpec,
                                                          commonExecutionContext),
                      accepted)) {
    return accepted;
  }

  ProductAutomationWorldSetupContext worldSetupExecutionContext{
      context.frontend,
      context.worldSetupDraft,
      context.window,
      context.currentOwner,
      context.activateAsciiRoom,
      context.routeInput,
  };
  // branch-gate: BG-1015
  if (returnIfHandled(applyProductWorldSetupAutomationCommand(
                          command, canonicalKey, worldSetupExecutionContext),
                      accepted)) {
    return accepted;
  }

  ProductAutomationRoomEditingContext roomEditingExecutionContext{
      context.frontend,
      context.window,
      context.activeSession != nullptr,
      context.currentOwner,
      context.routeEditorInput,
  };
  // branch-gate: BG-1015
  if (returnIfHandled(applyProductRoomEditingAutomationCommand(
                          command, automationSpec, roomEditingExecutionContext),
                      accepted)) {
    return accepted;
  }

  ProductAutomationGameplayContext gameplayExecutionContext{
      context.frontend,
      context.window,
      context.activeSession,
      context.currentOwner,
  };
  // branch-gate: BG-1015
  if (returnIfHandled(applyProductGameplayAutomationCommand(
                          command, automationSpec, gameplayExecutionContext),
                      accepted)) {
    return accepted;
  }

  ProductAutomationSaveBrowserContext saveBrowserExecutionContext{
      context.frontend,
      context.window,
      context.options,
      context.saves,
      context.currentOwner,
  };
  // branch-gate: BG-1015
  if (returnIfHandled(applyProductSaveBrowserAutomationCommand(
                          command, automationSpec, saveBrowserExecutionContext),
                      accepted)) {
    return accepted;
  }

  ProductAutomationSystemContext systemExecutionContext{
      context.frontend,
      context.window,
      context.currentOwner,
      context.routeInput,
      context.returnToTitle,
      context.requestQuit,
  };
  // branch-gate: BG-1015
  if (returnIfHandled(applyProductSystemAutomationCommand(
                          command, automationSpec, systemExecutionContext),
                      accepted)) {
    return accepted;
  }

  context.window.automationControlStatus = "unknown_key";
  return false;
}

}  // namespace iggy3d
