#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"

#include "EditorDesktopCommands.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorState.hpp"

namespace iggy3d_creative_app {

// Renders the main menu bar + Save As modal, emitting semantic command IDs
// into the frame. Widget code only — it reads editor/document state for
// enable-states and never mutates documents (plan DL-3). Call inside the ImGui
// frame (between begin/end desktop frame); the caller dispatches the frame.
void buildCreativeEditorDesktopMenuBar(
    CreativeEditorDesktopUiState& desktopUi,
    const iggy3d::creative::CreativeAppState& appState,
    CreativeDesktopCommandFrame& commands);

// Renders the empty docked workspace panels (Project left, Inspector right,
// Diagnostics bottom, toolbar above the viewport) that establish the target
// layout. Content is filled in later steps; these are placeholders that honor
// the View-menu visibility toggles. Must run inside the ImGui frame, after the
// dockspace is laid out.
void buildCreativeEditorDesktopPanels(CreativeEditorDesktopUiState& desktopUi);

// Renders the bottom status bar — a read-only projection of document + editor
// state (name, dirty, revision, selection count, active device, last result).
void buildCreativeEditorDesktopStatusBar(
    const CreativeEditorDesktopUiState& desktopUi,
    const CreativeEditorState& editor,
    const iggy3d::creative::CreativeAppState& appState);

}  // namespace iggy3d_creative_app
