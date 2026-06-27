#pragma once

#include <string_view>

namespace iggy3d {

enum class ProductInteractionMode : unsigned char {
  Player = 0,
  Creative = 1,
};

enum class ProductInputSurface : unsigned char {
  None = 0,
  Starter = 1,
  Pause = 2,
  Gameplay = 3,
  RoomEditor = 4,
  Settings = 5,
  DevTools = 6,
  SaveBrowser = 7,
  WorldSetup = 8,
};

struct ProductControllerModeChordSample {
  bool leftTriggerPressed = false;
  bool rightTriggerPressed = false;
  bool leftStickPressed = false;
  bool rightStickPressed = false;
};

struct ProductControllerModeChordState {
  bool chordWasPressed = false;
};

struct ProductInteractionModeToggleRequest {
  ProductInteractionMode mode = ProductInteractionMode::Player;
  ProductInputSurface surface = ProductInputSurface::None;
  ProductControllerModeChordSample sample;
  ProductControllerModeChordState chordState;
};

struct ProductInteractionModeToggleResult {
  ProductInteractionMode mode = ProductInteractionMode::Player;
  ProductControllerModeChordState chordState;
  bool toggleRequested = false;
  bool toggleAccepted = false;
  std::string_view status = "interaction_mode_chord_partial";
  std::string_view reasonCode = "interaction_mode_chord_partial";
};

std::string_view productInteractionModeName(ProductInteractionMode mode);
std::string_view productInputSurfaceName(ProductInputSurface surface);
bool isProductControllerModeChordPressed(
    ProductControllerModeChordSample sample);
bool productInputSurfaceAllowsInteractionModeToggle(ProductInputSurface surface);
ProductInteractionMode toggledProductInteractionMode(
    ProductInteractionMode mode);
ProductInteractionModeToggleResult applyProductInteractionModeToggle(
    ProductInteractionModeToggleRequest request);

}  // namespace iggy3d
