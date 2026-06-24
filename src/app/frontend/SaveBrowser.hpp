#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/frontend/FrontendRoute.hpp"
#include "app/frontend/SaveSlotModel.hpp"
#include "app/frontend/VerticalFadedSelector.hpp"

namespace iggy3d {

struct SaveBrowserModel {
  SaveSlotList slots;
  std::vector<VerticalSelectorItem> selectorItems;
  VerticalSelectorState selectorState;
  VerticalSelectorResult selectorResult;
  std::string selectedSaveId = "none";
  std::string selectedTitle = "none";
  std::string selectedTimestamp = "none";
  bool selectedSnapshotAvailable = false;
  bool selectedSnapshotFallback = true;
  std::string selectedSnapshotStatus = "missing";
  bool loadEnabled = false;
  bool deleteEnabled = false;
  std::string_view status = "save_browser_ready";
};

SaveBrowserModel buildSaveBrowserModel(const SaveSlotList& slots,
                                       std::string_view selectedSaveId);

FrontendRouteResult routeSaveBrowserAction(const SaveBrowserModel& model,
                                           MenuOwner parentOwner,
                                           FrontendAction action);

}  // namespace iggy3d
