#include "EditorDesktopPanels.hpp"

#include <array>
#include <string>
#include <string_view>

#include "imgui.h"

#include "EditorPlayMode.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/play/RuntimeInteractables.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

namespace {

constexpr std::array kInspectorLogicActions{
    cr::CreativeLogicLinkAction::Toggle,
    cr::CreativeLogicLinkAction::Open,
    cr::CreativeLogicLinkAction::Close,
};

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

void queueObjectNavigation(CreativeDesktopCommandFrame& commands,
                           cr::CreativeObjectId objectId,
                           bool playModeActive) {
  if (objectId == cr::kInvalidObjectId) {
    return;
  }
  commands.push(
      playModeActive ? CreativeDesktopCommandId::SelectObjects
                     : CreativeDesktopCommandId::FocusObject,
      CreativeDesktopSelectPayload{{objectId}, objectId});
}

void queueLogicSource(CreativeDesktopCommandFrame& commands,
                      cr::CreativeObjectId sourceObjectId) {
  commands.push(CreativeDesktopCommandId::SetLogicSource,
                CreativeDesktopLogicLinkPayload{sourceObjectId});
}

void queueSetLogicLink(CreativeDesktopCommandFrame& commands,
                       cr::CreativeObjectId sourceObjectId,
                       cr::CreativeObjectId targetObjectId,
                       cr::CreativeLogicLinkAction action) {
  commands.push(CreativeDesktopCommandId::SetLogicLink,
                CreativeDesktopLogicLinkPayload{sourceObjectId,
                                                targetObjectId, action});
}

void queueRemoveLogicLink(CreativeDesktopCommandFrame& commands,
                          cr::CreativeObjectId sourceObjectId,
                          cr::CreativeObjectId targetObjectId) {
  commands.push(CreativeDesktopCommandId::RemoveLogicLink,
                CreativeDesktopLogicLinkPayload{sourceObjectId,
                                                targetObjectId});
}

[[nodiscard]] const cr::CreativeLogicDiagnostic* diagnosticForSource(
    const cr::CreativeLogicDiagnosticReport& report,
    cr::CreativeObjectId sourceObjectId) noexcept {
  for (std::size_t index = 0U; index < report.issueCount; ++index) {
    if (report.issues[index].sourceObjectId == sourceObjectId ||
        report.issues[index].relatedSourceObjectId == sourceObjectId) {
      return &report.issues[index];
    }
  }
  return nullptr;
}

[[nodiscard]] ImVec4 logicDiagnosticColor(
    cr::CreativeLogicDiagnosticSeverity severity) noexcept {
  return severity == cr::CreativeLogicDiagnosticSeverity::Error
             ? ImVec4{1.0F, 0.34F, 0.30F, 1.0F}
             : ImVec4{1.0F, 0.82F, 0.25F, 1.0F};
}

[[nodiscard]] cr::CreativeObjectId inspectedObjectId(
    const CreativeEditorState& editor,
    const cr::CreativeAppState& appState) noexcept {
  const cr::TargetRef selected = appState.facade.selectionState().selectedTarget;
  return selected.value != cr::kInvalidId
             ? static_cast<cr::CreativeObjectId>(selected.value)
             : editor.logicLinks.sourceObjectId;
}

void appendLogicLinkRow(const cr::CreativeDocument& document,
                        const cr::CreativeLogicLink& link,
                        bool outgoing,
                        bool playModeActive,
                        CreativeDesktopCommandFrame& commands) {
  const cr::CreativeObjectId otherId =
      outgoing ? link.targetObjectId : link.sourceObjectId;
  const cr::CreativeObject* other = document.findObject(otherId);
  const std::string_view otherName =
      other != nullptr ? std::string_view{other->name}
                       : std::string_view{"<missing>"};
  const std::string label =
      std::string(outgoing ? "to " : "from ") + std::string(otherName) +
      "  [" + std::string(cr::toString(link.action)) + "]";
  ImGui::PushID(static_cast<int>(otherId));
  if (ImGui::Selectable(label.c_str())) {
    queueObjectNavigation(commands, otherId, playModeActive);
  }
  if (!playModeActive) {
    ImGui::SetNextItemWidth(132.0F);
    if (ImGui::BeginCombo("##logic_action",
                          std::string(cr::toString(link.action)).c_str())) {
      for (const cr::CreativeLogicLinkAction action :
           kInspectorLogicActions) {
        const bool selected = action == link.action;
        if (ImGui::Selectable(std::string(cr::toString(action)).c_str(),
                              selected) &&
            !selected) {
          queueSetLogicLink(commands, link.sourceObjectId,
                            link.targetObjectId, action);
        }
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("X")) {
      queueRemoveLogicLink(commands, link.sourceObjectId,
                           link.targetObjectId);
    }
    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      ImGui::TextUnformatted("Remove logic link");
      ImGui::EndTooltip();
    }
  }
  ImGui::PopID();
}

void appendNewLogicLinkControl(const cr::CreativeDocument& document,
                               const CreativeEditorState& editor,
                               const cr::CreativeObject& target,
                               bool playModeActive,
                               CreativeDesktopCommandFrame& commands) {
  if (playModeActive ||
      !cr::creativeObjectCanTargetLogicLink(target.kind) ||
      editor.logicLinks.sourceObjectId == cr::kInvalidObjectId) {
    return;
  }
  const cr::CreativeObject* source =
      document.findObject(editor.logicLinks.sourceObjectId);
  if (source == nullptr ||
      !cr::creativeObjectCanSourceLogicLink(source->kind)) {
    return;
  }
  const cr::CreativeLogicLink* existing = document.findLogicLink(
      source->id, target.id);
  if (existing != nullptr) {
    return;
  }
  ImGui::SeparatorText("Active source");
  ImGui::TextUnformatted(source->name.c_str());
  ImGui::SetNextItemWidth(160.0F);
  if (ImGui::BeginCombo("##new_logic_action", "Add link...")) {
    for (const cr::CreativeLogicLinkAction action : kInspectorLogicActions) {
      if (ImGui::Selectable(std::string(cr::toString(action)).c_str())) {
        queueSetLogicLink(commands, source->id, target.id, action);
      }
    }
    ImGui::EndCombo();
  }
}

void appendRuntimeLogicMonitor(
    const CreativeEditorPlayMode* playMode,
    cr::CreativeObjectId sourceObjectId,
    const cr::CreativeDocument& document,
    CreativeDesktopCommandFrame& commands) {
  if (playMode == nullptr || !playMode->sandbox.has_value()) {
    return;
  }
  const cr::CreativeRuntimeSandbox& sandbox = *playMode->sandbox;
  const cr::CreativeRuntimeInteractableState* source =
      cr::findCreativeRuntimeInteractableByObjectId(sandbox, sourceObjectId);
  if (source == nullptr ||
      source->definition.logicSourceMode ==
          cr::CreativeRuntimeLogicSourceMode::None) {
    return;
  }

  ImGui::SeparatorText("Play monitor");
  ImGui::Text("Mode: %s",
              std::string(cr::toString(source->definition.logicSourceMode))
                  .c_str());
  ImGui::Text("Occupants: %llu",
              static_cast<unsigned long long>(source->occupantCount));
  ImGui::TextColored(
      source->occupantCount > 0U ? ImVec4{0.20F, 1.0F, 0.35F, 1.0F}
                                : ImVec4{0.65F, 0.72F, 0.76F, 1.0F},
      "%s", source->occupantCount > 0U ? "ACTIVE" : "ARMED");
  ImGui::Text("Last transition: %s  tick %llu",
              std::string(cr::toString(source->lastOccupancyTransition))
                  .c_str(),
              static_cast<unsigned long long>(
                  source->lastOccupancyTransitionTick));
  ImGui::Text("Geometry revision: %llu",
              static_cast<unsigned long long>(sandbox.geometryRevision));
  ImGui::TextDisabled(
      "Last automatic pass: %s",
      std::string(cr::toString(playMode->lastAutomaticLogic.status)).c_str());

  for (const cr::CreativeRuntimeLogicLink& runtimeLink : sandbox.logicLinks) {
    if (runtimeLink.sourceObjectId != sourceObjectId) {
      continue;
    }
    const cr::CreativeRuntimeInteractableState* target =
        cr::findCreativeRuntimeInteractableByObjectId(
            sandbox, runtimeLink.targetObjectId);
    const cr::CreativeObject* authoredTarget =
        document.findObject(runtimeLink.targetObjectId);
    const std::string targetName =
        authoredTarget != nullptr ? authoredTarget->name : "<missing>";
    const char* doorState =
        target != nullptr &&
                target->definition.kind ==
                    cr::CreativeRuntimeInteractableKind::Door
            ? (target->doorOpen ? "OPEN" : "CLOSED")
            : "INVALID";
    const std::string label = targetName + "  [" +
                              std::string(cr::toString(runtimeLink.action)) +
                              "]  " + doorState;
    ImGui::PushID(static_cast<int>(runtimeLink.targetObjectId));
    if (ImGui::Selectable(label.c_str())) {
      queueObjectNavigation(commands, runtimeLink.targetObjectId,
                            /*playModeActive=*/true);
    }
    ImGui::PopID();
  }
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

void buildCreativeEditorDesktopPanels(
    CreativeEditorDesktopUiState& desktopUi,
    const CreativeEditorState& editor,
    const cr::CreativeAppState& appState,
    const CreativeEditorPlayMode* playMode,
    CreativeDesktopCommandFrame& commands) {
  const cr::CreativeDocument& document = appState.facade.document();
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
      const cr::CreativeObjectId inspectedId =
          inspectedObjectId(editor, appState);
      const cr::CreativeObject* inspected = document.findObject(inspectedId);
      if (inspected == nullptr) {
        ImGui::TextDisabled("Select an object to inspect its logic links.");
      } else {
        ImGui::TextUnformatted(inspected->name.c_str());
        ImGui::TextDisabled("%s  |  id %llu",
                            std::string(cr::toString(inspected->kind)).c_str(),
                            static_cast<unsigned long long>(inspected->id));
        if (cr::creativeObjectCanSourceLogicLink(inspected->kind)) {
          const cr::CreativeRuntimeLogicSourceMode sourceMode =
              cr::creativeRuntimeLogicSourceModeForObject(inspected->kind);
          ImGui::Text("Source mode: %s",
                      std::string(cr::toString(sourceMode)).c_str());
          const cr::CreativeLogicDiagnostic* sourceDiagnostic =
              diagnosticForSource(logicDiagnostics, inspected->id);
          if (sourceDiagnostic != nullptr) {
            ImGui::TextColored(
                logicDiagnosticColor(sourceDiagnostic->severity), "%s",
                std::string(cr::toString(sourceDiagnostic->code)).c_str());
          }
          const bool activeSource =
              editor.logicLinks.sourceObjectId == inspected->id;
          if (activeSource) {
            ImGui::TextColored(ImVec4{0.20F, 1.0F, 0.35F, 1.0F},
                               "ACTIVE SOURCE");
            if (!playModeActive) {
              ImGui::SameLine();
              if (ImGui::SmallButton("X##logic_source")) {
                commands.push(CreativeDesktopCommandId::ClearLogicSource);
              }
              if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Clear active logic source");
                ImGui::EndTooltip();
              }
            }
          } else if (!playModeActive &&
                     ImGui::Button("Set as source##logic_source")) {
            queueLogicSource(commands, inspected->id);
          }
        }
        ImGui::SeparatorText("Logic links");
        std::size_t shown = 0U;
        for (const cr::CreativeLogicLink& link : document.logicLinks()) {
          if (link.sourceObjectId != inspected->id &&
              link.targetObjectId != inspected->id) {
            continue;
          }
          const bool outgoing = link.sourceObjectId == inspected->id;
          appendLogicLinkRow(document, link, outgoing, playModeActive,
                             commands);
          ++shown;
        }
        if (shown == 0U) {
          ImGui::TextDisabled("No logic links");
        }
        appendNewLogicLinkControl(document, editor, *inspected,
                                  playModeActive, commands);
        ImGui::TextDisabled("Document links: %llu",
                            static_cast<unsigned long long>(
                                document.logicLinks().size()));
        appendRuntimeLogicMonitor(playMode, inspected->id, document, commands);
      }
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
            if (std::string_view{tab} == "Diagnostics") {
              ImGui::Text("Logic sources: %llu  linked: %llu",
                          static_cast<unsigned long long>(
                              logicDiagnostics.sourceCount),
                          static_cast<unsigned long long>(
                              logicDiagnostics.linkedSourceCount));
              ImGui::SameLine();
              ImGui::TextColored(
                  logicDiagnostics.errorCount == 0U
                      ? ImVec4{0.20F, 1.0F, 0.35F, 1.0F}
                      : ImVec4{1.0F, 0.34F, 0.30F, 1.0F},
                  "%llu errors  %llu warnings",
                  static_cast<unsigned long long>(logicDiagnostics.errorCount),
                  static_cast<unsigned long long>(
                      logicDiagnostics.warningCount));
              for (std::size_t index = 0U;
                   index < logicDiagnostics.issueCount; ++index) {
                const cr::CreativeLogicDiagnostic& issue =
                    logicDiagnostics.issues[index];
                const cr::CreativeObject* source =
                    document.findObject(issue.sourceObjectId);
                const std::string sourceName =
                    source != nullptr ? source->name : "<missing>";
                const std::string label =
                    std::string(cr::toString(issue.severity)) + "  " +
                    std::string(cr::toString(issue.code)) + "  " + sourceName;
                ImGui::PushID(static_cast<int>(index));
                ImGui::PushStyleColor(ImGuiCol_Text,
                                      logicDiagnosticColor(issue.severity));
                if (ImGui::Selectable(label.c_str())) {
                  const cr::CreativeObjectId navigationId =
                      source != nullptr ? issue.sourceObjectId
                                        : issue.targetObjectId;
                  queueObjectNavigation(commands, navigationId,
                                        playModeActive);
                }
                ImGui::PopStyleColor();
                ImGui::PopID();
              }
              if (logicDiagnostics.issueCount == 0U) {
                ImGui::TextDisabled("No logic diagnostics");
              }
              if (logicDiagnostics.capacityExceeded) {
                ImGui::TextColored(
                    ImVec4{1.0F, 0.34F, 0.30F, 1.0F},
                    "Diagnostic capacity exceeded; %llu issues omitted",
                    static_cast<unsigned long long>(
                        logicDiagnostics.droppedIssueCount));
              }
            } else if (std::string_view{tab} == "Pass Status") {
              ImGui::TextColored(
                  logicDiagnostics.errorCount == 0U
                      ? ImVec4{0.20F, 1.0F, 0.35F, 1.0F}
                      : ImVec4{1.0F, 0.34F, 0.30F, 1.0F},
                  "%s", logicDiagnostics.errorCount == 0U
                            ? "LOGIC CHECK PASSED"
                            : "LOGIC CHECK FAILED");
              ImGui::TextDisabled(
                  "Warnings do not block validation; errors do.");
            } else {
              ImGui::TextDisabled("%s — coming soon", tab);
            }
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
