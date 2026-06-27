#pragma once

#include <cstddef>
#include <span>
#include <string_view>

#include "app/iggy3d/ProductInteractionMode.hpp"
#include "app/input/InputAction.hpp"

namespace iggy3d {

inline constexpr std::size_t kProductControllerControlCount = 23;

enum class ProductControllerControl : unsigned char {
  None = 0,
  LeftStickUp = 1,
  LeftStickDown = 2,
  LeftStickLeft = 3,
  LeftStickRight = 4,
  RightStickUp = 5,
  RightStickDown = 6,
  RightStickLeft = 7,
  RightStickRight = 8,
  DpadUp = 9,
  DpadDown = 10,
  DpadLeft = 11,
  DpadRight = 12,
  SouthButton = 13,
  EastButton = 14,
  WestButton = 15,
  NorthButton = 16,
  LeftShoulder = 17,
  RightShoulder = 18,
  LeftTrigger = 19,
  RightTrigger = 20,
  LeftStickPress = 21,
  RightStickPress = 22,
};

struct ProductControllerActionMapRow {
  ProductInputSurface surface = ProductInputSurface::None;
  ProductInteractionMode interactionMode = ProductInteractionMode::Player;
  ProductControllerControl control = ProductControllerControl::None;
  InputAction action = InputAction::None;
  float actionValue = 0.0F;
};

struct ProductControllerActionMapRequest {
  ProductInputSurface surface = ProductInputSurface::None;
  ProductInteractionMode interactionMode = ProductInteractionMode::Player;
  ProductControllerControl control = ProductControllerControl::None;
};

struct ProductControllerActionMapResult {
  bool mapped = false;
  InputAction action = InputAction::None;
  float actionValue = 0.0F;
  std::string_view status = "controller_action_unmapped";
  std::string_view reasonCode = "controller_action_unmapped";
};

std::string_view productControllerControlName(ProductControllerControl control);
bool productControllerControlIsModeChordComponent(
    ProductControllerControl control);
std::span<const ProductControllerActionMapRow> productControllerActionMapRows();
ProductControllerActionMapResult mapProductControllerAction(
    ProductControllerActionMapRequest request);

}  // namespace iggy3d
