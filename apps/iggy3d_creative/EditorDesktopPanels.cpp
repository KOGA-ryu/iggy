#include "EditorDesktopPanels.hpp"

#include <string>

#include "imgui.h"

#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"

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

}  // namespace

void buildCreativeEditorDesktopMenuBar(
    CreativeEditorDesktopUiState& desktopUi,
    const cr::CreativeAppState& appState,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands) {
  const cr::CreativeSelectionState& selection = appState.facade.selectionState();
  const bool hasSelection = cr::selectedTargetCount(selection) > 0U;
  const bool canUndo = cr::creativeUndoAvailable(appState.history);
  const bool canRedo = cr::creativeRedoAvailable(appState.history);

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
      ImGui::Separator();
      if (ImGui::MenuItem("Reset Layout")) {
        desktopUi.resetLayoutRequested = true;
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

void buildCreativeEditorDesktopPanels(CreativeEditorDesktopUiState& desktopUi) {
  // Toolbar strip above the central viewport (placeholder tools for now).
  const ImGuiWindowFlags toolbarFlags =
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
  if (ImGui::Begin("Toolbar##desktop", nullptr, toolbarFlags)) {
    const char* const tools[] = {"Select", "Pan",  "Move",  "Rotate",
                                 "Scale",  "Place", "Erase", "Measure"};
    for (int index = 0; index < IM_ARRAYSIZE(tools); ++index) {
      if (index != 0) {
        ImGui::SameLine();
      }
      ImGui::Button(tools[index]);
    }
  }
  ImGui::End();

  // Project tree (left).
  if (desktopUi.showOutliner) {
    if (ImGui::Begin("Project", &desktopUi.showOutliner)) {
      ImGui::TextDisabled("Project tree");
      ImGui::TextDisabled("Sources / Floors / Objects — coming soon");
    }
    ImGui::End();
  }

  // Inspector (right).
  if (desktopUi.showInspector) {
    if (ImGui::Begin("Inspector", &desktopUi.showInspector)) {
      ImGui::TextDisabled("Inspector");
      ImGui::TextDisabled("Token / Provenance / Diagnostics — coming soon");
    }
    ImGui::End();
  }

  // Diagnostics (bottom) — the tabbed utility area.
  if (desktopUi.showDiagnostics) {
    if (ImGui::Begin("Diagnostics##bottom", &desktopUi.showDiagnostics)) {
      if (ImGui::BeginTabBar("##desktop_bottom_tabs")) {
        const char* const tabs[] = {"Diagnostics", "Pass Status", "Diffs",
                                     "Stale Outputs", "Proof Receipts"};
        for (const char* tab : tabs) {
          if (ImGui::BeginTabItem(tab)) {
            ImGui::TextDisabled("%s — coming soon", tab);
            ImGui::EndTabItem();
          }
        }
        ImGui::EndTabBar();
      }
    }
    ImGui::End();
  }
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
    const bool dirty = document.revision() != desktopUi.lastSavedRevision;
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
  }
  ImGui::End();
  ImGui::PopStyleVar(2);
}

}  // namespace iggy3d_creative_app
