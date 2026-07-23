#include "EditorDesktopWorldLayoutInspector.hpp"
#include "EditorDesktopWidgets.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/play/NpcSpawn.hpp"
#include "app/iggy3d/creative/play/PlayerSpawn.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"

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

void drawRetainingEdgeSettings(
    std::string_view profileKey,
    cr::CreativeTerrainLandformRecipe& landform,
    bool& enabled,
    cr::CreativeRetainingEdgeSourceRecipe& source,
    CreativeDesktopPropertyEditActivity& activity) {
  if (landform.edge != cr::CreativeTerrainLandformEdge::Retaining) {
    return;
  }

  ImGui::SeparatorText("Retaining edge kit");
  const bool enabledChanged = ImGui::Checkbox(
      "Generate retaining walls##layout_retaining_properties", &enabled);
  observeCreativeDesktopDiscretePropertyWidget(activity, enabledChanged);
  if (enabledChanged && enabled) {
    source = {};
    source.terrainProfileKey = std::string(profileKey);
  }
  if (!enabled) {
    return;
  }

  constexpr std::array selections{
      cr::CreativeRetainingEdgeSelection::All,
      cr::CreativeRetainingEdgeSelection::Internal,
      cr::CreativeRetainingEdgeSelection::Perimeter};
  constexpr std::array kits{cr::CreativeRetainingEdgeKit::Procedural,
                            cr::CreativeRetainingEdgeKit::InfrastructureStone};
  constexpr std::array materials{
      cr::CreativeStructuralMaterial::Blockout,
      cr::CreativeStructuralMaterial::Plaster,
      cr::CreativeStructuralMaterial::Timber,
      cr::CreativeStructuralMaterial::Stone,
      cr::CreativeStructuralMaterial::Brick};
  ImGui::SetNextItemWidth(160.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo(
          "Selection##layout_retaining_properties",
          source.settings.selection, selections,
          [](cr::CreativeRetainingEdgeSelection selection) {
            switch (selection) {
              case cr::CreativeRetainingEdgeSelection::All:
                return std::string_view{"All hard edges"};
              case cr::CreativeRetainingEdgeSelection::Internal:
                return std::string_view{"Internal risers"};
              case cr::CreativeRetainingEdgeSelection::Perimeter:
                return std::string_view{"Perimeter"};
              case cr::CreativeRetainingEdgeSelection::Count:
                break;
            }
            return std::string_view{"Invalid"};
          }));
  ImGui::SetNextItemWidth(180.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Kit##layout_retaining_properties", source.settings.kit,
                kits, [](cr::CreativeRetainingEdgeKit kit) {
                  switch (kit) {
                    case cr::CreativeRetainingEdgeKit::Procedural:
                      return std::string_view{"Procedural"};
                    case cr::CreativeRetainingEdgeKit::InfrastructureStone:
                      return std::string_view{"Infrastructure stone"};
                    case cr::CreativeRetainingEdgeKit::Count:
                      break;
                  }
                  return std::string_view{"Invalid"};
                }));
  ImGui::SetNextItemWidth(140.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Thickness##layout_retaining_properties",
                         &source.settings.thicknessMeters, 0.05, 0.25,
                         "%.2f m"));
  ImGui::SetNextItemWidth(160.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Maximum height##layout_retaining_properties",
                         &source.settings.maximumHeightMeters, 0.5, 2.0,
                         "%.2f m"));
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      ImGui::Checkbox("Close corners##layout_retaining_properties",
                      &source.settings.closeCorners));
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      ImGui::Checkbox("Cap open ends##layout_retaining_properties",
                      &source.settings.capEnds));
  ImGui::SetNextItemWidth(160.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Material##layout_retaining_properties",
                source.settings.material, materials,
                [](cr::CreativeStructuralMaterial material) {
                  return cr::toString(material);
                }));

  if (!ImGui::TreeNodeEx("Transitions##layout_retaining_properties")) {
    return;
  }
  std::size_t removeIndex = cr::kCreativeRetainingEdgeTransitionCapacity;
  constexpr std::array transitionKinds{
      cr::CreativeRetainingEdgeTransitionKind::Stair,
      cr::CreativeRetainingEdgeTransitionKind::Ramp};
  for (std::size_t index = 0U;
       index < source.settings.transitionCount; ++index) {
    cr::CreativeRetainingEdgeTransition& transition =
        source.settings.transitions[index];
    ImGui::PushID(static_cast<int>(index));
    ImGui::SeparatorText(("Transition " + std::to_string(index + 1U)).c_str());
    bool edgeChanged = false;
    const auto coord = [&](const char* label,
                           cr::CreativeTerrainCoord2& value) {
      ImGui::SetNextItemWidth(90.0F);
      edgeChanged =
          ImGui::InputScalar((std::string(label) + " X").c_str(),
                             ImGuiDataType_S32, &value.x) ||
          edgeChanged;
      ImGui::SameLine();
      ImGui::SetNextItemWidth(90.0F);
      edgeChanged =
          ImGui::InputScalar((std::string(label) + " Z").c_str(),
                             ImGuiDataType_S32, &value.z) ||
          edgeChanged;
    };
    coord("First", transition.edge.first);
    coord("Second", transition.edge.second);
    if (edgeChanged) {
      const std::int64_t deltaX =
          static_cast<std::int64_t>(transition.edge.second.x) -
          transition.edge.first.x;
      const std::int64_t deltaZ =
          static_cast<std::int64_t>(transition.edge.second.z) -
          transition.edge.first.z;
      if (std::abs(deltaX) + std::abs(deltaZ) == 1) {
        transition.edge = cr::canonicalCreativeTerrainHardEdge(
            transition.edge.first, transition.edge.second);
      }
      observeCreativeDesktopContinuousPropertyWidget(activity, true);
    }
    ImGui::SetNextItemWidth(130.0F);
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        enumCombo("Type", transition.kind, transitionKinds,
                  [](cr::CreativeRetainingEdgeTransitionKind kind) {
                    return kind ==
                                   cr::CreativeRetainingEdgeTransitionKind::Stair
                               ? std::string_view{"Stair"}
                               : std::string_view{"Ramp"};
                  }));
    ImGui::SetNextItemWidth(120.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Run cells", ImGuiDataType_U16,
                           &transition.runCells));
    if (ImGui::SmallButton("Remove transition")) {
      removeIndex = index;
    }
    ImGui::PopID();
  }
  if (removeIndex < source.settings.transitionCount) {
    for (std::size_t index = removeIndex + 1U;
         index < source.settings.transitionCount; ++index) {
      source.settings.transitions[index - 1U] =
          source.settings.transitions[index];
    }
    --source.settings.transitionCount;
    source.settings.transitions[source.settings.transitionCount] = {};
    observeCreativeDesktopDiscretePropertyWidget(activity, true);
  }

  ImGui::BeginDisabled(
      source.settings.transitionCount >=
      cr::kCreativeRetainingEdgeTransitionCapacity);
  if (ImGui::Button("Add stair/ramp transition")) {
    cr::CreativeTerrainCoord2 first = landform.bounds.minimum;
    cr::CreativeTerrainCoord2 second = first;
    if (landform.bounds.widthCells > 1U) {
      ++second.x;
    } else {
      ++second.z;
    }
    source.settings.transitions[source.settings.transitionCount] = {
        cr::canonicalCreativeTerrainHardEdge(first, second),
        cr::CreativeRetainingEdgeTransitionKind::Stair,
        3U,
    };
    ++source.settings.transitionCount;
    observeCreativeDesktopDiscretePropertyWidget(activity, true);
  }
  ImGui::EndDisabled();
  ImGui::TextDisabled("Transition seam must match a generated hard edge");
  ImGui::TreePop();
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
    commands.push(
        CreativeDesktopCommandId::WorldLayoutCreateRoofAperture,
        CreativeDesktopWorldLayoutRoofApertureCreatePayload{
            levelIndex,
            cr::CreativeStructuralRoofApertureKind::Skylight});
  }
  ImGui::SameLine();
  if (ImGui::Button("Add chimney clearance##layout_level")) {
    commands.push(
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
                             cr::CreativeTerrainRecipeKind::Terrace,
                             cr::CreativeTerrainRecipeKind::Cliff,
                             cr::CreativeTerrainRecipeKind::Hill,
                             cr::CreativeTerrainRecipeKind::Valley,
                             cr::CreativeTerrainRecipeKind::Crater,
                             cr::CreativeTerrainRecipeKind::Ridge};
  ImGui::SetNextItemWidth(160.0F);
  const bool shapeChanged =
      enumCombo("Shape##layout_profile_properties", draft.kind, kinds,
                [](cr::CreativeTerrainRecipeKind kind) {
                  return cr::toString(kind);
                });
  observeCreativeDesktopDiscretePropertyWidget(activity, shapeChanged);
  if (shapeChanged) {
    cr::CreativeTerrainLandformKind landformKind =
        cr::CreativeTerrainLandformKind::Count;
    draft.usesLandformRecipe =
        cr::creativeTerrainRecipeLandformKind(draft.kind, landformKind);
    if (draft.usesLandformRecipe) {
      draft.landform.kind = landformKind;
      if (landformKind != cr::CreativeTerrainLandformKind::Plateau &&
          draft.landform.baseHeightCells ==
              draft.landform.targetHeightCells) {
        draft.landform.targetHeightCells =
            draft.landform.baseHeightCells <
                    cr::kCreativeTerrainMaximumHeightCells
                ? static_cast<std::uint16_t>(
                      draft.landform.baseHeightCells + 1U)
                : static_cast<std::uint16_t>(
                      draft.landform.baseHeightCells - 1U);
      }
    } else {
      draft.usesRetainingEdgeRecipe = false;
    }
  }

  if (!draft.usesLandformRecipe &&
      draft.kind == cr::CreativeTerrainRecipeKind::Plateau) {
    if (ImGui::Button("Convert to bounded landform")) {
      draft.usesLandformRecipe = true;
      draft.landform = {};
      draft.landform.kind = cr::CreativeTerrainLandformKind::Plateau;
      const std::int32_t radius = draft.radiusCells;
      draft.landform.bounds.minimum = {draft.center.x - radius,
                                      draft.center.z - radius};
      draft.landform.bounds.widthCells =
          static_cast<std::uint16_t>(radius * 2);
      draft.landform.bounds.depthCells =
          static_cast<std::uint16_t>(radius * 2);
      draft.landform.baseHeightCells = draft.baseHeightCells;
      draft.landform.targetHeightCells = draft.baseHeightCells;
      observeCreativeDesktopDiscretePropertyWidget(activity, true);
    }
    ImGui::TextDisabled("Legacy radial plateau; conversion is explicit");
  }

  if (draft.usesLandformRecipe) {
    cr::CreativeTerrainLandformRecipe& landform = draft.landform;
    ImGui::SetNextItemWidth(110.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Min X##layout_landform_properties",
                           ImGuiDataType_S32,
                           &landform.bounds.minimum.x));
    ImGui::SetNextItemWidth(110.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Min Z##layout_landform_properties",
                           ImGuiDataType_S32,
                           &landform.bounds.minimum.z));
    ImGui::SetNextItemWidth(110.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Width##layout_landform_properties",
                           ImGuiDataType_U16,
                           &landform.bounds.widthCells));
    ImGui::SetNextItemWidth(110.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Depth##layout_landform_properties",
                           ImGuiDataType_U16,
                           &landform.bounds.depthCells));
    ImGui::SetNextItemWidth(130.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Base height##layout_landform_properties",
                           ImGuiDataType_U16,
                           &landform.baseHeightCells));
    ImGui::SetNextItemWidth(130.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Target height##layout_landform_properties",
                           ImGuiDataType_U16,
                           &landform.targetHeightCells));
    if (landform.kind == cr::CreativeTerrainLandformKind::Terrace) {
      ImGui::SetNextItemWidth(130.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputScalar("Terraces##layout_landform_properties",
                             ImGuiDataType_U8,
                             &landform.terraceCount));
    }
    if (landform.kind != cr::CreativeTerrainLandformKind::Plateau) {
      constexpr std::array directions{
          cr::CreativeTerrainLandformDirection::PositiveX,
          cr::CreativeTerrainLandformDirection::PositiveZ,
          cr::CreativeTerrainLandformDirection::NegativeX,
          cr::CreativeTerrainLandformDirection::NegativeZ};
      ImGui::SetNextItemWidth(160.0F);
      observeCreativeDesktopDiscretePropertyWidget(
          activity,
          enumCombo("Direction##layout_landform_properties",
                    landform.direction, directions,
                    [](cr::CreativeTerrainLandformDirection direction) {
                      return cr::toString(direction);
                    }));
    }
    constexpr std::array edges{cr::CreativeTerrainLandformEdge::Slope,
                               cr::CreativeTerrainLandformEdge::Retaining};
    ImGui::SetNextItemWidth(160.0F);
    const bool edgeChanged =
        enumCombo("Edge##layout_landform_properties", landform.edge, edges,
                  [](cr::CreativeTerrainLandformEdge edge) {
                    return cr::toString(edge);
                  });
    observeCreativeDesktopDiscretePropertyWidget(
        activity, edgeChanged);
    if (edgeChanged) {
      if (landform.edge == cr::CreativeTerrainLandformEdge::Retaining) {
        landform.edgeWidthCells = 0U;
      } else if (landform.edgeWidthCells == 0U) {
        landform.edgeWidthCells = 2U;
      }
      if (landform.edge != cr::CreativeTerrainLandformEdge::Retaining) {
        draft.usesRetainingEdgeRecipe = false;
      }
    }
    if (landform.edge == cr::CreativeTerrainLandformEdge::Slope) {
      ImGui::SetNextItemWidth(130.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputScalar("Edge width##layout_landform_properties",
                             ImGuiDataType_U16,
                             &landform.edgeWidthCells));
    }
    ImGui::SetNextItemWidth(130.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Feather##layout_landform_properties",
                           ImGuiDataType_U16,
                           &landform.featherCells));
    constexpr std::array materials{cr::CreativeTerrainMaterial::Grass,
                                   cr::CreativeTerrainMaterial::Dirt,
                                   cr::CreativeTerrainMaterial::Stone,
                                   cr::CreativeTerrainMaterial::Sand};
    ImGui::SetNextItemWidth(160.0F);
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        enumCombo("Material##layout_landform_properties", landform.material,
                  materials, [](cr::CreativeTerrainMaterial material) {
                    return cr::toString(material);
                  }));
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        ImGui::Checkbox("Paint surface##layout_landform_properties",
                        &landform.paintSurface));
    constexpr std::array erosionModes{
        cr::CreativeTerrainLandformErosion::Clean,
        cr::CreativeTerrainLandformErosion::Weathered};
    ImGui::SetNextItemWidth(160.0F);
    const bool erosionChanged = enumCombo(
        "Erosion##layout_landform_properties", landform.erosion, erosionModes,
        [](cr::CreativeTerrainLandformErosion erosion) {
          return cr::toString(erosion);
        });
    observeCreativeDesktopDiscretePropertyWidget(
        activity, erosionChanged);
    if (erosionChanged) {
      if (landform.erosion == cr::CreativeTerrainLandformErosion::Clean) {
        landform.erosionReliefCells = 0U;
      } else if (landform.erosionReliefCells == 0U) {
        landform.erosionReliefCells = 1U;
      }
    }
    if (landform.erosion == cr::CreativeTerrainLandformErosion::Weathered) {
      ImGui::SetNextItemWidth(130.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputScalar("Relief##layout_landform_properties",
                             ImGuiDataType_U16,
                             &landform.erosionReliefCells));
      ImGui::SetNextItemWidth(160.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputScalar("Seed##layout_landform_properties",
                             ImGuiDataType_U64, &landform.seed));
    }
    drawRetainingEdgeSettings(profile.stableKey, landform,
                              draft.usesRetainingEdgeRecipe,
                              draft.retainingEdge, activity);
  } else {
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
        u16Combo("Radius##layout_profile_properties", draft.radiusCells,
                 radii, " cells"));
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
  }

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
  cr::CreativeTerrainPathSourceRecipe& recipe = draft.recipe;
  CreativeDesktopPropertyEditActivity activity;

  ImGui::SeparatorText("Terrain path");
  drawStableKey(path.stableKey, "Polyline recipe");
  drawTerrainImpactSummary(state, document,
                           cr::CreativeWorldLayoutTable::TerrainPath,
                           pathIndex, path.stableKey, commands);
  constexpr std::array kinds{cr::CreativeTerrainPathKind::Road,
                             cr::CreativeTerrainPathKind::River,
                             cr::CreativeTerrainPathKind::Ridge,
                             cr::CreativeTerrainPathKind::Trench};
  constexpr std::array elevations{cr::CreativeTerrainPathElevation::Follow,
                                  cr::CreativeTerrainPathElevation::Level,
                                  cr::CreativeTerrainPathElevation::Grade};
  ImGui::SetNextItemWidth(150.0F);
  const cr::CreativeTerrainPathKind previousKind = recipe.kind;
  const bool kindChanged =
      enumCombo("Type##layout_path_properties", recipe.kind, kinds,
                [](cr::CreativeTerrainPathKind kind) {
                  return cr::toString(kind);
                });
  if (kindChanged &&
      (recipe.kind != cr::CreativeTerrainPathKind::River &&
       recipe.kind != cr::CreativeTerrainPathKind::Trench) &&
      (previousKind == cr::CreativeTerrainPathKind::River ||
       previousKind == cr::CreativeTerrainPathKind::Trench)) {
    recipe.watercourse = {};
  }
  observeCreativeDesktopDiscretePropertyWidget(activity, kindChanged);
  ImGui::SetNextItemWidth(150.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Elevation##layout_path_properties", recipe.elevation,
                elevations, [](cr::CreativeTerrainPathElevation elevation) {
                  return cr::toString(elevation);
                }));
  constexpr std::array curves{cr::CreativeTerrainPathCurvePolicy::Linear,
                              cr::CreativeTerrainPathCurvePolicy::CatmullRom};
  ImGui::SetNextItemWidth(150.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Curve##layout_path_properties", recipe.curve, curves,
                [](cr::CreativeTerrainPathCurvePolicy value) {
                  return cr::toString(value);
                }));
  constexpr std::array crossSections{
      cr::CreativeTerrainPathCrossSection::Flat,
      cr::CreativeTerrainPathCrossSection::Crowned,
      cr::CreativeTerrainPathCrossSection::Channel,
      cr::CreativeTerrainPathCrossSection::Berm,
      cr::CreativeTerrainPathCrossSection::Cut};
  ImGui::SetNextItemWidth(150.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Cross-section##layout_path_properties", recipe.crossSection,
                crossSections, [](cr::CreativeTerrainPathCrossSection value) {
                  return cr::toString(value);
                }));
  constexpr std::array joins{cr::CreativeTerrainPathEndpointJoin::Open,
                             cr::CreativeTerrainPathEndpointJoin::Blend,
                             cr::CreativeTerrainPathEndpointJoin::Intersection,
                             cr::CreativeTerrainPathEndpointJoin::Bridge,
                             cr::CreativeTerrainPathEndpointJoin::BuildingPad};
  ImGui::SetNextItemWidth(150.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Start join##layout_path_properties", recipe.startJoin, joins,
                [](cr::CreativeTerrainPathEndpointJoin value) {
                  return cr::toString(value);
                }));
  ImGui::SetNextItemWidth(150.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("End join##layout_path_properties", recipe.endJoin, joins,
                [](cr::CreativeTerrainPathEndpointJoin value) {
                  return cr::toString(value);
                }));
  ImGui::SetNextItemWidth(150.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputScalar("Falloff##layout_path_properties",
                         ImGuiDataType_U16, &recipe.falloffCells));
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      ImGui::Checkbox("Paint surface##layout_path_properties",
                      &recipe.paintSurface));
  constexpr std::array materials{cr::CreativeTerrainMaterial::Count,
                                 cr::CreativeTerrainMaterial::Grass,
                                 cr::CreativeTerrainMaterial::Dirt,
                                 cr::CreativeTerrainMaterial::Stone,
                                 cr::CreativeTerrainMaterial::Sand};
  ImGui::BeginDisabled(!recipe.paintSurface);
  ImGui::SetNextItemWidth(150.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Material##layout_path_properties", recipe.material,
                materials,
                [](cr::CreativeTerrainMaterial material) -> std::string_view {
                  return material == cr::CreativeTerrainMaterial::Count
                             ? "Semantic default"
                             : cr::toString(material);
                }));
  ImGui::EndDisabled();

  if (recipe.kind == cr::CreativeTerrainPathKind::Road) {
    ImGui::SeparatorText("Road construction");
    ImGui::SetNextItemWidth(150.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Shoulder##layout_path_properties",
                           ImGuiDataType_U16,
                           &recipe.road.shoulderWidthCells));
    ImGui::SetNextItemWidth(150.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Maximum grade (permille, 0 unlimited)##layout_path_properties",
                           ImGuiDataType_U16,
                           &recipe.road.maximumGradePermille));
    constexpr std::array edgeTreatments{
        cr::CreativeTerrainRoadEdgeTreatment::None,
        cr::CreativeTerrainRoadEdgeTreatment::Curb};
    ImGui::SetNextItemWidth(150.0F);
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        enumCombo("Edge treatment##layout_path_properties",
                  recipe.road.edgeTreatment, edgeTreatments,
                  [](cr::CreativeTerrainRoadEdgeTreatment value) {
                    return cr::toString(value);
                  }));
    if (recipe.road.edgeTreatment ==
        cr::CreativeTerrainRoadEdgeTreatment::Curb) {
      ImGui::SetNextItemWidth(150.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputDouble("Curb width (m)##layout_path_properties",
                             &recipe.road.edgeWidthMeters, 0.01, 0.1, "%.2f"));
      ImGui::SetNextItemWidth(150.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputDouble("Curb height (m)##layout_path_properties",
                             &recipe.road.edgeHeightMeters, 0.01, 0.1, "%.2f"));
      constexpr std::array edgeMaterials{
          cr::CreativeStructuralMaterial::Blockout,
          cr::CreativeStructuralMaterial::Plaster,
          cr::CreativeStructuralMaterial::Timber,
          cr::CreativeStructuralMaterial::Stone,
          cr::CreativeStructuralMaterial::Brick};
      ImGui::SetNextItemWidth(150.0F);
      observeCreativeDesktopDiscretePropertyWidget(
          activity,
          enumCombo("Curb material##layout_path_properties",
                    recipe.road.edgeMaterial, edgeMaterials,
                    [](cr::CreativeStructuralMaterial value) {
                      return cr::toString(value);
                    }));
    }
    ImGui::TextDisabled(
        "Half width is travel surface; shoulder extends each side; grade 0 is unlimited.");
  }

  const bool watercourse = recipe.kind == cr::CreativeTerrainPathKind::River ||
                           recipe.kind == cr::CreativeTerrainPathKind::Trench;
  if (watercourse) {
    ImGui::SeparatorText("Watercourse");
    ImGui::SetNextItemWidth(150.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Bank slope##layout_path_properties",
                           ImGuiDataType_U16,
                           &recipe.watercourse.bankSlopeCells));
    constexpr std::array drainageDirections{
        cr::CreativeTerrainWatercourseDrainageDirection::Unspecified,
        cr::CreativeTerrainWatercourseDrainageDirection::StartToEnd,
        cr::CreativeTerrainWatercourseDrainageDirection::EndToStart};
    ImGui::SetNextItemWidth(150.0F);
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        enumCombo("Drainage##layout_path_properties",
                  recipe.watercourse.drainageDirection, drainageDirections,
                  [](cr::CreativeTerrainWatercourseDrainageDirection value) {
                    return cr::toString(value);
                  }));
    constexpr std::array surfacePolicies{
        cr::CreativeTerrainWaterSurfacePolicy::None,
        cr::CreativeTerrainWaterSurfacePolicy::Reserved};
    ImGui::SetNextItemWidth(150.0F);
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        enumCombo("Water surface##layout_path_properties",
                  recipe.watercourse.surfacePolicy, surfacePolicies,
                  [](cr::CreativeTerrainWaterSurfacePolicy value) {
                    return cr::toString(value);
                  }));
    if (recipe.watercourse.surfacePolicy ==
        cr::CreativeTerrainWaterSurfacePolicy::Reserved) {
      ImGui::SetNextItemWidth(150.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputScalar("Surface inset##layout_path_properties",
                             ImGuiDataType_U16,
                             &recipe.watercourse.surfaceInsetCells));
    }
    ImGui::TextDisabled(
        "Half width is the bed; bank slope expands outward; reserved water is not rendered or simulated.");
  }

  ImGui::SeparatorText("Control points");
  std::size_t removePoint = std::numeric_limits<std::size_t>::max();
  std::size_t movePointFrom = std::numeric_limits<std::size_t>::max();
  std::size_t movePointTo = std::numeric_limits<std::size_t>::max();
  if (ImGui::BeginTable("##layout_path_points", 8,
                        ImGuiTableFlags_SizingStretchSame |
                            ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 28.0F);
    ImGui::TableSetupColumn("X");
    ImGui::TableSetupColumn("Z");
    ImGui::TableSetupColumn("Height");
    ImGui::TableSetupColumn("Half width");
    ImGui::TableSetupColumn("Depth / rise");
    ImGui::TableSetupColumn("Bank");
    ImGui::TableSetupColumn("Order", ImGuiTableColumnFlags_WidthFixed, 96.0F);
    ImGui::TableHeadersRow();
    for (std::size_t index = 0U; index < recipe.points.size(); ++index) {
      cr::CreativeTerrainPathSourcePoint& point = recipe.points[index];
      ImGui::PushID(static_cast<int>(point.id));
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
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-1.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputScalar("##width", ImGuiDataType_U16,
                             &point.halfWidthCells));
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-1.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputScalar("##amplitude", ImGuiDataType_U16,
                             &point.amplitudeCells));
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-1.0F);
      observeCreativeDesktopContinuousPropertyWidget(
          activity,
          ImGui::InputScalar("##bank", ImGuiDataType_S32,
                             &point.bankPermille));
      ImGui::TableNextColumn();
      ImGui::BeginDisabled(index == 0U);
      if (ImGui::SmallButton("Up")) {
        movePointFrom = index;
        movePointTo = index - 1U;
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::BeginDisabled(index + 1U >= recipe.points.size());
      if (ImGui::SmallButton("Down")) {
        movePointFrom = index;
        movePointTo = index + 1U;
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::BeginDisabled(recipe.points.size() <= 2U);
      if (ImGui::SmallButton("X")) {
        removePoint = index;
      }
      ImGui::EndDisabled();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  if (movePointFrom < recipe.points.size() &&
      movePointTo < recipe.points.size()) {
    std::swap(recipe.points[movePointFrom], recipe.points[movePointTo]);
    observeCreativeDesktopDiscretePropertyWidget(activity, true);
  }
  if (removePoint < recipe.points.size()) {
    const cr::CreativeTerrainPathSourcePointId removedId =
        recipe.points[removePoint].id;
    recipe.points.erase(recipe.points.begin() +
                        static_cast<std::ptrdiff_t>(removePoint));
    std::erase_if(recipe.watercourse.crossings,
                  [removedId](
                      const cr::CreativeTerrainWatercourseCrossing& crossing) {
                    return crossing.pointId == removedId;
                  });
    observeCreativeDesktopDiscretePropertyWidget(activity, true);
  }
  const bool canAddPoint =
      recipe.points.size() < cr::kCreativeTerrainPathPointCapacity &&
      recipe.nextPointId <
          std::numeric_limits<cr::CreativeTerrainPathSourcePointId>::max();
  ImGui::BeginDisabled(!canAddPoint);
  if (ImGui::Button("Add point##layout_path_properties") && canAddPoint) {
    cr::CreativeTerrainPathSourcePoint point = recipe.points.back();
    point.id = recipe.nextPointId++;
    if (point.coord.x < std::numeric_limits<std::int32_t>::max()) {
      ++point.coord.x;
    } else {
      --point.coord.x;
    }
    recipe.points.push_back(point);
    observeCreativeDesktopDiscretePropertyWidget(activity, true);
  }
  ImGui::EndDisabled();

  if (watercourse) {
    ImGui::SeparatorText("Crossings");
    for (std::size_t pointIndex = 0U; pointIndex < recipe.points.size();
         ++pointIndex) {
      const cr::CreativeTerrainPathSourcePoint& point =
          recipe.points[pointIndex];
      auto crossing = std::find_if(
          recipe.watercourse.crossings.begin(),
          recipe.watercourse.crossings.end(),
          [&point](const cr::CreativeTerrainWatercourseCrossing& candidate) {
            return candidate.pointId == point.id;
          });
      bool enabled = crossing != recipe.watercourse.crossings.end();
      ImGui::PushID(static_cast<int>(point.id));
      const bool canEnable =
          enabled ||
          (recipe.watercourse.crossings.size() <
               cr::kCreativeTerrainWatercourseCrossingCapacity &&
           recipe.watercourse.nextCrossingId <
               std::numeric_limits<
                   cr::CreativeTerrainWatercourseCrossingId>::max());
      ImGui::BeginDisabled(!canEnable);
      if (ImGui::Checkbox("##watercourse_crossing", &enabled)) {
        if (enabled) {
          recipe.watercourse.crossings.push_back(
              {recipe.watercourse.nextCrossingId++, point.id, 1U, 1U, 2U});
        } else {
          recipe.watercourse.crossings.erase(crossing);
        }
        observeCreativeDesktopDiscretePropertyWidget(activity, true);
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::Text("Point %llu (%d, %d)",
                  static_cast<unsigned long long>(pointIndex + 1U),
                  point.coord.x, point.coord.z);
      crossing = std::find_if(
          recipe.watercourse.crossings.begin(),
          recipe.watercourse.crossings.end(),
          [&point](const cr::CreativeTerrainWatercourseCrossing& candidate) {
            return candidate.pointId == point.id;
          });
      if (crossing != recipe.watercourse.crossings.end()) {
        ImGui::Indent();
        ImGui::SetNextItemWidth(96.0F);
        observeCreativeDesktopContinuousPropertyWidget(
            activity,
            ImGui::InputScalar("Bank clearance", ImGuiDataType_U16,
                               &crossing->bankClearanceCells));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(96.0F);
        observeCreativeDesktopContinuousPropertyWidget(
            activity,
            ImGui::InputScalar("Deck clearance", ImGuiDataType_U16,
                               &crossing->deckClearanceCells));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(96.0F);
        observeCreativeDesktopContinuousPropertyWidget(
            activity,
            ImGui::InputScalar("Approach length", ImGuiDataType_U16,
                               &crossing->approachLengthCells));
        ImGui::Unindent();
      }
      ImGui::PopID();
    }
    ImGui::TextDisabled(
        "Crossings derive bank and approach frames from stable path points; bridge generation is separate.");
  }

  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, draft, true, "Reset path", state,
      pathIndex, path.stableKey, commands);
}

void drawBridgeRecipeInspector(
    CreativeEditorWorldLayoutObjectSettings& draft,
    CreativeDesktopPropertyEditActivity& activity) {
  cr::CreativeBridgeSettings& settings = draft.bridge.settings;
  ImGui::SeparatorText("Bridge recipe");
  ImGui::TextDisabled("Attached path: %s",
                      draft.bridge.watercoursePathKey.c_str());
  ImGui::TextDisabled("Crossing id: %u",
                      static_cast<unsigned>(draft.bridge.crossingId));
  ImGui::TextDisabled(
      "Placement follows the crossing; the plan footprint is not movable.");

  const auto inputMeters = [&](const char* label, double& value,
                               double step = 0.05) {
    ImGui::SetNextItemWidth(160.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity, ImGui::InputDouble(label, &value, step, step * 5.0, "%.2f m"));
  };
  inputMeters("Deck width##layout_bridge_properties",
              settings.deckWidthMeters);
  inputMeters("Deck thickness##layout_bridge_properties",
              settings.deckThicknessMeters);
  inputMeters("Elevation offset##layout_bridge_properties",
              settings.deckElevationOffsetMeters);
  inputMeters("Maximum span##layout_bridge_properties",
              settings.maximumSpanMeters, 0.5);
  inputMeters("Minimum clearance##layout_bridge_properties",
              settings.minimumClearanceMeters);

  constexpr std::array supportStyles{
      cr::CreativeBridgeSupportStyle::None,
      cr::CreativeBridgeSupportStyle::PierPairs};
  ImGui::SetNextItemWidth(160.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Supports##layout_bridge_properties", settings.supportStyle,
                supportStyles, [](cr::CreativeBridgeSupportStyle style) {
                  return style == cr::CreativeBridgeSupportStyle::None
                             ? std::string_view("None")
                             : std::string_view("Pier pairs");
                }));
  if (settings.supportStyle == cr::CreativeBridgeSupportStyle::PierPairs) {
    inputMeters("Support spacing##layout_bridge_properties",
                settings.supportSpacingMeters, 0.25);
    inputMeters("Support width##layout_bridge_properties",
                settings.supportWidthMeters);
    inputMeters("Support depth##layout_bridge_properties",
                settings.supportDepthMeters);
  }

  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      ImGui::Checkbox("Rails##layout_bridge_properties", &settings.rails));
  if (settings.rails) {
    inputMeters("Rail height##layout_bridge_properties",
                settings.railHeightMeters);
    inputMeters("Rail thickness##layout_bridge_properties",
                settings.railThicknessMeters);
  }
  ImGui::SetNextItemWidth(160.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputScalar("Maximum approach grade (permille)##layout_bridge_properties",
                         ImGuiDataType_U16,
                         &settings.maximumApproachGradePermille));
  ImGui::SetNextItemWidth(160.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputScalar("Approach falloff##layout_bridge_properties",
                         ImGuiDataType_U16,
                         &settings.approachFalloffCells));

  constexpr std::array materials{
      cr::CreativeStructuralMaterial::Blockout,
      cr::CreativeStructuralMaterial::Plaster,
      cr::CreativeStructuralMaterial::Timber,
      cr::CreativeStructuralMaterial::Stone,
      cr::CreativeStructuralMaterial::Brick};
  const auto materialField = [&](const char* label,
                                 cr::CreativeStructuralMaterial& material) {
    ImGui::SetNextItemWidth(160.0F);
    observeCreativeDesktopDiscretePropertyWidget(
        activity, enumCombo(label, material, materials,
                            [](cr::CreativeStructuralMaterial value) {
                              return cr::toString(value);
                            }));
  };
  materialField("Deck material##layout_bridge_properties",
                settings.materials.deck);
  materialField("Support material##layout_bridge_properties",
                settings.materials.supports);
  materialField("Rail material##layout_bridge_properties",
                settings.materials.rails);
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
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      ImGui::Checkbox("Visible##layout_object_properties", &draft.visible));
  if (draft.usesBridgeRecipe) {
    drawBridgeRecipeInspector(draft, activity);
  } else {
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        inputText("Asset ID##layout_object_properties", draft.assetId));
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
      if (draft.hasAssetSourceBounds ||
          draft.kind == cr::CreativeObjectKind::SpawnPoint) {
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
      }
      if (draft.hasAssetSourceBounds) {
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
  }

  const bool spawnSettingsValid =
      draft.kind != cr::CreativeObjectKind::SpawnPoint ||
      cr::isValidCreativePlayerSpawnSettings(draft.playerSpawn);
  if (draft.kind == cr::CreativeObjectKind::SpawnPoint) {
    ImGui::SeparatorText("Player Spawn");
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        inputText("Player profile##layout_object_properties",
                  draft.playerSpawn.playerProfileId));
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        inputText("Spawn group##layout_object_properties",
                  draft.playerSpawn.spawnGroup));
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputDouble("Clearance radius##layout_object_properties",
                           &draft.playerSpawn.validationRadiusMeters, 0.05,
                           0.25, "%.2f m"));
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Fallback priority##layout_object_properties",
                           ImGuiDataType_U16,
                           &draft.playerSpawn.fallbackPriority));
    ImGui::TextDisabled("Lower priority wins; ties use object id");
    ImGui::TextDisabled("Facing follows Yaw");
    if (!spawnSettingsValid) {
      ImGui::TextColored(ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
                         "Profile/group identifiers or clearance radius are invalid");
    } else if (!cr::isSupportedCreativePlayerProfileId(
                   draft.playerSpawn.playerProfileId)) {
      ImGui::TextColored(ImVec4{1.0F, 0.72F, 0.22F, 1.0F},
                         "Profile is not available in the current runtime");
    }
  }

  const bool npcActor = draft.kind == cr::CreativeObjectKind::NpcSpawn ||
                        draft.kind == cr::CreativeObjectKind::EnemySpawn;
  const bool npcSettingsValid =
      !npcActor || cr::isValidCreativeNpcSpawnSettings(draft.npcSpawn);
  if (npcActor) {
    ImGui::SeparatorText("NPC Spawn");
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        inputText("Behavior profile##layout_object_properties",
                  draft.npcSpawn.behaviorProfileId));
    constexpr std::array teams{
        cr::CreativeNpcTeam::ActorDefault,
        cr::CreativeNpcTeam::PlayerAllied,
        cr::CreativeNpcTeam::Hostile,
    };
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        enumCombo("Team##layout_object_properties", draft.npcSpawn.team, teams,
                  [](cr::CreativeNpcTeam team) {
                    return cr::toString(team);
                  }));
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Health override##layout_object_properties",
                           ImGuiDataType_U16, &draft.npcSpawn.hitPoints));
    ImGui::TextDisabled("0 uses the actor-kind default");
    constexpr double kMinimumAlert = 0.0;
    constexpr double kMaximumAlert = 1.0;
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::SliderScalar("Initial alert##layout_object_properties",
                            ImGuiDataType_Double,
                            &draft.npcSpawn.initialAlertLevel, &kMinimumAlert,
                            &kMaximumAlert, "%.2f"));
    constexpr std::array policies{
        cr::CreativeNpcSpawnPolicy::AtPlayStart,
        cr::CreativeNpcSpawnPolicy::Disabled,
    };
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        enumCombo("Spawn policy##layout_object_properties",
                  draft.npcSpawn.spawnPolicy, policies,
                  [](cr::CreativeNpcSpawnPolicy policy) {
                    return cr::toString(policy);
                  }));
    ImGui::TextDisabled("Facing follows Yaw");
    ImGui::TextDisabled("Patrol ownership is authored in the 3D hierarchy");
    if (!npcSettingsValid) {
      ImGui::TextColored(
          ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
          "Profile identifier, health override, or initial alert is invalid");
    } else if (!cr::isSupportedCreativeNpcBehaviorProfileId(
                   draft.npcSpawn.behaviorProfileId)) {
      ImGui::TextColored(ImVec4{1.0F, 0.72F, 0.22F, 1.0F},
                         "Profile is not available in the current runtime");
    }
  }

  const bool lootPoint =
      draft.kind == cr::CreativeObjectKind::LootPoint;
  const bool lootSettingsValid =
      !lootPoint || cr::isValidCreativeLootPointSettings(draft.lootPoint);
  if (lootPoint) {
    ImGui::SeparatorText("Loot");
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        inputText("Item ID##layout_object_properties",
                  draft.lootPoint.itemId));
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Count##layout_object_properties",
                           ImGuiDataType_U32, &draft.lootPoint.itemCount));
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        ImGui::Checkbox("Remove after collection##layout_object_properties",
                        &draft.lootPoint.deactivateOnCollect));
    ImGui::TextDisabled(
        "Use an explicit item ID when another layout source requires it");
    if (!lootSettingsValid) {
      ImGui::TextColored(
          ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
          "Item ID or item count is invalid");
    }
  }

  const bool exitPoint =
      draft.kind == cr::CreativeObjectKind::ExitPoint;
  const bool exitSettingsValid =
      !exitPoint || cr::isValidCreativeExitPointSettings(draft.exitPoint);
  if (exitPoint) {
    ImGui::SeparatorText("Exit Objective");
    bool requirementChanged = false;
    const char* requirementLabel = draft.exitPoint.requiredItemId.empty()
                                       ? "No item required"
                                       : draft.exitPoint.requiredItemId.c_str();
    if (ImGui::BeginCombo("Loot requirement##layout_object_properties",
                          requirementLabel)) {
      const bool noRequirement = draft.exitPoint.requiredItemId.empty();
      if (ImGui::Selectable("No item required", noRequirement) &&
          !noRequirement) {
        draft.exitPoint.requiredItemId.clear();
        draft.exitPoint.requiredItemCount = 0U;
        requirementChanged = true;
      }
      for (const cr::CreativeWorldLayoutObject& candidate :
           state.source.objects) {
        if (candidate.kind != cr::CreativeObjectKind::LootPoint ||
            !candidate.visible || candidate.lootPoint.itemId.empty()) {
          continue;
        }
        const bool selected =
            candidate.lootPoint.itemId == draft.exitPoint.requiredItemId;
        const std::string label =
            candidate.name.empty()
                ? candidate.lootPoint.itemId
                : candidate.name + " (" + candidate.lootPoint.itemId + ")";
        if (ImGui::Selectable(label.c_str(), selected) && !selected) {
          draft.exitPoint.requiredItemId = candidate.lootPoint.itemId;
          draft.exitPoint.requiredItemCount =
              std::max<std::uint32_t>(
                  draft.exitPoint.requiredItemCount, 1U);
          requirementChanged = true;
        }
      }
      ImGui::EndCombo();
    }
    observeCreativeDesktopDiscretePropertyWidget(activity,
                                                 requirementChanged);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        inputText("Required item ID##layout_object_properties",
                  draft.exitPoint.requiredItemId));
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Required count##layout_object_properties",
                           ImGuiDataType_U32,
                           &draft.exitPoint.requiredItemCount));
    if (!exitSettingsValid) {
      ImGui::TextColored(
          ImVec4{0.94F, 0.45F, 0.32F, 1.0F},
          "Required item and count must both be empty/zero or both be set");
    }
  }

  finishCreativeDesktopWorldLayoutPropertyEdit(
      activity, current, draft,
      spawnSettingsValid && npcSettingsValid && lootSettingsValid &&
          exitSettingsValid,
      "Reset object", state, objectIndex, object.stableKey, commands);
}

}  // namespace

void drawCreativeEditorWorldLayoutRoofApertureInspector(
    CreativeEditorWorldLayoutState& state,
    std::size_t apertureIndex,
    CreativeDesktopCommandFrame& commands) {
  drawRoofApertureInspectorForIndex(state, apertureIndex, commands);
}

void drawCreativeEditorWorldLayoutSourceInspector(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    CreativeDesktopCommandFrame& commands) {
  drawLevelInspector(state, commands);
  if (state.selection.kind ==
      CreativeEditorWorldLayoutSelectionKind::RoofAperture) {
    drawCreativeEditorWorldLayoutRoofApertureInspector(
        state, state.selection.index, commands);
  } else {
    state.roofApertureSettingsDraft = {};
  }
  drawTerrainProfileInspector(state, document, commands);
  drawTerrainPathInspector(state, document, commands);
  drawObjectInspector(state, commands);
}

}  // namespace iggy3d_creative_app
