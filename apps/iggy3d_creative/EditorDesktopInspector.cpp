#include "EditorDesktopWidgets.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "EditorDesktopModel.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/play/RuntimeInteractables.hpp"

// UI-4A Inspector. General object properties (zero/single/multi) sit above the
// existing logic-link authoring, diagnostics, and Play-mode runtime monitor,
// which moved here from EditorDesktopPanels.cpp with their behavior unchanged.
// Every edit leaves as a typed command; nothing here mutates the document.

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr std::array kInspectorLogicActions{
    cr::CreativeLogicLinkAction::Toggle,
    cr::CreativeLogicLinkAction::Open,
    cr::CreativeLogicLinkAction::Close,
    cr::CreativeLogicLinkAction::Enable,
    cr::CreativeLogicLinkAction::Disable,
    cr::CreativeLogicLinkAction::Reverse,
};

// ---- shared small helpers -------------------------------------------------

void appendHoverTooltip(const char* text) {
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(text);
    ImGui::EndTooltip();
  }
}

// Non-truncating std::string InputText adapter (imgui_stdlib is not vendored),
// so an existing document name is never clipped into a fixed buffer.
int inputTextResizeCallback(ImGuiInputTextCallbackData* data) {
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    auto* str = static_cast<std::string*>(data->UserData);
    str->resize(static_cast<std::size_t>(data->BufTextLen));
    data->Buf = str->data();
  }
  return 0;
}

bool inputTextStdString(const char* label, std::string* str,
                        ImGuiInputTextFlags flags) {
  return ImGui::InputText(label, str->data(), str->capacity() + 1U,
                          flags | ImGuiInputTextFlags_CallbackResize,
                          inputTextResizeCallback, str);
}

// ---- logic helpers (moved from EditorDesktopPanels.cpp) -------------------

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
                CreativeDesktopLogicLinkPayload{sourceObjectId, targetObjectId,
                                                action});
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
      std::string(outgoing ? "to " : "from ") + std::string(otherName) + "  [" +
      std::string(cr::toString(link.action)) + "]";
  ImGui::PushID(static_cast<int>(otherId));
  if (ImGui::Selectable(label.c_str())) {
    queueCreativeDesktopObjectNavigation(commands, otherId, playModeActive);
  }
  if (!playModeActive) {
    const cr::CreativeObject* target =
        document.findObject(link.targetObjectId);
    ImGui::SetNextItemWidth(132.0F);
    if (ImGui::BeginCombo("##logic_action",
                          std::string(cr::toString(link.action)).c_str())) {
      for (const cr::CreativeLogicLinkAction action : kInspectorLogicActions) {
        if (target == nullptr ||
            !cr::creativeLogicLinkActionSupported(target->kind, action)) {
          continue;
        }
        const bool selected = action == link.action;
        if (ImGui::Selectable(std::string(cr::toString(action)).c_str(),
                              selected) &&
            !selected) {
          queueSetLogicLink(commands, link.sourceObjectId, link.targetObjectId,
                            action);
        }
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("X")) {
      queueRemoveLogicLink(commands, link.sourceObjectId, link.targetObjectId);
    }
    appendHoverTooltip("Remove logic link");
  }
  ImGui::PopID();
}

void appendNewLogicLinkControl(const cr::CreativeDocument& document,
                               const CreativeEditorState& editor,
                               const cr::CreativeObject& target,
                               bool playModeActive,
                               CreativeDesktopCommandFrame& commands) {
  if (playModeActive || !cr::creativeObjectCanTargetLogicLink(target.kind) ||
      editor.logicLinks.sourceObjectId == cr::kInvalidObjectId) {
    return;
  }
  const cr::CreativeObject* source =
      document.findObject(editor.logicLinks.sourceObjectId);
  if (source == nullptr ||
      !cr::creativeObjectCanSourceLogicLink(source->kind)) {
    return;
  }
  if (document.findLogicLink(source->id, target.id) != nullptr) {
    return;
  }
  ImGui::SeparatorText("Active source");
  ImGui::TextUnformatted(source->name.c_str());
  ImGui::SetNextItemWidth(160.0F);
  if (ImGui::BeginCombo("##new_logic_action", "Add link...")) {
    for (const cr::CreativeLogicLinkAction action : kInspectorLogicActions) {
      if (!cr::creativeLogicLinkActionSupported(target.kind, action)) {
        continue;
      }
      if (ImGui::Selectable(std::string(cr::toString(action)).c_str())) {
        queueSetLogicLink(commands, source->id, target.id, action);
      }
    }
    ImGui::EndCombo();
  }
}

void appendRuntimeLogicMonitor(const CreativeEditorPlayMode* playMode,
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
  ImGui::TextColored(source->occupantCount > 0U
                         ? ImVec4{0.20F, 1.0F, 0.35F, 1.0F}
                         : ImVec4{0.65F, 0.72F, 0.76F, 1.0F},
                     "%s", source->occupantCount > 0U ? "ACTIVE" : "ARMED");
  ImGui::Text("Last transition: %s  tick %llu",
              std::string(cr::toString(source->lastOccupancyTransition)).c_str(),
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
    const char* targetState = "INVALID";
    if (target != nullptr) {
      switch (target->definition.kind) {
        case cr::CreativeRuntimeInteractableKind::Door:
          targetState = target->targetActive ? "OPEN" : "CLOSED";
          break;
        case cr::CreativeRuntimeInteractableKind::Platform:
          targetState = target->targetActive ? "ENABLED" : "DISABLED";
          break;
        case cr::CreativeRuntimeInteractableKind::MovingPlatform:
          targetState = target->targetActive
                            ? (target->movingPlatform.blocked ? "BLOCKED"
                                                              : "MOVING")
                            : "PAUSED";
          break;
        case cr::CreativeRuntimeInteractableKind::Control:
        case cr::CreativeRuntimeInteractableKind::Pickup:
          break;
      }
    }
    const std::string label = targetName + "  [" +
                              std::string(cr::toString(runtimeLink.action)) +
                              "]  " + targetState;
    ImGui::PushID(static_cast<int>(runtimeLink.targetObjectId));
    if (ImGui::Selectable(label.c_str())) {
      queueCreativeDesktopObjectNavigation(commands, runtimeLink.targetObjectId,
                                           /*playModeActive=*/true);
    }
    ImGui::PopID();
  }
}

// The moved logic authoring block, unchanged in behavior. It now sits beneath
// the general single-object properties.
void appendLogicSection(CreativeEditorDesktopUiState& desktopUi,
                        const CreativeEditorState& editor,
                        const cr::CreativeDocument& document,
                        const cr::CreativeObject& inspected,
                        const CreativeEditorPlayMode* playMode,
                        bool playModeActive,
                        CreativeDesktopCommandFrame& commands) {
  if (cr::creativeObjectCanSourceLogicLink(inspected.kind)) {
    const cr::CreativeRuntimeLogicSourceMode sourceMode =
        cr::creativeRuntimeLogicSourceModeForObject(inspected.kind);
    ImGui::Text("Source mode: %s",
                std::string(cr::toString(sourceMode)).c_str());
    const cr::CreativeLogicDiagnostic* sourceDiagnostic =
        diagnosticForSource(desktopUi.logicDiagnostics, inspected.id);
    if (sourceDiagnostic != nullptr) {
      ImGui::TextColored(
          creativeDesktopLogicDiagnosticColor(sourceDiagnostic->severity), "%s",
          std::string(cr::toString(sourceDiagnostic->code)).c_str());
    }
    const bool activeSource = editor.logicLinks.sourceObjectId == inspected.id;
    if (activeSource) {
      ImGui::TextColored(ImVec4{0.20F, 1.0F, 0.35F, 1.0F}, "ACTIVE SOURCE");
      if (!playModeActive) {
        ImGui::SameLine();
        if (ImGui::SmallButton("X##logic_source")) {
          commands.push(CreativeDesktopCommandId::ClearLogicSource);
        }
        appendHoverTooltip("Clear active logic source");
      }
    } else if (!playModeActive &&
               ImGui::Button("Set as source##logic_source")) {
      queueLogicSource(commands, inspected.id);
    }
  }

  ImGui::SeparatorText("Logic links");
  std::size_t shown = 0U;
  for (const cr::CreativeLogicLink& link : document.logicLinks()) {
    if (link.sourceObjectId != inspected.id &&
        link.targetObjectId != inspected.id) {
      continue;
    }
    appendLogicLinkRow(document, link, link.sourceObjectId == inspected.id,
                       playModeActive, commands);
    ++shown;
  }
  if (shown == 0U) {
    ImGui::TextDisabled("No logic links");
  }
  appendNewLogicLinkControl(document, editor, inspected, playModeActive,
                            commands);
  ImGui::TextDisabled(
      "Document links: %llu",
      static_cast<unsigned long long>(document.logicLinks().size()));
  appendRuntimeLogicMonitor(playMode, inspected.id, document, commands);
}

// ---- general object properties -------------------------------------------

enum class TriState : std::uint8_t { Off, On, Mixed };

[[nodiscard]] const char* triStateLabel(TriState state) noexcept {
  switch (state) {
    case TriState::On:
      return "On";
    case TriState::Off:
      return "Off";
    case TriState::Mixed:
      break;
  }
  return "Mixed";
}

// Folds a flag across the resolved selection. Reading the document here is
// fine; only mutation is forbidden in widget code.
[[nodiscard]] TriState foldFlag(const cr::CreativeDocument& document,
                                const std::vector<cr::CreativeObjectId>& ids,
                                bool useLocked) {
  bool all = true;
  bool any = false;
  for (const cr::CreativeObjectId id : ids) {
    const cr::CreativeObject* object = document.findObject(id);
    if (object == nullptr) {
      continue;
    }
    const bool value = useLocked ? object->locked : object->visible;
    all = all && value;
    any = any || value;
  }
  return all ? TriState::On : (any ? TriState::Mixed : TriState::Off);
}

void appendMultiInspector(const cr::CreativeDocument& document,
                          const CreativeDesktopSelectionResolution& resolved,
                          bool playModeActive,
                          CreativeDesktopCommandFrame& commands) {
  ImGui::Text("%llu objects selected",
              static_cast<unsigned long long>(resolved.objectIds.size()));
  ImGui::Text("Visibility: %s",
              triStateLabel(foldFlag(document, resolved.objectIds, false)));
  if (!playModeActive) {
    ImGui::SameLine();
    if (ImGui::SmallButton("Show##multi")) {
      commands.push(CreativeDesktopCommandId::SetObjectsVisible,
                    CreativeDesktopObjectFlagPayload{resolved.objectIds, true});
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Hide##multi")) {
      commands.push(CreativeDesktopCommandId::SetObjectsVisible,
                    CreativeDesktopObjectFlagPayload{resolved.objectIds, false});
    }
  }
  ImGui::Text("Lock: %s",
              triStateLabel(foldFlag(document, resolved.objectIds, true)));
  if (!playModeActive) {
    ImGui::SameLine();
    if (ImGui::SmallButton("Lock##multi")) {
      commands.push(CreativeDesktopCommandId::SetObjectsLocked,
                    CreativeDesktopObjectFlagPayload{resolved.objectIds, true});
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Unlock##multi")) {
      commands.push(CreativeDesktopCommandId::SetObjectsLocked,
                    CreativeDesktopObjectFlagPayload{resolved.objectIds, false});
    }
    ImGui::Separator();
    if (ImGui::Button("Duplicate##multi")) {
      commands.push(CreativeDesktopCommandId::DuplicateSelection);
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete##multi")) {
      commands.push(CreativeDesktopCommandId::DeleteSelection);
    }
  }
}

// Refreshes the draft when the inspected object changes, or after a revision
// while no field is being edited. An active edit is never overwritten.
void refreshInspectorDraft(CreativeDesktopInspectorDraft& draft,
                           const cr::CreativeDocument& document,
                           const cr::CreativeObject& object) {
  const bool keyChanged =
      draft.documentId != document.id() || draft.objectId != object.id;
  const bool revisionMoved = draft.sourceRevision != document.revision();
  if (draft.valid && !keyChanged && (!revisionMoved || draft.editing)) {
    return;
  }
  draft.documentId = document.id();
  draft.objectId = object.id;
  draft.sourceRevision = document.revision();
  draft.name = object.name;
  draft.position = {object.transform.position.x, object.transform.position.y,
                    object.transform.position.z};
  const cr::CreativeVec3 degrees =
      creativeDesktopRadiansToDegrees(object.transform.rotationEulerRadians);
  draft.rotationDegrees = {degrees.x, degrees.y, degrees.z};
  draft.scale = {object.transform.scale.x, object.transform.scale.y,
                 object.transform.scale.z};
  draft.movingPlatform = object.movingPlatform;
  draft.validation.clear();
  draft.valid = true;
}

void appendMovingPlatformFields(CreativeDesktopInspectorDraft& draft,
                                const cr::CreativeObject& object,
                                const CreativeMovingPlatformPreviewState& preview,
                                bool fieldsDisabled,
                                CreativeDesktopCommandFrame& commands) {
  if (object.kind != cr::CreativeObjectKind::MovingPlatform) {
    return;
  }
  const auto commit = [&]() {
    if (!cr::isValidCreativeMovingPlatformSettings(draft.movingPlatform)) {
      draft.validation = "Speed must be finite and between 0 and 100 m/s";
      return;
    }
    draft.validation.clear();
    commands.push(
        CreativeDesktopCommandId::SetMovingPlatformSettings,
        CreativeDesktopMovingPlatformPayload{object.id,
                                             draft.movingPlatform});
  };

  ImGui::SeparatorText("Motion");
  ImGui::BeginDisabled(fieldsDisabled);
  ImGui::InputDouble("Speed (m/s)",
                     &draft.movingPlatform.speedMetersPerSecond, 0.1, 1.0,
                     "%.2f");
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }

  const std::string traversalLabel =
      std::string(cr::toString(draft.movingPlatform.traversalMode));
  if (ImGui::BeginCombo("Traversal", traversalLabel.c_str())) {
    constexpr std::array modes{
        cr::CreativeMovingPlatformTraversalMode::PingPong,
        cr::CreativeMovingPlatformTraversalMode::Loop,
    };
    for (const cr::CreativeMovingPlatformTraversalMode mode : modes) {
      const bool selected = mode == draft.movingPlatform.traversalMode;
      if (ImGui::Selectable(std::string(cr::toString(mode)).c_str(), selected) &&
          !selected) {
        draft.movingPlatform.traversalMode = mode;
        commit();
      }
    }
    ImGui::EndCombo();
  }
  if (ImGui::Checkbox("Starts moving",
                      &draft.movingPlatform.startsActive)) {
    commit();
  }
  ImGui::EndDisabled();
  ImGui::TextDisabled("Waypoints: %llu",
                      static_cast<unsigned long long>(
                          object.pathPoints.size()));

  ImGui::SeparatorText("Route Preview");
  ImGui::Text("%s  |  %.0f%%",
              std::string(creativeMovingPlatformPreviewStatusLabel(preview))
                  .c_str(),
              preview.normalizedProgress * 100.0);
  const bool previewDisabled =
      fieldsDisabled || !preview.available || preview.objectId != object.id;
  ImGui::BeginDisabled(previewDisabled);
  if (ImGui::Button(preview.playing ? "Pause" : "Play")) {
    commands.push(
        CreativeDesktopCommandId::ToggleMovingPlatformPreview,
        CreativeDesktopMovingPlatformPreviewPayload{object.id,
                                                    preview.normalizedProgress});
  }
  ImGui::SameLine();
  if (ImGui::Button("Restart")) {
    commands.push(
        CreativeDesktopCommandId::RestartMovingPlatformPreview,
        CreativeDesktopMovingPlatformPreviewPayload{object.id, 0.0});
  }
  float progressPercent =
      static_cast<float>(preview.normalizedProgress * 100.0);
  if (ImGui::SliderFloat("Progress", &progressPercent, 0.0F, 100.0F,
                         "%.0f%%")) {
    commands.push(
        CreativeDesktopCommandId::SeekMovingPlatformPreview,
        CreativeDesktopMovingPlatformPreviewPayload{
            object.id, static_cast<double>(progressPercent) / 100.0});
  }
  ImGui::EndDisabled();
}

void appendTransformFields(CreativeDesktopInspectorDraft& draft,
                           cr::CreativeObjectId objectId,
                           bool fieldsDisabled,
                           CreativeDesktopCommandFrame& commands) {
  // Each group commits exactly one component mask, on Enter or when the field
  // is deactivated after an edit — never per keystroke or drag sample.
  const auto commit = [&](bool setPosition, bool setRotation, bool setScale) {
    const CreativeDesktopTransformDraft validated =
        validateCreativeDesktopTransformDraft(
            {draft.position[0], draft.position[1], draft.position[2]},
            {draft.rotationDegrees[0], draft.rotationDegrees[1],
             draft.rotationDegrees[2]},
            {draft.scale[0], draft.scale[1], draft.scale[2]});
    if (!validated.valid) {
      draft.validation = validated.message;  // retain the draft, do not dispatch
      return;
    }
    draft.validation.clear();
    commands.push(CreativeDesktopCommandId::SetObjectTransform,
                  CreativeDesktopTransformPayload{objectId,
                                                  validated.transform,
                                                  setPosition, setRotation,
                                                  setScale});
  };

  ImGui::SeparatorText("Transform");
  ImGui::BeginDisabled(fieldsDisabled);
  ImGui::InputScalarN("Position", ImGuiDataType_Double, draft.position.data(),
                      3);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit(true, false, false);
  }
  ImGui::InputScalarN("Rotation (deg)", ImGuiDataType_Double,
                      draft.rotationDegrees.data(), 3);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit(false, true, false);
  }
  ImGui::InputScalarN("Scale", ImGuiDataType_Double, draft.scale.data(), 3);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit(false, false, true);
  }
  ImGui::EndDisabled();
  if (!draft.validation.empty()) {
    ImGui::TextColored(ImVec4{1.0F, 0.34F, 0.30F, 1.0F}, "%s",
                       draft.validation.c_str());
  }
}

void appendSingleInspector(CreativeEditorDesktopUiState& desktopUi,
                           const cr::CreativeDocument& document,
                           const cr::CreativeObject& object,
                           const CreativeMovingPlatformPreviewState& preview,
                           bool playModeActive,
                           CreativeDesktopCommandFrame& commands) {
  CreativeDesktopInspectorDraft& draft = desktopUi.inspectorDraft;
  refreshInspectorDraft(draft, document, object);
  draft.editing = false;  // recomputed from this frame's active items.

  // A locked object can still be unlocked, inspected, and navigated; only its
  // name and transform are frozen. Play freezes every document edit.
  const bool fieldsDisabled = playModeActive || object.locked;

  ImGui::BeginDisabled(fieldsDisabled);
  if (inputTextStdString("Name", &draft.name,
                         ImGuiInputTextFlags_EnterReturnsTrue) &&
      !draft.name.empty()) {
    commands.push(CreativeDesktopCommandId::RenameObject,
                  CreativeDesktopRenamePayload{object.id, draft.name});
  }
  draft.editing = draft.editing || ImGui::IsItemActive();
  ImGui::EndDisabled();
  ImGui::TextDisabled("%s  |  id %llu",
                      std::string(cr::toString(object.kind)).c_str(),
                      static_cast<unsigned long long>(object.id));

  ImGui::BeginDisabled(playModeActive);
  bool visible = object.visible;
  if (ImGui::Checkbox("Visible", &visible)) {
    commands.push(CreativeDesktopCommandId::SetObjectsVisible,
                  CreativeDesktopObjectFlagPayload{{object.id}, visible});
  }
  ImGui::SameLine();
  bool locked = object.locked;
  if (ImGui::Checkbox("Locked", &locked)) {
    commands.push(CreativeDesktopCommandId::SetObjectsLocked,
                  CreativeDesktopObjectFlagPayload{{object.id}, locked});
  }
  ImGui::EndDisabled();

  appendTransformFields(draft, object.id, fieldsDisabled, commands);
  appendMovingPlatformFields(draft, object, preview, fieldsDisabled, commands);

  // Read-only metadata.
  if (!object.assetId.empty()) {
    ImGui::TextDisabled("Asset: %s", object.assetId.c_str());
  }
  if (object.parentId.has_value()) {
    ImGui::TextDisabled("Parent: %llu",
                        static_cast<unsigned long long>(*object.parentId));
  }
  if (!object.attachmentSocket.empty()) {
    ImGui::TextDisabled("Socket: %s", object.attachmentSocket.c_str());
  }
  for (const std::string& tag : object.tags) {
    ImGui::BulletText("%s", tag.c_str());
  }

  ImGui::BeginDisabled(playModeActive);
  if (ImGui::Button("Duplicate##single")) {
    commands.push(CreativeDesktopCommandId::DuplicateSelection);
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete##single")) {
    commands.push(CreativeDesktopCommandId::DeleteSelection);
  }
  ImGui::EndDisabled();

  // Escape cancels the active draft; the next frame restores document truth.
  if (draft.editing && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
    draft.valid = false;
  }
}

}  // namespace

void queueCreativeDesktopObjectNavigation(CreativeDesktopCommandFrame& commands,
                                          cr::CreativeObjectId objectId,
                                          bool playModeActive) {
  if (objectId == cr::kInvalidObjectId) {
    return;
  }
  // Play navigates by selection only; editing focus moves the camera.
  commands.push(playModeActive ? CreativeDesktopCommandId::SelectObjects
                               : CreativeDesktopCommandId::FocusObject,
                CreativeDesktopSelectPayload{{objectId}, objectId});
}

ImVec4 creativeDesktopLogicDiagnosticColor(
    cr::CreativeLogicDiagnosticSeverity severity) noexcept {
  return severity == cr::CreativeLogicDiagnosticSeverity::Error
             ? ImVec4{1.0F, 0.34F, 0.30F, 1.0F}
             : ImVec4{1.0F, 0.82F, 0.25F, 1.0F};
}

void buildCreativeEditorDesktopInspectorPanel(
    CreativeEditorDesktopUiState& desktopUi,
    const CreativeEditorState& editor,
    const cr::CreativeAppState& appState,
    const CreativeEditorPlayMode* playMode,
    CreativeDesktopCommandFrame& commands) {
  const cr::CreativeDocument& document = appState.facade.document();
  const bool playModeActive =
      playMode != nullptr && creativeEditorPlayModeActive(*playMode);

  // Valid ids are resolved from document truth every frame; a cached id is
  // never trusted after a revision.
  const CreativeDesktopLiveSelection live = creativeDesktopLiveSelection(appState);
  const CreativeDesktopSelectionResolution resolved =
      resolveCreativeDesktopSelection(document, live.objectIds,
                                      live.primaryObjectId);

  if (resolved.objectIds.empty()) {
    ImGui::TextUnformatted("No object selected");
    return;
  }
  if (resolved.objectIds.size() > 1U) {
    appendMultiInspector(document, resolved, playModeActive, commands);
    return;
  }

  const cr::CreativeObject* object =
      document.findObject(resolved.primaryObjectId);
  if (object == nullptr) {
    ImGui::TextUnformatted("No object selected");
    return;
  }
  appendSingleInspector(desktopUi, document, *object,
                        editor.movingPlatformPreview, playModeActive,
                        commands);
  appendLogicSection(desktopUi, editor, document, *object, playMode,
                     playModeActive, commands);
}

}  // namespace iggy3d_creative_app
