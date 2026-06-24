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
         expect(model.selectorResult.selectedId == "none", "empty selector id") &&
         expect(model.selectorItems.empty(), "empty selector items") &&
         expect(!model.loadEnabled, "empty load disabled") &&
         expect(!model.deleteEnabled, "empty delete disabled") &&
         expect(model.status == "save_browser_empty", "empty status");
}

bool compatibleSaveMapsToSelectorAndEnablesActions() {
  iggy3d::SaveSlotList slots;
  slots.slots.push_back(compatibleSlot("save_001"));
  slots.compatibleCount = 1;
  const iggy3d::SaveBrowserModel model =
      iggy3d::buildSaveBrowserModel(slots, "save_001");
  return expect(model.selectorItems.size() == 1U, "selector item count") &&
         expect(model.selectorItems.front().id == "save_001", "selector item id") &&
         expect(model.selectorItems.front().kind == "save", "selector item kind") &&
         expect(model.selectorItems.front().title == "Title save_001",
                "selector title") &&
         expect(!model.selectorItems.front().snapshotRef.empty(), "selector snapshot") &&
         expect(model.selectorItems.front().timestamp == "file_time_100",
                "selector timestamp") &&
         expect(model.selectorItems.front().enabled, "selector enabled") &&
         expect(model.selectorResult.selectedId == "save_001", "selected id") &&
         expect(model.loadEnabled, "load enabled") &&
         expect(model.deleteEnabled, "delete enabled") &&
         expect(model.selectedTitle == "Title save_001", "selected title") &&
         expect(model.selectedTimestamp == "file_time_100", "selected timestamp") &&
         expect(model.selectedSnapshotAvailable, "selected snapshot available") &&
         expect(!model.selectedSnapshotFallback, "selected snapshot no fallback") &&
         expect(model.selectedSnapshotStatus == "available", "selected snapshot status") &&
         expect(model.status == "save_browser_selection_ready", "ready status");
}

bool corruptSaveMapsDisabledReasonAndCannotLoad() {
  iggy3d::SaveSlotList slots;
  slots.slots.push_back(corruptSlot("save_bad"));
  slots.corruptCount = 1;
  const iggy3d::SaveBrowserModel model =
      iggy3d::buildSaveBrowserModel(slots, "save_bad");
  return expect(model.selectorItems.size() == 1U, "corrupt selector item count") &&
         expect(!model.selectorItems.front().enabled, "corrupt selector disabled") &&
         expect(model.selectorItems.front().disabledReason == "save_file_decode_failed",
                "corrupt disabled reason") &&
         expect(!model.loadEnabled, "corrupt load disabled") &&
         expect(model.deleteEnabled, "corrupt delete preserved") &&
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
  return expect(model.selectorItems.size() == 1U, "missing selector item count") &&
         expect(model.selectorResult.selectedId == "save_001",
                "missing selector falls back to first") &&
         expect(!model.loadEnabled, "missing load disabled") &&
         expect(!model.deleteEnabled, "missing delete disabled") &&
         expect(model.selectedTitle == "none", "missing selected title") &&
         expect(model.status == "save_browser_selection_missing", "missing status");
}

}  // namespace

int main() {
  const bool ok = emptyBrowserBuildsEmptySelector() &&
                  compatibleSaveMapsToSelectorAndEnablesActions() &&
                  corruptSaveMapsDisabledReasonAndCannotLoad() &&
                  missingSelectionReportsMissing();
  return ok ? 0 : 1;
}
