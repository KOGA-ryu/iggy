#include "EditorDesktopInspectorInternal.hpp"

#include <cstdint>

#include "EditorObjectActions.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

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

[[nodiscard]] TriState foldFlag(
    const cr::CreativeDocument& document,
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
  return all ? TriState::On
             : (any ? TriState::Mixed : TriState::Off);
}

}  // namespace

void appendCreativeDesktopMultiSelectionInspector(
    const CreativeDesktopInspectorSelectionModel& selection,
    CreativeDesktopCommandFrame& commands) {
  const auto actionAvailable =
      [&](cr::CreativeSemanticObjectAction action) {
        return creativeEditorObjectActionAvailable(
            selection.objectActions.admissions, action);
      };
  ImGui::Text(
      "%llu objects selected",
      static_cast<unsigned long long>(
          selection.selection.objectIds.size()));
  ImGui::Text(
      "Visibility: %s",
      triStateLabel(foldFlag(
          selection.document, selection.selection.objectIds, false)));
  if (!selection.playModeActive) {
    ImGui::SameLine();
    ImGui::BeginDisabled(
        !actionAvailable(
            cr::CreativeSemanticObjectAction::SetVisible));
    if (ImGui::SmallButton("Show##multi")) {
      commands.enqueue(
          CreativeDesktopCommandId::SetObjectsVisible,
          CreativeDesktopObjectFlagPayload{
              selection.selection.objectIds, true});
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Hide##multi")) {
      commands.enqueue(
          CreativeDesktopCommandId::SetObjectsVisible,
          CreativeDesktopObjectFlagPayload{
              selection.selection.objectIds, false});
    }
    ImGui::EndDisabled();
  }
  ImGui::Text(
      "Lock: %s",
      triStateLabel(foldFlag(
          selection.document, selection.selection.objectIds, true)));
  if (!selection.playModeActive) {
    ImGui::SameLine();
    ImGui::BeginDisabled(
        !actionAvailable(
            cr::CreativeSemanticObjectAction::SetLocked));
    if (ImGui::SmallButton("Lock##multi")) {
      commands.enqueue(
          CreativeDesktopCommandId::SetObjectsLocked,
          CreativeDesktopObjectFlagPayload{
              selection.selection.objectIds, true});
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Unlock##multi")) {
      commands.enqueue(
          CreativeDesktopCommandId::SetObjectsLocked,
          CreativeDesktopObjectFlagPayload{
              selection.selection.objectIds, false});
    }
    ImGui::EndDisabled();
    ImGui::Separator();
    ImGui::BeginDisabled(
        !actionAvailable(
            cr::CreativeSemanticObjectAction::Duplicate));
    if (ImGui::Button("Duplicate##multi")) {
      commands.enqueue(
          CreativeDesktopCommandId::DuplicateSelection);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(
        !actionAvailable(
            cr::CreativeSemanticObjectAction::Delete));
    if (ImGui::Button("Delete##multi")) {
      commands.enqueue(CreativeDesktopCommandId::DeleteSelection);
    }
    ImGui::EndDisabled();
  }
}

}  // namespace iggy3d_creative_app
