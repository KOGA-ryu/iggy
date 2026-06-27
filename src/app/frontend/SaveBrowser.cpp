#include "app/frontend/SaveBrowser.hpp"

namespace iggy3d {
namespace {

VerticalSelectorItem selectorItemFromSaveSlot(const SaveSlotPreview& slot) {
  VerticalSelectorItem item;
  item.id = slot.id;
  item.kind = "save";
  item.title = slot.displayTitle.empty() ? slot.id : slot.displayTitle;
  item.snapshotRef =
      slot.snapshotAvailable ? slot.snapshotPath.string() : std::string{};
  item.timestamp = slot.timestampLabel.empty() ? "unknown" : slot.timestampLabel;
  item.enabled = slot.enabled;
  item.disabledReason = slot.enabled ? "none" : slot.reason;
  return item;
}

std::size_t selectedIndexForId(const SaveSlotList& slots, std::string_view selectedSaveId) {
  for (std::size_t index = 0; index < slots.slots.size(); ++index) {
    if (slots.slots[index].id == selectedSaveId) {
      return index;
    }
  }
  return 0;
}

void copySelectedPresentation(SaveBrowserModel& model, const SaveSlotPreview& slot) {
  model.selectedTitle = slot.displayTitle.empty() ? slot.id : slot.displayTitle;
  model.selectedTimestamp = slot.timestampLabel.empty() ? "unknown" : slot.timestampLabel;
  model.selectedSnapshotAvailable = slot.snapshotAvailable;
  model.selectedSnapshotFallback = slot.snapshotFallback;
  model.selectedSnapshotStatus = slot.snapshotStatus;
}

bool validSaveBrowserParent(MenuOwner parentOwner) {
  return parentOwner == MenuOwner::Starter || parentOwner == MenuOwner::Pause;
}

FrontendScreen saveBrowserParentScreen(MenuOwner parentOwner) {
  return parentOwner == MenuOwner::Pause ? FrontendScreen::Pause
                                         : FrontendScreen::Starter;
}

const SaveSlotPreview* selectedSaveSlot(const SaveBrowserModel& model) {
  for (const SaveSlotPreview& slot : model.slots.slots) {
    if (slot.id == model.selectedSaveId) {
      return &slot;
    }
  }
  return nullptr;
}

std::string_view loadDisabledReason(const SaveBrowserModel& model) {
  const SaveSlotPreview* slot = selectedSaveSlot(model);
  if (slot != nullptr && !slot->enabled) {
    return slot->reason;
  }
  return model.status;
}

std::string_view deleteDisabledReason(const SaveBrowserModel& model) {
  if (model.status == "save_browser_selection_ready" ||
      model.status == "save_browser_selection_disabled") {
    return "save_browser_delete_unavailable";
  }
  return model.status;
}

FrontendRouteResult ignoredSaveBrowserRoute(MenuOwner parentOwner,
                                            FrontendAction action,
                                            std::string_view status) {
  const bool validParent = validSaveBrowserParent(parentOwner);
  FrontendRouteResult result = makeIgnoredFrontendRouteResult(
      validParent ? parentOwner : MenuOwner::None,
      validParent ? saveBrowserParentScreen(parentOwner) : FrontendScreen::BootStatus,
      validParent ? FrontendScreen::LoadSave : FrontendScreen::Gameplay,
      action);
  result.gameplayInputSuppressed = true;
  result.status = status;
  result.receiptReason = status;
  return result;
}

FrontendRouteResult acceptedSaveBrowserRoute(MenuOwner owner,
                                             FrontendScreen nextScreen,
                                             FrontendScreen nextChildScreen,
                                             FrontendTransitionRequest transition,
                                             bool gameplayInputSuppressed,
                                             std::string_view status,
                                             FrontendAction action) {
  return makeAcceptedFrontendRouteResult(owner,
                                         nextScreen,
                                         nextChildScreen,
                                         transition,
                                         false,
                                         gameplayInputSuppressed,
                                         status,
                                         status,
                                         action);
}

}  // namespace

SaveBrowserModel buildSaveBrowserModel(const SaveSlotList& slots,
                                       std::string_view selectedSaveId) {
  SaveBrowserModel model;
  model.slots = slots;
  model.selectedSaveId = selectedSaveId.empty() ? "none" : std::string(selectedSaveId);
  model.selectorItems.reserve(model.slots.slots.size());
  for (const SaveSlotPreview& slot : model.slots.slots) {
    model.selectorItems.push_back(selectorItemFromSaveSlot(slot));
  }

  model.selectorState = makeVerticalSelectorState(
      model.selectorItems.size(),
      selectedIndexForId(model.slots, model.selectedSaveId),
      true);
  model.selectorResult = applyVerticalSelectorInput(
      model.selectorItems, model.selectorState, VerticalSelectorInput::None);

  for (const SaveSlotPreview& slot : model.slots.slots) {
    if (slot.id == model.selectedSaveId) {
      model.loadEnabled = slot.enabled;
      model.deleteEnabled = true;
      copySelectedPresentation(model, slot);
      model.selectorState = makeVerticalSelectorState(
          model.selectorItems.size(),
          selectedIndexForId(model.slots, model.selectedSaveId),
          true);
      model.selectorResult = applyVerticalSelectorInput(
          model.selectorItems, model.selectorState, VerticalSelectorInput::None);
      model.status = slot.enabled ? "save_browser_selection_ready"
                                  : "save_browser_selection_disabled";
      return model;
    }
  }

  model.status = model.slots.slots.empty() ? "save_browser_empty"
                                           : "save_browser_selection_missing";
  return model;
}

FrontendRouteResult routeSaveBrowserAction(const SaveBrowserModel& model,
                                           MenuOwner parentOwner,
                                           FrontendAction action) {
  if (!validSaveBrowserParent(parentOwner)) {
    return ignoredSaveBrowserRoute(parentOwner, action, "save_browser_invalid_parent");
  }

  switch (action) {
    case FrontendAction::Back:
      return acceptedSaveBrowserRoute(parentOwner,
                                      saveBrowserParentScreen(parentOwner),
                                      FrontendScreen::Gameplay,
                                      FrontendTransitionRequest::None,
                                      true,
                                      parentOwner == MenuOwner::Pause
                                          ? "save_browser_closed_to_pause"
                                          : "save_browser_closed_to_starter",
                                      action);
    case FrontendAction::Load:
      if (!model.loadEnabled) {
        return ignoredSaveBrowserRoute(parentOwner, action, loadDisabledReason(model));
      }
      return acceptedSaveBrowserRoute(MenuOwner::Gameplay,
                                      FrontendScreen::Gameplay,
                                      FrontendScreen::Gameplay,
                                      FrontendTransitionRequest::LaunchGameplay,
                                      false,
                                      "save_browser_load_requested",
                                      action);
    case FrontendAction::Delete:
      if (!model.deleteEnabled) {
        return ignoredSaveBrowserRoute(parentOwner, action, deleteDisabledReason(model));
      }
      return acceptedSaveBrowserRoute(parentOwner,
                                      saveBrowserParentScreen(parentOwner),
                                      FrontendScreen::DeleteConfirm,
                                      FrontendTransitionRequest::None,
                                      true,
                                      "save_browser_delete_confirm_requested",
                                      action);
    case FrontendAction::None:
    case FrontendAction::Continue:
    case FrontendAction::NewWorld:
    case FrontendAction::LoadSave:
    case FrontendAction::Settings:
    case FrontendAction::DevTools:
    case FrontendAction::Exit:
    case FrontendAction::CreateAndEnter:
    case FrontendAction::Apply:
    case FrontendAction::RestoreDefaults:
    case FrontendAction::Resume:
    case FrontendAction::EditRoom:
    case FrontendAction::LeaveEditor:
    case FrontendAction::Save:
    case FrontendAction::SaveAndExit:
    case FrontendAction::ReturnToTitle:
    case FrontendAction::ExitGame:
      return ignoredSaveBrowserRoute(parentOwner, action, "not_save_browser_action");
  }

  return ignoredSaveBrowserRoute(parentOwner, action, "not_save_browser_action");
}

}  // namespace iggy3d
