#include "app/iggy3d/automation/AutomationSaveBrowser.hpp"

#include <array>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/ProductAppOperations.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"

namespace iggy3d {

namespace {

ProductAutomationExecutionResult unhandledSaveBrowserAutomation() {
  return {};
}

ProductAutomationExecutionResult failSaveBrowserAutomation() {
  return {true, false};
}

ProductAutomationExecutionResult passSaveBrowserAutomation(bool accepted) {
  return {true, accepted};
}

bool requireLoadSaveChildScreen(ProductAutomationSaveBrowserContext& context,
                                const ProductAutomationCommand& command,
                                std::string_view operation) {
  // branch-gate: BG-1008
  if (context.frontend.childScreen == FrontendScreen::LoadSave) {
    return true;
  }
  context.window.automationControlStatus = "owner_unavailable";
  markAutomationApplied(context.window, command, operation,
                        context.currentOwner(), "failed");
  return false;
}

bool resolveSaveBrowserBool(ProductAutomationSaveBrowserContext& context,
                            std::string_view value,
                            bool& boolValue) {
  // branch-gate: BG-1008
  if (resolveProductSaveBrowserBoolAutomation(value, boolValue)) {
    return true;
  }
  context.window.automationControlStatus = "invalid_value";
  return false;
}

void recordSaveSelectionParseStatus(ProductAutomationSaveBrowserContext& context,
                                    bool valid) {
  // branch-gate: BG-1008
  context.window.automationControlStatus =
      std::array<std::string_view, 2>{
          context.window.automationControlStatus,
          std::string_view{"invalid_value"}}[!valid];
}

}  // namespace

ProductAutomationExecutionResult applyProductSaveBrowserAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationSaveBrowserContext& context) {
  const std::string_view value{command.value};
  bool boolValue = false;

  // branch-gate: BG-1008
  if (automationSpec.commandId == ProductAutomationCommandId::SaveSelect) {
    // branch-gate: BG-1008
    if (!requireLoadSaveChildScreen(context, command,
                                    automationSpec.canonicalKey)) {
      return failSaveBrowserAutomation();
    }
    const ProductSaveSelectionAutomationResult saveSelection =
        resolveProductSaveSelectionAutomation(value);
    recordSaveSelectionParseStatus(context, saveSelection.valid);
    // branch-gate: BG-1008
    if (!saveSelection.valid) {
      return failSaveBrowserAutomation();
    }

    const bool selected =
        selectProductSaveSlotById(context.saves.slots, saveSelection.saveId,
                                  context.window);
    // branch-gate: BG-1008
    markAutomationApplied(context.window, command, context.window.selectedProductSaveId,
                          context.currentOwner(), selected ? "applied" : "ignored");
    return passSaveBrowserAutomation(selected);
  }

  // branch-gate: BG-1008
  if (automationSpec.commandId == ProductAutomationCommandId::SaveDelete) {
    // branch-gate: BG-1008
    if (!resolveSaveBrowserBool(context, value, boolValue)) {
      return failSaveBrowserAutomation();
    }
    // branch-gate: BG-1008
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passSaveBrowserAutomation(true);
    }
    // branch-gate: BG-1008
    if (!requireLoadSaveChildScreen(context, command,
                                    automationSpec.canonicalKey)) {
      return failSaveBrowserAutomation();
    }
    openProductSaveDeleteConfirmation(context.saves.slots, context.window,
                                      context.frontend);
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(), "applied");
    return passSaveBrowserAutomation(true);
  }

  // branch-gate: BG-1008
  if (automationSpec.commandId == ProductAutomationCommandId::SaveShowDeleted) {
    // branch-gate: BG-1008
    if (!resolveSaveBrowserBool(context, value, boolValue)) {
      return failSaveBrowserAutomation();
    }
    // branch-gate: BG-1008
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passSaveBrowserAutomation(true);
    }
    // branch-gate: BG-1008
    if (!requireLoadSaveChildScreen(context, command,
                                    automationSpec.canonicalKey)) {
      return failSaveBrowserAutomation();
    }
    openDeletedProductSaveBrowser(context.options, context.window, context.frontend);
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(), "applied");
    return passSaveBrowserAutomation(true);
  }

  // branch-gate: BG-1008
  if (automationSpec.commandId == ProductAutomationCommandId::SaveDeletedSelect) {
    // branch-gate: BG-1008
    if (!requireLoadSaveChildScreen(context, command,
                                    automationSpec.canonicalKey)) {
      return failSaveBrowserAutomation();
    }
    // branch-gate: BG-1008
    if (!context.window.deletedSaveBrowserOpen) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "failed");
      return failSaveBrowserAutomation();
    }
    const ProductSaveSelectionAutomationResult saveSelection =
        resolveProductSaveSelectionAutomation(value);
    recordSaveSelectionParseStatus(context, saveSelection.valid);
    // branch-gate: BG-1008
    if (!saveSelection.valid) {
      return failSaveBrowserAutomation();
    }

    const ProductSaveBridgeResult deletedSaves =
        scanDeletedProductSavesForOptions(context.options);
    recordDeletedProductSaveSlots(deletedSaves, context.window);
    const bool selected =
        selectDeletedProductSaveSlotById(deletedSaves.slots, saveSelection.saveId,
                                         context.window);
    markAutomationApplied(context.window, command,
                          context.window.deletedSelectedSaveId,
                          context.currentOwner(),
                          // branch-gate: BG-1008
                          selected ? "applied" : "ignored");
    return passSaveBrowserAutomation(selected);
  }

  // branch-gate: BG-1008
  if (automationSpec.commandId == ProductAutomationCommandId::SaveRecover) {
    // branch-gate: BG-1008
    if (!resolveSaveBrowserBool(context, value, boolValue)) {
      return failSaveBrowserAutomation();
    }
    // branch-gate: BG-1008
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passSaveBrowserAutomation(true);
    }
    // branch-gate: BG-1008
    if (!requireLoadSaveChildScreen(context, command,
                                    automationSpec.canonicalKey)) {
      return failSaveBrowserAutomation();
    }
    // branch-gate: BG-1008
    if (!context.window.deletedSaveBrowserOpen) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "failed");
      return failSaveBrowserAutomation();
    }
    executeProductSaveRecover(context.options, context.window, context.frontend);
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          // branch-gate: BG-1008
                          context.window.saveRecoverExecuted ? "applied" : "failed");
    return passSaveBrowserAutomation(context.window.saveRecoverExecuted);
  }

  return unhandledSaveBrowserAutomation();
}

}  // namespace iggy3d
