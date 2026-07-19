#include "EditorDesktopWidgets.hpp"

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
  int wallHeight = settings.wallHeightCells;
  int floorLayers = settings.floorThicknessLayers;
  int ceilingLayers = settings.ceilingThicknessLayers;
  int roofLayers = settings.roofThicknessLayers;

  ImGui::BeginDisabled(disabled);
  ImGui::SeparatorText("Level shell");
  ImGui::SetNextItemWidth(188.0F);
  bool edited = creativeDesktopInputTextStdString(
      "Name##generated_level", &settings.name);
  ImGui::SetNextItemWidth(112.0F);
  edited = ImGui::InputDouble("Floor top##generated_level",
                              &settings.floorTopLayer, 0.5, 1.0, "%.2f") ||
           edited;
  ImGui::SetNextItemWidth(112.0F);
  const bool wallHeightEdited = ImGui::InputInt(
      "Wall height##generated_level", &wallHeight, 1, 2);

  ImGui::SeparatorText("Slabs");
  ImGui::SetNextItemWidth(112.0F);
  const bool floorLayersEdited = ImGui::InputInt(
      "Floor layers##generated_level", &floorLayers, 1, 2);
  ImGui::SetNextItemWidth(112.0F);
  const bool ceilingLayersEdited = ImGui::InputInt(
      "Ceiling layers##generated_level", &ceilingLayers, 1, 2);
  ImGui::SetNextItemWidth(112.0F);
  const bool roofLayersEdited = ImGui::InputInt(
      "Roof layers##generated_level", &roofLayers, 1, 2);

  ImGui::SeparatorText("Roof shape");
  ImGui::SetNextItemWidth(148.0F);
  if (ImGui::BeginCombo(
          "Style##generated_level",
          settings.roofStyle == cr::CreativeStructuralRoofStyle::Gable
              ? "Gable"
              : "Flat")) {
    for (const cr::CreativeStructuralRoofStyle style :
         {cr::CreativeStructuralRoofStyle::Flat,
          cr::CreativeStructuralRoofStyle::Gable}) {
      const bool selected = settings.roofStyle == style;
      if (ImGui::Selectable(
              style == cr::CreativeStructuralRoofStyle::Gable ? "Gable"
                                                               : "Flat",
              selected)) {
        settings.roofStyle = style;
        edited = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::SetNextItemWidth(112.0F);
  edited = ImGui::InputDouble("Overhang##generated_level",
                              &settings.roofOverhangCells, 0.25, 1.0,
                              "%.2f") ||
           edited;
  if (settings.roofStyle == cr::CreativeStructuralRoofStyle::Gable) {
    ImGui::SetNextItemWidth(148.0F);
    if (ImGui::BeginCombo(
            "Ridge##generated_level",
            settings.roofRidgeAxis == cr::CreativeStructuralRoofRidgeAxis::Z
                ? "Z axis"
                : "X axis")) {
      for (const cr::CreativeStructuralRoofRidgeAxis axis :
           {cr::CreativeStructuralRoofRidgeAxis::X,
            cr::CreativeStructuralRoofRidgeAxis::Z}) {
        const bool selected = settings.roofRidgeAxis == axis;
        if (ImGui::Selectable(
                axis == cr::CreativeStructuralRoofRidgeAxis::Z ? "Z axis"
                                                                : "X axis",
                selected)) {
          settings.roofRidgeAxis = axis;
          edited = true;
        }
        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::SetNextItemWidth(112.0F);
    edited = ImGui::InputDouble("Pitch##generated_level",
                                &settings.roofPitchDegrees, 1.0, 5.0,
                                "%.1f deg") ||
             edited;
  }
  ImGui::EndDisabled();

  edited = wallHeightEdited || floorLayersEdited || ceilingLayersEdited ||
           roofLayersEdited || edited;
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
          settings.roofPitchDegrees, settings.roofOverhangCells) &&
      settings.roofOverhangCells <=
          cr::kMaximumCreativeWorldLayoutRoofOverhangCells;
  if (!representable) {
    ImGui::TextColored({0.94F, 0.45F, 0.32F, 1.0F},
                       "Level shell settings are outside valid bounds");
  }
  if (edited) {
    if (representable) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutPreviewGeneratedLevelSettings,
          CreativeDesktopGeneratedLevelSettingsPayload{
              objectId, provenance.index, level.stableKey, settings});
    } else if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }

  const bool dirty = !(current == settings);
  ImGui::BeginDisabled(disabled || !dirty || !representable);
  if (ImGui::Button("Update level shell in 3D")) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutApplyGeneratedLevelSettings,
        CreativeDesktopGeneratedLevelSettingsPayload{
            objectId, provenance.index, level.stableKey, settings});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(disabled || !dirty);
  if (ImGui::Button("Reset##generated_level")) {
    worldLayout.generatedLevelSettingsDraft.settings = current;
    if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }
  ImGui::EndDisabled();
}

}  // namespace iggy3d_creative_app
