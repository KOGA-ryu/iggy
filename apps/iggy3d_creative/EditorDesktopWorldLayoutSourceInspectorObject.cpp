#include "EditorDesktopWorldLayoutSourceInspectorInternal.hpp"

namespace iggy3d_creative_app::detail {

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

}  // namespace iggy3d_creative_app::detail
