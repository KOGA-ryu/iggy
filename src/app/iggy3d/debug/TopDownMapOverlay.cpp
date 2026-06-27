#include "app/iggy3d/debug/TopDownMapOverlay.hpp"

#include <array>
#include <string_view>

namespace iggy3d {
namespace {

enum class TopDownMapContext {
  Hidden,
  Minimap,
  EditorOverview,
  DebugFallback,
  UnknownMode,
};

struct TopDownMapDescriptor {
  TopDownMapContext context;
  bool visible;
  std::string_view purpose;
  std::string_view size;
  std::string_view status;
  std::string_view reasonCode;
};

constexpr std::array<TopDownMapDescriptor, 5> kTopDownMapDescriptors{
    TopDownMapDescriptor{
        TopDownMapContext::Hidden,
        false,
        "hidden",
        "hidden",
        "top_down_map_hidden",
        "top_down_map_gameplay_inactive",
    },
    TopDownMapDescriptor{
        TopDownMapContext::Minimap,
        true,
        "minimap",
        "compact",
        "top_down_map_ready",
        "top_down_map_minimap",
    },
    TopDownMapDescriptor{
        TopDownMapContext::EditorOverview,
        true,
        "editor_overview",
        "editor",
        "top_down_map_ready",
        "top_down_map_editor_overview",
    },
    TopDownMapDescriptor{
        TopDownMapContext::DebugFallback,
        true,
        "top_down_debug_fallback",
        "fallback_full",
        "top_down_map_debug_fallback",
        "top_down_map_null_renderer_fallback",
    },
    TopDownMapDescriptor{
        TopDownMapContext::UnknownMode,
        false,
        "hidden",
        "hidden",
        "top_down_map_unknown_mode",
        "top_down_map_unknown_mode",
    },
};

const TopDownMapDescriptor& descriptorFor(TopDownMapContext context) {
  for (const TopDownMapDescriptor& descriptor : kTopDownMapDescriptors) {
    // branch-gate: BG-1070
    if (descriptor.context == context) {
      return descriptor;
    }
  }
  return kTopDownMapDescriptors.front();
}

TopDownMapContext contextFor(TopDownMapOverlayRequest request) {
  // branch-gate: BG-1070
  if (!request.gameplayActive) {
    return TopDownMapContext::Hidden;
  }
  // branch-gate: BG-1070
  if (request.interactionMode == ProductInteractionMode::Creative ||
      request.roomEditingReady) {
    return TopDownMapContext::EditorOverview;
  }
  // branch-gate: BG-1070
  if (request.rendererRequest == ProductRendererRequest::Null) {
    return TopDownMapContext::DebugFallback;
  }
  // branch-gate: BG-1070
  if (request.interactionMode == ProductInteractionMode::Player) {
    return TopDownMapContext::Minimap;
  }
  return TopDownMapContext::UnknownMode;
}

}  // namespace

TopDownMapOverlay buildTopDownMapOverlay(
    TopDownMapOverlayRequest request) {
  const TopDownMapDescriptor& descriptor =
      descriptorFor(contextFor(request));
  TopDownMapOverlay overlay;
  overlay.visible = descriptor.visible;
  overlay.purpose = std::string(descriptor.purpose);
  overlay.size = std::string(descriptor.size);
  overlay.status = std::string(descriptor.status);
  overlay.reasonCode = std::string(descriptor.reasonCode);
  overlay.itemCount = request.itemCount;
  return overlay;
}

}  // namespace iggy3d
