#include "EditorDesktopWidgets.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "EditorDesktopModel.hpp"
#include "EditorMeasurement.hpp"
#include "EditorWorldLayout.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/play/NpcSpawn.hpp"
#include "app/iggy3d/creative/play/PlayerSpawn.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

// UI-4A Inspector. General object properties (zero/single/multi) sit above the
// existing logic-link authoring, diagnostics, and Play-mode runtime monitor,
// which moved here from EditorDesktopPanels.cpp with their behavior unchanged.
// Every edit leaves as a typed command; nothing here mutates the document.

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

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

}  // namespace

bool creativeDesktopInputTextStdString(const char* label, std::string* str,
                                       ImGuiInputTextFlags flags) {
  return ImGui::InputText(label, str->data(), str->capacity() + 1U,
                          flags | ImGuiInputTextFlags_CallbackResize,
                          inputTextResizeCallback, str);
}

namespace {

// ---- shared small helpers -------------------------------------------------

void appendHoverTooltip(const char* text) {
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(text);
    ImGui::EndTooltip();
  }
}

void appendMeasurementInspector(
    const cr::CreativeMeasurementState& measurement,
    const cr::CreativeMeasurementAnnotationStore& annotations,
    bool playModeActive, CreativeDesktopCommandFrame& commands) {
  const cr::CreativeMeasurementReadout readout =
      cr::buildCreativeMeasurementReadout(measurement);
  if (!readout.visible && annotations.annotations.empty()) {
    return;
  }

  ImGui::SeparatorText("Measurement");
  if (readout.visible) {
    ImGui::Text("%s  |  Snap %s", cr::toString(readout.mode).data(),
                cr::toString(readout.snapKind).data());
    ImGui::TextDisabled("%llu points  |  %llu segments  |  %s",
                        static_cast<unsigned long long>(readout.pointCount),
                        static_cast<unsigned long long>(readout.segmentCount),
                        readout.completed ? "Complete" : "Active");
    if (!readout.valid) {
      ImGui::TextColored(ImVec4{1.0F, 0.72F, 0.22F, 1.0F}, "%s",
                         cr::toString(measurement.plan.status).data());
    } else {
      char primary[96]{};
      std::snprintf(primary, sizeof(primary), "%.6f %s", readout.primaryValue,
                    readout.primaryUnit.data());
      ImGui::SetNextItemWidth(-46.0F);
      ImGui::InputText(readout.primaryLabel.data(), primary, sizeof(primary),
                       ImGuiInputTextFlags_ReadOnly |
                           ImGuiInputTextFlags_AutoSelectAll);
      ImGui::SameLine();
      if (ImGui::SmallButton("Copy##measurement_primary")) {
        ImGui::SetClipboardText(primary);
      }
      if (readout.hasSecondaryValue) {
        char secondary[96]{};
        std::snprintf(secondary, sizeof(secondary), "%.6f %s",
                      readout.secondaryValue, readout.secondaryUnit.data());
        ImGui::SetNextItemWidth(-46.0F);
        ImGui::InputText(readout.secondaryLabel.data(), secondary,
                         sizeof(secondary),
                         ImGuiInputTextFlags_ReadOnly |
                             ImGuiInputTextFlags_AutoSelectAll);
        ImGui::SameLine();
        if (ImGui::SmallButton("Copy##measurement_secondary")) {
          ImGui::SetClipboardText(secondary);
        }
      }
    }

    const bool canSave =
        !playModeActive && readout.valid && readout.completed &&
        annotations.annotations.size() < cr::kCreativeMeasurementAnnotationCapacity;
    ImGui::BeginDisabled(!canSave);
    if (ImGui::Button("Save annotation")) {
      const std::string name =
          std::string{cr::toString(readout.mode)} + " " +
          std::to_string(annotations.nextAnnotationId);
      commands.push(
          CreativeDesktopCommandId::SaveMeasurementAnnotation,
          CreativeDesktopMeasurementAnnotationPayload{
              cr::kInvalidCreativeMeasurementAnnotationId, name});
    }
    ImGui::EndDisabled();
    if (readout.completed && annotations.annotations.size() >=
                                 cr::kCreativeMeasurementAnnotationCapacity) {
      ImGui::TextDisabled("Annotation capacity reached");
    }
  }

  if (!annotations.annotations.empty()) {
    ImGui::SeparatorText("Saved measurements");
  }
  for (const cr::CreativeMeasurementAnnotation& annotation :
       annotations.annotations) {
    ImGui::PushID(static_cast<int>(annotation.id));
    ImGui::TextUnformatted(annotation.name.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("%s  |  %llu points",
                        cr::toString(annotation.mode).data(),
                        static_cast<unsigned long long>(annotation.pointCount));
    if (!playModeActive) {
      ImGui::SameLine();
      if (ImGui::SmallButton("Remove")) {
        commands.push(
            CreativeDesktopCommandId::RemoveMeasurementAnnotation,
            CreativeDesktopMeasurementAnnotationPayload{annotation.id, {}});
      }
    }
    ImGui::PopID();
  }
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
      if (target != nullptr) {
        for (const cr::CreativeLogicLinkAction action :
             cr::creativeLogicTargetActionsForObject(target->kind)) {
          const bool selected = action == link.action;
          if (ImGui::Selectable(std::string(cr::toString(action)).c_str(),
                                selected) &&
              !selected) {
            queueSetLogicLink(commands, link.sourceObjectId,
                              link.targetObjectId, action);
          }
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
    for (const cr::CreativeLogicLinkAction action :
         cr::creativeLogicTargetActionsForObject(target.kind)) {
      if (ImGui::Selectable(std::string(cr::toString(action)).c_str())) {
        queueSetLogicLink(commands, source->id, target.id, action);
      }
    }
    ImGui::EndCombo();
  }
}

// the general single-object properties.
void appendLogicSection(CreativeEditorDesktopUiState& desktopUi,
                        const CreativeEditorState& editor,
                        const cr::CreativeDocument& document,
                        const cr::CreativeObject& inspected,
                        bool playModeActive,
                        CreativeDesktopCommandFrame& commands) {
  if (cr::creativeObjectCanSourceLogicLink(inspected.kind)) {
    const cr::CreativeLogicSourceEvent sourceEvent =
        cr::creativeLogicSourceEventForObject(inspected.kind);
    ImGui::Text("Source event: %s",
                std::string(
                    cr::creativeLogicSourceEventLabel(sourceEvent))
                    .c_str());
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
  draft.playerSpawn = object.playerSpawn;
  draft.npcSpawn = object.npcSpawn;
  draft.lootPoint = object.lootPoint;
  draft.exitPoint = object.exitPoint;
  draft.movingPlatformWaypointIndex = 0U;
  draft.movingPlatformWaypointDwellSeconds =
      object.pathPoints.empty() ? 0.0 : object.pathPoints.front().dwellSeconds;
  draft.validation.clear();
  draft.valid = true;
}

void appendPlayerSpawnFields(CreativeDesktopInspectorDraft& draft,
                             const cr::CreativeObject& object,
                             bool fieldsDisabled,
                             CreativeDesktopCommandFrame& commands) {
  if (object.kind != cr::CreativeObjectKind::SpawnPoint) {
    return;
  }
  const auto commit = [&]() {
    if (!cr::isValidCreativePlayerSpawnSettings(draft.playerSpawn)) {
      draft.validation =
          "Profile/group IDs must be non-empty identifiers; clearance must be 0.30-4.00 m";
      return;
    }
    draft.validation.clear();
    commands.push(
        CreativeDesktopCommandId::SetPlayerSpawnSettings,
        CreativeDesktopPlayerSpawnPayload{object.id, draft.playerSpawn});
  };

  ImGui::SeparatorText("Player Spawn");
  ImGui::BeginDisabled(fieldsDisabled);
  const bool profileCommitted = creativeDesktopInputTextStdString(
      "Player profile", &draft.playerSpawn.playerProfileId,
      ImGuiInputTextFlags_EnterReturnsTrue);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (profileCommitted || ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  const bool groupCommitted = creativeDesktopInputTextStdString(
      "Spawn group", &draft.playerSpawn.spawnGroup,
      ImGuiInputTextFlags_EnterReturnsTrue);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (groupCommitted || ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  ImGui::InputDouble("Clearance radius (m)",
                     &draft.playerSpawn.validationRadiusMeters, 0.05, 0.25,
                     "%.2f");
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  constexpr std::uint16_t kPriorityStep = 1U;
  constexpr std::uint16_t kPriorityFastStep = 10U;
  if (ImGui::InputScalar("Fallback priority", ImGuiDataType_U16,
                         &draft.playerSpawn.fallbackPriority, &kPriorityStep,
                         &kPriorityFastStep, "%u")) {
    commit();
  }
  ImGui::EndDisabled();

  ImGui::TextDisabled("Lower priority wins; ties use object id");
  ImGui::TextDisabled("Facing follows Rotation Y");
  if (cr::isValidCreativePlayerProfileId(
          draft.playerSpawn.playerProfileId) &&
      !cr::isSupportedCreativePlayerProfileId(
          draft.playerSpawn.playerProfileId)) {
    ImGui::TextColored(ImVec4{1.0F, 0.72F, 0.22F, 1.0F},
                       "Profile is not available in the current runtime");
  }
}

void appendNpcSpawnFields(CreativeDesktopInspectorDraft& draft,
                          const cr::CreativeDocument& document,
                          const cr::CreativeObject& object,
                          bool fieldsDisabled,
                          CreativeDesktopCommandFrame& commands) {
  const bool npcActor = object.kind == cr::CreativeObjectKind::NpcSpawn ||
                        object.kind == cr::CreativeObjectKind::EnemySpawn;
  if (!npcActor) {
    return;
  }
  const auto commit = [&]() {
    if (!cr::isValidCreativeNpcSpawnSettings(draft.npcSpawn)) {
      draft.validation =
          "Profile must be a lowercase identifier; health is 0-32767; alert is 0-1";
      return;
    }
    draft.validation.clear();
    commands.push(
        CreativeDesktopCommandId::SetNpcSpawnSettings,
        CreativeDesktopNpcSpawnPayload{object.id, draft.npcSpawn});
  };

  ImGui::SeparatorText("NPC Spawn");
  ImGui::BeginDisabled(fieldsDisabled);
  const bool profileCommitted = creativeDesktopInputTextStdString(
      "Behavior profile", &draft.npcSpawn.behaviorProfileId,
      ImGuiInputTextFlags_EnterReturnsTrue);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (profileCommitted || ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }

  constexpr std::array teams{
      cr::CreativeNpcTeam::ActorDefault,
      cr::CreativeNpcTeam::PlayerAllied,
      cr::CreativeNpcTeam::Hostile,
  };
  const std::string teamLabel =
      std::string(cr::toString(draft.npcSpawn.team));
  if (ImGui::BeginCombo("Team", teamLabel.c_str())) {
    for (const cr::CreativeNpcTeam team : teams) {
      const bool selected = team == draft.npcSpawn.team;
      const std::string label = std::string(cr::toString(team));
      if (ImGui::Selectable(label.c_str(), selected) && !selected) {
        draft.npcSpawn.team = team;
        commit();
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  constexpr std::uint16_t kHealthStep = 1U;
  constexpr std::uint16_t kHealthFastStep = 10U;
  if (ImGui::InputScalar("Health override", ImGuiDataType_U16,
                         &draft.npcSpawn.hitPoints, &kHealthStep,
                         &kHealthFastStep, "%u")) {
    commit();
  }
  ImGui::TextDisabled("0 uses the actor-kind default");

  constexpr double kMinimumAlert = 0.0;
  constexpr double kMaximumAlert = 1.0;
  ImGui::SliderScalar("Initial alert", ImGuiDataType_Double,
                      &draft.npcSpawn.initialAlertLevel, &kMinimumAlert,
                      &kMaximumAlert, "%.2f");
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }

  constexpr std::array policies{
      cr::CreativeNpcSpawnPolicy::AtPlayStart,
      cr::CreativeNpcSpawnPolicy::Disabled,
  };
  const std::string policyLabel =
      std::string(cr::toString(draft.npcSpawn.spawnPolicy));
  if (ImGui::BeginCombo("Spawn policy", policyLabel.c_str())) {
    for (const cr::CreativeNpcSpawnPolicy policy : policies) {
      const bool selected = policy == draft.npcSpawn.spawnPolicy;
      const std::string label = std::string(cr::toString(policy));
      if (ImGui::Selectable(label.c_str(), selected) && !selected) {
        draft.npcSpawn.spawnPolicy = policy;
        commit();
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::EndDisabled();

  ImGui::TextDisabled("Facing follows Rotation Y");
  const cr::CreativeObject* patrol =
      object.parentId.has_value()
          ? document.findObject(object.parentId.value())
          : nullptr;
  if (patrol != nullptr &&
      patrol->kind == cr::CreativeObjectKind::PatrolRoute) {
    ImGui::TextDisabled("Patrol: %s", patrol->name.c_str());
  } else {
    ImGui::TextDisabled("Patrol: stationary");
  }
  if (cr::isValidCreativeNpcBehaviorProfileId(
          draft.npcSpawn.behaviorProfileId) &&
      !cr::isSupportedCreativeNpcBehaviorProfileId(
          draft.npcSpawn.behaviorProfileId)) {
    ImGui::TextColored(ImVec4{1.0F, 0.72F, 0.22F, 1.0F},
                       "Profile is not available in the current runtime");
  }
}

void appendLootPointFields(CreativeDesktopInspectorDraft& draft,
                           const cr::CreativeObject& object,
                           bool fieldsDisabled,
                           CreativeDesktopCommandFrame& commands) {
  if (object.kind != cr::CreativeObjectKind::LootPoint) {
    return;
  }
  const auto commit = [&]() {
    if (!cr::isValidCreativeLootPointSettings(draft.lootPoint)) {
      draft.validation =
          "Item ID must be empty or a valid identifier; count must be 1-65535";
      return;
    }
    draft.validation.clear();
    commands.push(
        CreativeDesktopCommandId::SetLootPointSettings,
        CreativeDesktopLootPointPayload{object.id, draft.lootPoint});
  };

  ImGui::SeparatorText("Loot");
  ImGui::BeginDisabled(fieldsDisabled);
  const bool itemCommitted = creativeDesktopInputTextStdString(
      "Item ID", &draft.lootPoint.itemId,
      ImGuiInputTextFlags_EnterReturnsTrue);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (itemCommitted || ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  constexpr std::uint32_t kCountStep = 1U;
  constexpr std::uint32_t kCountFastStep = 10U;
  ImGui::InputScalar("Count", ImGuiDataType_U32,
                     &draft.lootPoint.itemCount, &kCountStep,
                     &kCountFastStep, "%u");
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  if (ImGui::Checkbox("Remove after collection",
                      &draft.lootPoint.deactivateOnCollect)) {
    commit();
  }
  ImGui::EndDisabled();

  const std::string effectiveId =
      draft.lootPoint.itemId.empty()
          ? cr::makeCreativeAutomaticLootItemId(object.id)
          : draft.lootPoint.itemId;
  ImGui::TextDisabled("Effective item: %s", effectiveId.c_str());
}

void appendExitPointFields(CreativeDesktopInspectorDraft& draft,
                           const cr::CreativeDocument& document,
                           const cr::CreativeObject& object,
                           bool fieldsDisabled,
                           CreativeDesktopCommandFrame& commands) {
  if (object.kind != cr::CreativeObjectKind::ExitPoint) {
    return;
  }
  const auto commit = [&]() {
    if (!cr::isValidCreativeExitPointSettings(draft.exitPoint)) {
      draft.validation =
          "Required item and count must both be empty/zero or both be set";
      return;
    }
    draft.validation.clear();
    commands.push(
        CreativeDesktopCommandId::SetExitPointSettings,
        CreativeDesktopExitPointPayload{object.id, draft.exitPoint});
  };

  ImGui::SeparatorText("Exit Objective");
  ImGui::BeginDisabled(fieldsDisabled);
  const char* sourceLabel = draft.exitPoint.requiredItemId.empty()
                                ? "No item required"
                                : draft.exitPoint.requiredItemId.c_str();
  if (ImGui::BeginCombo("Loot requirement", sourceLabel)) {
    const bool noRequirement = draft.exitPoint.requiredItemId.empty();
    if (ImGui::Selectable("No item required", noRequirement) &&
        !noRequirement) {
      draft.exitPoint.requiredItemId.clear();
      draft.exitPoint.requiredItemCount = 0U;
      commit();
    }
    for (const cr::CreativeObject& candidate : document.objects()) {
      if (candidate.kind != cr::CreativeObjectKind::LootPoint ||
          !cr::creativeObjectEffectivelyVisible(document, candidate.id)) {
        continue;
      }
      const std::string itemId =
          cr::effectiveCreativeLootPointItemId(candidate);
      const bool selected = itemId == draft.exitPoint.requiredItemId;
      const std::string label =
          candidate.name.empty() ? itemId
                                 : candidate.name + " (" + itemId + ")";
      if (ImGui::Selectable(label.c_str(), selected) && !selected) {
        draft.exitPoint.requiredItemId = itemId;
        draft.exitPoint.requiredItemCount =
            std::max<std::uint32_t>(draft.exitPoint.requiredItemCount, 1U);
        commit();
      }
    }
    ImGui::EndCombo();
  }
  const bool itemCommitted = creativeDesktopInputTextStdString(
      "Required item ID", &draft.exitPoint.requiredItemId,
      ImGuiInputTextFlags_EnterReturnsTrue);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (itemCommitted || ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  constexpr std::uint32_t kCountStep = 1U;
  constexpr std::uint32_t kCountFastStep = 10U;
  ImGui::InputScalar("Required count", ImGuiDataType_U32,
                     &draft.exitPoint.requiredItemCount, &kCountStep,
                     &kCountFastStep, "%u");
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  ImGui::EndDisabled();
  const std::string objectiveId =
      cr::makeCreativeExitObjectiveId(object.id);
  ImGui::TextDisabled("Objective: %s", objectiveId.c_str());
}

void appendMovingPlatformFields(CreativeDesktopInspectorDraft& draft,
                                const cr::CreativeObject& object,
                                const CreativeMovingPlatformPreviewState& preview,
                                const CreativeMovingPlatformPathEditState& pathEdit,
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

  if (object.pathPoints.empty()) {
    return;
  }

  std::size_t waypointIndex = 0U;
  if (pathEdit.available && pathEdit.pointSelected &&
      pathEdit.objectId == object.id &&
      pathEdit.selectedPointIndex < object.pathPoints.size()) {
    waypointIndex = pathEdit.selectedPointIndex;
  }
  if (!draft.editing &&
      draft.movingPlatformWaypointIndex != waypointIndex) {
    draft.movingPlatformWaypointIndex = waypointIndex;
    draft.movingPlatformWaypointDwellSeconds =
        object.pathPoints[waypointIndex].dwellSeconds;
  }

  const std::string waypointLabel =
      "Point " + std::to_string(waypointIndex + 1U);
  ImGui::BeginDisabled(fieldsDisabled);
  if (ImGui::BeginCombo("Waypoint", waypointLabel.c_str())) {
    for (std::size_t index = 0U; index < object.pathPoints.size(); ++index) {
      const std::string label = "Point " + std::to_string(index + 1U);
      const bool selected = index == waypointIndex;
      if (ImGui::Selectable(label.c_str(), selected) && !selected) {
        draft.movingPlatformWaypointIndex = index;
        draft.movingPlatformWaypointDwellSeconds =
            object.pathPoints[index].dwellSeconds;
        commands.push(
            CreativeDesktopCommandId::SelectMovingPlatformWaypoint,
            CreativeDesktopMovingPlatformWaypointPayload{object.id, index,
                                                         0.0});
      }
    }
    ImGui::EndCombo();
  }
  ImGui::InputDouble("Wait (s)",
                     &draft.movingPlatformWaypointDwellSeconds, 0.25, 1.0,
                     "%.2f");
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    const double dwell = draft.movingPlatformWaypointDwellSeconds;
    if (!std::isfinite(dwell) || dwell < 0.0 ||
        dwell > cr::kCreativePathPointMaximumDwellSeconds) {
      draft.validation = "Wait must be between 0 and 60 seconds";
    } else {
      draft.validation.clear();
      commands.push(
          CreativeDesktopCommandId::SetMovingPlatformWaypointDwell,
          CreativeDesktopMovingPlatformWaypointPayload{
              object.id, draft.movingPlatformWaypointIndex, dwell});
    }
  }
  ImGui::EndDisabled();

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

void appendGroupPivotFields(CreativeDesktopInspectorDraft& draft,
                            cr::CreativeObjectId groupObjectId,
                            bool fieldsDisabled,
                            CreativeDesktopCommandFrame& commands) {
  ImGui::SeparatorText("Group Pivot");
  ImGui::TextDisabled("Moves the manipulation pivot, not the members");
  ImGui::BeginDisabled(fieldsDisabled);
  ImGui::InputScalarN("Pivot", ImGuiDataType_Double, draft.position.data(), 3);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    const cr::CreativeVec3 pivot{draft.position[0], draft.position[1],
                                 draft.position[2]};
    if (cr::isFiniteCreativeVec3(pivot)) {
      draft.validation.clear();
      commands.push(CreativeDesktopCommandId::SetGroupPivot,
                    CreativeDesktopGroupPivotPayload{groupObjectId, pivot});
    } else {
      draft.validation = "group pivot must be finite";
    }
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
                           CreativeEditorWorldLayoutState& worldLayout,
                           CreativeDesktopGeneratedSourceScopeCache&
                               generatedSourceScopeCache,
                           const CreativeMovingPlatformPreviewState& preview,
                           const CreativeMovingPlatformPathEditState& pathEdit,
                           bool playModeActive,
                           const CreativeEditorUiInputFrame& input,
                           CreativeDesktopCommandFrame& commands) {
  CreativeDesktopInspectorDraft& draft = desktopUi.inspectorDraft;
  refreshInspectorDraft(draft, document, object);
  draft.editing = false;  // recomputed from this frame's active items.

  const cr::CreativeSemanticSelectionResolution semanticSelection =
      cr::resolveCreativeSemanticSelection(document, object.id,
                                           &worldLayout.source);
  const cr::CreativeWorldLayoutObjectProvenance& provenance =
      semanticSelection.worldLayoutSource;
  const bool sourceSupportsAdoption =
      creativeDesktopGeneratedSourceSupportsAdoption(provenance);
  const bool sourceOwnedOnly = provenance.owned && !sourceSupportsAdoption;
  const bool generatedSettingsSource = provenance.owned;
  if (!generatedSettingsSource &&
      (worldLayout.generatedBuildingDraft.active ||
       worldLayout.generatedLevelSettingsDraft.active ||
       worldLayout.roomSettingsDraft.active ||
       worldLayout.verticalConnectorSettingsDraft.active ||
       worldLayout.wallSettingsDraft.active ||
       worldLayout.openingSettingsDraft.active)) {
    worldLayout.generatedBuildingDraft = {};
    worldLayout.generatedLevelSettingsDraft = {};
    worldLayout.roomSettingsDraft = {};
    worldLayout.verticalConnectorSettingsDraft = {};
    worldLayout.wallSettingsDraft = {};
    worldLayout.openingSettingsDraft = {};
    if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }

  // A locked object can still be unlocked, inspected, and navigated; only its
  // name and transform are frozen. Play freezes every document edit.
  const cr::CreativeObjectHierarchyState hierarchyState =
      cr::resolveCreativeObjectHierarchyState(document, object.id);
  const bool effectivelyLocked =
      !hierarchyState.resolved || hierarchyState.effectivelyLocked;
  const bool fieldsDisabled =
      playModeActive || effectivelyLocked || sourceOwnedOnly;

  ImGui::BeginDisabled(fieldsDisabled);
  if (creativeDesktopInputTextStdString(
          "Name", &draft.name, ImGuiInputTextFlags_EnterReturnsTrue) &&
      !draft.name.empty()) {
    commands.push(CreativeDesktopCommandId::RenameObject,
                  CreativeDesktopRenamePayload{object.id, draft.name});
  }
  draft.editing = draft.editing || ImGui::IsItemActive();
  ImGui::EndDisabled();
  ImGui::TextDisabled("%s  |  id %llu",
                      std::string(cr::toString(object.kind)).c_str(),
                      static_cast<unsigned long long>(object.id));

  ImGui::BeginDisabled(playModeActive || sourceOwnedOnly);
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

  if (hierarchyState.hiddenByObjectId != cr::kInvalidObjectId &&
      hierarchyState.hiddenByObjectId != object.id) {
    ImGui::TextDisabled("Hidden by ancestor %llu",
                        static_cast<unsigned long long>(
                            hierarchyState.hiddenByObjectId));
  }
  if (hierarchyState.lockedByObjectId != cr::kInvalidObjectId &&
      hierarchyState.lockedByObjectId != object.id) {
    ImGui::TextDisabled("Locked by ancestor %llu",
                        static_cast<unsigned long long>(
                            hierarchyState.lockedByObjectId));
  }

  if (object.kind == cr::CreativeObjectKind::Group) {
    appendGroupPivotFields(draft, object.id, fieldsDisabled, commands);
  } else if (cr::creativeObjectIsHierarchyContainer(object.kind)) {
    ImGui::SeparatorText("Transform");
    ImGui::TextDisabled("Use Transform Selection to move the complete instance");
  } else {
    appendTransformFields(draft, object.id, fieldsDisabled, commands);
  }
  appendPlayerSpawnFields(draft, object, fieldsDisabled, commands);
  appendNpcSpawnFields(draft, document, object, fieldsDisabled, commands);
  appendLootPointFields(draft, object, fieldsDisabled, commands);
  appendExitPointFields(draft, document, object, fieldsDisabled, commands);
  appendMovingPlatformFields(draft, object, preview, pathEdit, fieldsDisabled,
                             commands);

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

  if (provenance.owned) {
    ImGui::SeparatorText("World Layout");
    const bool sourceSynchronized =
        worldLayout.generatedRevision == worldLayout.revision;
    ImGui::BeginDisabled(playModeActive || !sourceSynchronized ||
                         !sourceSupportsAdoption);
    if (ImGui::Button("Adopt 3D Edit")) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutAdoptObjectSource,
          CreativeDesktopWorldLayoutObjectSourcePayload{object.id});
    }
    ImGui::EndDisabled();
    if (!sourceSynchronized) {
      ImGui::TextDisabled("Generate pending 2D edits before adoption");
    } else if (sourceOwnedOnly) {
      ImGui::TextDisabled("Source-owned geometry; raw object edits are locked");
    }
    appendCreativeDesktopGeneratedSourceSettings(
        worldLayout, generatedSourceScopeCache, document, object.id, provenance,
        playModeActive || !sourceSynchronized, commands);
  }

  ImGui::BeginDisabled(playModeActive || sourceOwnedOnly);
  if (ImGui::Button("Duplicate##single")) {
    commands.push(CreativeDesktopCommandId::DuplicateSelection);
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete##single")) {
    commands.push(CreativeDesktopCommandId::DeleteSelection);
  }
  ImGui::EndDisabled();

  // Escape cancels the active draft; the next frame restores document truth.
  if (draft.editing && input.cancelPressed) {
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
    CreativeEditorState& editor,
    const cr::CreativeAppState& appState,
    const CreativeEditorUiInputFrame& input,
    CreativeDesktopCommandFrame& commands) {
  const cr::CreativeDocument& document = appState.facade.document();
  // The playtest runs out of process (i3dp); no in-process play state.
  constexpr bool playModeActive = false;

  // Valid ids are resolved from document truth every frame; a cached id is
  // never trusted after a revision.
  const CreativeDesktopLiveSelection live = creativeDesktopLiveSelection(appState);
  const CreativeDesktopSelectionResolution resolved =
      resolveCreativeDesktopSelection(document, live.objectIds,
                                      live.primaryObjectId);

  appendMeasurementInspector(
      appState.facade.measurementState(), document.measurementAnnotationStore(),
      playModeActive, commands);
  appendCreativeDesktopActiveTransformInspector(editor, appState,
                                                playModeActive);
  appendCreativeDesktopVolumeInspector(editor, document, resolved.objectIds);

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
  appendSingleInspector(desktopUi, document, *object, editor.worldLayout,
                        editor.generatedSourceScopeCache,
                        editor.movingPlatformPreview,
                        editor.interaction.movingPlatformPathEdit,
                        playModeActive, input,
                        commands);
  appendLogicSection(desktopUi, editor, document, *object, playModeActive, commands);
}

}  // namespace iggy3d_creative_app
