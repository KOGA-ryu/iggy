#include "EditorWorldLayoutPanelInternal.hpp"

#include "EditorWorldLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

void drawSelectedRoomSettings(CreativeEditorWorldLayoutState& state,
                              CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Room ||
      state.selection.index >= state.source.rooms.size()) {
    return;
  }

  const cr::CreativeWorldLayoutRoom& room =
      state.source.rooms[state.selection.index];
  const cr::CreativeWorldLayoutLevel* level =
      cr::creativeWorldLayoutLevelForRoom(state.source,
                                          state.selection.index);
  if (level == nullptr) {
    ImGui::TextDisabled("Room level is unavailable");
    return;
  }
  const std::int64_t widthCells =
      static_cast<std::int64_t>(room.footprint.maximum.x) -
      room.footprint.minimum.x;
  const std::int64_t depthCells =
      static_cast<std::int64_t>(room.footprint.maximum.z) -
      room.footprint.minimum.z;
  int width = static_cast<int>(std::min<std::int64_t>(
      widthCells, std::numeric_limits<int>::max()));
  int depth = static_cast<int>(std::min<std::int64_t>(
      depthCells, std::numeric_limits<int>::max()));
  double floorTopLayer = level->floorTopLayer;
  int wallHeight = level->wallHeightCells;
  double wallThickness = room.wallThicknessCells;
  int floorLayers = level->floorThicknessLayers;
  int roofLayers = level->roofThicknessLayers;
  int roofStyle = static_cast<int>(level->roofStyle);
  int roofRidgeAxis = static_cast<int>(level->roofRidgeAxis);
  double roofPitch = level->roofPitchDegrees;
  double roofOverhang = level->roofOverhangCells;

  ImGui::SetNextItemWidth(128.0F);
  bool changed = ImGui::InputInt("Width##room_shell", &width, 1, 4);
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputInt("Depth##room_shell", &depth, 1, 4) || changed;
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputDouble("Level floor##room_shell", &floorTopLayer, 0.5,
                               1.0, "%.3f") ||
            changed;

  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputInt("Level wall height##room_shell", &wallHeight, 1,
                            4) ||
            changed;
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputDouble("Room wall##room_shell", &wallThickness,
                               0.05, 0.25, "%.3f") ||
            changed;
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputInt("Level floor layers##room_shell", &floorLayers, 1,
                            2) ||
            changed;

  ImGui::SeparatorText("Level roof");
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::Combo("Style##room_roof", &roofStyle,
                         "Flat\0Gable\0") ||
            changed;
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputInt("Layers##room_roof", &roofLayers, 1, 2) ||
            changed;
  ImGui::SetNextItemWidth(128.0F);
  changed = ImGui::InputDouble("Overhang##room_roof", &roofOverhang, 0.25,
                               1.0, "%.2f") ||
            changed;
  if (roofStyle == static_cast<int>(cr::CreativeStructuralRoofStyle::Gable)) {
    ImGui::SetNextItemWidth(128.0F);
    changed = ImGui::Combo("Ridge##room_roof", &roofRidgeAxis,
                           "X axis\0Z axis\0") ||
              changed;
    ImGui::SetNextItemWidth(128.0F);
    changed = ImGui::InputDouble("Pitch##room_roof", &roofPitch, 1.0, 5.0,
                                 "%.1f deg") ||
              changed;
  }

  if (!changed) {
    return;
  }

  const std::int64_t maximumX =
      static_cast<std::int64_t>(room.footprint.minimum.x) + width;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(room.footprint.minimum.z) + depth;
  const int maximumLayerCount = std::numeric_limits<std::uint16_t>::max();
  const bool valid =
      width > 0 && depth > 0 &&
      maximumX <= std::numeric_limits<std::int32_t>::max() &&
      maximumZ <= std::numeric_limits<std::int32_t>::max() &&
      wallHeight > 0 && wallHeight <= maximumLayerCount &&
      floorLayers > 0 && floorLayers <= maximumLayerCount &&
      roofLayers > 0 && roofLayers <= maximumLayerCount &&
      std::isfinite(floorTopLayer) && std::isfinite(wallThickness) &&
      roofStyle >= 0 &&
      roofStyle < static_cast<int>(cr::CreativeStructuralRoofStyle::Count) &&
      roofRidgeAxis >= 0 &&
      roofRidgeAxis <
          static_cast<int>(cr::CreativeStructuralRoofRidgeAxis::Count) &&
      cr::validCreativeStructuralRoofSettings(
          static_cast<cr::CreativeStructuralRoofStyle>(roofStyle),
          static_cast<cr::CreativeStructuralRoofRidgeAxis>(roofRidgeAxis),
          roofPitch, roofOverhang) &&
      roofOverhang <= cr::kMaximumCreativeWorldLayoutRoofOverhangCells &&
      wallThickness > 0.0 &&
      static_cast<double>(width) > wallThickness * 2.0 &&
      static_cast<double>(depth) > wallThickness * 2.0;
  if (!valid) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "room shell settings are outside valid bounds");
    return;
  }

  CreativeEditorWorldLayoutRoomSettings settings;
  settings.footprint = room.footprint;
  settings.footprint.maximum.x = static_cast<std::int32_t>(maximumX);
  settings.footprint.maximum.z = static_cast<std::int32_t>(maximumZ);
  settings.floorTopLayer = floorTopLayer;
  settings.wallHeightCells = static_cast<std::uint16_t>(wallHeight);
  settings.wallThicknessCells = wallThickness;
  settings.floorThicknessLayers = static_cast<std::uint16_t>(floorLayers);
  settings.roofThicknessLayers = static_cast<std::uint16_t>(roofLayers);
  settings.roofStyle =
      static_cast<cr::CreativeStructuralRoofStyle>(roofStyle);
  settings.roofRidgeAxis =
      static_cast<cr::CreativeStructuralRoofRidgeAxis>(roofRidgeAxis);
  settings.roofPitchDegrees = roofPitch;
  settings.roofOverhangCells = roofOverhang;
  commands.push(
      CreativeDesktopCommandId::WorldLayoutSetRoomSettings,
      CreativeDesktopWorldLayoutRoomSettingsPayload{state.selection.index,
                                                    settings});
}

bool sameOpeningSettings(
    const CreativeEditorWorldLayoutOpeningSettings& lhs,
    const CreativeEditorWorldLayoutOpeningSettings& rhs) noexcept {
  return lhs.centerOffsetCells == rhs.centerOffsetCells &&
         lhs.widthCells == rhs.widthCells &&
         lhs.sillHeightCells == rhs.sillHeightCells &&
         lhs.heightCells == rhs.heightCells && lhs.pose == rhs.pose &&
         lhs.includeInsert == rhs.includeInsert;
}

bool catalogContainsAsset(const cr::CreativeCatalogState& catalog,
                          std::string_view assetId) {
  return std::any_of(
      catalog.entries.begin(), catalog.entries.end(),
      [assetId](const cr::CreativeCatalogEntry& entry) {
        return entry.category == cr::CreativeCatalogEntryCategory::Asset &&
               cr::creativeHotbarAssetId(entry.hotbarEntry) == assetId;
      });
}

void drawSelectedOpeningSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog,
    CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind !=
          CreativeEditorWorldLayoutSelectionKind::Opening ||
      state.selection.index >= state.source.openings.size()) {
    state.openingSettingsDraft = {};
    return;
  }
  const std::size_t openingIndex = state.selection.index;
  CreativeEditorWorldLayoutOpeningSettings current;
  if (!readCreativeEditorWorldLayoutOpeningSettings(state, openingIndex,
                                                    current)) {
    state.openingSettingsDraft = {};
    return;
  }
  if (!state.openingSettingsDraft.active ||
      state.openingSettingsDraft.openingIndex != openingIndex ||
      state.openingSettingsDraft.sourceRevision != state.revision) {
    state.openingSettingsDraft = {true, openingIndex, state.revision, current};
  }

  const cr::CreativeWorldLayoutOpening& opening =
      state.source.openings[openingIndex];
  CreativeEditorWorldLayoutOpeningSettings& settings =
      state.openingSettingsDraft.settings;
  const bool isDoor = opening.kind == cr::CreativeBuildingOpeningKind::Door;
  ImGui::TextUnformatted(isDoor ? "Door settings" : "Window settings");
  if (!opening.insertAssetId.empty()) {
    ImGui::TextDisabled("Asset: %s", opening.insertAssetId.c_str());
    if (!catalogContainsAsset(catalog, opening.insertAssetId)) {
      ImGui::TextColored({1.0F, 0.72F, 0.20F, 1.0F},
                         "Asset unavailable: procedural preview");
    }
  }
  ImGui::SetNextItemWidth(128.0F);
  ImGui::InputDouble("Offset##opening", &settings.centerOffsetCells, 0.25, 1.0,
                     "%.2f");
  ImGui::SetNextItemWidth(128.0F);
  ImGui::InputDouble("Width##opening", &settings.widthCells, 0.25, 1.0,
                     "%.2f");
  ImGui::SetNextItemWidth(128.0F);
  ImGui::InputDouble("Height##opening", &settings.heightCells, 0.25, 1.0,
                     "%.2f");

  if (isDoor) {
    settings.sillHeightCells = 0.0;
    constexpr std::array<const char*, 5U> kPoseLabels = {
        "Closed", "Start hinge / side A", "Start hinge / side B",
        "End hinge / side A", "End hinge / side B"};
    int pose = static_cast<int>(settings.pose);
    ImGui::SetNextItemWidth(188.0F);
    if (ImGui::Combo("Pose##opening", &pose, kPoseLabels.data(),
                     static_cast<int>(kPoseLabels.size()))) {
      settings.pose = static_cast<cr::CreativeBuildingOpeningPose>(pose);
    }
  } else {
    settings.pose = cr::CreativeBuildingOpeningPose::Closed;
    ImGui::SetNextItemWidth(128.0F);
    ImGui::InputDouble("Sill##opening", &settings.sillHeightCells, 0.25, 1.0,
                       "%.2f");
  }
  ImGui::Checkbox("Insert##opening", &settings.includeInsert);

  const bool dirty = !sameOpeningSettings(current, settings);
  if (!opening.insertAssetId.empty()) {
    ImGui::BeginDisabled(dirty || state.openingManipulation.active);
    if (ImGui::Button("Use procedural insert")) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutSetOpeningInsert,
          CreativeDesktopWorldLayoutOpeningInsertPayload{
              openingIndex,
              CreativeEditorWorldLayoutOpeningInsertOperation::
                  UseProceduralInsert,
              {},
              {1.0, 1.0, 1.0}});
    }
    ImGui::EndDisabled();
    if (dirty && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      ImGui::SetTooltip("Apply or reset opening edits first");
    }
  }
  ImGui::BeginDisabled(!dirty || state.openingManipulation.active);
  if (ImGui::Button("Apply opening")) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutSetOpeningSettings,
        CreativeDesktopWorldLayoutOpeningSettingsPayload{openingIndex,
                                                         settings});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!dirty);
  if (ImGui::Button("Reset opening")) {
    state.openingSettingsDraft.settings = current;
  }
  ImGui::EndDisabled();
}


}  // namespace

void drawCreativeEditorWorldLayoutSelectionProperties(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog,
    CreativeDesktopCommandFrame& commands) {
  drawSelectedRoomSettings(state, commands);
  drawSelectedOpeningSettings(state, catalog, commands);
}

}  // namespace iggy3d_creative_app
