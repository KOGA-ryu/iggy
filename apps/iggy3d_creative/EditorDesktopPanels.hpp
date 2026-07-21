#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"

#include "EditorDesktopCommands.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorState.hpp"

namespace iggy3d_creative_app {

struct PlaytestMonitorState;
struct CreativePlaySession;

// Renders the main menu bar + Save As modal, emitting semantic command IDs
// into the frame. Widget code only — it reads editor/document state for
// enable-states and never mutates documents (plan DL-3). Call inside the ImGui
// frame (between begin/end desktop frame); the caller dispatches the frame.
void buildCreativeEditorDesktopMenuBar(
    CreativeEditorDesktopUiState& desktopUi,
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorWorldLayoutState* worldLayout,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands);

// Renders the docked workspace panels. Inspector and Diagnostics project the
// bounded authored-logic report plus read-only Play monitor facts; clickable
// rows emit semantic selection/focus commands and never mutate the document.
// Must run inside the ImGui frame, after the dockspace is laid out.
void buildCreativeEditorDesktopPanels(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeAppState& appState,
    const CreativePlaySession* playMode,
    const PlaytestMonitorState* playtestMonitor,
    CreativeDesktopCommandFrame& commands);

// Renders the bottom status bar — a read-only projection of document + editor
// state (name, dirty, revision, selection count, active device, last result).
void buildCreativeEditorDesktopStatusBar(
    const CreativeEditorDesktopUiState& desktopUi,
    const CreativeEditorState& editor,
    const iggy3d::creative::CreativeAppState& appState);

}  // namespace iggy3d_creative_app
