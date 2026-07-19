#include "EditorDesktopPanels.hpp"

#include <cstdint>
#include <string>
#include <string_view>

#include "imgui.h"

#include "EditorDesktopModel.hpp"
#include "EditorDesktopWidgets.hpp"
#include "EditorInteraction.hpp"
#include "EditorPlacementFeedback.hpp"
#include "EditorPlayMode.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"
#include "EditorWorldLayoutPanel.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"

// Desktop shell chrome and panel orchestration. The Project and Inspector
// bodies live in EditorDesktopOutliner.cpp / EditorDesktopInspector.cpp; this TU
// keeps the menu, the read-only toolbar header, the bottom diagnostics tabs, the
// status bar, and the dock-window plumbing that hosts them.

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

namespace {

const char* controlDeviceName(cr::CreativeControlDevice device) noexcept {
  switch (device) {
    case cr::CreativeControlDevice::KeyboardMouse:
      return "keyboard/mouse";
    case cr::CreativeControlDevice::Gamepad:
      return "gamepad";
    case cr::CreativeControlDevice::Count:
      break;
  }
  return "unknown";
}

// The bounded authored-logic report, projected read-only. Rows navigate by
// emitting a semantic command; nothing here mutates the document.
void appendDiagnosticsTab(const cr::CreativeDocument& document,
                          const cr::CreativeLogicDiagnosticReport& diagnostics,
                          bool playModeActive,
                          CreativeDesktopCommandFrame& commands) {
  ImGui::Text("Logic sources: %llu  linked: %llu",
              static_cast<unsigned long long>(diagnostics.sourceCount),
              static_cast<unsigned long long>(diagnostics.linkedSourceCount));
  ImGui::SameLine();
  ImGui::TextColored(diagnostics.errorCount == 0U
                         ? ImVec4{0.20F, 1.0F, 0.35F, 1.0F}
                         : ImVec4{1.0F, 0.34F, 0.30F, 1.0F},
                     "%llu errors  %llu warnings",
                     static_cast<unsigned long long>(diagnostics.errorCount),
                     static_cast<unsigned long long>(diagnostics.warningCount));
  for (std::size_t index = 0U; index < diagnostics.issueCount; ++index) {
    const cr::CreativeLogicDiagnostic& issue = diagnostics.issues[index];
    const cr::CreativeObject* source = document.findObject(issue.sourceObjectId);
    const std::string sourceName =
        source != nullptr ? source->name : "<missing>";
    const std::string label = std::string(cr::toString(issue.severity)) + "  " +
                              std::string(cr::toString(issue.code)) + "  " +
                              sourceName;
    ImGui::PushID(static_cast<int>(index));
    ImGui::PushStyleColor(ImGuiCol_Text,
                          creativeDesktopLogicDiagnosticColor(issue.severity));
    if (ImGui::Selectable(label.c_str())) {
      queueCreativeDesktopObjectNavigation(
          commands,
          source != nullptr ? issue.sourceObjectId : issue.targetObjectId,
          playModeActive);
    }
    ImGui::PopStyleColor();
    ImGui::PopID();
  }
  if (diagnostics.issueCount == 0U) {
    ImGui::TextDisabled("No logic diagnostics");
  }
  if (diagnostics.capacityExceeded) {
    ImGui::TextColored(ImVec4{1.0F, 0.34F, 0.30F, 1.0F},
                       "Diagnostic capacity exceeded; %llu issues omitted",
                       static_cast<unsigned long long>(
                           diagnostics.droppedIssueCount));
  }
}

void appendPassStatusTab(const cr::CreativeLogicDiagnosticReport& diagnostics) {
  ImGui::TextColored(diagnostics.errorCount == 0U
                         ? ImVec4{0.20F, 1.0F, 0.35F, 1.0F}
                         : ImVec4{1.0F, 0.34F, 0.30F, 1.0F},
                     "%s", diagnostics.errorCount == 0U ? "LOGIC CHECK PASSED"
                                                        : "LOGIC CHECK FAILED");
  ImGui::TextDisabled("Warnings do not block validation; errors do.");
}

// A compact read-only viewport header. The tool buttons it replaced performed no
// command, and a visible control that does nothing is a false affordance.
void appendToolbarHeader(const cr::CreativeAppState& appState,
                         const CreativeEditorState& editor,
                         bool playModeActive) {
  const cr::CreativeDocument& document = appState.facade.document();
  const CreativeDesktopLiveSelection live = creativeDesktopLiveSelection(appState);
  const CreativeDesktopSelectionResolution resolved =
      resolveCreativeDesktopSelection(document, live.objectIds,
                                      live.primaryObjectId);
  const std::string_view name = document.name();
  ImGui::Text("%.*s", static_cast<int>(name.size()), name.data());
  ImGui::SameLine();
  ImGui::TextDisabled("|");
  ImGui::SameLine();
  ImGui::Text("selection %zu", resolved.objectIds.size());
  ImGui::SameLine();
  ImGui::TextDisabled("|");
  ImGui::SameLine();
  ImGui::Text("device %s", controlDeviceName(editor.activeControlDevice));
  if (playModeActive) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4{0.20F, 1.0F, 0.35F, 1.0F}, "PLAY");
  }
}

}  // namespace

void buildCreativeEditorDesktopMenuBar(
    CreativeEditorDesktopUiState& desktopUi,
    const cr::CreativeAppState& appState,
    const CreativeEditorWorldLayoutState* worldLayout,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands) {
  const cr::CreativeSelectionState& selection = appState.facade.selectionState();
  const bool hasSelection = cr::selectedTargetCount(selection) > 0U;
  const bool sourceSynchronized =
      worldLayout == nullptr ||
      worldLayout->revision == worldLayout->generatedRevision;
  const bool canUndo =
      (worldLayout != nullptr &&
       creativeEditorWorldLayoutSourceUndoAvailable(*worldLayout)) ||
      (sourceSynchronized && cr::creativeUndoAvailable(appState.history));
  const bool canRedo =
      (worldLayout != nullptr &&
       creativeEditorWorldLayoutSourceRedoAvailable(*worldLayout)) ||
      (sourceSynchronized && cr::creativeRedoAvailable(appState.history));

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New")) {
        commands.push(CreativeDesktopCommandId::NewDocument);
      }
      if (ImGui::MenuItem("Open")) {
        commands.push(CreativeDesktopCommandId::OpenDocument);
      }
      if (ImGui::MenuItem("Save", "Ctrl+S")) {
        commands.push(CreativeDesktopCommandId::SaveDocument);
      }
      if (ImGui::MenuItem("Save As...")) {
        desktopUi.saveAsModalOpen = true;
        desktopUi.saveAsNameBuffer[0] = '\0';
      }
      // Import lands with the Asset Library slice (UI-2b) where the catalog
      // reload glue lives; disabled until then.
      ImGui::MenuItem("Import...", nullptr, false, false);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo)) {
        commands.push(CreativeDesktopCommandId::Undo);
      }
      if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo)) {
        commands.push(CreativeDesktopCommandId::Redo);
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, hasSelection)) {
        commands.push(CreativeDesktopCommandId::DuplicateSelection);
      }
      if (ImGui::MenuItem("Delete", "Del", false, hasSelection)) {
        commands.push(CreativeDesktopCommandId::DeleteSelection);
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Project", nullptr, &desktopUi.showOutliner);
      ImGui::MenuItem("Inspector", nullptr, &desktopUi.showInspector);
      ImGui::MenuItem("Diagnostics", nullptr, &desktopUi.showDiagnostics);
      ImGui::MenuItem("World Layout", nullptr, &desktopUi.showWorldLayout);
      ImGui::Separator();
      if (ImGui::MenuItem("Reset Layout")) {
        desktopUi.resetLayoutRequested = true;
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Tools")) {
      if (ImGui::MenuItem("Terrain Generator")) {
        if (worldLayout != nullptr &&
            creativeEditorWorldLayoutPreviewActive(*worldLayout)) {
          commands.push(CreativeDesktopCommandId::WorldLayoutCancelPreview);
        }
        desktopUi.showWorldLayout = false;
        desktopUi.showInspector = true;
        desktopUi.terrainGeneratorFocusRequested = true;
      }
      ImGui::EndMenu();
    }
    if (ImGui::MenuItem(playModeActive ? "Stop" : "Play")) {
      commands.push(CreativeDesktopCommandId::Play);
    }
    ImGui::EndMainMenuBar();
  }

  // One-shot trigger -> ImGui owns the popup's open state from here.
  if (desktopUi.saveAsModalOpen) {
    ImGui::OpenPopup("Save As##desktop");
    desktopUi.saveAsModalOpen = false;
  }
  if (ImGui::BeginPopupModal("Save As##desktop", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted("Save the current document as:");
    ImGui::InputText("Name", desktopUi.saveAsNameBuffer.data(),
                     desktopUi.saveAsNameBuffer.size());
    const bool nameOk = desktopUi.saveAsNameBuffer[0] != '\0';
    ImGui::BeginDisabled(!nameOk);
    if (ImGui::Button("Save")) {
      commands.push(CreativeDesktopCommandId::SaveDocumentAs,
                    std::string(desktopUi.saveAsNameBuffer.data()));
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void buildCreativeEditorDesktopPanels(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorState& editor,
    const cr::CreativeAppState& appState,
    const CreativeEditorPlayMode* playMode,
    CreativeDesktopCommandFrame& commands) {
  const cr::CreativeDocument& document = appState.facade.document();
  // Logic topology is document-revision owned, so idle UI frames reuse one
  // bounded report instead of rescanning links.
  if (!desktopUi.logicDiagnosticsCached ||
      desktopUi.logicDiagnosticDocumentId != document.id() ||
      desktopUi.logicDiagnosticRevision != document.revision()) {
    desktopUi.logicDiagnostics = cr::buildCreativeLogicDiagnostics(
        document.logicLinks(), document.objects());
    desktopUi.logicDiagnosticDocumentId = document.id();
    desktopUi.logicDiagnosticRevision = document.revision();
    desktopUi.logicDiagnosticsCached = true;
  }
  const cr::CreativeLogicDiagnosticReport& logicDiagnostics =
      desktopUi.logicDiagnostics;
  const bool playModeActive =
      playMode != nullptr && creativeEditorPlayModeActive(*playMode);

  const ImGuiWindowFlags toolbarFlags =
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
  if (ImGui::Begin("Toolbar##desktop", nullptr, toolbarFlags)) {
    appendToolbarHeader(appState, editor, playModeActive);
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    if (ImGui::Button(desktopUi.showWorldLayout ? "Close World Layout"
                                                : "World Layout")) {
      if (!desktopUi.showWorldLayout &&
          editor.terrainGeneration.previewActive) {
        commands.push(CreativeDesktopCommandId::TerrainGenerationCancel);
      }
      desktopUi.showWorldLayout = !desktopUi.showWorldLayout;
    }
    if (creativeEditorWorldLayoutPreviewActive(editor.worldLayout)) {
      ImGui::SameLine();
      if (ImGui::Button("Close Preview")) {
        commands.push(CreativeDesktopCommandId::WorldLayoutCancelPreview);
      }
    }
  }
  ImGui::End();

  // World Layout is a focused workspace: its tools, properties, and build
  // status reuse the established left/right/bottom dock identities. Closing
  // it restores the normal Project/Inspector/Diagnostics contents without
  // rebuilding or disturbing the user's dock arrangement.
  if (!desktopUi.showWorldLayout) {
    // Project / Outliner (left).
    if (desktopUi.showOutliner) {
      if (ImGui::Begin("Project", &desktopUi.showOutliner)) {
        buildCreativeEditorDesktopOutlinerPanel(desktopUi, appState,
                                                playModeActive, commands);
      }
      ImGui::End();
    }

    // Inspector (right).
    if (desktopUi.showInspector) {
      if (ImGui::Begin("Inspector", &desktopUi.showInspector)) {
        if (ImGui::BeginTabBar("##creative_desktop_inspector_tabs")) {
          if (ImGui::BeginTabItem("Selection")) {
            buildCreativeEditorDesktopInspectorPanel(
                desktopUi, editor, appState, playMode, commands);
            ImGui::EndTabItem();
          }
          const ImGuiTabItemFlags terrainFlags =
              desktopUi.terrainGeneratorFocusRequested
                  ? ImGuiTabItemFlags_SetSelected
                  : ImGuiTabItemFlags_None;
          if (ImGui::BeginTabItem("Terrain Generator", nullptr,
                                  terrainFlags)) {
            buildCreativeEditorDesktopTerrainGenerationPanel(
                editor, appState, playModeActive, commands);
            ImGui::EndTabItem();
          }
          desktopUi.terrainGeneratorFocusRequested = false;
          ImGui::EndTabBar();
        }
      }
      ImGui::End();
    }

    // Diagnostics (bottom). Only the two tabs that project real products
    // remain; Diffs / Stale Outputs / Proof Receipts do not exist yet.
    if (desktopUi.showDiagnostics) {
      if (ImGui::Begin("Diagnostics##bottom", &desktopUi.showDiagnostics)) {
        if (ImGui::BeginTabBar("##desktop_bottom_tabs")) {
          if (ImGui::BeginTabItem("Diagnostics")) {
            appendDiagnosticsTab(document, logicDiagnostics, playModeActive,
                                 commands);
            ImGui::EndTabItem();
          }
          if (ImGui::BeginTabItem("Pass Status")) {
            appendPassStatusTab(logicDiagnostics);
            ImGui::EndTabItem();
          }
          ImGui::EndTabBar();
        }
      }
      ImGui::End();
    }
  }

  buildCreativeEditorWorldLayoutPanel(desktopUi, editor, document,
                                      playModeActive, commands);
}

void buildCreativeEditorDesktopStatusBar(
    const CreativeEditorDesktopUiState& desktopUi,
    const CreativeEditorState& editor,
    const cr::CreativeAppState& appState) {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const float barHeight = ImGui::GetFrameHeight();
  ImGui::SetNextWindowPos(
      ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - barHeight));
  ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, barHeight));
  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
  if (ImGui::Begin("##creative_desktop_status", nullptr, flags)) {
    const cr::CreativeDocument& document = appState.facade.document();
    const bool dirty = document.revision() != desktopUi.lastSavedRevision ||
                       creativeEditorWorldLayoutDirty(editor.worldLayout);
    const std::uint64_t selectionCount =
        cr::selectedTargetCount(appState.facade.selectionState());
    const std::string_view name = document.name();
    ImGui::Text("%.*s%s", static_cast<int>(name.size()), name.data(),
                dirty ? " *" : "");
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("rev %llu", static_cast<unsigned long long>(document.revision()));
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("objects %llu",
                static_cast<unsigned long long>(document.objectCount()));
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("selection %llu",
                static_cast<unsigned long long>(selectionCount));
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("device %s", controlDeviceName(editor.activeControlDevice));
    if (!desktopUi.statusMessage.empty()) {
      ImGui::SameLine();
      ImGui::TextDisabled("|");
      ImGui::SameLine();
      ImGui::TextUnformatted(desktopUi.statusMessage.c_str());
    }
    const CreativeEditorPlacementFeedback& feedback =
        editor.interaction.placementFeedback;
    const CreativeEditorPlacementFeedbackViewModel feedbackView =
        creativeEditorPlacementFeedbackViewModel(
            feedback, editor.frameIndex, &document);
    if (feedbackView.visible &&
        feedbackView.status ==
            CreativeEditorPlacementFeedbackStatus::Rejected &&
        !feedbackView.label.empty()) {
      ImGui::SameLine();
      ImGui::TextDisabled("|");
      ImGui::SameLine();
      ImGui::TextColored(
          ImVec4(feedbackView.color.r, feedbackView.color.g,
                 feedbackView.color.b, feedbackView.color.a),
          "%.*s", static_cast<int>(feedbackView.label.length),
          feedbackView.label.bytes.data());
    }
  }
  ImGui::End();
  ImGui::PopStyleVar(2);
}

}  // namespace iggy3d_creative_app
