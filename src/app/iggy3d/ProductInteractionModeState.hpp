#pragma once

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/ProductInteractionMode.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/input/GamepadInput.hpp"

namespace iggy3d {

struct ProductInteractionModeFrameToggleRequest {
  const FrontendState& frontend;
  ProductAppWindowState& window;
  ProductControllerModeChordState& chordState;
  ProductControllerModeChordSample sample;
};

ProductControllerModeChordSample productControllerModeChordSampleFromGamepad(
    GamepadControllerModeChordSample sample);
ProductInputSurface productInputSurfaceFor(const FrontendState& frontend,
                                           const ProductAppWindowState& window);
ProductInteractionModeToggleResult applyProductInteractionModeFrameToggle(
    ProductInteractionModeFrameToggleRequest request);

}  // namespace iggy3d
