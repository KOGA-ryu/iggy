#include "EditorDesktopWorldLayoutInspector.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

bool sameBoxSettings(const CreativeEditorWorldLayoutBoxSettings& lhs,
                     const CreativeEditorWorldLayoutBoxSettings& rhs) noexcept {
  return lhs.footprint.minimum == rhs.footprint.minimum &&
         lhs.footprint.maximum == rhs.footprint.maximum &&
         lhs.baseLayer == rhs.baseLayer && lhs.heightCells == rhs.heightCells;
}

bool sameWallSettings(
    const CreativeEditorWorldLayoutWallSettings& lhs,
    const CreativeEditorWorldLayoutWallSettings& rhs) noexcept {
  return lhs.start == rhs.start && lhs.end == rhs.end &&
         lhs.baseLayer == rhs.baseLayer &&
         lhs.heightCells == rhs.heightCells &&
         lhs.thicknessCells == rhs.thicknessCells;
}

void drawBuildingActions(CreativeEditorWorldLayoutState& state,
                         CreativeDesktopCommandFrame& commands) {
  const std::size_t buildingIndex =
      creativeEditorWorldLayoutSelectedBuilding(state);
  if (buildingIndex >= state.source.buildings.size()) {
    return;
  }
  const cr::CreativeWorldLayoutBuilding& building =
      state.source.buildings[buildingIndex];
  ImGui::Text("Building: %s", building.name.c_str());
  ImGui::SameLine();
  if (state.selection.kind !=
      CreativeEditorWorldLayoutSelectionKind::Building) {
    if (ImGui::Button("Select building")) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutSelectBuilding,
          CreativeDesktopWorldLayoutBuildingSelectionPayload{buildingIndex});
    }
    ImGui::Separator();
    return;
  }

  std::uint64_t roomCount = 0U;
  std::uint64_t boxCount = 0U;
  std::uint64_t wallCount = 0U;
  for (const cr::CreativeWorldLayoutRoom& room : state.source.rooms) {
    roomCount += room.buildingIndex == buildingIndex ? 1U : 0U;
  }
  for (const cr::CreativeWorldLayoutBox& box : state.source.boxes) {
    boxCount += box.buildingIndex == buildingIndex ? 1U : 0U;
  }
  for (const cr::CreativeWorldLayoutWall& wall : state.source.walls) {
    wallCount += wall.buildingIndex == buildingIndex ? 1U : 0U;
  }
  ImGui::TextDisabled("rooms %llu  floors %llu  partitions %llu",
                      static_cast<unsigned long long>(roomCount),
                      static_cast<unsigned long long>(boxCount),
                      static_cast<unsigned long long>(wallCount));

  std::int64_t deltaXCells = 0;
  std::int64_t deltaZCells = 0;
  const bool canDuplicate =
      defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
          state, buildingIndex, deltaXCells, deltaZCells);
  ImGui::BeginDisabled(!canDuplicate || state.buildingManipulation.active);
  if (ImGui::Button("Duplicate building")) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutDuplicateBuilding,
        CreativeDesktopWorldLayoutBuildingDuplicatePayload{
            buildingIndex, deltaXCells, deltaZCells});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(state.buildingManipulation.active);
  if (ImGui::Button("Edit contents")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutClearSelection);
  }
  ImGui::EndDisabled();
  ImGui::Separator();
}

void drawBoxSettings(CreativeEditorWorldLayoutState& state,
                     CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Box ||
      state.selection.index >= state.source.boxes.size()) {
    state.boxSettingsDraft = {};
    return;
  }
  const std::size_t boxIndex = state.selection.index;
  CreativeEditorWorldLayoutBoxSettings current;
  if (!readCreativeEditorWorldLayoutBoxSettings(state, boxIndex, current)) {
    state.boxSettingsDraft = {};
    return;
  }
  if (!state.boxSettingsDraft.active ||
      state.boxSettingsDraft.boxIndex != boxIndex ||
      state.boxSettingsDraft.sourceRevision != state.revision) {
    state.boxSettingsDraft = {true, boxIndex, state.revision, current};
  }

  CreativeEditorWorldLayoutBoxSettings& settings =
      state.boxSettingsDraft.settings;
  int originX = settings.footprint.minimum.x;
  int originZ = settings.footprint.minimum.z;
  const std::int64_t width64 =
      static_cast<std::int64_t>(settings.footprint.maximum.x) -
      settings.footprint.minimum.x;
  const std::int64_t depth64 =
      static_cast<std::int64_t>(settings.footprint.maximum.z) -
      settings.footprint.minimum.z;
  int width = static_cast<int>(std::clamp<std::int64_t>(
      width64, std::numeric_limits<int>::min(),
      std::numeric_limits<int>::max()));
  int depth = static_cast<int>(std::clamp<std::int64_t>(
      depth64, std::numeric_limits<int>::min(),
      std::numeric_limits<int>::max()));
  int baseLayer = settings.baseLayer;
  int height = settings.heightCells;
  ImGui::TextUnformatted(state.source.boxes[boxIndex].kind ==
                                 cr::CreativeObjectKind::Floor
                             ? "Floor settings"
                             : "Surface settings");
  ImGui::SetNextItemWidth(84.0F);
  bool edited = ImGui::InputInt("X##layout_box", &originX, 1, 4);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  edited = ImGui::InputInt("Z##layout_box", &originZ, 1, 4) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  edited = ImGui::InputInt("Width##layout_box", &width, 1, 4) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  edited = ImGui::InputInt("Depth##layout_box", &depth, 1, 4) || edited;

  ImGui::SetNextItemWidth(84.0F);
  edited = ImGui::InputInt("Base##layout_box", &baseLayer, 1, 4) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(84.0F);
  edited = ImGui::InputInt("Layers##layout_box", &height, 1, 2) || edited;

  const std::int64_t maximumX =
      static_cast<std::int64_t>(originX) + width;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(originZ) + depth;
  const bool valuesRepresentable =
      maximumX >= std::numeric_limits<std::int32_t>::min() &&
      maximumX <= std::numeric_limits<std::int32_t>::max() &&
      maximumZ >= std::numeric_limits<std::int32_t>::min() &&
      maximumZ <= std::numeric_limits<std::int32_t>::max() && width > 0 &&
      depth > 0 && height > 0 &&
      height <= std::numeric_limits<std::uint16_t>::max();
  if (edited && valuesRepresentable) {
    settings.footprint.minimum = {static_cast<std::int32_t>(originX),
                                  static_cast<std::int32_t>(originZ)};
    settings.footprint.maximum = {static_cast<std::int32_t>(maximumX),
                                  static_cast<std::int32_t>(maximumZ)};
    settings.baseLayer = static_cast<std::int32_t>(baseLayer);
    settings.heightCells = static_cast<std::uint16_t>(height);
  }
  if (!valuesRepresentable) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "floor dimensions must be positive and in range");
  }

  const bool dirty = !sameBoxSettings(current, settings);
  ImGui::BeginDisabled(!dirty || !valuesRepresentable ||
                       state.boxManipulation.active);
  if (ImGui::Button("Apply floor")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutSetBoxSettings,
                  CreativeDesktopWorldLayoutBoxSettingsPayload{boxIndex,
                                                               settings});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!dirty);
  if (ImGui::Button("Reset floor")) {
    state.boxSettingsDraft.settings = current;
  }
  ImGui::EndDisabled();
}

void drawWallSettings(CreativeEditorWorldLayoutState& state,
                      CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Wall ||
      state.selection.index >= state.source.walls.size()) {
    state.wallSettingsDraft = {};
    return;
  }
  const std::size_t wallIndex = state.selection.index;
  CreativeEditorWorldLayoutWallSettings current;
  if (!readCreativeEditorWorldLayoutWallSettings(state, wallIndex, current)) {
    state.wallSettingsDraft = {};
    return;
  }
  if (!state.wallSettingsDraft.active ||
      state.wallSettingsDraft.wallIndex != wallIndex ||
      state.wallSettingsDraft.sourceRevision != state.revision) {
    state.wallSettingsDraft = {true, wallIndex, state.revision, current};
  }

  CreativeEditorWorldLayoutWallSettings& settings =
      state.wallSettingsDraft.settings;
  int startX = settings.start.x;
  int startZ = settings.start.z;
  int endX = settings.end.x;
  int endZ = settings.end.z;
  double baseLayer = settings.baseLayer;
  int height = settings.heightCells;
  double thicknessCells = settings.thicknessCells;
  ImGui::TextUnformatted("Partition settings");
  ImGui::SetNextItemWidth(82.0F);
  bool edited = ImGui::InputInt("Start X##layout_wall", &startX, 1, 4);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(82.0F);
  edited = ImGui::InputInt("Start Z##layout_wall", &startZ, 1, 4) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(82.0F);
  edited = ImGui::InputInt("End X##layout_wall", &endX, 1, 4) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(82.0F);
  edited = ImGui::InputInt("End Z##layout_wall", &endZ, 1, 4) || edited;

  ImGui::SetNextItemWidth(92.0F);
  edited = ImGui::InputDouble("Base##layout_wall", &baseLayer, 0.5, 1.0,
                              "%.2f") ||
           edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(92.0F);
  edited = ImGui::InputInt("Height##layout_wall", &height, 1, 2) || edited;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(110.0F);
  edited = ImGui::InputDouble("Thickness##layout_wall", &thicknessCells, 0.05,
                              0.25, "%.3f") ||
           edited;

  const bool valuesRepresentable =
      height > 0 && height <= std::numeric_limits<std::uint16_t>::max() &&
      ((startX == endX) != (startZ == endZ)) &&
      std::isfinite(baseLayer) && std::isfinite(thicknessCells) &&
      thicknessCells > 0.0;
  if (edited && valuesRepresentable) {
    settings.start = {static_cast<std::int32_t>(startX),
                      static_cast<std::int32_t>(startZ)};
    settings.end = {static_cast<std::int32_t>(endX),
                    static_cast<std::int32_t>(endZ)};
    settings.baseLayer = baseLayer;
    settings.heightCells = static_cast<std::uint16_t>(height);
    settings.thicknessCells = thicknessCells;
  }
  if (!valuesRepresentable) {
    ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                       "partition must be cardinal, positive, and in range");
  }

  const bool dirty = !sameWallSettings(current, settings);
  ImGui::BeginDisabled(!dirty || !valuesRepresentable ||
                       state.wallManipulation.active);
  if (ImGui::Button("Apply partition")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutSetWallSettings,
                  CreativeDesktopWorldLayoutWallSettingsPayload{wallIndex,
                                                                settings});
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!dirty);
  if (ImGui::Button("Reset partition")) {
    state.wallSettingsDraft.settings = current;
  }
  ImGui::EndDisabled();
}

}  // namespace

void drawCreativeEditorWorldLayoutStructureInspector(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands) {
  drawBuildingActions(state, commands);
  drawBoxSettings(state, commands);
  drawWallSettings(state, commands);
}

}  // namespace iggy3d_creative_app
