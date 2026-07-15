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
  // Whether the viewport owns the pointer (relative-mouse fly-look). The
  // resting state in desktop mode is a free cursor; a click on the viewport
  // captures it and leaving the viewport context releases it (plan DD-9).
  bool viewportPointerCaptured = false;
};

// Result of the free-pointer capture policy: the new capture state and whether
// it changed (so the caller only touches relative-mouse mode on a transition).
struct CreativeDesktopPointerDecision {
  bool captured = false;
  bool changed = false;
};

// Pure DD-9 policy. Free cursor is the resting state in desktop mode; a primary
// click that landed on the viewport (not an ImGui panel) with no modal open
// captures the pointer for fly-look; leaving the viewport context (a modal
// opened — e.g. Esc routed to Controls — or focus lost) releases it. Shell-off
// forces released, so capture mode / non-desktop runs are unaffected.
[[nodiscard]] CreativeDesktopPointerDecision decideCreativeDesktopPointerCapture(
    bool shellEnabled,
    bool currentlyCaptured,
    bool primaryPressedOverViewport,
    bool viewportContext,
    bool windowFocused) noexcept;

// ImGui NewFrame + the full-window passthru dockspace. No-op returning false
// when the shell is disabled or the bridge is dormant. When this returns
// true, endCreativeEditorDesktopFrame MUST run before submit this frame.
bool beginCreativeEditorDesktopFrame(CreativeEditorDesktopUiState& desktopUi,
                                     iggy3d::VulkanBackend& backend);

// ImGui::Render(). Never called from the renderer's record hook — a skipped
// submit must still leave the frame Rendered or the next NewFrame asserts.
void endCreativeEditorDesktopFrame(CreativeEditorDesktopUiState& desktopUi);

}  // namespace iggy3d_creative_app
