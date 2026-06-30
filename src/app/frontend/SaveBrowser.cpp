#include "app/frontend/SaveBrowser.hpp"

namespace iggy3d {
namespace {

std::size_t selectedIndexForId(const SaveSlotList& slots, std::string_view selectedSaveId) {
  for (std::size_t index = 0; index < slots.slots.size(); ++index) {
    if (slots.slots[index].id == selectedSaveId) {
      return index;
    }
  }
  return 0;
}

SaveSlotRingItem ringItemFromSaveSlot(const SaveSlotPreview& slot) {
  SaveSlotRingItem item;
  // branch-gate: BG-1020
  item.id = slot.id.empty() ? "none" : slot.id;
  // branch-gate: BG-1020
  item.title = slot.displayTitle.empty() ? item.id : slot.displayTitle;
  item.enabled = slot.enabled;
  item.corrupt = slot.corrupt;
  // branch-gate: BG-1020
  item.status = slot.enabled ? "selected" : slot.reason;
  return item;
}

SaveSlotActionSpec makeBackActionSpec() {
  return SaveSlotActionSpec{
      FrontendAction::Back,
      SaveSlotCommand::Back,
      "BACK",
      true,
      false,
      "none",
  };
}

std::vector<SaveSlotActionSpec> buildSaveSlotActionSpecs(
    FrontendSaveBrowserMode mode,
    const SaveSlotRingModel& ring,
    std::string_view modelStatus) {
  std::vector<SaveSlotActionSpec> actions;
  // branch-gate: BG-1020
  actions.reserve(mode == FrontendSaveBrowserMode::Delete ? 2U : 3U);

  const bool selectedPresent =
      modelStatus == "save_browser_selection_ready" ||
      modelStatus == "save_browser_selection_disabled";
  // branch-gate: BG-1020
  if (mode == FrontendSaveBrowserMode::Delete) {
    actions.push_back(SaveSlotActionSpec{
        FrontendAction::Delete,
        SaveSlotCommand::Delete,
        "DELETE SELECTED",
        selectedPresent,
        true,
        // branch-gate: BG-1020
        selectedPresent ? std::string{"none"} : std::string{modelStatus},
    });
    actions.push_back(makeBackActionSpec());
    return actions;
  }

  actions.push_back(SaveSlotActionSpec{
      FrontendAction::Load,
      SaveSlotCommand::Load,
      "LOAD SELECTED",
      selectedPresent && ring.selectedEnabled,
      false,
      // branch-gate: BG-1020
      (selectedPresent && ring.selectedEnabled)
          ? std::string{"none"}
          // branch-gate: BG-1020
          : (selectedPresent ? ring.selectedStatus : std::string{modelStatus}),
  });
  actions.push_back(SaveSlotActionSpec{
      FrontendAction::Delete,
      SaveSlotCommand::Delete,
      "DELETE SELECTED",
      selectedPresent,
      true,
      // branch-gate: BG-1020
      selectedPresent ? std::string{"none"} : std::string{modelStatus},
  });
  actions.push_back(makeBackActionSpec());
  return actions;
}

void copySelectedPresentation(SaveBrowserModel& model, const SaveSlotPreview& slot) {
  // branch-gate: BG-1020
  model.selectedTitle = slot.displayTitle.empty() ? slot.id : slot.displayTitle;
  // branch-gate: BG-1020
  model.selectedTimestamp = slot.timestampLabel.empty() ? "unknown" : slot.timestampLabel;
  model.selectedSnapshotAvailable = slot.snapshotAvailable;
  model.selectedSnapshotFallback = slot.snapshotFallback;
  model.selectedSnapshotStatus = slot.snapshotStatus;
}

}  // namespace

std::string_view saveSlotCommandName(SaveSlotCommand command) {
  // branch-gate: BG-1020
  switch (command) {
    case SaveSlotCommand::None:
      return "none";
    case SaveSlotCommand::Load:
      return "load";
    case SaveSlotCommand::Delete:
      return "delete";
    case SaveSlotCommand::Back:
      return "back";
  }
  return "none";
}

SaveSlotRingModel buildSaveSlotRingModel(const SaveSlotList& slots,
                                         std::string_view selectedSaveId) {
  SaveSlotRingModel ring;
  ring.compatibleCount = slots.compatibleCount;
  ring.corruptCount = slots.corruptCount;
  ring.items.reserve(slots.slots.size());
  for (const SaveSlotPreview& slot : slots.slots) {
    ring.items.push_back(ringItemFromSaveSlot(slot));
  }

  ring.empty = ring.items.empty();
  // branch-gate: BG-1020
  if (ring.empty) {
    return ring;
  }

  ring.selectedIndex =
      static_cast<std::uint64_t>(selectedIndexForId(slots, selectedSaveId));
  // branch-gate: BG-1020
  if (ring.selectedIndex >= ring.items.size()) {
    ring.selectedIndex = 0;
  }
  const SaveSlotRingItem& selected =
      ring.items[static_cast<std::size_t>(ring.selectedIndex)];
  ring.selectedSlotId = selected.id;
  ring.selectedTitle = selected.title;
  ring.selectedEnabled = selected.enabled;
  ring.selectedCorrupt = selected.corrupt;
  ring.selectedStatus = selected.status;
  return ring;
}

std::string nextSaveSlotRingSelection(const SaveSlotList& slots,
                                      std::string_view selectedSaveId,
                                      bool previous) {
  const SaveSlotRingModel ring = buildSaveSlotRingModel(slots, selectedSaveId);
  // branch-gate: BG-1020
  if (ring.items.empty()) {
    return "none";
  }
  const std::size_t size = ring.items.size();
  std::size_t index = static_cast<std::size_t>(ring.selectedIndex);
  // branch-gate: BG-1020
  if (previous) {
    // branch-gate: BG-1020
    index = index == 0U ? size - 1U : index - 1U;
  } else {
    index = (index + 1U) % size;
  }
  return ring.items[index].id;
}

SaveBrowserModel buildSaveBrowserModel(const SaveSlotList& slots,
                                       std::string_view selectedSaveId,
                                       FrontendSaveBrowserMode mode) {
  SaveBrowserModel model;
  model.mode = mode;
  model.slots = slots;
  model.ring = buildSaveSlotRingModel(slots, selectedSaveId);
  model.selectedSaveId = selectedSaveId.empty() ? "none" : std::string(selectedSaveId);

  for (const SaveSlotPreview& slot : model.slots.slots) {
    if (slot.id == model.selectedSaveId) {
      copySelectedPresentation(model, slot);
      model.status = slot.enabled ? "save_browser_selection_ready"
                                  : "save_browser_selection_disabled";
      model.actions = buildSaveSlotActionSpecs(model.mode, model.ring, model.status);
      return model;
    }
  }

  // branch-gate: BG-1020
  model.status = model.slots.slots.empty() ? "save_browser_empty"
                                           : "save_browser_selection_missing";
  model.actions = buildSaveSlotActionSpecs(model.mode, model.ring, model.status);
  return model;
}

}  // namespace iggy3d
