#include "EditorDesktopWorldLayoutInspector.hpp"
#include "EditorDesktopWidgets.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/Geometry.hpp"

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

void drawStableKey(std::string_view stableKey, std::string_view type) {
  ImGui::TextDisabled("%.*s  %.*s", static_cast<int>(type.size()),
                      type.data(), static_cast<int>(stableKey.size()),
                      stableKey.data());
}

void drawTerrainImpactSummary(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    cr::CreativeWorldLayoutTable table,
    std::size_t index,
    std::string_view stableKey,
    CreativeDesktopCommandFrame& commands) {
  const CreativeEditorWorldLayoutDiagnosticCache& cache =
      state.diagnosticCache;
  const bool cacheCurrent =
      cache.valid && cache.sourceEpoch == state.sourceEpoch &&
      cache.layoutRevision == state.revision &&
      cache.documentId == document.id() &&
      cache.documentRevision == document.revision() &&
      cache.terrainRevision == document.terrainField().revision() &&
      cache.materialRevision == document.terrainMaterialField().revision();
  const cr::CreativeWorldLayoutTerrainSourceImpact* impact =
      cacheCurrent
          ? cr::findCreativeWorldLayoutTerrainSourceImpact(
                cache.report.terrainImpactPlan, table, index)
          : nullptr;
  if (impact == nullptr) {
    ImGui::TextDisabled("Impact unavailable");
    return;
  }

  const bool pending = state.generatedRevision != state.revision;
  const bool current =
      !pending && impact->status ==
                      cr::CreativeWorldLayoutTerrainImpactStatus::Current;
  const bool noEffect =
      impact->status == cr::CreativeWorldLayoutTerrainImpactStatus::NoEffect;
  const ImVec4 statusColor =
      current ? ImVec4{0.45F, 0.95F, 0.48F, 1.0F}
              : noEffect ? ImVec4{0.65F, 0.68F, 0.72F, 1.0F}
                         : ImVec4{1.0F, 0.32F, 0.28F, 1.0F};
  const std::string_view status = pending ? "Pending generation"
                                          : cr::toString(impact->status);
  ImGui::TextColored(statusColor, "Impact: %.*s",
                     static_cast<int>(status.size()), status.data());
  ImGui::TextDisabled("%zu controls  %zu material cells",
                      impact->controls.size(), impact->materials.size());

  cr::CreativeBounds worldBounds{};
  const bool hasWorldBounds =
      cr::creativeWorldLayoutTerrainImpactWorldBounds(
          *impact, document.gridSettings(), worldBounds);
  if (hasWorldBounds) {
    const cr::CreativeBoundsMetrics metrics =
        cr::measureCreativeBounds(worldBounds);
    if (metrics.valid) {
      ImGui::TextDisabled("Bounds %.2f x %.2f x %.2f m", metrics.size.x,
                          metrics.size.y, metrics.size.z);
    }
  }
  ImGui::BeginDisabled(pending || !hasWorldBounds);
  if (ImGui::Button("Frame in 3D##terrain_impact")) {
    commands.push(CreativeDesktopCommandId::WorldLayoutFrameSourceScope3D,
                  CreativeDesktopWorldLayoutSourcePayload{
                      table, index, std::string(stableKey)});
  }
  ImGui::EndDisabled();
}

void drawVec3Table(const char* id, const char* firstLabel,
                   cr::CreativeVec3& first,
                   CreativeDesktopPropertyEditActivity& activity,
                   const char* secondLabel = nullptr,
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
  const auto row = [&activity](const char* label, cr::CreativeVec3& value) {
    ImGui::PushID(label);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputDouble("##x", &value.x, 0.25, 1.0, "%.3f"));
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputDouble("##y", &value.y, 0.25, 1.0, "%.3f"));
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputDouble("##z", &value.z, 0.25, 1.0, "%.3f"));
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
  CreativeDesktopPropertyEditActivity activity;

  ImGui::SeparatorText("Level");
  drawStableKey(level.stableKey, "Building level");
  observeCreativeDesktopContinuousPropertyWidget(
      activity, inputText("Name##layout_level_properties", draft.name));
  ImGui::SetNextItemWidth(140.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Floor top##layout_level_properties",
                         &draft.floorTopLayer, 0.25, 1.0, "%.3f"));
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
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Style##layout_level_properties", draft.roofStyle,
                roofStyles, roofStyleLabel));
  ImGui::SetNextItemWidth(140.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Overhang##layout_level_properties",
                         &draft.roofOverhangCells, 0.25, 1.0, "%.2f"));
  if (draft.roofStyle == cr::CreativeStructuralRoofStyle::Gable) {
    ImGui::SetNextItemWidth(140.0F);
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        enumCombo("Ridge##layout_level_properties", draft.roofRidgeAxis,
                  ridgeAxes, ridgeLabel));
    ImGui::SetNextItemWidth(140.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputDouble("Pitch##layout_level_properties",
                           &draft.roofPitchDegrees, 1.0, 5.0, "%.1f deg"));
  }

  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, draft, true, "Reset level", state,
      levelIndex, level.stableKey, commands);
}

void drawTerrainProfileInspector(CreativeEditorWorldLayoutState& state,
                                 const cr::CreativeDocument& document,
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
  CreativeDesktopPropertyEditActivity activity;

  ImGui::SeparatorText("Terrain profile");
  drawStableKey(profile.stableKey, "Absolute height field");
  drawTerrainImpactSummary(state, document,
                           cr::CreativeWorldLayoutTable::TerrainProfile,
                           profileIndex, profile.stableKey, commands);
  constexpr std::array kinds{cr::CreativeTerrainRecipeKind::Plateau,
                             cr::CreativeTerrainRecipeKind::Hill,
                             cr::CreativeTerrainRecipeKind::Valley,
                             cr::CreativeTerrainRecipeKind::Crater,
                             cr::CreativeTerrainRecipeKind::Ridge};
  ImGui::SetNextItemWidth(160.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Shape##layout_profile_properties", draft.kind, kinds,
                [](cr::CreativeTerrainRecipeKind kind) {
                  return cr::toString(kind);
                }));
  ImGui::SetNextItemWidth(110.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputScalar("Center X##layout_profile_properties",
                         ImGuiDataType_S32, &draft.center.x));
  ImGui::SetNextItemWidth(110.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputScalar("Center Z##layout_profile_properties",
                         ImGuiDataType_S32, &draft.center.z));
  ImGui::SetNextItemWidth(120.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputScalar("Base height##layout_profile_properties",
                         ImGuiDataType_U16, &draft.baseHeightCells));
  constexpr std::array<std::uint16_t, 3U> radii{2U, 4U, 8U};
  constexpr std::array<std::uint16_t, 5U> amplitudes{1U, 2U, 4U, 8U, 16U};
  constexpr std::array<std::uint16_t, 3U> spacings{1U, 2U, 4U};
  ImGui::SetNextItemWidth(140.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      u16Combo("Radius##layout_profile_properties", draft.radiusCells, radii,
               " cells"));
  ImGui::SetNextItemWidth(140.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity, u16Combo("Amplitude##layout_profile_properties",
                         draft.amplitudeCells, amplitudes, " cells"));
  ImGui::SetNextItemWidth(140.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      u16Combo("Spacing##layout_profile_properties", draft.spacingCells,
               spacings, " cells"));
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
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        enumCombo(
            "Direction##layout_profile_properties", draft.direction,
            directions, [](cr::CreativeTerrainProfileDirection direction) {
              return cr::toString(direction);
            }));
  }
  constexpr std::array<std::uint8_t, 2U> frequencies{1U, 2U};
  ImGui::SetNextItemWidth(140.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Frequency##layout_profile_properties", draft.frequency,
                frequencies, [](std::uint8_t frequency) {
                  return std::to_string(static_cast<unsigned>(frequency)) +
                         (frequency == 1U ? " cycle" : " cycles");
                }));
  ImGui::TextDisabled("Blend: Set  Rods: Fill");

  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, draft, true, "Reset terrain", state,
      profileIndex, profile.stableKey, commands);
}

void drawTerrainPathInspector(CreativeEditorWorldLayoutState& state,
                              const cr::CreativeDocument& document,
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
  CreativeDesktopPropertyEditActivity activity;

  ImGui::SeparatorText("Terrain path");
  drawStableKey(path.stableKey, "Polyline recipe");
  drawTerrainImpactSummary(state, document,
                           cr::CreativeWorldLayoutTable::TerrainPath,
                           pathIndex, path.stableKey, commands);
  constexpr std::array kinds{cr::CreativeTerrainRecipeKind::Road,
                             cr::CreativeTerrainRecipeKind::River,
                             cr::CreativeTerrainRecipeKind::Ditch,
                             cr::CreativeTerrainRecipeKind::RidgeLine};
  constexpr std::array elevations{cr::CreativeTerrainPathElevation::Level,
                                  cr::CreativeTerrainPathElevation::Grade};
  ImGui::SetNextItemWidth(150.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Type##layout_path_properties", draft.kind, kinds,
                [](cr::CreativeTerrainRecipeKind kind) {
                  return cr::toString(kind);
                }));
  ImGui::SetNextItemWidth(150.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Elevation##layout_path_properties", draft.elevation,
                elevations, [](cr::CreativeTerrainPathElevation elevation) {
                  return cr::toString(elevation);
                }));
  constexpr std::array<std::uint16_t, 4U> halfWidths{0U, 1U, 2U, 3U};
  constexpr std::array<std::uint16_t, 4U> amplitudes{1U, 2U, 4U, 8U};
  ImGui::SetNextItemWidth(150.0F);
  const auto widthLabel = [](std::uint16_t halfWidth) {
    return std::to_string(halfWidth * 2U + 1U) + " cells";
  };
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Width##layout_path_properties", draft.halfWidthCells,
                halfWidths, widthLabel));
  ImGui::SetNextItemWidth(150.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity, u16Combo("Depth / rise##layout_path_properties",
                         draft.amplitudeCells, amplitudes, " cells"));
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      ImGui::Checkbox("Paint surface##layout_path_properties",
                      &draft.paintSurface));
  constexpr std::array materials{cr::CreativeTerrainMaterial::Count,
                                 cr::CreativeTerrainMaterial::Grass,
                                 cr::CreativeTerrainMaterial::Dirt,
                                 cr::CreativeTerrainMaterial::Stone,
                                 cr::CreativeTerrainMaterial::Sand};
  ImGui::BeginDisabled(!draft.paintSurface);
  ImGui::SetNextItemWidth(150.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Material##layout_path_properties", draft.material,
                materials,
                [](cr::CreativeTerrainMaterial material) -> std::string_view {
                  return material == cr::CreativeTerrainMaterial::Count
                             ? "Semantic default"
                             : cr::toString(material);
                }));
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
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputScalar("##x", ImGuiDataType_S32, &point.coord.x));
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-1.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputScalar("##z", ImGuiDataType_S32, &point.coord.z));
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-1.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputScalar("##height", ImGuiDataType_U16,
                             &point.heightCells));
      ImGui::PopID();
    }
    ImGui::EndTable();
  }

  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, draft, true, "Reset path", state,
      pathIndex, path.stableKey, commands);
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
  CreativeDesktopPropertyEditActivity activity;

  ImGui::SeparatorText("Placed object");
  drawStableKey(object.stableKey, cr::toString(object.kind));
  observeCreativeDesktopContinuousPropertyWidget(
      activity, inputText("Name##layout_object_properties", draft.name));
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      inputText("Asset ID##layout_object_properties", draft.assetId));
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      ImGui::Checkbox("Visible##layout_object_properties", &draft.visible));
  ImGui::TextDisabled("Placement: %s",
                      cr::toString(draft.mode).data());
  if (draft.mode == cr::CreativeObjectLibraryPlacementMode::Bounds) {
    ImGui::SeparatorText("Bounds in grid cells");
    drawVec3Table("##layout_object_bounds", "Min", draft.boundsCells.min,
                  activity, "Max", &draft.boundsCells.max);
  } else {
    ImGui::SeparatorText("Point in grid cells");
    drawVec3Table("##layout_object_point", "Point", draft.pointCells,
                  activity);
    if (draft.hasAssetSourceBounds) {
      constexpr double kRadiansToDegrees =
          57.295779513082320876798154814105;
      constexpr double kDegreesToRadians =
          0.01745329251994329576923690768489;
      double yawDegrees = draft.yawRadians * kRadiansToDegrees;
      const bool yawChanged = ImGui::InputDouble(
          "Yaw##layout_object_properties", &yawDegrees, 15.0, 90.0,
          "%.1f deg");
      if (yawChanged) {
        draft.yawRadians = yawDegrees * kDegreesToRadians;
      }
      observeCreativeDesktopContinuousPropertyWidget(activity, yawChanged);
      drawVec3Table("##layout_object_scale", "Scale", draft.scale,
                    activity);
      const cr::CreativeBoundsMetrics sourceBounds =
          cr::measureCreativeBounds(draft.assetSourceBoundsMeters);
      if (sourceBounds.valid) {
        ImGui::TextDisabled("Catalog bounds: %.2f x %.2f x %.2f m",
                            sourceBounds.size.x, sourceBounds.size.y,
                            sourceBounds.size.z);
      }
    }
  }

  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, draft, true, "Reset object", state,
      objectIndex, object.stableKey, commands);
}

}  // namespace

void drawCreativeEditorWorldLayoutSourceInspector(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    CreativeDesktopCommandFrame& commands) {
  drawLevelInspector(state, commands);
  drawTerrainProfileInspector(state, document, commands);
  drawTerrainPathInspector(state, document, commands);
  drawObjectInspector(state, commands);
}

}  // namespace iggy3d_creative_app
