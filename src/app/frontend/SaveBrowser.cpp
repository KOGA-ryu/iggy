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

}  // namespace iggy3d
