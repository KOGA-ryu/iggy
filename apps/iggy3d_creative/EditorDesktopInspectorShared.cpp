#include "EditorDesktopInspectorInternal.hpp"

#include "app/iggy3d/creative/document/Object.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

int inputTextResizeCallback(ImGuiInputTextCallbackData* data) {
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    auto* str = static_cast<std::string*>(data->UserData);
    str->resize(static_cast<std::size_t>(data->BufTextLen));
    data->Buf = str->data();
  }
  return 0;
}

}  // namespace

bool creativeDesktopInputTextStdString(
    const char* label,
    std::string* str,
    ImGuiInputTextFlags flags) {
  return ImGui::InputText(
      label, str->data(), str->capacity() + 1U,
      flags | ImGuiInputTextFlags_CallbackResize,
      inputTextResizeCallback, str);
}

void appendCreativeDesktopInspectorHoverTooltip(const char* text) {
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(text);
    ImGui::EndTooltip();
  }
}

void refreshCreativeDesktopInspectorDraft(
    CreativeDesktopInspectorDraft& draft,
    const cr::CreativeDocument& document,
    const cr::CreativeObject& object) {
  const bool keyChanged =
      draft.documentId != document.id() || draft.objectId != object.id;
  const bool revisionMoved =
      draft.sourceRevision != document.revision();
  if (draft.valid && !keyChanged &&
      (!revisionMoved || draft.editing)) {
    return;
  }
  draft.documentId = document.id();
  draft.objectId = object.id;
  draft.sourceRevision = document.revision();
  draft.name = object.name;
  draft.position = {
      object.transform.position.x, object.transform.position.y,
      object.transform.position.z};
  const cr::CreativeVec3 degrees =
      creativeDesktopRadiansToDegrees(
          object.transform.rotationEulerRadians);
  draft.rotationDegrees = {degrees.x, degrees.y, degrees.z};
  draft.scale = {
      object.transform.scale.x, object.transform.scale.y,
      object.transform.scale.z};
  draft.movingPlatform = object.movingPlatform;
  draft.playerSpawn = object.playerSpawn;
  draft.npcSpawn = object.npcSpawn;
  draft.lootPoint = object.lootPoint;
  draft.exitPoint = object.exitPoint;
  draft.movingPlatformWaypointIndex = 0U;
  draft.movingPlatformWaypointDwellSeconds =
      object.pathPoints.empty()
          ? 0.0
          : object.pathPoints.front().dwellSeconds;
  draft.validation.clear();
  draft.valid = true;
}

void queueCreativeDesktopObjectNavigation(
    CreativeDesktopCommandFrame& commands,
    cr::CreativeObjectId objectId,
    bool playModeActive) {
  if (objectId == cr::kInvalidObjectId) {
    return;
  }
  commands.enqueue(
      playModeActive ? CreativeDesktopCommandId::SelectObjects
                     : CreativeDesktopCommandId::FocusObject,
      CreativeDesktopSelectPayload{{objectId}, objectId});
}

ImVec4 creativeDesktopLogicDiagnosticColor(
    cr::CreativeLogicDiagnosticSeverity severity) noexcept {
  return severity == cr::CreativeLogicDiagnosticSeverity::Error
             ? ImVec4{1.0F, 0.34F, 0.30F, 1.0F}
             : ImVec4{1.0F, 0.82F, 0.25F, 1.0F};
}

}  // namespace iggy3d_creative_app
