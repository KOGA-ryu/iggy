#include "CreativeEditorPlacementInput.hpp"

#include <SDL3/SDL.h>

#include "StandalonePlacement.hpp"

namespace iggy3d_creative_app {

void applyCreativeEditorPlacementInput(
    iggy3d::SdlWindow& window,
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    iggy3d::Vec3 aimCellCenter,
    bool captureMode) {
  if (!editor.placeMode || captureMode) {
    return;
  }

  const bool* plKeys = SDL_GetKeyboardState(nullptr);
  const bool altHeld =
      plKeys != nullptr && (plKeys[SDL_SCANCODE_LALT] != 0);
  if (altHeld) {
    window.setRelativeMouseMode(false);
    float mx = 0.0F;
    float my = 0.0F;
    const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
    const bool lDown = (buttons & SDL_BUTTON_LMASK) != 0U;
    if (lDown && !editor.placeButtonDown) {
      editor.placeButtonDown = true;
      (void)placeBrushObjectWithUndo(appState.facade,
                                     editor.undoStack,
                                     editor.placeBrush,
                                     aimCellCenter,
                                     ++editor.placedCount,
                                     "place_interactive");
    } else if (!lDown) {
      editor.placeButtonDown = false;
    }
  } else {
    window.setRelativeMouseMode(true);
    editor.placeButtonDown = false;
  }
}

}  // namespace iggy3d_creative_app
