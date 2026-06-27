#include "app/iggy3d/ProductInteractionModeHud.hpp"

#include <array>

namespace iggy3d {
namespace {

struct ProductInteractionModeHudDescriptor {
  ProductInteractionMode mode;
  std::string_view label;
  ProductFeedbackTone tone;
};

constexpr std::array<ProductInteractionModeHudDescriptor, 2> kModeHudDescriptors{
    ProductInteractionModeHudDescriptor{
        ProductInteractionMode::Player,
        "player",
        ProductFeedbackTone::Neutral,
    },
    ProductInteractionModeHudDescriptor{
        ProductInteractionMode::Creative,
        "creative",
        ProductFeedbackTone::Warn,
    },
};

const ProductInteractionModeHudDescriptor* descriptorFor(
    ProductInteractionMode mode) {
  for (const ProductInteractionModeHudDescriptor& descriptor :
       kModeHudDescriptors) {
    // branch-gate: BG-1064
    if (descriptor.mode == mode) {
      return &descriptor;
    }
  }
  return nullptr;
}

}  // namespace

ProductInteractionModeHud buildProductInteractionModeHud(
    ProductInteractionModeHudRequest request) {
  ProductInteractionModeHud hud;
  hud.roomEditingReady = request.roomEditingReady;

  const ProductInteractionModeHudDescriptor* descriptor =
      descriptorFor(request.mode);
  // branch-gate: BG-1064
  if (descriptor == nullptr) {
    hud.status = "interaction_mode_hud_unknown_mode";
    hud.reasonCode = hud.status;
    hud.mode = "unknown";
    hud.label = "unknown";
    return hud;
  }

  hud.mode = std::string(productInteractionModeName(request.mode));
  hud.label = std::string(descriptor->label);
  hud.tone = descriptor->tone;

  // branch-gate: BG-1064
  if (!request.gameplayActive) {
    return hud;
  }

  hud.visible = true;
  hud.status = "interaction_mode_hud_ready";
  hud.reasonCode = hud.status;
  return hud;
}

}  // namespace iggy3d
