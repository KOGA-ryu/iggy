#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/frontend/FrontendRoute.hpp"
#include "app/frontend/SaveSlotModel.hpp"

namespace iggy3d {

enum class SaveSlotCommand : std::uint8_t {
  None,
  Load,
  Delete,
  Back,
};

struct SaveSlotRingItem {
  std::string id = "none";
  std::string title = "none";
  bool enabled = false;
  bool corrupt = false;
  std::string status = "missing";
};

struct SaveSlotRingModel {
  std::vector<SaveSlotRingItem> items;
  std::uint64_t selectedIndex = 0;
  std::string selectedSlotId = "none";
  std::string selectedTitle = "none";
  bool selectedEnabled = false;
  bool selectedCorrupt = false;
  std::string selectedStatus = "empty";
  bool empty = true;
  std::uint64_t compatibleCount = 0;
  std::uint64_t corruptCount = 0;
};

struct SaveSlotActionSpec {
  FrontendAction action = FrontendAction::None;
  SaveSlotCommand command = SaveSlotCommand::None;
  std::string_view label = "";
  bool enabled = false;
  bool confirmationRequired = false;
  std::string disabledReason = "not_available";
};

struct SaveBrowserModel {
  FrontendSaveBrowserMode mode = FrontendSaveBrowserMode::Load;
  SaveSlotList slots;
  SaveSlotRingModel ring;
  std::vector<SaveSlotActionSpec> actions;
  std::string selectedSaveId = "none";
  std::string selectedTitle = "none";
  std::string selectedTimestamp = "none";
  bool selectedSnapshotAvailable = false;
  bool selectedSnapshotFallback = true;
  std::string selectedSnapshotStatus = "missing";
  std::string_view status = "save_browser_ready";
};

std::string_view saveSlotCommandName(SaveSlotCommand command);
SaveSlotRingModel buildSaveSlotRingModel(const SaveSlotList& slots,
                                         std::string_view selectedSaveId);
std::string nextSaveSlotRingSelection(const SaveSlotList& slots,
                                      std::string_view selectedSaveId,
                                      bool previous);

SaveBrowserModel buildSaveBrowserModel(const SaveSlotList& slots,
                                       std::string_view selectedSaveId,
                                       FrontendSaveBrowserMode mode =
                                           FrontendSaveBrowserMode::Load);

FrontendRouteResult routeSaveBrowserAction(const SaveBrowserModel& model,
                                           MenuOwner parentOwner,
                                           FrontendAction action);

}  // namespace iggy3d
