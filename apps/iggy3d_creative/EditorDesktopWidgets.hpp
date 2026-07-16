#pragma once

#include <vector>

#include "imgui.h"

#include "EditorDesktopCommands.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorPlayMode.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

// Internal cluster header for the desktop panel widget owners. One header for
// the pair, not one per TU: EditorDesktopPanels.cpp keeps the menu, read-only
// toolbar, bottom diagnostics, status bar, and panel orchestration, and
// delegates the two panel bodies to EditorDesktopOutliner.cpp and
// EditorDesktopInspector.cpp. Widgets emit typed commands only; the dispatcher
// remains the sole mutator.

namespace iggy3d_creative_app {

// The persistent selection as the facade currently reports it (ids + primary),
// before it is resolved against document truth.
struct CreativeDesktopLiveSelection {
  std::vector<iggy3d::creative::CreativeObjectId> objectIds;
  iggy3d::creative::CreativeObjectId primaryObjectId =
      iggy3d::creative::kInvalidObjectId;
};

[[nodiscard]] CreativeDesktopLiveSelection creativeDesktopLiveSelection(
    const iggy3d::creative::CreativeAppState& appState);

// Project-panel body. Renders the searchable hierarchy and converts pure
// selection plans into SelectObjects commands. Called inside the caller's
// Begin/End for the `Project` dock window.
void buildCreativeEditorDesktopOutlinerPanel(
    CreativeEditorDesktopUiState& desktopUi,
    const iggy3d::creative::CreativeAppState& appState,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands);

// Inspector-panel body: zero/single/multi general object properties, then the
// existing logic-link authoring, diagnostics, and Play-mode runtime monitor.
// Called inside the caller's Begin/End for the `Inspector` dock window.
void buildCreativeEditorDesktopInspectorPanel(
    CreativeEditorDesktopUiState& desktopUi,
    const CreativeEditorState& editor,
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorPlayMode* playMode,
    CreativeDesktopCommandFrame& commands);

// Shared by the Inspector's logic rows and the bottom Diagnostics tab, so the
// helper is not duplicated across the two TUs. Defined in
// EditorDesktopInspector.cpp.
void queueCreativeDesktopObjectNavigation(
    CreativeDesktopCommandFrame& commands,
    iggy3d::creative::CreativeObjectId objectId,
    bool playModeActive);
[[nodiscard]] ImVec4 creativeDesktopLogicDiagnosticColor(
    iggy3d::creative::CreativeLogicDiagnosticSeverity severity) noexcept;

}  // namespace iggy3d_creative_app
