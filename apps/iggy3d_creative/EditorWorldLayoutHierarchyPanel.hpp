#pragma once

namespace iggy3d_creative_app {

struct CreativeDesktopCommandFrame;
struct CreativeEditorDesktopUiState;
struct CreativeEditorWorldLayoutState;

void drawCreativeEditorWorldLayoutHierarchy(
    CreativeEditorDesktopUiState& desktopUi,
    const CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands,
    bool editingDisabled);

}  // namespace iggy3d_creative_app
