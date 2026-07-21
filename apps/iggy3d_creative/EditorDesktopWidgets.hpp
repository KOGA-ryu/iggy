#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "imgui.h"

#include "EditorDesktopCommands.hpp"
#include "EditorDesktopModel.hpp"
#include "EditorDesktopUi.hpp"
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
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeAppState& appState,
    CreativeDesktopCommandFrame& commands);

// Terrain generator body for the Inspector's dedicated tab. Widgets edit only
// transient recipe state and emit semantic workflow commands; the dispatcher
// owns preview/apply/cancel behavior and document history.
void buildCreativeEditorDesktopTerrainGenerationPanel(
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeAppState& appState,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands);

void appendCreativeDesktopGeneratedSourceSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    CreativeDesktopGeneratedSourceScopeCache& scopeCache,
    const iggy3d::creative::CreativeDocument& document,
    iggy3d::creative::CreativeObjectId objectId,
    iggy3d::creative::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands);
void appendCreativeDesktopGeneratedBuildingSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    iggy3d::creative::CreativeObjectId objectId,
    std::size_t buildingIndex,
    bool disabled,
    CreativeDesktopCommandFrame& commands);
void appendCreativeDesktopGeneratedRoomSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    iggy3d::creative::CreativeObjectId objectId,
    iggy3d::creative::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands);
void appendCreativeDesktopGeneratedLevelSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    iggy3d::creative::CreativeObjectId objectId,
    iggy3d::creative::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands);

template <typename Payload>
void queueCreativeDesktopGeneratedPropertyEdit(
    CreativeDesktopPropertyEditIntent intent,
    CreativeDesktopCommandId previewCommand,
    CreativeDesktopCommandId commitCommand,
    Payload payload,
    CreativeDesktopCommandFrame& commands) {
  switch (intent) {
    case CreativeDesktopPropertyEditIntent::Preview:
      commands.push(previewCommand, std::move(payload));
      return;
    case CreativeDesktopPropertyEditIntent::Commit:
      commands.push(commitCommand, std::move(payload));
      return;
    case CreativeDesktopPropertyEditIntent::Cancel:
      commands.push(
          CreativeDesktopCommandId::WorldLayoutCancelGeneratedSettingsPreview);
      return;
    case CreativeDesktopPropertyEditIntent::None:
    default:
      return;
  }
}

template <typename Settings>
void queueCreativeDesktopWorldLayoutPropertyEdit(
    CreativeDesktopPropertyEditIntent intent,
    std::size_t index,
    std::string_view stableKey,
    const Settings& settings,
    CreativeDesktopCommandFrame& commands) {
  CreativeDesktopWorldLayoutPropertyEditPhase phase{};
  switch (intent) {
    case CreativeDesktopPropertyEditIntent::Preview:
      phase = CreativeDesktopWorldLayoutPropertyEditPhase::Preview;
      break;
    case CreativeDesktopPropertyEditIntent::Commit:
      phase = CreativeDesktopWorldLayoutPropertyEditPhase::Commit;
      break;
    case CreativeDesktopPropertyEditIntent::Cancel:
      phase = CreativeDesktopWorldLayoutPropertyEditPhase::Cancel;
      break;
    case CreativeDesktopPropertyEditIntent::None:
    default:
      return;
  }
  constexpr iggy3d::creative::CreativeWorldLayoutTable table =
      creativeDesktopWorldLayoutPropertyTable<Settings>();
  commands.push(
      CreativeDesktopCommandId::WorldLayoutEditSourceProperty,
      CreativeDesktopWorldLayoutPropertyEditPayload{
          phase, table, index, std::string(stableKey), settings});
}

inline void observeCreativeDesktopContinuousPropertyWidget(
    CreativeDesktopPropertyEditActivity& activity,
    bool changed) noexcept {
  observeCreativeDesktopContinuousPropertyEdit(
      activity, changed, ImGui::IsItemDeactivatedAfterEdit());
}

inline void observeCreativeDesktopDiscretePropertyWidget(
    CreativeDesktopPropertyEditActivity& activity,
    bool changed) noexcept {
  observeCreativeDesktopDiscretePropertyEdit(activity, changed);
}

[[nodiscard]] inline bool creativeDesktopWorldLayoutPropertyPreviewActive(
    const CreativeEditorWorldLayoutState& state,
    iggy3d::creative::CreativeWorldLayoutTable table,
    std::size_t index) noexcept {
  return state.liveEditPreviewVisible && state.propertyPreviewKey.active &&
         state.propertyPreviewKey.sourceRevision == state.revision &&
         state.propertyPreviewKey.table == table &&
         state.propertyPreviewKey.index == index;
}

template <typename Settings>
void finishCreativeDesktopWorldLayoutPropertyEdit(
    CreativeDesktopPropertyEditActivity activity,
    const Settings& current,
    Settings& draft,
    bool valid,
    const char* resetLabel,
    CreativeEditorWorldLayoutState& state,
    std::size_t index,
    std::string_view stableKey,
    CreativeDesktopCommandFrame& commands) {
  constexpr iggy3d::creative::CreativeWorldLayoutTable table =
      creativeDesktopWorldLayoutPropertyTable<Settings>();
  bool dirty = !(current == draft);
  ImGui::BeginDisabled(!dirty);
  const bool resetRequested = ImGui::Button(resetLabel);
  ImGui::EndDisabled();
  if (resetRequested) {
    draft = current;
    dirty = false;
  }
  const CreativeDesktopPropertyEditIntent intent =
      resolveCreativeDesktopPropertyEditIntent(
          activity, dirty, valid,
          creativeDesktopWorldLayoutPropertyPreviewActive(state, table,
                                                          index),
          resetRequested);
  queueCreativeDesktopWorldLayoutPropertyEdit(
      intent, index, stableKey, draft, commands);
}

// Non-truncating adapter shared by inspector owners; imgui_stdlib is not
// vendored in this checkout.
bool creativeDesktopInputTextStdString(const char* label, std::string* value,
                                       ImGuiInputTextFlags flags = 0);

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
