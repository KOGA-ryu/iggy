#include "EditorDesktopWorldLayoutInspector.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

int inputTextResizeCallback(ImGuiInputTextCallbackData* data) {
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    auto* value = static_cast<std::string*>(data->UserData);
    value->resize(static_cast<std::size_t>(data->BufTextLen));
    data->Buf = value->data();
  }
  return 0;
}

bool inputText(const char* label, std::string& value) {
  return ImGui::InputText(label, value.data(), value.capacity() + 1U,
                          ImGuiInputTextFlags_CallbackResize,
                          inputTextResizeCallback, &value);
}

template <typename Enum, std::size_t Size, typename Label>
bool enumCombo(const char* id, Enum& value,
               const std::array<Enum, Size>& choices, Label label) {
  bool changed = false;
  const std::string preview(label(value));
  if (ImGui::BeginCombo(id, preview.c_str())) {
    for (const Enum choice : choices) {
      const bool selected = choice == value;
      const std::string choiceLabel(label(choice));
      if (ImGui::Selectable(choiceLabel.c_str(), selected)) {
        value = choice;
        changed = true;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  return changed;
}

template <std::size_t Size>
bool u16Combo(const char* id, std::uint16_t& value,
              const std::array<std::uint16_t, Size>& choices,
              const char* suffix = "") {
  const auto label = [suffix](std::uint16_t choice) {
    return std::to_string(choice) + suffix;
  };
  return enumCombo(id, value, choices, label);
}

void drawApplyReset(bool dirty, const char* applyLabel,
                    const char* resetLabel, bool& apply, bool& reset) {
  ImGui::BeginDisabled(!dirty);
  apply = ImGui::Button(applyLabel);
  ImGui::SameLine();
  reset = ImGui::Button(resetLabel);
  ImGui::EndDisabled();
}

void drawStableKey(std::string_view stableKey, std::string_view type) {
  ImGui::TextDisabled("%.*s  %.*s", static_cast<int>(type.size()),
                      type.data(), static_cast<int>(stableKey.size()),
                      stableKey.data());
}

void drawVec3Table(const char* id, const char* firstLabel,
                   cr::CreativeVec3& first, const char* secondLabel = nullptr,
                   cr::CreativeVec3* second = nullptr) {
  if (!ImGui::BeginTable(id, 4, ImGuiTableFlags_SizingStretchSame |
                                   ImGuiTableFlags_BordersInnerV)) {
    return;
  }
  ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 42.0F);
  ImGui::TableSetupColumn("X");
  ImGui::TableSetupColumn("Y");
  ImGui::TableSetupColumn("Z");
  ImGui::TableHeadersRow();
  const auto row = [](const char* label, cr::CreativeVec3& value) {
    ImGui::PushID(label);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputDouble("##x", &value.x, 0.25, 1.0, "%.3f");
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputDouble("##y", &value.y, 0.25, 1.0, "%.3f");
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputDouble("##z", &value.z, 0.25, 1.0, "%.3f");
    ImGui::PopID();
  };
  row(firstLabel, first);
  if (secondLabel != nullptr && second != nullptr) {
    row(secondLabel, *second);
  }
  ImGui::EndTable();
}

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

  ImGui::SeparatorText("Level");
  drawStableKey(level.stableKey, "Building level");
  inputText("Name##layout_level_properties", draft.name);
  ImGui::SetNextItemWidth(140.0F);
  ImGui::InputDouble("Floor top##layout_level_properties",
                     &draft.floorTopLayer, 0.25, 1.0, "%.3f");
  ImGui::SetNextItemWidth(140.0F);
  ImGui::InputScalar("Wall height##layout_level_properties",
                     ImGuiDataType_U16, &draft.wallHeightCells);

  ImGui::SeparatorText("Slabs");
  ImGui::SetNextItemWidth(120.0F);
  ImGui::InputScalar("Floor##layout_level_properties", ImGuiDataType_U16,
                     &draft.floorThicknessLayers);
  ImGui::SetNextItemWidth(120.0F);
  ImGui::InputScalar("Ceiling##layout_level_properties", ImGuiDataType_U16,
                     &draft.ceilingThicknessLayers);
  ImGui::SetNextItemWidth(120.0F);
  ImGui::InputScalar("Roof##layout_level_properties", ImGuiDataType_U16,
                     &draft.roofThicknessLayers);

  constexpr std::array roofStyles{cr::CreativeStructuralRoofStyle::Flat,
                                  cr::CreativeStructuralRoofStyle::Gable};
  constexpr std::array ridgeAxes{cr::CreativeStructuralRoofRidgeAxis::X,
                                 cr::CreativeStructuralRoofRidgeAxis::Z};
  const auto roofStyleLabel = [](cr::CreativeStructuralRoofStyle style) {
    return style == cr::CreativeStructuralRoofStyle::Gable ? "Gable" :
                                                              "Flat";
  };
  const auto ridgeLabel = [](cr::CreativeStructuralRoofRidgeAxis axis) {
    return axis == cr::CreativeStructuralRoofRidgeAxis::Z ? "Z axis" :
                                                             "X axis";
  };
  ImGui::SeparatorText("Roof shape");
  ImGui::SetNextItemWidth(140.0F);
  enumCombo("Style##layout_level_properties", draft.roofStyle, roofStyles,
            roofStyleLabel);
  ImGui::SetNextItemWidth(140.0F);
  ImGui::InputDouble("Overhang##layout_level_properties",
                     &draft.roofOverhangCells, 0.25, 1.0, "%.2f");
  if (draft.roofStyle == cr::CreativeStructuralRoofStyle::Gable) {
    ImGui::SetNextItemWidth(140.0F);
    enumCombo("Ridge##layout_level_properties", draft.roofRidgeAxis,
              ridgeAxes, ridgeLabel);
    ImGui::SetNextItemWidth(140.0F);
    ImGui::InputDouble("Pitch##layout_level_properties",
                       &draft.roofPitchDegrees, 1.0, 5.0, "%.1f deg");
  }

  bool apply = false;
  bool reset = false;
  drawApplyReset(!(current == draft), "Apply level", "Reset level", apply,
                 reset);
  if (apply) {
    commands.push(CreativeDesktopCommandId::WorldLayoutSetLevelSettings,
                  CreativeDesktopWorldLayoutLevelSettingsPayload{
                      levelIndex, level.stableKey, draft});
  } else if (reset) {
    draft = std::move(current);
  }
}

void drawTerrainProfileInspector(CreativeEditorWorldLayoutState& state,
                                 CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind !=
          CreativeEditorWorldLayoutSelectionKind::TerrainProfile ||
      state.selection.index >= state.source.terrainProfiles.size()) {
    state.terrainProfileSettingsDraft = {};
    return;
  }
  const std::size_t profileIndex = state.selection.index;
  CreativeEditorWorldLayoutTerrainProfileSettings current;
  if (!readCreativeEditorWorldLayoutTerrainProfileSettings(
          state, profileIndex, current)) {
    state.terrainProfileSettingsDraft = {};
    return;
  }
  if (!state.terrainProfileSettingsDraft.active ||
      state.terrainProfileSettingsDraft.profileIndex != profileIndex ||
      state.terrainProfileSettingsDraft.sourceRevision != state.revision) {
    state.terrainProfileSettingsDraft = {true, profileIndex, state.revision,
                                         current};
  }
  CreativeEditorWorldLayoutTerrainProfileSettings& draft =
      state.terrainProfileSettingsDraft.settings;
  const auto& profile = state.source.terrainProfiles[profileIndex];

  ImGui::SeparatorText("Terrain profile");
  drawStableKey(profile.stableKey, "Absolute height field");
  constexpr std::array kinds{cr::CreativeTerrainRecipeKind::Plateau,
                             cr::CreativeTerrainRecipeKind::Hill,
                             cr::CreativeTerrainRecipeKind::Valley,
                             cr::CreativeTerrainRecipeKind::Crater,
                             cr::CreativeTerrainRecipeKind::Ridge};
  ImGui::SetNextItemWidth(160.0F);
  enumCombo("Shape##layout_profile_properties", draft.kind, kinds,
            [](cr::CreativeTerrainRecipeKind kind) {
              return cr::toString(kind);
            });
  ImGui::SetNextItemWidth(110.0F);
  ImGui::InputScalar("Center X##layout_profile_properties",
                     ImGuiDataType_S32, &draft.center.x);
  ImGui::SetNextItemWidth(110.0F);
  ImGui::InputScalar("Center Z##layout_profile_properties",
                     ImGuiDataType_S32, &draft.center.z);
  ImGui::SetNextItemWidth(120.0F);
  ImGui::InputScalar("Base height##layout_profile_properties",
                     ImGuiDataType_U16, &draft.baseHeightCells);
  constexpr std::array<std::uint16_t, 3U> radii{2U, 4U, 8U};
  constexpr std::array<std::uint16_t, 5U> amplitudes{1U, 2U, 4U, 8U, 16U};
  constexpr std::array<std::uint16_t, 3U> spacings{1U, 2U, 4U};
  ImGui::SetNextItemWidth(140.0F);
  u16Combo("Radius##layout_profile_properties", draft.radiusCells, radii,
           " cells");
  ImGui::SetNextItemWidth(140.0F);
  u16Combo("Amplitude##layout_profile_properties", draft.amplitudeCells,
           amplitudes, " cells");
  ImGui::SetNextItemWidth(140.0F);
  u16Combo("Spacing##layout_profile_properties", draft.spacingCells, spacings,
           " cells");
  if (draft.kind == cr::CreativeTerrainRecipeKind::Ridge) {
    constexpr std::array directions{
        cr::CreativeTerrainProfileDirection::PositiveX,
        cr::CreativeTerrainProfileDirection::PositiveXPositiveZ,
        cr::CreativeTerrainProfileDirection::PositiveZ,
        cr::CreativeTerrainProfileDirection::NegativeXPositiveZ,
        cr::CreativeTerrainProfileDirection::NegativeX,
        cr::CreativeTerrainProfileDirection::NegativeXNegativeZ,
        cr::CreativeTerrainProfileDirection::NegativeZ,
        cr::CreativeTerrainProfileDirection::PositiveXNegativeZ};
    ImGui::SetNextItemWidth(180.0F);
    enumCombo("Direction##layout_profile_properties", draft.direction,
              directions, [](cr::CreativeTerrainProfileDirection direction) {
                return cr::toString(direction);
              });
  }
  constexpr std::array<std::uint8_t, 2U> frequencies{1U, 2U};
  ImGui::SetNextItemWidth(140.0F);
  enumCombo("Frequency##layout_profile_properties", draft.frequency,
            frequencies, [](std::uint8_t frequency) {
              return std::to_string(static_cast<unsigned>(frequency)) +
                     (frequency == 1U ? " cycle" : " cycles");
            });
  ImGui::TextDisabled("Blend: Set  Rods: Fill");

  bool apply = false;
  bool reset = false;
  drawApplyReset(!(current == draft), "Apply terrain", "Reset terrain",
                 apply, reset);
  if (apply) {
    commands.push(
        CreativeDesktopCommandId::WorldLayoutSetTerrainProfileSettings,
        CreativeDesktopWorldLayoutTerrainProfileSettingsPayload{
            profileIndex, profile.stableKey, draft});
  } else if (reset) {
    draft = current;
  }
}

void drawTerrainPathInspector(CreativeEditorWorldLayoutState& state,
                              CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind !=
          CreativeEditorWorldLayoutSelectionKind::TerrainPath ||
      state.selection.index >= state.source.terrainPaths.size()) {
    state.terrainPathSettingsDraft = {};
    return;
  }
  const std::size_t pathIndex = state.selection.index;
  CreativeEditorWorldLayoutTerrainPathSettings current;
  if (!readCreativeEditorWorldLayoutTerrainPathSettings(state, pathIndex,
                                                        current)) {
    state.terrainPathSettingsDraft = {};
    return;
  }
  if (!state.terrainPathSettingsDraft.active ||
      state.terrainPathSettingsDraft.pathIndex != pathIndex ||
      state.terrainPathSettingsDraft.sourceRevision != state.revision) {
    state.terrainPathSettingsDraft = {true, pathIndex, state.revision,
                                      current};
  }
  CreativeEditorWorldLayoutTerrainPathSettings& draft =
      state.terrainPathSettingsDraft.settings;
  const auto& path = state.source.terrainPaths[pathIndex];

  ImGui::SeparatorText("Terrain path");
  drawStableKey(path.stableKey, "Polyline recipe");
  constexpr std::array kinds{cr::CreativeTerrainRecipeKind::Road,
                             cr::CreativeTerrainRecipeKind::River,
                             cr::CreativeTerrainRecipeKind::Ditch,
                             cr::CreativeTerrainRecipeKind::RidgeLine};
  constexpr std::array elevations{cr::CreativeTerrainPathElevation::Level,
                                  cr::CreativeTerrainPathElevation::Grade};
  ImGui::SetNextItemWidth(150.0F);
  enumCombo("Type##layout_path_properties", draft.kind, kinds,
            [](cr::CreativeTerrainRecipeKind kind) {
              return cr::toString(kind);
            });
  ImGui::SetNextItemWidth(150.0F);
  enumCombo("Elevation##layout_path_properties", draft.elevation, elevations,
            [](cr::CreativeTerrainPathElevation elevation) {
              return cr::toString(elevation);
            });
  constexpr std::array<std::uint16_t, 4U> halfWidths{0U, 1U, 2U, 3U};
  constexpr std::array<std::uint16_t, 4U> amplitudes{1U, 2U, 4U, 8U};
  ImGui::SetNextItemWidth(150.0F);
  const auto widthLabel = [](std::uint16_t halfWidth) {
    return std::to_string(halfWidth * 2U + 1U) + " cells";
  };
  enumCombo("Width##layout_path_properties", draft.halfWidthCells,
            halfWidths, widthLabel);
  ImGui::SetNextItemWidth(150.0F);
  u16Combo("Depth / rise##layout_path_properties", draft.amplitudeCells,
           amplitudes, " cells");
  ImGui::Checkbox("Paint surface##layout_path_properties",
                  &draft.paintSurface);
  constexpr std::array materials{cr::CreativeTerrainMaterial::Count,
                                 cr::CreativeTerrainMaterial::Grass,
                                 cr::CreativeTerrainMaterial::Dirt,
                                 cr::CreativeTerrainMaterial::Stone,
                                 cr::CreativeTerrainMaterial::Sand};
  ImGui::BeginDisabled(!draft.paintSurface);
  ImGui::SetNextItemWidth(150.0F);
  enumCombo("Material##layout_path_properties", draft.material, materials,
            [](cr::CreativeTerrainMaterial material) -> std::string_view {
              return material == cr::CreativeTerrainMaterial::Count
                         ? "Semantic default"
                         : cr::toString(material);
            });
  ImGui::EndDisabled();

  ImGui::SeparatorText("Control points");
  if (ImGui::BeginTable("##layout_path_points", 4,
                        ImGuiTableFlags_SizingStretchSame |
                            ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 28.0F);
    ImGui::TableSetupColumn("X");
    ImGui::TableSetupColumn("Z");
    ImGui::TableSetupColumn("Height");
    ImGui::TableHeadersRow();
    for (std::size_t index = 0U; index < draft.points.size(); ++index) {
      cr::CreativeTerrainPathPoint& point = draft.points[index];
      ImGui::PushID(static_cast<int>(index));
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%llu", static_cast<unsigned long long>(index + 1U));
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-1.0F);
      ImGui::InputScalar("##x", ImGuiDataType_S32, &point.coord.x);
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-1.0F);
      ImGui::InputScalar("##z", ImGuiDataType_S32, &point.coord.z);
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-1.0F);
      ImGui::InputScalar("##height", ImGuiDataType_U16,
                         &point.heightCells);
      ImGui::PopID();
    }
    ImGui::EndTable();
  }

  bool apply = false;
  bool reset = false;
  drawApplyReset(!(current == draft), "Apply path", "Reset path", apply,
                 reset);
  if (apply) {
    commands.push(CreativeDesktopCommandId::WorldLayoutSetTerrainPathSettings,
                  CreativeDesktopWorldLayoutTerrainPathSettingsPayload{
                      pathIndex, path.stableKey, draft});
  } else if (reset) {
    draft = std::move(current);
  }
}

void drawObjectInspector(CreativeEditorWorldLayoutState& state,
                         CreativeDesktopCommandFrame& commands) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Object ||
      state.selection.index >= state.source.objects.size()) {
    state.objectSettingsDraft = {};
    return;
  }
  const std::size_t objectIndex = state.selection.index;
  CreativeEditorWorldLayoutObjectSettings current;
  if (!readCreativeEditorWorldLayoutObjectSettings(state, objectIndex,
                                                   current)) {
    state.objectSettingsDraft = {};
    return;
  }
  if (!state.objectSettingsDraft.active ||
      state.objectSettingsDraft.objectIndex != objectIndex ||
      state.objectSettingsDraft.sourceRevision != state.revision) {
    state.objectSettingsDraft = {true, objectIndex, state.revision, current};
  }
  CreativeEditorWorldLayoutObjectSettings& draft =
      state.objectSettingsDraft.settings;
  const auto& object = state.source.objects[objectIndex];

  ImGui::SeparatorText("Placed object");
  drawStableKey(object.stableKey, cr::toString(object.kind));
  inputText("Name##layout_object_properties", draft.name);
  inputText("Asset ID##layout_object_properties", draft.assetId);
  ImGui::Checkbox("Visible##layout_object_properties", &draft.visible);
  ImGui::TextDisabled("Placement: %s",
                      cr::toString(draft.mode).data());
  if (draft.mode == cr::CreativeObjectLibraryPlacementMode::Bounds) {
    ImGui::SeparatorText("Bounds in grid cells");
    drawVec3Table("##layout_object_bounds", "Min", draft.boundsCells.min,
                  "Max", &draft.boundsCells.max);
  } else {
    ImGui::SeparatorText("Point in grid cells");
    drawVec3Table("##layout_object_point", "Point", draft.pointCells);
  }

  bool apply = false;
  bool reset = false;
  drawApplyReset(!(current == draft), "Apply object", "Reset object", apply,
                 reset);
  if (apply) {
    commands.push(CreativeDesktopCommandId::WorldLayoutSetObjectSettings,
                  CreativeDesktopWorldLayoutObjectSettingsPayload{
                      objectIndex, object.stableKey, draft});
  } else if (reset) {
    draft = std::move(current);
  }
}

}  // namespace

void drawCreativeEditorWorldLayoutSourceInspector(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands) {
  drawLevelInspector(state, commands);
  drawTerrainProfileInspector(state, commands);
  drawTerrainPathInspector(state, commands);
  drawObjectInspector(state, commands);
}

}  // namespace iggy3d_creative_app
