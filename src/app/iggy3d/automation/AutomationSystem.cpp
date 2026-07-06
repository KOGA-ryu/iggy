#include "app/iggy3d/automation/AutomationSystem.hpp"

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"

namespace iggy3d {

namespace {

ProductAutomationExecutionResult unhandledSystemAutomation() {
  return {};
}

ProductAutomationExecutionResult failSystemAutomation() {
  return {true, false};
}

ProductAutomationExecutionResult passSystemAutomation(bool accepted) {
  return {true, accepted};
}

bool resolveSystemBool(ProductAutomationSystemContext& context,
                       std::string_view value,
                       bool& boolValue) {
  // branch-gate: BG-1012
  if (resolveProductAutomationBool(value, boolValue)) {
    return true;
  }
  context.window.automationControl.status = "invalid_value";
  return false;
}

ProductAutomationExecutionResult applyRoutedSystemInput(
    const ProductAutomationCommand& command,
    ProductAutomationSystemContext& context,
    InputAction action) {
  const bool routed = context.routeInput(action);
  markAutomationApplied(context.window, command, inputActionName(action),
                        context.window.automationControl.lastOwner,
                        // branch-gate: BG-1012
                        routed ? "applied" : "ignored");
  return passSystemAutomation(routed);
}

ProductAutomationExecutionResult ignoreSystemBool(
    const ProductAutomationCommand& command,
    ProductAutomationSystemContext& context) {
  markAutomationApplied(context.window, command, "none", context.currentOwner(),
                        "ignored");
  return passSystemAutomation(true);
}

}  // namespace

ProductAutomationExecutionResult applyProductSystemAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationSystemContext& context) {
  const std::string_view value{command.value};
  bool boolValue = false;

  // branch-gate: BG-1012
  if (automationSpec.commandId == ProductAutomationCommandId::FrontendExecute) {
    // branch-gate: BG-1012
    if (!resolveSystemBool(context, value, boolValue)) {
      return failSystemAutomation();
    }
    // branch-gate: BG-1012
    if (!boolValue) {
      return ignoreSystemBool(command, context);
    }
    return applyRoutedSystemInput(command, context, InputAction::MenuConfirm);
  }

  // branch-gate: BG-1012
  if (automationSpec.commandId == ProductAutomationCommandId::SettingsApply ||
      automationSpec.commandId ==
          ProductAutomationCommandId::SettingsRestoreDefaults) {
    // branch-gate: BG-1012
    if (!resolveSystemBool(context, value, boolValue)) {
      return failSystemAutomation();
    }
    // branch-gate: BG-1012
    if (boolValue) {
      context.frontend.status =
          automationSpec.commandId == ProductAutomationCommandId::SettingsApply
              ? "settings_applied"
              : "settings_defaults_restored";
    }
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          // branch-gate: BG-1012
                          boolValue ? "applied" : "ignored");
    return passSystemAutomation(true);
  }

  // branch-gate: BG-1012
  if (automationSpec.commandId == ProductAutomationCommandId::SettingsBack) {
    // branch-gate: BG-1012
    if (!resolveSystemBool(context, value, boolValue)) {
      return failSystemAutomation();
    }
    // branch-gate: BG-1012
    if (!boolValue) {
      return ignoreSystemBool(command, context);
    }
    return applyRoutedSystemInput(command, context, InputAction::MenuBack);
  }

  // branch-gate: BG-1012
  if (automationSpec.commandId ==
      ProductAutomationCommandId::FrontendReturnToTitle) {
    // branch-gate: BG-1012
    if (!resolveSystemBool(context, value, boolValue)) {
      return failSystemAutomation();
    }
    // branch-gate: BG-1012
    if (boolValue) {
      context.returnToTitle();
    }
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          // branch-gate: BG-1012
                          boolValue ? "applied" : "ignored");
    return passSystemAutomation(true);
  }

  // branch-gate: BG-1012
  if (automationSpec.commandId == ProductAutomationCommandId::SystemPause) {
    // branch-gate: BG-1012
    if (!resolveSystemBool(context, value, boolValue)) {
      return failSystemAutomation();
    }
    // branch-gate: BG-1012
    if (!boolValue) {
      return ignoreSystemBool(command, context);
    }
    return applyRoutedSystemInput(command, context, InputAction::SystemPause);
  }

  // branch-gate: BG-1012
  if (automationSpec.commandId == ProductAutomationCommandId::SystemQuit) {
    // branch-gate: BG-1012
    if (!resolveSystemBool(context, value, boolValue)) {
      return failSystemAutomation();
    }
    // branch-gate: BG-1012
    if (boolValue) {
      context.requestQuit();
    }
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          // branch-gate: BG-1012
                          boolValue ? "applied" : "ignored");
    return passSystemAutomation(true);
  }

  return unhandledSystemAutomation();
}

}  // namespace iggy3d
