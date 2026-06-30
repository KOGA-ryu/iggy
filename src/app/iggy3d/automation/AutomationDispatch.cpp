#include "app/iggy3d/automation/AutomationDispatch.hpp"

#include "app/iggy3d/automation/AutomationGameplay.hpp"
#include "app/iggy3d/automation/AutomationRoomEditing.hpp"
#include "app/iggy3d/automation/AutomationSaveBrowser.hpp"
#include "app/iggy3d/automation/AutomationSystem.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/iggy3d/ascii_room/Activation.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"

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

bool routeAutomationInput(FrontendState& frontend,
                          ProductSaveBridgeResult& saves,
                          const ProductAppOptions& options,
                          FrontendSettings& settings,
                          FrontendSettingsTab& settingsTab,
                          std::optional<Session>& activeSession,
                          WorldSetupDraft& worldSetupDraft,
                          ProductAppWindowState& window,
                          InputAction action,
                          bool& closeRequested) {
  ActionState actionState;
  ProductOpeningMenuInputContext menuContext{
      frontend, saves, options, settingsTab, activeSession, worldSetupDraft,
      window, closeRequested, settings};
  routeProductOpeningMenuInput(action, actionState, menuContext);
  window.automationControlLastOwner = productInputOwnerFor(frontend, window);
  return window.lastInputAccepted || action == InputAction::None;
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
      context.activeSession,
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

bool applyProductAutomationAppCommand(const ProductAutomationCommand& command,
                                      ProductAutomationAppContext context) {
  ProductAutomationDispatchContext dispatchContext{
      context.frontend, context.saves, context.options, context.settingsTab,
      // branch-gate: BG-1033
      context.activeSession.has_value() ? &*context.activeSession : nullptr,
      context.worldSetupDraft, context.window,
      [&context]() {
        return productInputOwnerFor(context.frontend, context.window);
      },
      [&context](InputAction action) {
        return routeAutomationInput(context.frontend, context.saves,
                                    context.options, context.settings,
                                    context.settingsTab,
                                    context.activeSession, context.worldSetupDraft,
                                    context.window, action, context.closeRequested);
      },
      [&context]() {
        const ProductAsciiRoomActivationResult activated =
            activateProductAsciiRoomPreview(context.activeSession, context.window);
        // branch-gate: BG-1033
        if (activated.ok) {
          enterProductGameplayTransition(context.frontend, context.window,
                                         FrontendAction::CreateAndEnter);
        }
        return activated.ok;
      },
      [&context](InputAction action) {
        InputRoutingContext routingContext;
        routingContext.owners.editor = context.window.roomEditing.ready;
        routingContext.owners.gameplay = true;
        return routeInputAction(routingContext, action);
      },
      [&context]() {
        returnProductToTitleTransition(context.frontend, context.window,
                                       context.settings);
        context.activeSession.reset();
      },
      [&context]() { context.closeRequested = true; },
  };
  return applyProductAutomationCommand(command, dispatchContext);
}

}  // namespace iggy3d
