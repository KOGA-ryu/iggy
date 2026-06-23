#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "app/frontend/SaveSlotModel.hpp"

namespace iggy3d {

struct SaveBrowserModel {
  SaveSlotList slots;
  std::string selectedSaveId = "none";
  bool loadEnabled = false;
  bool deleteEnabled = false;
  std::string_view status = "save_browser_ready";
};

SaveBrowserModel buildSaveBrowserModel(const SaveSlotList& slots,
                                       std::string_view selectedSaveId);

}  // namespace iggy3d
