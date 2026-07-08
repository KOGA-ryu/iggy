#include "app/iggy3d/save/SaveSlotOperations.hpp"

#include <string>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SaveBrowser.hpp"
#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"

namespace iggy3d {
namespace {

const SaveSlotPreview* saveSlotById(const SaveSlotList& slots,
                                    std::string_view selectedId) {
  for (const SaveSlotPreview& slot : slots.slots) {
    if (slot.id == selectedId) {
      return &slot;
    }
  }
  return nullptr;
}

const SaveSlotPreview* firstSelectableSaveSlot(const SaveSlotList& slots) {
  for (const SaveSlotPreview& slot : slots.slots) {
    if (slot.enabled) {
      return &slot;
    }
  }
  return slots.slots.empty() ? nullptr : &slots.slots.front();
}

void recordSelectedProductSaveSlot(const SaveSlotList& slots,
                                   const SaveSlotPreview* slot,
                                   ProductAppWindowState& window) {
  const SaveSlotRingModel ring =
      // branch-gate: BG-1020
      buildSaveSlotRingModel(slots, slot == nullptr ? "none" : slot->id);
  window.saveSession.saveSlotRingCount = static_cast<std::uint64_t>(ring.items.size());
  window.saveSession.saveSlotRingSelectedIndex = ring.selectedIndex;
  window.saveSession.saveSlotRingSelectedId = ring.selectedSlotId;
  window.saveSession.saveSlotRingSelectedStatus = ring.selectedStatus;

  if (slots.slots.empty()) {
    window.saveSession.selectedProductSave.id = "none";
    window.saveSession.selectedProductSave.enabled = false;
    window.saveSession.selectedProductSave.status = "empty";
    return;
  }
  if (slot == nullptr) {
    window.saveSession.selectedProductSave.id = "none";
    window.saveSession.selectedProductSave.enabled = false;
    window.saveSession.selectedProductSave.status = "missing";
    return;
  }
  window.saveSession.selectedProductSave.id = slot->id.empty() ? "none" : slot->id;
  window.saveSession.selectedProductSave.enabled = slot->enabled;
  window.saveSession.selectedProductSave.status = slot->enabled ? "selected" : "disabled";
}

void recordProductSaveSlotAction(ProductAppWindowState& window,
                                 const SaveSlotActionSpec& action,
                                 std::string_view status) {
  window.saveSession.saveSlotActionCommand = std::string(saveSlotCommandName(action.command));
  window.saveSession.saveSlotActionEnabled = action.enabled;
  window.saveSession.saveSlotActionConfirmationRequired = action.confirmationRequired;
  window.saveSession.saveSlotActionStatus = std::string(status);
}

void recordProductSaveFlowRequest(const ProductSaveFlowRequest& request,
                                  ProductAppWindowState& window) {
  window.saveSession.saveFlow.operation =
      std::string(productSaveFlowOperationName(request.operation));
  // branch-gate: BG-1020
  window.saveSession.saveFlow.sourceSurface = request.sourceSurface.empty()
                                     ? "none"
                                     : request.sourceSurface;
  window.saveSession.saveFlow.affectedSlotId =
      // branch-gate: BG-1020
      request.slotId.empty() ? "none" : request.slotId;
}

void recordProductSaveFlowResult(ProductSaveFlowOperation operation,
                                 std::string_view sourceSurface,
                                 const ProductSaveFlowResult& result,
                                 ProductAppWindowState& window) {
  window.saveSession.saveFlow.operation =
      std::string(productSaveFlowOperationName(operation));
  // branch-gate: BG-1020
  window.saveSession.saveFlow.sourceSurface =
      sourceSurface.empty() ? "none" : std::string(sourceSurface);
  window.saveSession.saveFlow.status = result.status;
  window.saveSession.saveFlow.reasonCode = result.reason;
  window.saveSession.saveFlow.affectedSlotId =
      // branch-gate: BG-1020
      result.affectedSlotId.empty() ? "none" : result.affectedSlotId;
  window.saveSession.saveFlow.activeCountBefore = result.activeCountBefore;
  window.saveSession.saveFlow.activeCountAfter = result.activeCountAfter;
  window.saveSession.saveFlow.deletedCountAfter = result.deletedCountAfter;
  window.saveSession.saveFlow.selectedSlotAfter =
      // branch-gate: BG-1020
      result.selectedSlotAfter.empty() ? "none" : result.selectedSlotAfter;
}

void recordSelectedDeletedProductSaveSlot(const SaveSlotList& slots,
                                          const SaveSlotPreview* slot,
                                          ProductAppWindowState& window) {
  if (slots.slots.empty()) {
    window.saveSession.deletedSelectedSaveId = "none";
    window.saveSession.deletedSelectedSaveEnabled = false;
    window.saveSession.deletedSelectedSaveStatus = "empty";
    return;
  }
  if (slot == nullptr) {
    window.saveSession.deletedSelectedSaveId = "none";
    window.saveSession.deletedSelectedSaveEnabled = false;
    window.saveSession.deletedSelectedSaveStatus = "missing";
    return;
  }
  window.saveSession.deletedSelectedSaveId = slot->id.empty() ? "none" : slot->id;
  window.saveSession.deletedSelectedSaveEnabled = slot->enabled;
  window.saveSession.deletedSelectedSaveStatus = slot->enabled ? "selected" : "disabled";
}

const SaveSlotPreview* initializeSelectedDeletedProductSaveSlot(
    const SaveSlotList& slots,
    ProductAppWindowState& window) {
  const SaveSlotPreview* current =
      window.saveSession.deletedSelectedSaveId == "none"
          ? nullptr
          : saveSlotById(slots, window.saveSession.deletedSelectedSaveId);
  const SaveSlotPreview* selected =
      current == nullptr ? firstSelectableSaveSlot(slots) : current;
  recordSelectedDeletedProductSaveSlot(slots, selected, window);
  return selected;
}

}  // namespace

std::string_view productSaveFlowOperationName(ProductSaveFlowOperation operation) {
  // branch-gate: BG-1020
  switch (operation) {
    case ProductSaveFlowOperation::None:
      return "none";
    case ProductSaveFlowOperation::Delete:
      return "delete";
  }
  return "none";
}

const SaveSlotPreview* initializeSelectedProductSaveSlot(
    const SaveSlotList& slots,
    ProductAppWindowState& window) {
  const SaveSlotPreview* current =
      window.saveSession.selectedProductSave.id == "none"
          ? nullptr
          : saveSlotById(slots, window.saveSession.selectedProductSave.id);
  const SaveSlotPreview* selected =
      current == nullptr ? firstSelectableSaveSlot(slots) : current;
  recordSelectedProductSaveSlot(slots, selected, window);
  return selected;
}

const SaveSlotPreview* moveSelectedProductSaveSlot(const SaveSlotList& slots,
                                                   InputAction action,
                                                   ProductAppWindowState& window) {
  if (slots.slots.empty()) {
    recordSelectedProductSaveSlot(slots, nullptr, window);
    return nullptr;
  }

  const bool previous = action == InputAction::MenuUp;
  const std::string selectedId =
      nextSaveSlotRingSelection(slots, window.saveSession.selectedProductSave.id, previous);
  const SaveSlotPreview* selected = saveSlotById(slots, selectedId);
  recordSelectedProductSaveSlot(slots, selected, window);
  return selected;
}

bool selectProductSaveSlotById(const SaveSlotList& slots,
                               std::string_view selectedId,
                               ProductAppWindowState& window) {
  const SaveSlotPreview* slot = saveSlotById(slots, selectedId);
  recordSelectedProductSaveSlot(slots, slot, window);
  return slot != nullptr;
}

ProductSaveBridgeResult scanDeletedProductSavesForOptions(
    const ProductAppOptions& options) {
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  return scanDeletedProductSaves(options.saveRoot, world.packageId, world.scenarioId);
}

void recordDeletedProductSaveSlots(const ProductSaveBridgeResult& deletedSaves,
                                   ProductAppWindowState& window) {
  window.saveSession.deletedSaveCount =
      static_cast<std::uint64_t>(deletedSaves.slots.slots.size());
  window.saveSession.deletedCompatibleSaveCount = deletedSaves.slots.compatibleCount;
}

bool selectDeletedProductSaveSlotById(const SaveSlotList& slots,
                                      std::string_view selectedId,
                                      ProductAppWindowState& window) {
  const SaveSlotPreview* slot = saveSlotById(slots, selectedId);
  recordSelectedDeletedProductSaveSlot(slots, slot, window);
  return slot != nullptr;
}

void openDeletedProductSaveBrowser(const ProductAppOptions& options,
                                   ProductAppWindowState& window,
                                   FrontendState& frontend) {
  const ProductSaveBridgeResult deletedSaves =
      scanDeletedProductSavesForOptions(options);
  recordDeletedProductSaveSlots(deletedSaves, window);
  window.saveSession.deletedSaveBrowserOpen = true;
  initializeSelectedDeletedProductSaveSlot(deletedSaves.slots, window);
  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.saveBrowserMode = FrontendSaveBrowserMode::Load;
  window.saveSession.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
  frontend.status = "deleted_save_browser_open";
}

void executeProductSaveRecover(const ProductAppOptions& options,
                               ProductAppWindowState& window,
                               FrontendState& frontend) {
  const ProductSaveBridgeResult deletedBefore =
      scanDeletedProductSavesForOptions(options);
  recordDeletedProductSaveSlots(deletedBefore, window);
  const SaveSlotPreview* selected =
      window.saveSession.deletedSelectedSaveId == "none"
          ? initializeSelectedDeletedProductSaveSlot(deletedBefore.slots, window)
          : saveSlotById(deletedBefore.slots, window.saveSession.deletedSelectedSaveId);
  recordSelectedDeletedProductSaveSlot(deletedBefore.slots, selected, window);

  const std::string recoverId =
      selected == nullptr || selected->id.empty() ? "none" : selected->id;
  window.saveSession.saveRecover.saveId = recoverId;
  window.saveSession.saveRecover.snapshotRecovered = false;
  window.saveSession.saveRecover.snapshotMissing = false;
  if (recoverId == "none") {
    window.saveSession.saveRecover.status = "product_save_recover_id_missing";
    window.saveSession.saveRecover.reasonCode = "product_save_recover_id_missing";
    window.saveSession.saveRecover.executed = false;
    frontend.childScreen = FrontendScreen::LoadSave;
    frontend.status = "save_recover_failed";
    return;
  }

  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  const ProductSaveMutationResult mutation = recoverProductSaveAndRefresh({
      options.saveRoot,
      recoverId,
      world.packageId,
      world.scenarioId,
  });
  const ProductSaveRecoverResult& recovered = mutation.recover;
  window.saveSession.saveRecover.status = recovered.status;
  window.saveSession.saveRecover.reasonCode = recovered.reasonCode;
  window.saveSession.saveRecover.executed = recovered.ok;
  window.saveSession.saveRecover.saveId = recovered.saveId.empty() ? "none" : recovered.saveId;
  window.saveSession.saveRecover.snapshotRecovered = recovered.snapshotRecovered;
  window.saveSession.saveRecover.snapshotMissing = recovered.snapshotMissing;

  recordDeletedProductSaveSlots(mutation.deletedSaves, window);
  if (recovered.ok) {
    window.saveSession.deletedSaveBrowserOpen = false;
    recordSelectedDeletedProductSaveSlot(mutation.deletedSaves.slots, nullptr, window);
    selectProductSaveSlotById(mutation.activeSaves.slots, recovered.saveId, window);
  }

  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.selectedAction = FrontendAction::LoadSave;
  frontend.status = recovered.ok ? "save_recover_recovered" : "save_recover_failed";
}

void openProductSaveDeleteConfirmation(const SaveSlotList& slots,
                                       ProductAppWindowState& window,
                                       FrontendState& frontend) {
  frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
  window.saveSession.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
  const SaveSlotPreview* slot =
      window.saveSession.selectedProductSave.id == "none"
          ? nullptr
          : saveSlotById(slots, window.saveSession.selectedProductSave.id);
  if (slot == nullptr) {
    recordSelectedProductSaveSlot(slots, nullptr, window);
    recordProductSaveSlotAction(
        window,
        SaveSlotActionSpec{FrontendAction::Delete,
                           SaveSlotCommand::Delete,
                           "DELETE SELECTED",
                           false,
                           true,
                           "save_delete_unavailable"},
        "save_slot_action_disabled");
    window.saveSession.saveDelete.confirmationOpen = false;
    window.saveSession.saveDelete.candidateId = "none";
    window.saveSession.saveDelete.candidateEnabled = false;
    window.saveSession.saveDelete.status =
        slots.slots.empty() ? "save_delete_unavailable" : "save_delete_missing";
    window.saveSession.saveDelete.reasonCode = window.saveSession.saveDelete.status;
    window.saveSession.saveDelete.type = "soft";
    window.saveSession.saveDelete.recoverable = false;
    window.saveSession.saveDelete.executed = false;
    frontend.status = window.saveSession.saveDelete.status;
    return;
  }

  recordSelectedProductSaveSlot(slots, slot, window);
  recordProductSaveSlotAction(
      window,
      SaveSlotActionSpec{FrontendAction::Delete,
                         SaveSlotCommand::Delete,
                         "DELETE SELECTED",
                         true,
                         true,
                         "none"},
      "save_slot_action_confirm_requested");
  window.saveSession.saveDelete.confirmationOpen = true;
  window.saveSession.saveDelete.candidateId = slot->id.empty() ? "none" : slot->id;
  window.saveSession.saveDelete.candidateEnabled = slot->enabled;
  window.saveSession.saveDelete.status = "confirm_open";
  window.saveSession.saveDelete.reasonCode = "confirm_open";
  window.saveSession.saveDelete.type = "soft";
  window.saveSession.saveDelete.recoverable = false;
  window.saveSession.saveDelete.executed = false;
  frontend.childScreen = FrontendScreen::DeleteConfirm;
  frontend.selectedAction = FrontendAction::Delete;
  frontend.status = "save_delete_confirm_open";
}

void cancelProductSaveDeleteConfirmation(ProductAppWindowState& window,
                                         FrontendState& frontend) {
  window.saveSession.saveDelete.confirmationOpen = false;
  window.saveSession.saveDelete.status = "cancelled";
  window.saveSession.saveDelete.reasonCode = "cancelled";
  window.saveSession.saveDelete.type = "soft";
  window.saveSession.saveDelete.recoverable = false;
  window.saveSession.saveDelete.executed = false;
  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
  window.saveSession.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
  frontend.status = "save_delete_cancelled";
}

ProductSaveFlowResult executeProductSaveSoftDelete(
    const ProductAppOptions& options,
    ProductSaveBridgeResult& saves,
    ProductAppWindowState& window,
    FrontendState& frontend) {
  ProductSaveFlowResult flow;
  flow.activeCountBefore = static_cast<std::uint64_t>(saves.slots.slots.size());
  ProductSaveFlowRequest request;
  request.operation = ProductSaveFlowOperation::Delete;
  request.slotId = window.saveSession.saveDelete.candidateId;
  request.sourceSurface = "delete_world_browser";
  request.confirmationToken = window.saveSession.saveDelete.confirmationOpen
                                  ? "delete_confirm_open"
                                  : "delete_confirm_missing";
  recordProductSaveFlowRequest(request, window);
  window.saveSession.saveDelete.confirmationOpen = false;
  window.saveSession.saveDelete.type = "soft";
  window.saveSession.saveDelete.recoverable = false;
  if (window.saveSession.saveDelete.candidateId == "none" ||
      window.saveSession.saveDelete.candidateId.empty()) {
    window.saveSession.saveDelete.status = "product_save_delete_id_missing";
    window.saveSession.saveDelete.reasonCode = "product_save_delete_id_missing";
    window.saveSession.saveDelete.executed = false;
    frontend.childScreen = FrontendScreen::LoadSave;
    frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
    window.saveSession.saveSlotBrowserMode =
        std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
    frontend.status = "save_delete_failed";
    flow.status = window.saveSession.saveDelete.status;
    flow.reason = window.saveSession.saveDelete.reasonCode;
    flow.affectedSlotId = "none";
    flow.activeCountAfter = flow.activeCountBefore;
    flow.deletedCountAfter = window.saveSession.deletedSaveCount;
    flow.selectedSlotAfter = window.saveSession.selectedProductSave.id;
    recordProductSaveFlowResult(ProductSaveFlowOperation::Delete,
                                "delete_world_browser",
                                flow,
                                window);
    return flow;
  }

  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  const ProductSaveMutationResult mutation = softDeleteProductSaveAndRefresh({
      options.saveRoot,
      window.saveSession.saveDelete.candidateId,
      world.packageId,
      world.scenarioId,
  });
  const ProductSaveSoftDeleteResult& deleted = mutation.softDelete;
  window.saveSession.saveDelete.status = deleted.status;
  window.saveSession.saveDelete.reasonCode = deleted.reasonCode;
  window.saveSession.saveDelete.executed = deleted.ok;
  window.saveSession.saveDelete.recoverable = deleted.ok;
  flow.ok = deleted.ok;
  flow.status = deleted.status;
  flow.reason = deleted.reasonCode;
  // branch-gate: BG-1020
  flow.affectedSlotId = deleted.saveId.empty() ? "none" : deleted.saveId;
  if (deleted.ok) {
    window.saveSession.selectedProductSave.enabled = false;
    window.saveSession.selectedProductSave.status = "missing";
    saves = mutation.activeSaves;
    initializeSelectedProductSaveSlot(saves.slots, window);
  }
  recordDeletedProductSaveSlots(mutation.deletedSaves, window);
  flow.activeCountAfter = static_cast<std::uint64_t>(saves.slots.slots.size());
  flow.deletedCountAfter =
      static_cast<std::uint64_t>(mutation.deletedSaves.slots.slots.size());
  flow.selectedSlotAfter = window.saveSession.selectedProductSave.id;
  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
  window.saveSession.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
  frontend.status = deleted.ok ? "save_delete_soft_deleted" : "save_delete_failed";
  recordProductSaveFlowResult(ProductSaveFlowOperation::Delete,
                              "delete_world_browser",
                              flow,
                              window);
  return flow;
}

}  // namespace iggy3d
