#include "app/iggy3d/input/InteractionModeState.hpp"

namespace iggy3d {

ProductControllerModeChordSample productControllerModeChordSampleFromGamepad(
    GamepadControllerModeChordSample sample) {
  return {
      sample.leftTriggerDown,
      sample.rightTriggerDown,
      sample.leftStickPressDown,
      sample.rightStickPressDown,
  };
}

ProductControllerModeChordSample productControllerModeChordSampleFromGamepad(
    GamepadControllerActionSample sample) {
  return {
      sample.leftTriggerDown,
      sample.rightTriggerDown,
      sample.leftStickPressDown,
      sample.rightStickPressDown,
  };
}

ProductInputSurface productInputSurfaceFor(const FrontendState& frontend,
                                           const ProductAppWindowState& window) {
  // branch-gate: BG-1058
  if (frontend.screen == FrontendScreen::Settings ||
      frontend.childScreen == FrontendScreen::Settings) {
    return ProductInputSurface::Settings;
  }
  // branch-gate: BG-1058
  if (frontend.screen == FrontendScreen::DevOverlay ||
      frontend.childScreen == FrontendScreen::StarterDevTools) {
    return ProductInputSurface::DevTools;
  }
  // branch-gate: BG-1058
  if (frontend.screen == FrontendScreen::NewWorld ||
      frontend.childScreen == FrontendScreen::NewWorld) {
    return ProductInputSurface::WorldSetup;
  }
  // branch-gate: BG-1058
  if (frontend.screen == FrontendScreen::LoadSave ||
      frontend.childScreen == FrontendScreen::LoadSave ||
      frontend.screen == FrontendScreen::DeleteConfirm ||
      frontend.childScreen == FrontendScreen::DeleteConfirm) {
    return ProductInputSurface::SaveBrowser;
  }
  if (frontend.screen == FrontendScreen::Pause) {  // branch-gate: BG-1058
    return ProductInputSurface::Pause;
  }
  if (frontend.screen == FrontendScreen::Starter) {  // branch-gate: BG-1058
    return ProductInputSurface::Starter;
  }
  // branch-gate: BG-1058
  if (frontend.screen == FrontendScreen::Gameplay &&
      window.gameplayActive) {
    if (window.roomEditing.ready) {  // branch-gate: BG-1058
      return ProductInputSurface::RoomEditor;
    }
    return ProductInputSurface::Gameplay;
  }
  return ProductInputSurface::None;
}

ProductInteractionModeToggleResult applyProductInteractionModeFrameToggle(
    ProductInteractionModeFrameToggleRequest request) {
  const ProductInputSurface surface =
      productInputSurfaceFor(request.frontend, request.window);
  const ProductInteractionModeToggleResult result =
      applyProductInteractionModeToggle({
          request.window.interactionMode,
          surface,
          request.sample,
          request.chordState,
      });

  request.chordState = result.chordState;
  request.window.interactionMode = result.mode;
  request.window.controllerModeToggleRequested = result.toggleRequested;
  request.window.controllerModeToggleAccepted = result.toggleAccepted;
  request.window.controllerModeToggleStatus = result.status;
  request.window.controllerModeToggleReasonCode = result.reasonCode;
  request.window.controllerModeToggleSurface = productInputSurfaceName(surface);
  return result;
}

}  // namespace iggy3d
