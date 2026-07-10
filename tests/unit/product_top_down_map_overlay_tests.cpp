#include "app/iggy3d/debug/TopDownMapOverlay.hpp"

#include <iostream>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/world/WorldTemplate.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "render/RenderDiagnostics.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

bool inactiveGameplayHidesTopDownMap() {
  const iggy3d::TopDownMapOverlay overlay =
      iggy3d::buildTopDownMapOverlay(
          {iggy3d::ProductRendererRequest::Vulkan,
           iggy3d::ProductInteractionMode::Player,
           false,
           false,
           7U});
  return expect(!overlay.visible, "inactive hidden") &&
         expect(overlay.purpose == "hidden", "inactive purpose") &&
         expect(overlay.size == "hidden", "inactive size") &&
         expect(overlay.status == "top_down_map_hidden", "inactive status") &&
         expect(overlay.reasonCode == "top_down_map_gameplay_inactive",
                "inactive reason") &&
         expect(overlay.itemCount == 7U, "inactive item count copied");
}

bool playerGameplayUsesCompactMinimap() {
  const iggy3d::TopDownMapOverlay overlay =
      iggy3d::buildTopDownMapOverlay(
          {iggy3d::ProductRendererRequest::Vulkan,
           iggy3d::ProductInteractionMode::Player,
           true,
           false,
           11U});
  return expect(overlay.visible, "player minimap visible") &&
         expect(overlay.purpose == "minimap", "player minimap purpose") &&
         expect(overlay.size == "compact", "player minimap compact") &&
         expect(overlay.status == "top_down_map_ready", "player minimap ready") &&
         expect(overlay.reasonCode == "top_down_map_minimap",
                "player minimap reason") &&
         expect(overlay.itemCount == 11U, "player minimap item count");
}

bool creativeEditingUsesEditorOverview() {
  const iggy3d::TopDownMapOverlay overlay =
      iggy3d::buildTopDownMapOverlay(
          {iggy3d::ProductRendererRequest::Vulkan,
           iggy3d::ProductInteractionMode::Creative,
           true,
           true,
           17U});
  return expect(overlay.visible, "creative overview visible") &&
         expect(overlay.purpose == "editor_overview",
                "creative overview purpose") &&
         expect(overlay.size == "editor", "creative overview size") &&
         expect(overlay.status == "top_down_map_ready",
                "creative overview ready") &&
         expect(overlay.reasonCode == "top_down_map_editor_overview",
                "creative overview reason") &&
         expect(overlay.itemCount == 17U, "creative overview item count");
}

bool creativeWorldHidesTopDownMap() {
  const iggy3d::TopDownMapOverlay overlay =
      iggy3d::buildTopDownMapOverlay(
          {iggy3d::ProductRendererRequest::Vulkan,
           iggy3d::ProductInteractionMode::Creative,
           true,
           false,
           13U,
           true});
  return expect(!overlay.visible, "creative world hidden") &&
         expect(overlay.purpose == "hidden", "creative world purpose") &&
         expect(overlay.size == "hidden", "creative world size") &&
         expect(overlay.status == "top_down_map_hidden",
                "creative world status") &&
         expect(overlay.reasonCode == "top_down_map_creative_world_active",
                "creative world reason") &&
         expect(overlay.itemCount == 13U, "creative world item count");
}

bool nullPlayerGameplayUsesDiagnosticFallback() {
  const iggy3d::TopDownMapOverlay overlay =
      iggy3d::buildTopDownMapOverlay(
          {iggy3d::ProductRendererRequest::Null,
           iggy3d::ProductInteractionMode::Player,
           true,
           false,
           19U});
  return expect(overlay.visible, "null fallback visible") &&
         expect(overlay.purpose == "top_down_debug_fallback",
                "null fallback purpose") &&
         expect(overlay.size == "fallback_full", "null fallback full") &&
         expect(overlay.status == "top_down_map_debug_fallback",
                "null fallback status") &&
         expect(overlay.reasonCode == "top_down_map_null_renderer_fallback",
                "null fallback reason") &&
         expect(overlay.itemCount == 19U, "null fallback item count");
}

bool unknownInteractionModeFailsClosed() {
  const iggy3d::TopDownMapOverlay overlay =
      iggy3d::buildTopDownMapOverlay(
          {iggy3d::ProductRendererRequest::Vulkan,
           static_cast<iggy3d::ProductInteractionMode>(255U),
           true,
           false,
           23U});
  return expect(!overlay.visible, "unknown hidden") &&
         expect(overlay.purpose == "hidden", "unknown purpose") &&
         expect(overlay.status == "top_down_map_unknown_mode",
                "unknown status") &&
         expect(overlay.reasonCode == "top_down_map_unknown_mode",
                "unknown reason");
}

bool receiptCarriesTopDownMapFields() {
  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductAppWindowState window;
  const iggy3d::TopDownMapOverlay overlay =
      iggy3d::buildTopDownMapOverlay(
          {iggy3d::ProductRendererRequest::Vulkan,
           iggy3d::ProductInteractionMode::Player,
           true,
           false,
           29U});
  window.debugHud.topDownMap.visible = overlay.visible;
  window.debugHud.topDownMap.purpose = overlay.purpose;
  window.debugHud.topDownMap.size = overlay.size;
  window.debugHud.topDownMap.status = overlay.status;
  window.debugHud.topDownMap.reasonCode = overlay.reasonCode;
  window.debugHud.topDownMap.itemCount = overlay.itemCount;
  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window, saves);
  return expect(iggy3d::hasReceiptField(receipt, "top_down_map_visible", "true"),
                "receipt visible") &&
         expect(iggy3d::hasReceiptField(receipt, "top_down_map_purpose", "minimap"),
                "receipt purpose") &&
         expect(iggy3d::hasReceiptField(receipt, "top_down_map_size", "compact"),
                "receipt size") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "top_down_map_status",
                                        "top_down_map_ready"),
                "receipt status") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "top_down_map_reason_code",
                                        "top_down_map_minimap"),
                "receipt reason") &&
         expect(iggy3d::hasReceiptField(receipt, "top_down_map_item_count", "29"),
                "receipt item count");
}

}  // namespace

int main() {
  bool ok = true;
  ok &= inactiveGameplayHidesTopDownMap();
  ok &= playerGameplayUsesCompactMinimap();
  ok &= creativeEditingUsesEditorOverview();
  ok &= creativeWorldHidesTopDownMap();
  ok &= nullPlayerGameplayUsesDiagnosticFallback();
  ok &= unknownInteractionModeFailsClosed();
  ok &= receiptCarriesTopDownMapFields();
  if (!ok) {
    return 1;
  }
  std::cout << "product_top_down_map_overlay_tests=pass\n";
  return 0;
}
