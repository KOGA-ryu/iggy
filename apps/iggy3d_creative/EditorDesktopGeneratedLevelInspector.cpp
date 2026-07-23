#include "EditorDesktopWidgets.hpp"
#include "EditorWorldLayout.hpp"

#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

void appendCreativeDesktopGeneratedLevelSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    cr::CreativeObjectId objectId,
    cr::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutLevelSettings current;
  if (provenance.table != cr::CreativeWorldLayoutTable::Level ||
      provenance.index >= worldLayout.source.levels.size() ||
      !readCreativeEditorWorldLayoutLevelSettings(
          worldLayout, provenance.index, current)) {
    return;
  }

  const cr::CreativeWorldLayoutLevel& level =
      worldLayout.source.levels[provenance.index];
  if (level.buildingIndex >= worldLayout.source.buildings.size()) {
    return;
  }
  const bool draftChanged =
      !worldLayout.generatedLevelSettingsDraft.active ||
      worldLayout.generatedLevelSettingsDraft.levelIndex != provenance.index ||
      worldLayout.generatedLevelSettingsDraft.sourceRevision !=
          worldLayout.revision;
  if (draftChanged) {
    if (worldLayout.generatedLevelSettingsDraft.active &&
        creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
    worldLayout.generatedLevelSettingsDraft =
        {true, provenance.index, worldLayout.revision, current};
  }

  const cr::CreativeWorldLayoutBuilding& building =
      worldLayout.source.buildings[level.buildingIndex];
  const std::size_t affectedRoomCount =
      static_cast<std::size_t>(std::count_if(
          worldLayout.source.rooms.begin(), worldLayout.source.rooms.end(),
          [&level, &provenance](const cr::CreativeWorldLayoutRoom& room) {
            return room.buildingIndex == level.buildingIndex &&
                   room.levelIndex == provenance.index;
          }));
  ImGui::TextDisabled("%s  |  %s", building.name.c_str(), level.name.c_str());
  ImGui::TextDisabled("Affects %zu room%s on this level", affectedRoomCount,
                      affectedRoomCount == 1U ? "" : "s");

  CreativeEditorWorldLayoutLevelSettings& settings =
      worldLayout.generatedLevelSettingsDraft.settings;
  CreativeDesktopPropertyEditActivity editActivity;
  int wallHeight = settings.wallHeightCells;
  int floorLayers = settings.floorThicknessLayers;
  int ceilingLayers = settings.ceilingThicknessLayers;
  int roofLayers = settings.roofThicknessLayers;

  ImGui::BeginDisabled(disabled);
  ImGui::SeparatorText("Level shell");
  ImGui::SetNextItemWidth(188.0F);
  const bool nameEdited = creativeDesktopInputTextStdString(
      "Name##generated_level", &settings.name);
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, nameEdited, ImGui::IsItemDeactivatedAfterEdit());
  ImGui::SetNextItemWidth(112.0F);
  const bool floorTopEdited = ImGui::InputDouble(
      "Floor top##generated_level", &settings.floorTopLayer, 0.5, 1.0,
      "%.2f");
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, floorTopEdited, ImGui::IsItemDeactivatedAfterEdit());
  ImGui::SetNextItemWidth(112.0F);
  const bool wallHeightEdited = ImGui::InputInt(
      "Wall height##generated_level", &wallHeight, 1, 2);
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, wallHeightEdited, ImGui::IsItemDeactivatedAfterEdit());

  ImGui::SeparatorText("Slabs");
  ImGui::SetNextItemWidth(112.0F);
  const bool floorLayersEdited = ImGui::InputInt(
      "Floor layers##generated_level", &floorLayers, 1, 2);
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, floorLayersEdited, ImGui::IsItemDeactivatedAfterEdit());
  ImGui::SetNextItemWidth(112.0F);
  const bool ceilingLayersEdited = ImGui::InputInt(
      "Ceiling layers##generated_level", &ceilingLayers, 1, 2);
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, ceilingLayersEdited,
      ImGui::IsItemDeactivatedAfterEdit());
  ImGui::SetNextItemWidth(112.0F);
  const bool roofLayersEdited = ImGui::InputInt(
      "Roof layers##generated_level", &roofLayers, 1, 2);
  observeCreativeDesktopContinuousPropertyEdit(
      editActivity, roofLayersEdited, ImGui::IsItemDeactivatedAfterEdit());

  ImGui::SeparatorText("Roof shape");
  drawCreativeStructuralRoofSettingsWidgets(
      settings, editActivity, "generated_level_roof");
  ImGui::EndDisabled();

  const bool layersRepresentable =
      wallHeight > 0 && wallHeight <= 65535 && floorLayers > 0 &&
      floorLayers <= 65535 && ceilingLayers > 0 &&
      ceilingLayers <= 65535 && roofLayers > 0 && roofLayers <= 65535;
  if (layersRepresentable) {
    settings.wallHeightCells = static_cast<std::uint16_t>(wallHeight);
    settings.floorThicknessLayers =
        static_cast<std::uint16_t>(floorLayers);
    settings.ceilingThicknessLayers =
        static_cast<std::uint16_t>(ceilingLayers);
    settings.roofThicknessLayers = static_cast<std::uint16_t>(roofLayers);
  }
  const bool representable =
      !settings.name.empty() && layersRepresentable &&
      std::isfinite(settings.floorTopLayer) &&
      cr::validCreativeStructuralRoofSettings(
          settings.roofStyle, settings.roofRidgeAxis,
          settings.roofSlopeDirection, settings.roofPitchDegrees,
          settings.roofOverhangCells, settings.roofMaterial) &&
      settings.roofOverhangCells <=
          cr::kMaximumCreativeWorldLayoutRoofOverhangCells;
  if (!representable) {
    ImGui::TextColored({0.94F, 0.45F, 0.32F, 1.0F},
                       "Level shell settings are outside valid bounds");
  }
  bool dirty = !(current == settings);
  bool resetRequested = false;
  ImGui::BeginDisabled(disabled || !dirty);
  if (ImGui::Button("Reset##generated_level")) {
    worldLayout.generatedLevelSettingsDraft.settings = current;
    dirty = false;
    resetRequested = true;
  }
  ImGui::EndDisabled();

  const CreativeDesktopPropertyEditIntent intent =
      resolveCreativeDesktopPropertyEditIntent(
          editActivity, dirty, representable,
          creativeEditorWorldLayoutPreviewActive(worldLayout),
          resetRequested);
  queueCreativeDesktopGeneratedPropertyEdit(
      intent,
      CreativeDesktopCommandId::WorldLayoutPreviewGeneratedLevelSettings,
      CreativeDesktopCommandId::WorldLayoutApplyGeneratedLevelSettings,
      CreativeDesktopGeneratedLevelSettingsPayload{
          objectId, provenance.index, level.stableKey,
          worldLayout.generatedLevelSettingsDraft.settings},
      commands);
}

}  // namespace iggy3d_creative_app
