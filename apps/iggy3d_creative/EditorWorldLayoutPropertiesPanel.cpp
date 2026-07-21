#include "EditorWorldLayoutPanelInternal.hpp"

#include "EditorDesktopWidgets.hpp"
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
    state.roomSettingsDraft = {};
    return;
  }

  const std::size_t roomIndex = state.selection.index;
  CreativeEditorWorldLayoutRoomSettings current;
  if (!readCreativeEditorWorldLayoutRoomSettings(state, roomIndex, current)) {
    state.roomSettingsDraft = {};
    ImGui::TextDisabled("Room level is unavailable");
    return;
  }
  if (!state.roomSettingsDraft.active ||
      state.roomSettingsDraft.roomIndex != roomIndex ||
      state.roomSettingsDraft.sourceRevision != state.revision) {
    state.roomSettingsDraft = {true, roomIndex, state.revision, current};
  }

  const cr::CreativeWorldLayoutRoom& room = state.source.rooms[roomIndex];
  CreativeEditorWorldLayoutRoomSettings& settings =
      state.roomSettingsDraft.settings;
  const std::int64_t widthCells =
      static_cast<std::int64_t>(settings.footprint.maximum.x) -
      settings.footprint.minimum.x;
  const std::int64_t depthCells =
      static_cast<std::int64_t>(settings.footprint.maximum.z) -
      settings.footprint.minimum.z;
  int width = static_cast<int>(std::min<std::int64_t>(
      widthCells, std::numeric_limits<int>::max()));
  int depth = static_cast<int>(std::min<std::int64_t>(
      depthCells, std::numeric_limits<int>::max()));
  double floorTopLayer = settings.floorTopLayer;
  int wallHeight = settings.wallHeightCells;
  double wallThickness = settings.wallThicknessCells;
  int floorLayers = settings.floorThicknessLayers;
  int roofLayers = settings.roofThicknessLayers;
  int roofStyle = static_cast<int>(settings.roofStyle);
  int roofRidgeAxis = static_cast<int>(settings.roofRidgeAxis);
  double roofPitch = settings.roofPitchDegrees;
  double roofOverhang = settings.roofOverhangCells;
  CreativeDesktopPropertyEditActivity activity;
  bool continuousChanged = false;
  const auto observe = [&](bool changed) {
    continuousChanged = continuousChanged || changed;
    observeCreativeDesktopContinuousPropertyWidget(activity, changed);
  };

  ImGui::BeginDisabled(state.roomManipulation.active);
  ImGui::SetNextItemWidth(128.0F);
  observe(ImGui::InputInt("Width##room_shell", &width, 1, 4));
  ImGui::SetNextItemWidth(128.0F);
  observe(ImGui::InputInt("Depth##room_shell", &depth, 1, 4));
  ImGui::SetNextItemWidth(128.0F);
  observe(ImGui::InputDouble("Level floor##room_shell", &floorTopLayer, 0.5,
                             1.0, "%.3f"));

  ImGui::SetNextItemWidth(128.0F);
  observe(ImGui::InputInt("Level wall height##room_shell", &wallHeight, 1,
                          4));
  ImGui::SetNextItemWidth(128.0F);
  observe(ImGui::InputDouble("Room wall##room_shell", &wallThickness, 0.05,
                             0.25, "%.3f"));
  ImGui::SetNextItemWidth(128.0F);
  observe(ImGui::InputInt("Level floor layers##room_shell", &floorLayers, 1,
                          2));

  ImGui::SeparatorText("Level roof");
  ImGui::SetNextItemWidth(128.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      ImGui::Combo("Style##room_roof", &roofStyle, "Flat\0Gable\0"));
  ImGui::SetNextItemWidth(128.0F);
  observe(ImGui::InputInt("Layers##room_roof", &roofLayers, 1, 2));
  ImGui::SetNextItemWidth(128.0F);
  observe(ImGui::InputDouble("Overhang##room_roof", &roofOverhang, 0.25,
                             1.0, "%.2f"));
  if (roofStyle == static_cast<int>(cr::CreativeStructuralRoofStyle::Gable)) {
    ImGui::SetNextItemWidth(128.0F);
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        ImGui::Combo("Ridge##room_roof", &roofRidgeAxis,
                     "X axis\0Z axis\0"));
    ImGui::SetNextItemWidth(128.0F);
    observe(ImGui::InputDouble("Pitch##room_roof", &roofPitch, 1.0, 5.0,
                               "%.1f deg"));
  }
  ImGui::EndDisabled();

  const std::int64_t maximumX =
      static_cast<std::int64_t>(settings.footprint.minimum.x) + width;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(settings.footprint.minimum.z) + depth;
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
  }

  if (continuousChanged && valid) {
    settings.footprint.maximum.x = static_cast<std::int32_t>(maximumX);
    settings.footprint.maximum.z = static_cast<std::int32_t>(maximumZ);
    settings.floorTopLayer = floorTopLayer;
    settings.wallHeightCells = static_cast<std::uint16_t>(wallHeight);
    settings.wallThicknessCells = wallThickness;
    settings.floorThicknessLayers = static_cast<std::uint16_t>(floorLayers);
    settings.roofThicknessLayers = static_cast<std::uint16_t>(roofLayers);
    settings.roofPitchDegrees = roofPitch;
    settings.roofOverhangCells = roofOverhang;
  }
  if (valid) {
    settings.roofStyle =
        static_cast<cr::CreativeStructuralRoofStyle>(roofStyle);
    settings.roofRidgeAxis =
        static_cast<cr::CreativeStructuralRoofRidgeAxis>(roofRidgeAxis);
  }

  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, settings, valid, "Reset room", state,
      roomIndex, room.stableKey, commands);
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
  CreativeDesktopPropertyEditActivity activity;
  ImGui::TextUnformatted(isDoor ? "Door settings" : "Window settings");
  if (!opening.insertAssetId.empty()) {
    ImGui::TextDisabled("Asset: %s", opening.insertAssetId.c_str());
    if (!catalogContainsAsset(catalog, opening.insertAssetId)) {
      ImGui::TextColored({1.0F, 0.72F, 0.20F, 1.0F},
                         "Asset unavailable: procedural preview");
    }
  }
  ImGui::BeginDisabled(state.openingManipulation.active);
  ImGui::SetNextItemWidth(128.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Offset##opening", &settings.centerOffsetCells,
                         0.25, 1.0, "%.2f"));
  ImGui::SetNextItemWidth(128.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Width##opening", &settings.widthCells, 0.25, 1.0,
                         "%.2f"));
  ImGui::SetNextItemWidth(128.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Height##opening", &settings.heightCells, 0.25, 1.0,
                         "%.2f"));

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
      observeCreativeDesktopDiscretePropertyWidget(activity, true);
    }
  } else {
    settings.pose = cr::CreativeBuildingOpeningPose::Closed;
    ImGui::SetNextItemWidth(128.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputDouble("Sill##opening", &settings.sillHeightCells, 0.25,
                           1.0, "%.2f"));
  }
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      ImGui::Checkbox("Insert##opening", &settings.includeInsert));
  ImGui::EndDisabled();

  const bool dirty = !(current == settings);
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
  const bool valuesRepresentable =
      std::isfinite(settings.centerOffsetCells) &&
      std::isfinite(settings.widthCells) &&
      std::isfinite(settings.sillHeightCells) &&
      std::isfinite(settings.heightCells) && settings.widthCells > 0.0 &&
      settings.sillHeightCells >= 0.0 && settings.heightCells > 0.0 &&
      settings.pose <=
          cr::CreativeBuildingOpeningPose::OpenFromEndPositiveNormal;
  if (!valuesRepresentable) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "opening dimensions must be finite and positive");
  }
  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, settings, valuesRepresentable, "Reset opening",
      state, openingIndex, opening.stableKey, commands);
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
