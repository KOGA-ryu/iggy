#include "EditorDesktopWorldLayoutArchitectureInspector.hpp"

#include "EditorDesktopUi.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <cmath>
#include <cstdint>

#include "imgui.h"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void initializeDraft(
    CreativeEditorDesktopArchitectureDraft& draft,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayoutBuildingDimensions& dimensions,
    std::size_t buildingIndex) {
  draft = {};
  draft.active = true;
  draft.buildingIndex = buildingIndex;
  draft.sourceRevision = state.revision;
  draft.profile = cr::defaultCreativeWorldLayoutArchitecturalProfile(
      cr::CreativeWorldLayoutArchitecturalProfileKind::Custom);
  if (dimensions.accepted) {
    draft.profile.floorToFloorMeters =
        dimensions.occupiedLevelCount > 1U &&
                dimensions.minimumFloorToFloorMeters > 0.0
            ? dimensions.minimumFloorToFloorMeters
            : dimensions.minimumWallHeightMeters;
  }
  for (const cr::CreativeWorldLayoutLevel& level : state.source.levels) {
    if (level.buildingIndex != buildingIndex) {
      continue;
    }
    draft.profile.floorThicknessLayers = level.floorThicknessLayers;
    draft.profile.ceilingThicknessLayers = level.ceilingThicknessLayers;
    draft.profile.roofThicknessLayers = level.roofThicknessLayers;
    break;
  }
}

bool drawLayerField(const char* label,
                    std::uint16_t& layers,
                    cr::CreativeObjectKind kind) {
  constexpr std::uint16_t step = 1U;
  ImGui::SetNextItemWidth(112.0F);
  const bool edited = ImGui::InputScalar(
      label, ImGuiDataType_U16, &layers, &step);
  ImGui::SameLine();
  ImGui::TextDisabled(
      "%.2f m",
      layers * cr::defaultCreativeStructuralLayerThicknessMeters(kind));
  return edited;
}

}  // namespace

void drawCreativeEditorWorldLayoutArchitectureInspector(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayoutBuildingDimensions& dimensions,
    std::size_t buildingIndex,
    const cr::CreativeWorldLayoutBuilding& building,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorDesktopArchitectureDraft& draft =
      desktopUi.worldLayoutArchitecture;
  if (!draft.active || draft.buildingIndex != buildingIndex ||
      draft.sourceRevision != state.revision) {
    if (draft.active && creativeEditorWorldLayoutPreviewActive(state)) {
      commands.enqueue(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
    initializeDraft(draft, state, dimensions, buildingIndex);
  }
  if (draft.previewReady &&
      !creativeEditorWorldLayoutPreviewActive(state)) {
    draft.previewReady = false;
  }

  ImGui::SeparatorText("Architectural profile");
  bool edited = false;
  int profileKind = static_cast<int>(draft.profile.kind);
  constexpr const char* kProfileLabels[] = {
      "Residential (3.0 m)", "Grand (5.0 m)", "Custom"};
  if (ImGui::Combo("Profile##layout_architecture", &profileKind,
                   kProfileLabels, 3)) {
    const auto kind =
        static_cast<cr::CreativeWorldLayoutArchitecturalProfileKind>(
            profileKind);
    if (kind == cr::CreativeWorldLayoutArchitecturalProfileKind::Custom) {
      draft.profile.kind = kind;
    } else {
      draft.profile =
          cr::defaultCreativeWorldLayoutArchitecturalProfile(kind);
    }
    edited = true;
  }

  constexpr double floorStep = 0.5;
  constexpr double floorFastStep = 1.0;
  ImGui::SetNextItemWidth(140.0F);
  if (ImGui::InputDouble("Floor to floor##layout_architecture",
                         &draft.profile.floorToFloorMeters, floorStep,
                         floorFastStep, "%.2f m")) {
    draft.profile.kind =
        cr::CreativeWorldLayoutArchitecturalProfileKind::Custom;
    edited = true;
  }
  const bool floorEdited = drawLayerField(
      "Floor slab##layout_architecture",
      draft.profile.floorThicknessLayers, cr::CreativeObjectKind::Floor);
  const bool ceilingEdited = drawLayerField(
      "Ceiling##layout_architecture", draft.profile.ceilingThicknessLayers,
      cr::CreativeObjectKind::Ceiling);
  const bool roofEdited = drawLayerField(
      "Roof##layout_architecture", draft.profile.roofThicknessLayers,
      cr::CreativeObjectKind::Roof);
  if (floorEdited || ceilingEdited || roofEdited) {
    draft.profile.kind =
        cr::CreativeWorldLayoutArchitecturalProfileKind::Custom;
    edited = true;
  }

  const cr::CreativeGridSettings grid = document.gridSettings();
  const double cellCount =
      grid.cellSizeMeters > 0.0
          ? draft.profile.floorToFloorMeters / grid.cellSizeMeters
          : 0.0;
  const bool gridAligned = std::isfinite(cellCount) && cellCount >= 1.0 &&
                           std::fabs(cellCount - std::round(cellCount)) <=
                               1.0e-9;
  if (gridAligned) {
    ImGui::TextDisabled("%lld grid cells at %.2f m",
                        static_cast<long long>(std::llround(cellCount)),
                        grid.cellSizeMeters);
  } else {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "Height must align to the %.2f m grid",
                       grid.cellSizeMeters);
  }
  if (draft.profile.ceilingThicknessLayers != 1U) {
    ImGui::TextColored(ImVec4{0.92F, 0.72F, 0.20F, 1.0F},
                       "Custom ceiling marks blockout locally modified");
  }

  if (edited && draft.previewReady) {
    draft.previewReady = false;
    if (creativeEditorWorldLayoutPreviewActive(state)) {
      commands.enqueue(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }

  const bool locked = state.buildingManipulation.active ||
                      state.buildingTransform.active ||
                      state.generatedRevision != state.revision;
  const bool valid = gridAligned &&
                     cr::validCreativeWorldLayoutArchitecturalProfile(
                         draft.profile);
  ImGui::BeginDisabled(locked || !valid);
  if (ImGui::Button("Preview profile##layout_architecture")) {
    draft.previewReady = true;
    commands.enqueue(
        CreativeDesktopCommandId::WorldLayoutPreviewBuildingArchitecture,
        CreativeDesktopWorldLayoutBuildingArchitecturePayload{
            buildingIndex, building.stableKey, draft.profile});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(
      locked || !draft.previewReady ||
      !creativeEditorWorldLayoutPreviewActive(state));
  if (ImGui::Button("Apply##layout_architecture")) {
    commands.enqueue(
        CreativeDesktopCommandId::WorldLayoutApplyBuildingArchitecture,
        CreativeDesktopWorldLayoutBuildingArchitecturePayload{
            buildingIndex, building.stableKey, draft.profile});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!draft.previewReady);
  if (ImGui::Button("Cancel##layout_architecture")) {
    draft.previewReady = false;
    if (creativeEditorWorldLayoutPreviewActive(state)) {
      commands.enqueue(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }
  ImGui::EndDisabled();
}

}  // namespace iggy3d_creative_app
