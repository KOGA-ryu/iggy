#pragma once

#include "render/FrameInput.hpp"

namespace iggy3d {
class VulkanBackend;
}

namespace iggy3d_creative_app {

// Desktop shell state — app-window UI state, NOT document truth (it lives in
// CreativeEditorState next to the other panel sub-states, never inside
// creative::CreativeAppState). No ImGui types here (plan DL-2); RenderContentViewport
// is a plain render struct, not an ImGui type.
struct CreativeEditorDesktopUiState {
  bool shellEnabled = false;  // false under --capture (plan DL-1)
  bool frameActive = false;   // NewFrame issued this frame, Render still owed
  // The drawable-pixel sub-rectangle the 3D scene occupies (the ImGui central
  // dock node). The all-zero sentinel means full-frame; it stays full-frame
  // until docked panels shrink the central node (UI-3).
  iggy3d::RenderContentViewport contentViewport;
};

// ImGui NewFrame + the full-window passthru dockspace. No-op returning false
// when the shell is disabled or the bridge is dormant. When this returns
// true, endCreativeEditorDesktopFrame MUST run before submit this frame.
bool beginCreativeEditorDesktopFrame(CreativeEditorDesktopUiState& desktopUi,
                                     iggy3d::VulkanBackend& backend);

// ImGui::Render(). Never called from the renderer's record hook — a skipped
// submit must still leave the frame Rendered or the next NewFrame asserts.
void endCreativeEditorDesktopFrame(CreativeEditorDesktopUiState& desktopUi);

}  // namespace iggy3d_creative_app
