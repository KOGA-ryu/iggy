#pragma once

namespace iggy3d_creative_app {

// One immutable sample for desktop pointer gestures. ImGui is read once by
// the panel orchestrator; individual panels consume this fixed-layout record.
struct CreativeEditorPointerFrame {
  float x = 0.0F;
  float y = 0.0F;
  float deltaX = 0.0F;
  float deltaY = 0.0F;
  float wheelY = 0.0F;
  bool primaryPressed = false;
  bool primaryDown = false;
  bool primaryReleased = false;
  bool primaryDoubleClicked = false;
  bool secondaryPressed = false;
  bool middleDragging = false;
  bool focusLost = false;
};

// UI command edges are semantic. Keyboard Enter/Escape remain a compatibility
// fallback while routed keyboard and PS5 bindings feed the same flags.
struct CreativeEditorUiInputFrame {
  CreativeEditorPointerFrame pointer;
  bool confirmPressed = false;
  bool cancelPressed = false;
  bool selectionAdditiveDown = false;
  bool selectionToggleDown = false;
};

}  // namespace iggy3d_creative_app
