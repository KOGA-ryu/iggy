#include "app/frontend/SaveBrowser.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::SaveSlotPreview compatibleSlot(std::string_view id) {
  iggy3d::SaveSlotPreview slot;
  slot.id = std::string(id);
  slot.path = std::filesystem::temp_directory_path() / (slot.id + ".iggy3d.save");
  slot.enabled = true;
  slot.reason = "compatible";
  slot.displayTitle = "Title " + slot.id;
  slot.timestampLabel = "file_time_100";
  slot.snapshotPath =
      std::filesystem::temp_directory_path() / (slot.id + ".snapshot.png");
  slot.snapshotAvailable = true;
  slot.snapshotFallback = false;
  slot.snapshotStatus = "available";
  return slot;
}

iggy3d::SaveSlotPreview corruptSlot(std::string_view id) {
  iggy3d::SaveSlotPreview slot = compatibleSlot(id);
  slot.enabled = false;
  slot.corrupt = true;
  slot.compatibility = iggy3d::SaveSlotCompatibility::DecodeFailed;
  slot.reason = "save_file_decode_failed";
  slot.snapshotAvailable = false;
  slot.snapshotFallback = true;
  slot.snapshotStatus = "missing";
  return slot;
}

bool emptyBrowserBuildsEmptySelector() {
  const iggy3d::SaveSlotList slots;
  const iggy3d::SaveBrowserModel model = iggy3d::buildSaveBrowserModel(slots, "");
  return expect(model.selectedSaveId == "none", "empty selected id") &&
         expect(model.ring.empty, "empty ring") &&
         expect(model.ring.items.empty(), "empty ring items") &&
         expect(model.status == "save_browser_empty", "empty status");
}

bool compatibleSaveMapsToSelectorAndEnablesActions() {
  iggy3d::SaveSlotList slots;
  slots.slots.push_back(compatibleSlot("save_001"));
  slots.compatibleCount = 1;
  const iggy3d::SaveBrowserModel model =
      iggy3d::buildSaveBrowserModel(slots, "save_001");
  return expect(model.ring.items.size() == 1U, "ring item count") &&
         expect(model.ring.selectedSlotId == "save_001", "ring selected id") &&
         expect(model.ring.items.front().title == "Title save_001",
                "ring item title") &&
         expect(model.ring.selectedEnabled, "ring selected enabled") &&
         expect(model.selectedTitle == "Title save_001", "selected title") &&
         expect(model.selectedTimestamp == "file_time_100", "selected timestamp") &&
         expect(model.selectedSnapshotAvailable, "selected snapshot available") &&
         expect(!model.selectedSnapshotFallback, "selected snapshot no fallback") &&
         expect(model.selectedSnapshotStatus == "available", "selected snapshot status") &&
         expect(model.status == "save_browser_selection_ready", "ready status");
}

const iggy3d::SaveSlotActionSpec* actionSpecFor(
    const iggy3d::SaveBrowserModel& model,
    iggy3d::FrontendAction action) {
  for (const iggy3d::SaveSlotActionSpec& spec : model.actions) {
    if (spec.action == action) {
      return &spec;
    }
  }
  return nullptr;
}

bool slotRingWrapsAndClampsSelection() {
  iggy3d::SaveSlotList slots;
  slots.slots.push_back(compatibleSlot("save_001"));
  slots.slots.push_back(compatibleSlot("save_002"));
  slots.compatibleCount = 2;

  const iggy3d::SaveSlotRingModel selected =
      iggy3d::buildSaveSlotRingModel(slots, "save_002");
  const iggy3d::SaveSlotRingModel missing =
      iggy3d::buildSaveSlotRingModel(slots, "missing");
  const std::string previous =
      iggy3d::nextSaveSlotRingSelection(slots, "save_001", true);
  const std::string next =
      iggy3d::nextSaveSlotRingSelection(slots, "save_002", false);

  return expect(selected.items.size() == 2U, "ring item count") &&
         expect(selected.selectedIndex == 1U, "ring selected index") &&
         expect(selected.selectedSlotId == "save_002", "ring selected id") &&
         expect(missing.selectedIndex == 0U, "missing clamps to first index") &&
         expect(missing.selectedSlotId == "save_001",
                "missing clamps to first id") &&
         expect(previous == "save_002", "previous wraps to last") &&
         expect(next == "save_001", "next wraps to first");
}

bool deleteModeUsesExplicitActionDescriptors() {
  iggy3d::SaveSlotList slots;
  slots.slots.push_back(compatibleSlot("save_001"));
  slots.compatibleCount = 1;
  const iggy3d::SaveBrowserModel model = iggy3d::buildSaveBrowserModel(
      slots, "save_001", iggy3d::FrontendSaveBrowserMode::Delete);

  const iggy3d::SaveSlotActionSpec* load =
      actionSpecFor(model, iggy3d::FrontendAction::Load);
  const iggy3d::SaveSlotActionSpec* del =
      actionSpecFor(model, iggy3d::FrontendAction::Delete);
  const iggy3d::SaveSlotActionSpec* back =
      actionSpecFor(model, iggy3d::FrontendAction::Back);

  return expect(model.mode == iggy3d::FrontendSaveBrowserMode::Delete,
                "delete mode recorded") &&
         expect(model.actions.size() == 2U, "delete mode action count") &&
         expect(load == nullptr, "delete mode has no load action") &&
         expect(del != nullptr, "delete mode has delete action") &&
         expect(del->command == iggy3d::SaveSlotCommand::Delete,
                "delete command") &&
         expect(del->enabled, "delete action enabled") &&
         expect(del->confirmationRequired, "delete requires confirmation") &&
         expect(back != nullptr && back->command == iggy3d::SaveSlotCommand::Back,
                "back command");
}

bool emptyDeleteModeDisablesDeleteWithStableReason() {
  const iggy3d::SaveSlotList slots;
  const iggy3d::SaveBrowserModel model = iggy3d::buildSaveBrowserModel(
      slots, "", iggy3d::FrontendSaveBrowserMode::Delete);
  const iggy3d::SaveSlotActionSpec* del =
      actionSpecFor(model, iggy3d::FrontendAction::Delete);

  return expect(model.ring.empty, "empty ring") &&
         expect(del != nullptr, "empty delete spec present") &&
         expect(!del->enabled, "empty delete disabled") &&
         expect(del->disabledReason == "save_browser_empty",
                "empty delete disabled reason");
}

bool corruptSaveMapsDisabledReasonAndCannotLoad() {
  iggy3d::SaveSlotList slots;
  slots.slots.push_back(corruptSlot("save_bad"));
  slots.corruptCount = 1;
  const iggy3d::SaveBrowserModel model =
      iggy3d::buildSaveBrowserModel(slots, "save_bad");
  return expect(model.ring.items.size() == 1U, "corrupt ring item count") &&
         expect(!model.ring.selectedEnabled, "corrupt ring selected disabled") &&
         expect(model.ring.selectedStatus == "save_file_decode_failed",
                "corrupt disabled reason") &&
         expect(model.selectedTitle == "Title save_bad", "corrupt selected title") &&
         expect(model.selectedSnapshotFallback, "corrupt snapshot fallback") &&
         expect(model.selectedSnapshotStatus == "missing", "corrupt snapshot status") &&
         expect(model.status == "save_browser_selection_disabled",
                "corrupt disabled status");
}

bool missingSelectionReportsMissing() {
  iggy3d::SaveSlotList slots;
  slots.slots.push_back(compatibleSlot("save_001"));
  const iggy3d::SaveBrowserModel model =
      iggy3d::buildSaveBrowserModel(slots, "save_missing");
  return expect(model.ring.items.size() == 1U, "missing ring item count") &&
         expect(model.ring.selectedSlotId == "save_001",
                "missing ring falls back to first") &&
         expect(model.selectedTitle == "none", "missing selected title") &&
         expect(model.status == "save_browser_selection_missing", "missing status");
}

}  // namespace

int main() {
  const bool ok = emptyBrowserBuildsEmptySelector() &&
                  compatibleSaveMapsToSelectorAndEnablesActions() &&
                  slotRingWrapsAndClampsSelection() &&
                  deleteModeUsesExplicitActionDescriptors() &&
                  emptyDeleteModeDisablesDeleteWithStableReason() &&
                  corruptSaveMapsDisabledReasonAndCannotLoad() &&
                  missingSelectionReportsMissing();
  return ok ? 0 : 1;
}
