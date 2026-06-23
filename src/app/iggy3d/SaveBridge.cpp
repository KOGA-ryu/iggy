#include "app/iggy3d/SaveBridge.hpp"

namespace iggy3d {

ProductSaveBridgeResult scanProductSaves(const std::filesystem::path& saveRoot,
                                         std::string_view packageId,
                                         std::string_view scenarioId) {
  ProductSaveBridgeResult result;
  result.saveRoot = saveRoot;
  result.slots = buildSaveSlotList(saveRoot, packageId, scenarioId);
  return result;
}

}  // namespace iggy3d
