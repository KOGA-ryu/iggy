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

  const auto loadRoute =
      iggy3d::routeSaveBrowserAction(model,
                                     iggy3d::MenuOwner::Starter,
                                     iggy3d::FrontendAction::Load);
  const auto deleteRoute =
      iggy3d::routeSaveBrowserAction(model,
                                     iggy3d::MenuOwner::Starter,
                                     iggy3d::FrontendAction::Delete);

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
                "back command") &&
         expect(!loadRoute.accepted, "load rejected in delete mode") &&
         expect(loadRoute.status == "not_save_browser_action",
                "load rejected reason") &&
         expect(deleteRoute.accepted, "delete accepted in delete mode") &&
         expect(deleteRoute.nextChildScreen ==
                    iggy3d::FrontendScreen::DeleteConfirm,
                "delete routes to confirm");
}

bool emptyDeleteModeDisablesDeleteWithStableReason() {
  const iggy3d::SaveSlotList slots;
  const iggy3d::SaveBrowserModel model = iggy3d::buildSaveBrowserModel(
      slots, "", iggy3d::FrontendSaveBrowserMode::Delete);
  const iggy3d::SaveSlotActionSpec* del =
      actionSpecFor(model, iggy3d::FrontendAction::Delete);
  const auto route =
      iggy3d::routeSaveBrowserAction(model,
                                     iggy3d::MenuOwner::Starter,
                                     iggy3d::FrontendAction::Delete);

  return expect(model.ring.empty, "empty ring") &&
         expect(del != nullptr, "empty delete spec present") &&
         expect(!del->enabled, "empty delete disabled") &&
         expect(del->disabledReason == "save_browser_empty",
                "empty delete disabled reason") &&
         expect(!route.accepted, "empty delete route rejected") &&
         expect(route.status == "save_browser_empty",
                "empty delete route reason");
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

bool backRoutesCloseToParentSurfaces() {
  iggy3d::SaveSlotList slots;
  slots.slots.push_back(compatibleSlot("save_001"));
  const iggy3d::SaveBrowserModel model =
      iggy3d::buildSaveBrowserModel(slots, "save_001");
  const auto starter =
      iggy3d::routeSaveBrowserAction(model,
                                     iggy3d::MenuOwner::Starter,
                                     iggy3d::FrontendAction::Back);
  const auto pause =
      iggy3d::routeSaveBrowserAction(model,
                                     iggy3d::MenuOwner::Pause,
                                     iggy3d::FrontendAction::Back);

  return expect(starter.accepted, "starter back accepted") &&
         expect(starter.inputOwner == iggy3d::MenuOwner::Starter,
                "starter back owner") &&
         expect(starter.nextScreen == iggy3d::FrontendScreen::Starter,
                "starter back screen") &&
         expect(starter.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "starter back child") &&
         expect(starter.status == "save_browser_closed_to_starter",
                "starter back status") &&
         expect(pause.accepted, "pause back accepted") &&
         expect(pause.inputOwner == iggy3d::MenuOwner::Pause, "pause back owner") &&
         expect(pause.nextScreen == iggy3d::FrontendScreen::Pause,
                "pause back screen") &&
         expect(pause.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "pause back child") &&
         expect(pause.status == "save_browser_closed_to_pause",
                "pause back status");
}

bool loadRoutesOnlyCompatibleSelection() {
  iggy3d::SaveSlotList slots;
  slots.slots.push_back(compatibleSlot("save_001"));
  const iggy3d::SaveBrowserModel model =
      iggy3d::buildSaveBrowserModel(slots, "save_001");
  const auto route =
      iggy3d::routeSaveBrowserAction(model,
                                     iggy3d::MenuOwner::Starter,
                                     iggy3d::FrontendAction::Load);

  iggy3d::SaveSlotList corruptSlots;
  corruptSlots.slots.push_back(corruptSlot("save_bad"));
  const iggy3d::SaveBrowserModel corrupt =
      iggy3d::buildSaveBrowserModel(corruptSlots, "save_bad");
  const auto corruptRoute =
      iggy3d::routeSaveBrowserAction(corrupt,
                                     iggy3d::MenuOwner::Starter,
                                     iggy3d::FrontendAction::Load);

  return expect(route.accepted, "load accepted") &&
         expect(route.inputOwner == iggy3d::MenuOwner::Gameplay, "load owner") &&
         expect(route.nextScreen == iggy3d::FrontendScreen::Gameplay,
                "load screen") &&
         expect(route.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "load child") &&
         expect(route.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::LaunchGameplay,
                "load transition") &&
         expect(!route.gameplayInputSuppressed, "load unsuppressed") &&
         expect(route.status == "save_browser_load_requested",
                "load status") &&
         expect(!corruptRoute.accepted, "corrupt load rejected") &&
         expect(corruptRoute.status == "save_file_decode_failed",
                "corrupt load reason") &&
         expect(corruptRoute.gameplayInputSuppressed,
                "corrupt load suppresses");
}

bool deleteRoutesToConfirmWithoutDeleting() {
  iggy3d::SaveSlotList slots;
  slots.slots.push_back(compatibleSlot("save_001"));
  const iggy3d::SaveBrowserModel model =
      iggy3d::buildSaveBrowserModel(slots, "save_001");
  const auto starter =
      iggy3d::routeSaveBrowserAction(model,
                                     iggy3d::MenuOwner::Starter,
                                     iggy3d::FrontendAction::Delete);

  iggy3d::SaveSlotList corruptSlots;
  corruptSlots.slots.push_back(corruptSlot("save_bad"));
  const iggy3d::SaveBrowserModel corrupt =
      iggy3d::buildSaveBrowserModel(corruptSlots, "save_bad");
  const auto corruptDelete =
      iggy3d::routeSaveBrowserAction(corrupt,
                                     iggy3d::MenuOwner::Pause,
                                     iggy3d::FrontendAction::Delete);

  return expect(starter.accepted, "starter delete accepted") &&
         expect(starter.inputOwner == iggy3d::MenuOwner::Starter,
                "starter delete owner") &&
         expect(starter.nextScreen == iggy3d::FrontendScreen::Starter,
                "starter delete screen") &&
         expect(starter.nextChildScreen == iggy3d::FrontendScreen::DeleteConfirm,
                "starter delete confirm") &&
         expect(starter.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::None,
                "starter delete no transition") &&
         expect(starter.status == "save_browser_delete_confirm_requested",
                "starter delete status") &&
         expect(corruptDelete.accepted, "corrupt delete accepted") &&
         expect(corruptDelete.inputOwner == iggy3d::MenuOwner::Pause,
                "corrupt delete owner") &&
         expect(corruptDelete.nextScreen == iggy3d::FrontendScreen::Pause,
                "corrupt delete screen") &&
         expect(corruptDelete.nextChildScreen ==
                    iggy3d::FrontendScreen::DeleteConfirm,
                "corrupt delete confirm") &&
         expect(corruptDelete.status == "save_browser_delete_confirm_requested",
                "corrupt delete status");
}

bool emptyMissingUnsupportedAndInvalidParentAreDeterministic() {
  const iggy3d::SaveSlotList emptySlots;
  const iggy3d::SaveBrowserModel empty =
      iggy3d::buildSaveBrowserModel(emptySlots, "");
  const auto emptyLoad =
      iggy3d::routeSaveBrowserAction(empty,
                                     iggy3d::MenuOwner::Starter,
                                     iggy3d::FrontendAction::Load);
  const auto emptyDelete =
      iggy3d::routeSaveBrowserAction(empty,
                                     iggy3d::MenuOwner::Starter,
                                     iggy3d::FrontendAction::Delete);

  iggy3d::SaveSlotList slots;
  slots.slots.push_back(compatibleSlot("save_001"));
  const iggy3d::SaveBrowserModel missing =
      iggy3d::buildSaveBrowserModel(slots, "missing");
  const auto missingLoad =
      iggy3d::routeSaveBrowserAction(missing,
                                     iggy3d::MenuOwner::Pause,
                                     iggy3d::FrontendAction::Load);
  const auto unsupported =
      iggy3d::routeSaveBrowserAction(missing,
                                     iggy3d::MenuOwner::Pause,
                                     iggy3d::FrontendAction::Apply);
  const auto invalid =
      iggy3d::routeSaveBrowserAction(missing,
                                     iggy3d::MenuOwner::None,
                                     iggy3d::FrontendAction::Back);

  return expect(!emptyLoad.accepted, "empty load rejected") &&
         expect(emptyLoad.status == "save_browser_empty", "empty load status") &&
         expect(!emptyDelete.accepted, "empty delete rejected") &&
         expect(emptyDelete.status == "save_browser_empty",
                "empty delete status") &&
         expect(!missingLoad.accepted, "missing load rejected") &&
         expect(missingLoad.status == "save_browser_selection_missing",
                "missing load status") &&
         expect(!unsupported.accepted, "unsupported rejected") &&
         expect(unsupported.status == "not_save_browser_action",
                "unsupported status") &&
         expect(!invalid.accepted, "invalid parent rejected") &&
         expect(invalid.status == "save_browser_invalid_parent",
                "invalid parent status") &&
         expect(invalid.inputOwner == iggy3d::MenuOwner::None,
                "invalid parent owner");
}

}  // namespace

int main() {
  const bool ok = emptyBrowserBuildsEmptySelector() &&
                  compatibleSaveMapsToSelectorAndEnablesActions() &&
                  slotRingWrapsAndClampsSelection() &&
                  deleteModeUsesExplicitActionDescriptors() &&
                  emptyDeleteModeDisablesDeleteWithStableReason() &&
                  corruptSaveMapsDisabledReasonAndCannotLoad() &&
                  missingSelectionReportsMissing() &&
                  backRoutesCloseToParentSurfaces() &&
                  loadRoutesOnlyCompatibleSelection() &&
                  deleteRoutesToConfirmWithoutDeleting() &&
                  emptyMissingUnsupportedAndInvalidParentAreDeterministic();
  return ok ? 0 : 1;
}
