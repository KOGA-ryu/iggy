#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "app/iggy3d/ProductControllerActionMap.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/GamepadInput.hpp"

namespace iggy3d {

struct ProductControllerActionRoutingState {
  std::array<bool, kProductControllerControlCount> controlWasDown{};
};

struct ProductControllerActionRoutingRequest {
  ProductInputSurface surface = ProductInputSurface::None;
  ProductInteractionMode interactionMode = ProductInteractionMode::Player;
  GamepadControllerActionSample sample;
  ProductControllerActionRoutingState& state;
  ActionState& actions;
};

struct ProductControllerActionRoutingResult {
  bool mapped = false;
  std::uint64_t mappedCount = 0;
  ProductControllerControl control = ProductControllerControl::None;
  ProductInputSurface surface = ProductInputSurface::None;
  ProductInteractionMode interactionMode = ProductInteractionMode::Player;
  InputAction action = InputAction::None;
  std::string_view status = "controller_action_unmapped";
  std::string_view reasonCode = "controller_action_unmapped";
};

ProductControllerActionRoutingResult recordProductControllerMappedActions(
    ProductControllerActionRoutingRequest request);
ProductControllerActionRoutingResult productControllerActionRoutingSkipped(
    ProductInputSurface surface,
    ProductInteractionMode interactionMode,
    std::string_view status);
void recordProductControllerActionRoutingResult(
    ProductAppWindowState& window,
    const ProductControllerActionRoutingResult& result);

}  // namespace iggy3d
