#pragma once

#include <filesystem>
#include <string_view>

#include "app/frontend/SaveSlotModel.hpp"

namespace iggy3d {

struct ProductSaveBridgeResult {
  std::filesystem::path saveRoot;
  SaveSlotList slots;
  std::string_view status = "save_bridge_ready";
};

ProductSaveBridgeResult scanProductSaves(const std::filesystem::path& saveRoot,
                                         std::string_view packageId,
                                         std::string_view scenarioId);

}  // namespace iggy3d
