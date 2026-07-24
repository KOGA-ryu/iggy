#include "EditorDesktopWorldLayoutSourceInspectorInternal.hpp"

namespace iggy3d_creative_app::detail {

void drawLevelInspector(CreativeEditorWorldLayoutState& state,
                        CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Level ||
      state.selection.index >= state.source.levels.size()) {
    state.levelSettingsDraft = {};
    return;
  }
  const std::size_t levelIndex = state.selection.index;
  CreativeEditorWorldLayoutLevelSettings current;
  if (!readCreativeEditorWorldLayoutLevelSettings(state, levelIndex, current)) {
    state.levelSettingsDraft = {};
    return;
  }
  if (!state.levelSettingsDraft.active ||
      state.levelSettingsDraft.levelIndex != levelIndex ||
      state.levelSettingsDraft.sourceRevision != state.revision) {
    state.levelSettingsDraft = {true, levelIndex, state.revision, current};
  }
  CreativeEditorWorldLayoutLevelSettings& draft =
      state.levelSettingsDraft.settings;
  const cr::CreativeWorldLayoutLevel& level = state.source.levels[levelIndex];
  CreativeDesktopPropertyEditActivity activity;

  ImGui::SeparatorText("Level");
  drawStableKey(level.stableKey, "Building level");
  observeCreativeDesktopContinuousPropertyWidget(
      activity, inputText("Name##layout_level_properties", draft.name));
  ImGui::SetNextItemWidth(140.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Floor top##layout_level_properties",
                         &draft.floorTopLayer, 1.0, 4.0, "%.3f"));
  ImGui::SetNextItemWidth(140.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputScalar("Wall height##layout_level_properties",
                         ImGuiDataType_U16, &draft.wallHeightCells));

  ImGui::SeparatorText("Slabs");
  ImGui::SetNextItemWidth(120.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputScalar("Floor##layout_level_properties", ImGuiDataType_U16,
                         &draft.floorThicknessLayers));
  ImGui::SetNextItemWidth(120.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputScalar("Ceiling##layout_level_properties",
                         ImGuiDataType_U16,
                         &draft.ceilingThicknessLayers));
  ImGui::SetNextItemWidth(120.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputScalar("Roof##layout_level_properties", ImGuiDataType_U16,
                         &draft.roofThicknessLayers));

  ImGui::SeparatorText("Roof shape");
  drawCreativeStructuralRoofSettingsWidgets(
      draft, activity, "layout_level_roof");

  const std::size_t apertureCount = static_cast<std::size_t>(std::count_if(
      state.source.roofApertures.begin(), state.source.roofApertures.end(),
      [levelIndex](const cr::CreativeWorldLayoutRoofAperture& aperture) {
        return aperture.levelIndex == levelIndex;
      }));
  const bool topmost = cr::creativeWorldLayoutLevelIsTopmostOccupied(
      state.source, levelIndex);
  const bool apertureUnsupported =
      level.roofStyle == cr::CreativeStructuralRoofStyle::Hip;
  const bool apertureCapacityReached =
      apertureCount >= cr::kCreativeStructuralRoofApertureCapacity;
  ImGui::SeparatorText("Roof apertures");
  ImGui::TextDisabled("%zu of %zu", apertureCount,
                      cr::kCreativeStructuralRoofApertureCapacity);
  ImGui::BeginDisabled(!topmost || apertureUnsupported ||
                       apertureCapacityReached);
  if (ImGui::Button("Add skylight##layout_level")) {
    commands.enqueue(
        CreativeDesktopCommandId::WorldLayoutCreateRoofAperture,
        CreativeDesktopWorldLayoutRoofApertureCreatePayload{
            levelIndex,
            cr::CreativeStructuralRoofApertureKind::Skylight});
  }
  ImGui::SameLine();
  if (ImGui::Button("Add chimney clearance##layout_level")) {
    commands.enqueue(
        CreativeDesktopCommandId::WorldLayoutCreateRoofAperture,
        CreativeDesktopWorldLayoutRoofApertureCreatePayload{
            levelIndex,
            cr::CreativeStructuralRoofApertureKind::ChimneyClearance});
  }
  ImGui::EndDisabled();
  if (!topmost) {
    ImGui::TextDisabled("Apertures belong to the top occupied roof");
  } else if (apertureUnsupported) {
    ImGui::TextDisabled("Hip apertures await polygon panel support");
  } else if (apertureCapacityReached) {
    ImGui::TextDisabled("This roof has reached the four-aperture limit");
  }

  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, draft, true, "Reset level", state,
      levelIndex, level.stableKey, commands);
}

void drawRoofApertureInspectorForIndex(
    CreativeEditorWorldLayoutState& state,
    std::size_t apertureIndex,
    CreativeDesktopCommandFrame& commands) {
  if (apertureIndex >= state.source.roofApertures.size()) {
    state.roofApertureSettingsDraft = {};
    return;
  }
  CreativeEditorWorldLayoutRoofApertureSettings current;
  if (!readCreativeEditorWorldLayoutRoofApertureSettings(
          state, apertureIndex, current)) {
    state.roofApertureSettingsDraft = {};
    return;
  }
  if (!state.roofApertureSettingsDraft.active ||
      state.roofApertureSettingsDraft.apertureIndex != apertureIndex ||
      state.roofApertureSettingsDraft.sourceRevision != state.revision) {
    state.roofApertureSettingsDraft = {
        true, apertureIndex, state.revision, current};
  }
  CreativeEditorWorldLayoutRoofApertureSettings& draft =
      state.roofApertureSettingsDraft.settings;
  const cr::CreativeWorldLayoutRoofAperture& aperture =
      state.source.roofApertures[apertureIndex];
  CreativeDesktopPropertyEditActivity activity;

  ImGui::SeparatorText("Roof aperture");
  drawStableKey(aperture.stableKey, "Plan-space roof opening");
  constexpr std::array kinds{
      cr::CreativeStructuralRoofApertureKind::Skylight,
      cr::CreativeStructuralRoofApertureKind::ChimneyClearance};
  ImGui::SetNextItemWidth(190.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Type##layout_roof_aperture", draft.kind, kinds,
                [](cr::CreativeStructuralRoofApertureKind kind) {
                  return kind ==
                                 cr::CreativeStructuralRoofApertureKind::
                                     Skylight
                             ? std::string_view{"Skylight"}
                             : std::string_view{"Chimney clearance"};
                }));
  if (ImGui::BeginTable("##layout_roof_aperture_bounds", 3,
                        ImGuiTableFlags_SizingStretchSame |
                            ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupColumn("");
    ImGui::TableSetupColumn("Min");
    ImGui::TableSetupColumn("Max");
    ImGui::TableHeadersRow();
    const auto row = [&activity](const char* axis, double& minimum,
                                 double& maximum) {
      ImGui::PushID(axis);
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(axis);
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-1.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputDouble("##min", &minimum, 0.25, 1.0, "%.3f"));
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-1.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputDouble("##max", &maximum, 0.25, 1.0, "%.3f"));
      ImGui::PopID();
    };
    row("X", draft.minimumXCells, draft.maximumXCells);
    row("Z", draft.minimumZCells, draft.maximumZCells);
    ImGui::EndTable();
  }
  const bool representable =
      draft.kind < cr::CreativeStructuralRoofApertureKind::Count &&
      std::isfinite(draft.minimumXCells) &&
      std::isfinite(draft.maximumXCells) &&
      std::isfinite(draft.minimumZCells) &&
      std::isfinite(draft.maximumZCells) &&
      draft.minimumXCells < draft.maximumXCells &&
      draft.minimumZCells < draft.maximumZCells;
  if (!representable) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "Bounds must be finite and have positive area");
  } else {
    ImGui::TextDisabled("Exact grid-cell bounds; roof clearance is validated");
  }
  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, draft, representable, "Reset aperture", state,
      apertureIndex, aperture.stableKey, commands);
}


}  // namespace iggy3d_creative_app::detail
