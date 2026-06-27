#include "app/iggy3d/input/InteractionMode.hpp"

#include <array>
#include <cstddef>

namespace iggy3d {
namespace {

constexpr std::array<std::string_view, 2> kInteractionModeNames{
    "player",
    "creative",
};

constexpr std::array<std::string_view, 9> kInputSurfaceNames{
    "none",
    "starter",
    "pause",
    "gameplay",
    "room_editor",
    "settings",
    "dev_tools",
    "save_browser",
    "world_setup",
};

constexpr std::array<bool, 9> kInputSurfaceAllowsToggle{
    false,
    false,
    false,
    true,
    true,
    false,
    false,
    false,
    false,
};

}  // namespace

std::string_view productInteractionModeName(ProductInteractionMode mode) {
  return kInteractionModeNames[static_cast<std::size_t>(mode)];
}

std::string_view productInputSurfaceName(ProductInputSurface surface) {
  return kInputSurfaceNames[static_cast<std::size_t>(surface)];
}

bool isProductControllerModeChordPressed(
    ProductControllerModeChordSample sample) {
  return sample.leftTriggerPressed && sample.rightTriggerPressed &&
         sample.leftStickPressed && sample.rightStickPressed;
}

bool productInputSurfaceAllowsInteractionModeToggle(ProductInputSurface surface) {
  return kInputSurfaceAllowsToggle[static_cast<std::size_t>(surface)];
}

ProductInteractionMode toggledProductInteractionMode(
    ProductInteractionMode mode) {
  if (mode == ProductInteractionMode::Player) {  // branch-gate: BG-1056
    return ProductInteractionMode::Creative;
  }
  return ProductInteractionMode::Player;
}

ProductInteractionModeToggleResult applyProductInteractionModeToggle(
    ProductInteractionModeToggleRequest request) {
  ProductInteractionModeToggleResult result;
  result.mode = request.mode;
  const bool chordPressed = isProductControllerModeChordPressed(request.sample);
  result.chordState.chordWasPressed = chordPressed;

  if (!chordPressed) {  // branch-gate: BG-1056
    result.status = "interaction_mode_chord_partial";
    result.reasonCode = result.status;
    return result;
  }

  result.toggleRequested = true;
  if (request.chordState.chordWasPressed) {  // branch-gate: BG-1056
    result.status = "interaction_mode_chord_held";
    result.reasonCode = result.status;
    return result;
  }

  const bool surfaceAllowsToggle =
      productInputSurfaceAllowsInteractionModeToggle(request.surface);
  if (!surfaceAllowsToggle) {  // branch-gate: BG-1056
    result.status = "interaction_mode_surface_blocked";
    result.reasonCode = result.status;
    return result;
  }

  result.mode = toggledProductInteractionMode(request.mode);
  result.toggleAccepted = true;
  result.status = "interaction_mode_toggled";
  result.reasonCode = result.status;
  return result;
}

}  // namespace iggy3d
