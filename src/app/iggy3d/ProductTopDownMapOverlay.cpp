#include "app/iggy3d/ProductTopDownMapOverlay.hpp"

#include <array>
#include <string_view>

namespace iggy3d {
namespace {

enum class ProductTopDownMapContext {
  Hidden,
  Minimap,
  EditorOverview,
  DebugFallback,
  UnknownMode,
};

struct ProductTopDownMapDescriptor {
  ProductTopDownMapContext context;
  bool visible;
  std::string_view purpose;
  std::string_view size;
  std::string_view status;
  std::string_view reasonCode;
};

constexpr std::array<ProductTopDownMapDescriptor, 5> kTopDownMapDescriptors{
    ProductTopDownMapDescriptor{
        ProductTopDownMapContext::Hidden,
        false,
        "hidden",
        "hidden",
        "top_down_map_hidden",
        "top_down_map_gameplay_inactive",
    },
    ProductTopDownMapDescriptor{
        ProductTopDownMapContext::Minimap,
        true,
        "minimap",
        "compact",
        "top_down_map_ready",
        "top_down_map_minimap",
    },
    ProductTopDownMapDescriptor{
        ProductTopDownMapContext::EditorOverview,
        true,
        "editor_overview",
        "editor",
        "top_down_map_ready",
        "top_down_map_editor_overview",
    },
    ProductTopDownMapDescriptor{
        ProductTopDownMapContext::DebugFallback,
        true,
        "top_down_debug_fallback",
        "fallback_full",
        "top_down_map_debug_fallback",
        "top_down_map_null_renderer_fallback",
    },
    ProductTopDownMapDescriptor{
        ProductTopDownMapContext::UnknownMode,
        false,
        "hidden",
        "hidden",
        "top_down_map_unknown_mode",
        "top_down_map_unknown_mode",
    },
};

const ProductTopDownMapDescriptor& descriptorFor(ProductTopDownMapContext context) {
  for (const ProductTopDownMapDescriptor& descriptor : kTopDownMapDescriptors) {
    // branch-gate: BG-1070
    if (descriptor.context == context) {
      return descriptor;
    }
  }
  return kTopDownMapDescriptors.front();
}

ProductTopDownMapContext contextFor(ProductTopDownMapOverlayRequest request) {
  // branch-gate: BG-1070
  if (!request.gameplayActive) {
    return ProductTopDownMapContext::Hidden;
  }
  // branch-gate: BG-1070
  if (request.interactionMode == ProductInteractionMode::Creative ||
      request.roomEditingReady) {
    return ProductTopDownMapContext::EditorOverview;
  }
  // branch-gate: BG-1070
  if (request.rendererRequest == ProductRendererRequest::Null) {
    return ProductTopDownMapContext::DebugFallback;
  }
  // branch-gate: BG-1070
  if (request.interactionMode == ProductInteractionMode::Player) {
    return ProductTopDownMapContext::Minimap;
  }
  return ProductTopDownMapContext::UnknownMode;
}

}  // namespace

ProductTopDownMapOverlay buildProductTopDownMapOverlay(
    ProductTopDownMapOverlayRequest request) {
  const ProductTopDownMapDescriptor& descriptor =
      descriptorFor(contextFor(request));
  ProductTopDownMapOverlay overlay;
  overlay.visible = descriptor.visible;
  overlay.purpose = std::string(descriptor.purpose);
  overlay.size = std::string(descriptor.size);
  overlay.status = std::string(descriptor.status);
  overlay.reasonCode = std::string(descriptor.reasonCode);
  overlay.itemCount = request.itemCount;
  return overlay;
}

}  // namespace iggy3d
