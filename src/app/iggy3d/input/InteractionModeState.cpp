#include "app/iggy3d/input/InteractionModeState.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"

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
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  return surface.inputSurface;
}

ProductInteractionModeToggleResult applyProductInteractionModeFrameToggle(
    ProductInteractionModeFrameToggleRequest request) {
  const ProductInputSurface surface =
      productInputSurfaceFor(request.frontend, request.window);
  const ProductInteractionModeToggleResult result =
      applyProductInteractionModeToggle({
          request.window.inputDevice.interactionMode,
          surface,
          request.sample,
          request.chordState,
      });

  request.chordState = result.chordState;
  request.window.inputDevice.interactionMode = result.mode;
  request.window.inputDevice.controllerModeToggle.requested =
      result.toggleRequested;
  request.window.inputDevice.controllerModeToggle.accepted =
      result.toggleAccepted;
  request.window.inputDevice.controllerModeToggle.status = result.status;
  request.window.inputDevice.controllerModeToggle.reasonCode = result.reasonCode;
  request.window.inputDevice.controllerModeToggle.surface =
      productInputSurfaceName(surface);
  return result;
}

}  // namespace iggy3d
