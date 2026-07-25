#include "EditorDesktopInspectorInternal.hpp"

#include <cstdio>
#include <string>

#include "EditorMeasurement.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void queueLogicSource(
    CreativeDesktopCommandFrame& commands,
    cr::CreativeObjectId sourceObjectId) {
  commands.enqueue(
      CreativeDesktopCommandId::SetLogicSource,
      CreativeDesktopLogicLinkPayload{sourceObjectId});
}

void queueSetLogicLink(
    CreativeDesktopCommandFrame& commands,
    cr::CreativeObjectId sourceObjectId,
    cr::CreativeObjectId targetObjectId,
    cr::CreativeLogicLinkAction action) {
  commands.enqueue(
      CreativeDesktopCommandId::SetLogicLink,
      CreativeDesktopLogicLinkPayload{
          sourceObjectId, targetObjectId, action});
}

void queueRemoveLogicLink(
    CreativeDesktopCommandFrame& commands,
    cr::CreativeObjectId sourceObjectId,
    cr::CreativeObjectId targetObjectId) {
  commands.enqueue(
      CreativeDesktopCommandId::RemoveLogicLink,
      CreativeDesktopLogicLinkPayload{
          sourceObjectId, targetObjectId});
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

void appendLogicLinkRow(
    const cr::CreativeDocument& document,
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
      std::string(outgoing ? "to " : "from ") +
      std::string(otherName) + "  [" +
      std::string(cr::toString(link.action)) + "]";
  ImGui::PushID(static_cast<int>(otherId));
  if (ImGui::Selectable(label.c_str())) {
    queueCreativeDesktopObjectNavigation(
        commands, otherId, playModeActive);
  }
  if (!playModeActive) {
    const cr::CreativeObject* target =
        document.findObject(link.targetObjectId);
    ImGui::SetNextItemWidth(132.0F);
    if (ImGui::BeginCombo(
            "##logic_action",
            std::string(cr::toString(link.action)).c_str())) {
      if (target != nullptr) {
        for (const cr::CreativeLogicLinkAction action :
             cr::creativeLogicTargetActionsForObject(target->kind)) {
          const bool selected = action == link.action;
          if (ImGui::Selectable(
                  std::string(cr::toString(action)).c_str(), selected) &&
              !selected) {
            queueSetLogicLink(
                commands, link.sourceObjectId, link.targetObjectId,
                action);
          }
        }
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("X")) {
      queueRemoveLogicLink(
          commands, link.sourceObjectId, link.targetObjectId);
    }
    appendCreativeDesktopInspectorHoverTooltip("Remove logic link");
  }
  ImGui::PopID();
}

void appendNewLogicLinkControl(
    const cr::CreativeDocument& document,
    const CreativeEditorLogicLinkState& logicLinks,
    const cr::CreativeObject& target,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands) {
  if (playModeActive ||
      !cr::creativeObjectCanTargetLogicLink(target.kind) ||
      logicLinks.sourceObjectId == cr::kInvalidObjectId) {
    return;
  }
  const cr::CreativeObject* source =
      document.findObject(logicLinks.sourceObjectId);
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
      if (ImGui::Selectable(
              std::string(cr::toString(action)).c_str())) {
        queueSetLogicLink(commands, source->id, target.id, action);
      }
    }
    ImGui::EndCombo();
  }
}

}  // namespace

void appendCreativeDesktopMeasurementInspector(
    const cr::CreativeMeasurementState& measurement,
    const cr::CreativeMeasurementAnnotationStore& annotations,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands) {
  const cr::CreativeMeasurementReadout readout =
      cr::buildCreativeMeasurementReadout(measurement);
  if (!readout.visible && annotations.annotations.empty()) {
    return;
  }

  ImGui::SeparatorText("Measurement");
  if (readout.visible) {
    ImGui::Text(
        "%s  |  Snap %s", cr::toString(readout.mode).data(),
        cr::toString(readout.snapKind).data());
    ImGui::TextDisabled(
        "%llu points  |  %llu segments  |  %s",
        static_cast<unsigned long long>(readout.pointCount),
        static_cast<unsigned long long>(readout.segmentCount),
        readout.completed ? "Complete" : "Active");
    if (!readout.valid) {
      ImGui::TextColored(
          ImVec4{1.0F, 0.72F, 0.22F, 1.0F}, "%s",
          cr::toString(measurement.plan.status).data());
    } else {
      char primary[96]{};
      std::snprintf(
          primary, sizeof(primary), "%.6f %s", readout.primaryValue,
          readout.primaryUnit.data());
      ImGui::SetNextItemWidth(-46.0F);
      ImGui::InputText(
          readout.primaryLabel.data(), primary, sizeof(primary),
          ImGuiInputTextFlags_ReadOnly |
              ImGuiInputTextFlags_AutoSelectAll);
      ImGui::SameLine();
      if (ImGui::SmallButton("Copy##measurement_primary")) {
        ImGui::SetClipboardText(primary);
      }
      if (readout.hasSecondaryValue) {
        char secondary[96]{};
        std::snprintf(
            secondary, sizeof(secondary), "%.6f %s",
            readout.secondaryValue, readout.secondaryUnit.data());
        ImGui::SetNextItemWidth(-46.0F);
        ImGui::InputText(
            readout.secondaryLabel.data(), secondary,
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
        annotations.annotations.size() <
            cr::kCreativeMeasurementAnnotationCapacity;
    ImGui::BeginDisabled(!canSave);
    if (ImGui::Button("Save annotation")) {
      const std::string name =
          std::string{cr::toString(readout.mode)} + " " +
          std::to_string(annotations.nextAnnotationId);
      commands.enqueue(
          CreativeDesktopCommandId::SaveMeasurementAnnotation,
          CreativeDesktopMeasurementAnnotationPayload{
              cr::kInvalidCreativeMeasurementAnnotationId, name});
    }
    ImGui::EndDisabled();
    if (readout.completed &&
        annotations.annotations.size() >=
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
    ImGui::TextDisabled(
        "%s  |  %llu points", cr::toString(annotation.mode).data(),
        static_cast<unsigned long long>(annotation.pointCount));
    if (!playModeActive) {
      ImGui::SameLine();
      if (ImGui::SmallButton("Remove")) {
        commands.enqueue(
            CreativeDesktopCommandId::RemoveMeasurementAnnotation,
            CreativeDesktopMeasurementAnnotationPayload{
                annotation.id, {}});
      }
    }
    ImGui::PopID();
  }
}

void appendCreativeDesktopLogicInspector(
    CreativeEditorDesktopUiState& desktopUi,
    const CreativeEditorLogicLinkState& logicLinks,
    const CreativeDesktopInspectorSelectionModel& selection,
    CreativeDesktopCommandFrame& commands) {
  const cr::CreativeObject& inspected = *selection.object;
  if (cr::creativeObjectCanSourceLogicLink(inspected.kind)) {
    const cr::CreativeLogicSourceEvent sourceEvent =
        cr::creativeLogicSourceEventForObject(inspected.kind);
    ImGui::Text(
        "Source event: %s",
        std::string(
            cr::creativeLogicSourceEventLabel(sourceEvent)).c_str());
    const cr::CreativeLogicDiagnostic* sourceDiagnostic =
        diagnosticForSource(desktopUi.logicDiagnostics, inspected.id);
    if (sourceDiagnostic != nullptr) {
      ImGui::TextColored(
          creativeDesktopLogicDiagnosticColor(
              sourceDiagnostic->severity),
          "%s", std::string(cr::toString(sourceDiagnostic->code)).c_str());
    }
    const bool activeSource =
        logicLinks.sourceObjectId == inspected.id;
    if (activeSource) {
      ImGui::TextColored(
          ImVec4{0.20F, 1.0F, 0.35F, 1.0F}, "ACTIVE SOURCE");
      if (!selection.playModeActive) {
        ImGui::SameLine();
        if (ImGui::SmallButton("X##logic_source")) {
          commands.enqueue(
              CreativeDesktopCommandId::ClearLogicSource);
        }
        appendCreativeDesktopInspectorHoverTooltip(
            "Clear active logic source");
      }
    } else if (!selection.playModeActive &&
               ImGui::Button("Set as source##logic_source")) {
      queueLogicSource(commands, inspected.id);
    }
  }

  ImGui::SeparatorText("Logic links");
  std::size_t shown = 0U;
  for (const cr::CreativeLogicLink& link :
       selection.document.logicLinks()) {
    if (link.sourceObjectId != inspected.id &&
        link.targetObjectId != inspected.id) {
      continue;
    }
    appendLogicLinkRow(
        selection.document, link,
        link.sourceObjectId == inspected.id,
        selection.playModeActive, commands);
    ++shown;
  }
  if (shown == 0U) {
    ImGui::TextDisabled("No logic links");
  }
  appendNewLogicLinkControl(
      selection.document, logicLinks, inspected,
      selection.playModeActive, commands);
  ImGui::TextDisabled(
      "Document links: %llu",
      static_cast<unsigned long long>(
          selection.document.logicLinks().size()));
}

}  // namespace iggy3d_creative_app
