#include "EditorDesktopWorldLayoutSourceInspectorInternal.hpp"

namespace iggy3d_creative_app::detail {

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


}  // namespace iggy3d_creative_app::detail
