#include "EditorDesktopWidgets.hpp"

#include "EditorDesktopWorldLayoutInspector.hpp"

#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

bool sameRoomSettings(
    const CreativeEditorWorldLayoutRoomSettings& lhs,
    const CreativeEditorWorldLayoutRoomSettings& rhs) noexcept {
  return lhs.footprint.minimum == rhs.footprint.minimum &&
         lhs.footprint.maximum == rhs.footprint.maximum &&
         lhs.floorTopLayer == rhs.floorTopLayer &&
         lhs.wallHeightCells == rhs.wallHeightCells &&
         lhs.wallThicknessCells == rhs.wallThicknessCells &&
         lhs.floorThicknessLayers == rhs.floorThicknessLayers &&
         lhs.roofThicknessLayers == rhs.roofThicknessLayers &&
         lhs.roofStyle == rhs.roofStyle &&
         lhs.roofRidgeAxis == rhs.roofRidgeAxis &&
         lhs.roofPitchDegrees == rhs.roofPitchDegrees &&
         lhs.roofOverhangCells == rhs.roofOverhangCells;
}

}  // namespace

void appendCreativeDesktopGeneratedRoomSettings(
    CreativeEditorWorldLayoutState& worldLayout,
    cr::CreativeObjectId objectId,
    cr::CreativeWorldLayoutObjectProvenance provenance,
    bool disabled,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutRoomSettings current;
  if (provenance.table != cr::CreativeWorldLayoutTable::Room ||
      provenance.index >= worldLayout.source.rooms.size() ||
      !readCreativeEditorWorldLayoutRoomSettings(
          worldLayout, provenance.index, current)) {
    return;
  }
  const cr::CreativeWorldLayoutRoom& room =
      worldLayout.source.rooms[provenance.index];
  if (room.levelIndex >= worldLayout.source.levels.size()) {
    return;
  }
  const bool draftChanged =
      !worldLayout.roomSettingsDraft.active ||
      worldLayout.roomSettingsDraft.roomIndex != provenance.index ||
      worldLayout.roomSettingsDraft.sourceRevision != worldLayout.revision;
  if (draftChanged) {
    if (worldLayout.roomSettingsDraft.active &&
        creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
    worldLayout.roomSettingsDraft =
        {true, provenance.index, worldLayout.revision, current};
  }

  const cr::CreativeWorldLayoutLevel& level =
      worldLayout.source.levels[room.levelIndex];
  const std::size_t levelRoomCount = static_cast<std::size_t>(std::count_if(
      worldLayout.source.rooms.begin(), worldLayout.source.rooms.end(),
      [&room](const cr::CreativeWorldLayoutRoom& candidate) {
        return candidate.buildingIndex == room.buildingIndex &&
               candidate.levelIndex == room.levelIndex;
      }));
  ImGui::TextDisabled("%s  |  %s", room.name.c_str(), level.name.c_str());
  if (provenance.roomEdge < cr::CreativeWorldLayoutRoomEdge::Count) {
    const std::string edgeLabel(cr::toString(provenance.roomEdge));
    if (provenance.contributorCount > 1U) {
      ImGui::TextColored({0.94F, 0.72F, 0.28F, 1.0F},
                         "Shared %s edge; editing %s", edgeLabel.c_str(),
                         room.name.c_str());
    } else {
      ImGui::TextDisabled("%s shell fragment", edgeLabel.c_str());
    }
  }

  CreativeEditorWorldLayoutRoomSettings& settings =
      worldLayout.roomSettingsDraft.settings;
  std::array<int, 2U> origin = {settings.footprint.minimum.x,
                                settings.footprint.minimum.z};
  const std::int64_t width64 =
      static_cast<std::int64_t>(settings.footprint.maximum.x) -
      settings.footprint.minimum.x;
  const std::int64_t depth64 =
      static_cast<std::int64_t>(settings.footprint.maximum.z) -
      settings.footprint.minimum.z;
  std::array<int, 2U> size = {
      static_cast<int>(std::clamp<std::int64_t>(
          width64, std::numeric_limits<int>::min(),
          std::numeric_limits<int>::max())),
      static_cast<int>(std::clamp<std::int64_t>(
          depth64, std::numeric_limits<int>::min(),
          std::numeric_limits<int>::max()))};
  int wallHeight = settings.wallHeightCells;
  int floorLayers = settings.floorThicknessLayers;
  int roofLayers = settings.roofThicknessLayers;

  bool edited = false;
  ImGui::BeginDisabled(disabled);
  ImGui::SeparatorText("Room footprint");
  ImGui::SetNextItemWidth(188.0F);
  const bool originEdited =
      ImGui::InputInt2("Origin X/Z##generated_room", origin.data());
  ImGui::SetNextItemWidth(188.0F);
  const bool sizeEdited =
      ImGui::InputInt2("Size W/D##generated_room", size.data());
  ImGui::SetNextItemWidth(112.0F);
  edited = ImGui::InputDouble("Wall thickness##generated_room",
                              &settings.wallThicknessCells, 0.05, 0.25,
                              "%.3f") ||
           edited;

  ImGui::SeparatorText("Level-wide shell");
  ImGui::TextDisabled("Affects %zu room%s on this level", levelRoomCount,
                      levelRoomCount == 1U ? "" : "s");
  ImGui::SetNextItemWidth(112.0F);
  edited = ImGui::InputDouble("Floor top##generated_room",
                              &settings.floorTopLayer, 0.5, 1.0, "%.2f") ||
           edited;
  ImGui::SetNextItemWidth(112.0F);
  const bool wallHeightEdited = ImGui::InputInt(
      "Wall height##generated_room", &wallHeight, 1, 2);
  ImGui::SetNextItemWidth(112.0F);
  const bool floorLayersEdited = ImGui::InputInt(
      "Floor layers##generated_room", &floorLayers, 1, 2);

  ImGui::SeparatorText("Level roof");
  ImGui::SetNextItemWidth(148.0F);
  if (ImGui::BeginCombo(
          "Style##generated_room",
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
  const bool roofLayersEdited = ImGui::InputInt(
      "Roof layers##generated_room", &roofLayers, 1, 2);
  ImGui::SetNextItemWidth(112.0F);
  edited = ImGui::InputDouble("Overhang##generated_room",
                              &settings.roofOverhangCells, 0.25, 1.0,
                              "%.2f") ||
           edited;
  if (settings.roofStyle == cr::CreativeStructuralRoofStyle::Gable) {
    ImGui::SetNextItemWidth(148.0F);
    if (ImGui::BeginCombo(
            "Ridge##generated_room",
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
    edited = ImGui::InputDouble("Pitch##generated_room",
                                &settings.roofPitchDegrees, 1.0, 5.0,
                                "%.1f deg") ||
             edited;
  }
  ImGui::EndDisabled();

  edited = originEdited || sizeEdited || wallHeightEdited ||
           floorLayersEdited || roofLayersEdited || edited;
  const std::int64_t maximumX =
      static_cast<std::int64_t>(origin[0]) + size[0];
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(origin[1]) + size[1];
  const bool footprintRepresentable =
      maximumX >= std::numeric_limits<std::int32_t>::min() &&
      maximumX <= std::numeric_limits<std::int32_t>::max() &&
      maximumZ >= std::numeric_limits<std::int32_t>::min() &&
      maximumZ <= std::numeric_limits<std::int32_t>::max() && size[0] > 0 &&
      size[1] > 0;
  const bool layersRepresentable =
      wallHeight > 0 && wallHeight <= 65535 && floorLayers > 0 &&
      floorLayers <= 65535 && roofLayers > 0 && roofLayers <= 65535;
  if ((originEdited || sizeEdited) && footprintRepresentable) {
    settings.footprint = {
        {static_cast<std::int32_t>(origin[0]),
         static_cast<std::int32_t>(origin[1])},
        {static_cast<std::int32_t>(maximumX),
         static_cast<std::int32_t>(maximumZ)}};
  }
  if (layersRepresentable) {
    settings.wallHeightCells = static_cast<std::uint16_t>(wallHeight);
    settings.floorThicknessLayers =
        static_cast<std::uint16_t>(floorLayers);
    settings.roofThicknessLayers = static_cast<std::uint16_t>(roofLayers);
  }
  const bool representable =
      footprintRepresentable && layersRepresentable &&
      std::isfinite(settings.floorTopLayer) &&
      std::isfinite(settings.wallThicknessCells) &&
      settings.wallThicknessCells > 0.0 &&
      static_cast<double>(size[0]) > settings.wallThicknessCells * 2.0 &&
      static_cast<double>(size[1]) > settings.wallThicknessCells * 2.0 &&
      cr::validCreativeStructuralRoofSettings(
          settings.roofStyle, settings.roofRidgeAxis,
          settings.roofPitchDegrees, settings.roofOverhangCells) &&
      settings.roofOverhangCells <=
          cr::kMaximumCreativeWorldLayoutRoofOverhangCells;
  if (!representable) {
    ImGui::TextColored({0.94F, 0.45F, 0.32F, 1.0F},
                       "Room shell settings are outside valid bounds");
  }
  if (edited) {
    if (representable) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutPreviewGeneratedRoomSettings,
          CreativeDesktopGeneratedRoomSettingsPayload{
              objectId, provenance.index, room.stableKey, settings});
    } else if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }

  const bool dirty = !sameRoomSettings(current, settings);
  ImGui::BeginDisabled(disabled || !dirty || !representable ||
                       worldLayout.roomManipulation.active);
  if (ImGui::Button("Update room shell in 3D")) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutApplyGeneratedRoomSettings,
        CreativeDesktopGeneratedRoomSettingsPayload{
            objectId, provenance.index, room.stableKey, settings});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(disabled || (!dirty && representable));
  if (ImGui::Button("Reset##generated_room")) {
    worldLayout.roomSettingsDraft.settings = current;
    if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.push(CreativeDesktopCommandId::
                        WorldLayoutCancelGeneratedSettingsPreview);
    }
  }
  ImGui::EndDisabled();
}

}  // namespace iggy3d_creative_app
