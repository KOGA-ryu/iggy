#include "app/iggy3d/receipt/ReceiptFields.hpp"

namespace iggy3d {

void appendProductStartupWorldBuildoutFields(RenderReceipt& receipt, const FrontendState& frontend, const ProductAppWindowState& window, const ProductSaveBridgeResult& saves) {
  appendProductStartupProbeFields(receipt, frontend, window, saves);
  appendProductWorldAuthoringFields(receipt, window);
  appendProductActiveRoomFields(receipt, window);
}

}  // namespace iggy3d
