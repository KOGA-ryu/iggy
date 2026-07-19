#include "EditorDesktopWidgets.hpp"
#include "EditorWorldLayout.hpp"

#include <cstdint>
#include <string>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

const char* operationLabel(
    CreativeEditorWorldLayoutGeneratedBuildingOperation operation) noexcept {
  switch (operation) {
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::Move:
      return "Move";
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::RotateLeft90:
      return "Rotate left";
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::RotateRight90:
      return "Rotate right";
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::MirrorX:
      return "Mirror X";
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::MirrorZ:
      return "Mirror Z";
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::Duplicate:
      return "Duplicate";
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::Count:
      return "Invalid";
  }
  return "Invalid";
}

}  // namespace

void appendCreativeDesktopGeneratedBuildingSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    cr::CreativeObjectId objectId,
    std::size_t buildingIndex,
    bool disabled,
    CreativeDesktopCommandFrame& commands) {
  if (buildingIndex >= worldLayout.source.buildings.size()) {
    return;
  }
  const cr::CreativeWorldLayoutBuilding& building =
      worldLayout.source.buildings[buildingIndex];
  CreativeEditorWorldLayoutGeneratedBuildingDraft& draft =
      worldLayout.generatedBuildingDraft;
  if (!draft.active || draft.buildingIndex != buildingIndex ||
      draft.sourceRevision != worldLayout.revision) {
    if (draft.active && creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
    draft = {};
    draft.active = true;
    draft.buildingIndex = buildingIndex;
    draft.sourceRevision = worldLayout.revision;
  }

  std::size_t levelCount = 0U;
  std::size_t roomCount = 0U;
  for (const cr::CreativeWorldLayoutLevel& level : worldLayout.source.levels) {
    levelCount += level.buildingIndex == buildingIndex ? 1U : 0U;
  }
  for (const cr::CreativeWorldLayoutRoom& room : worldLayout.source.rooms) {
    roomCount += room.buildingIndex == buildingIndex ? 1U : 0U;
  }
  ImGui::TextDisabled("%s  |  %zu level%s  |  %zu room%s",
                      building.name.c_str(), levelCount,
                      levelCount == 1U ? "" : "s", roomCount,
                      roomCount == 1U ? "" : "s");

  ImGui::BeginDisabled(disabled);
  ImGui::SeparatorText("Terrain placement");
  int groundingMode = static_cast<int>(building.groundingMode);
  constexpr const char* kGroundingModes[] = {"Absolute elevation",
                                              "Grounded foundation"};
  if (ImGui::Combo("Placement##generated_building", &groundingMode,
                   kGroundingModes, 2)) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutApplyGeneratedBuildingGrounding,
        CreativeDesktopWorldLayoutBuildingGroundingPayload{
            objectId, buildingIndex, building.stableKey,
            {static_cast<cr::CreativeWorldLayoutGroundingMode>(groundingMode),
             building.maximumGroundReliefCells}});
  }
  if (building.groundingMode ==
      cr::CreativeWorldLayoutGroundingMode::Foundation) {
    std::uint16_t maximumRelief = building.maximumGroundReliefCells;
    constexpr std::uint16_t reliefStep = 1U;
    ImGui::SetNextItemWidth(112.0F);
    static_cast<void>(ImGui::InputScalar(
        "Max relief##generated_building", ImGuiDataType_U16, &maximumRelief,
        &reliefStep));
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      commands.push(
          CreativeDesktopCommandId::
              WorldLayoutApplyGeneratedBuildingGrounding,
          CreativeDesktopWorldLayoutBuildingGroundingPayload{
              objectId, buildingIndex, building.stableKey,
              {building.groundingMode, maximumRelief}});
    }
    ImGui::TextDisabled("cells; higher relief adds a buried foundation");
  }

  ImGui::SeparatorText("Move on grid");
  const std::int64_t one = 1;
  ImGui::SetNextItemWidth(112.0F);
  const bool xEdited = ImGui::InputScalar(
      "X cells##generated_building", ImGuiDataType_S64, &draft.deltaXCells,
      &one);
  ImGui::SetNextItemWidth(112.0F);
  const bool zEdited = ImGui::InputScalar(
      "Z cells##generated_building", ImGuiDataType_S64, &draft.deltaZCells,
      &one);
  if ((xEdited || zEdited) && draft.previewReady) {
    draft.previewReady = false;
    if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }

  const auto preview = [&](
                           CreativeEditorWorldLayoutGeneratedBuildingOperation
                               operation,
                           std::int64_t deltaX,
                           std::int64_t deltaZ) {
    draft.operation = operation;
    draft.deltaXCells = deltaX;
    draft.deltaZCells = deltaZ;
    draft.previewReady = true;
    commands.push(
        CreativeDesktopCommandId::
            WorldLayoutPreviewGeneratedBuildingOperation,
        CreativeDesktopGeneratedBuildingOperationPayload{
            objectId, buildingIndex, building.stableKey, operation, deltaX,
            deltaZ});
  };

  ImGui::BeginDisabled(draft.deltaXCells == 0 && draft.deltaZCells == 0);
  if (ImGui::Button("Preview move##generated_building")) {
    preview(CreativeEditorWorldLayoutGeneratedBuildingOperation::Move,
            draft.deltaXCells, draft.deltaZCells);
  }
  ImGui::EndDisabled();

  ImGui::SeparatorText("Rigid transform");
  if (ImGui::Button("Rotate left##generated_building")) {
    preview(CreativeEditorWorldLayoutGeneratedBuildingOperation::RotateLeft90,
            0, 0);
  }
  ImGui::SameLine();
  if (ImGui::Button("Rotate right##generated_building")) {
    preview(
        CreativeEditorWorldLayoutGeneratedBuildingOperation::RotateRight90,
        0, 0);
  }
  if (ImGui::Button("Mirror X##generated_building")) {
    preview(CreativeEditorWorldLayoutGeneratedBuildingOperation::MirrorX, 0,
            0);
  }
  ImGui::SameLine();
  if (ImGui::Button("Mirror Z##generated_building")) {
    preview(CreativeEditorWorldLayoutGeneratedBuildingOperation::MirrorZ, 0,
            0);
  }

  std::int64_t duplicateX = 0;
  std::int64_t duplicateZ = 0;
  const bool canDuplicate =
      defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
          worldLayout, buildingIndex, duplicateX, duplicateZ);
  ImGui::BeginDisabled(!canDuplicate);
  if (ImGui::Button("Preview duplicate##generated_building")) {
    preview(CreativeEditorWorldLayoutGeneratedBuildingOperation::Duplicate,
            duplicateX, duplicateZ);
  }
  ImGui::EndDisabled();
  ImGui::EndDisabled();

  if (draft.previewReady) {
    ImGui::TextColored({0.20F, 0.78F, 0.38F, 1.0F}, "Preview: %s",
                       operationLabel(draft.operation));
  }
  ImGui::BeginDisabled(disabled || !draft.previewReady ||
                       !creativeEditorWorldLayoutPreviewActive(worldLayout));
  if (ImGui::Button("Apply building operation")) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutApplyGeneratedBuildingOperation,
        CreativeDesktopGeneratedBuildingOperationPayload{
            objectId, buildingIndex, building.stableKey, draft.operation,
            draft.deltaXCells, draft.deltaZCells});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(disabled || !draft.previewReady);
  if (ImGui::Button("Cancel##generated_building")) {
    draft.previewReady = false;
    if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }
  ImGui::EndDisabled();
}

}  // namespace iggy3d_creative_app
