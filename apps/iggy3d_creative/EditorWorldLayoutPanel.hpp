#pragma once

#include "EditorDesktopCommands.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorState.hpp"

namespace iggy3d_creative_app {

// ImGui-only projection of EditorWorldLayout. It may update canvas pan/zoom,
// but every semantic source edit is emitted through the desktop dispatcher.
void buildCreativeEditorWorldLayoutPanel(
    CreativeEditorDesktopUiState& desktopUi, CreativeEditorState& editor,
    bool playModeActive, CreativeDesktopCommandFrame& commands);

}  // namespace iggy3d_creative_app
