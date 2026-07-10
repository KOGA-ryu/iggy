#include "app/iggy3d/receipt/ReceiptFields.hpp"

#include <charconv>
#include <string>

namespace iggy3d {

std::string floatReceiptValue(float value) {
  char buffer[32]{};
  const auto [ptr, error] =
      std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::fixed, 3);
  if (error != std::errc{}) {
    return "unavailable";
  }
  return std::string(buffer, static_cast<std::size_t>(ptr - buffer));
}

void appendProductStartupWorldBuildoutFields(RenderReceipt& receipt, const FrontendState& frontend, const ProductAppWindowState& window, const ProductSaveBridgeResult& saves) {
  appendProductStartupProbeFields(receipt, frontend, window, saves);
  appendProductWorldAuthoringFields(receipt, window);
  appendProductActiveRoomFields(receipt, window);
}

}  // namespace iggy3d
