#include "EditorDesktopInspectorInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/play/NpcSpawn.hpp"
#include "app/iggy3d/creative/play/PlayerSpawn.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

void appendCreativeDesktopPlayerSpawnFields(
    CreativeDesktopInspectorDraft& draft,
    const cr::CreativeObject& object,
    bool fieldsDisabled,
    CreativeDesktopCommandFrame& commands) {
  if (object.kind != cr::CreativeObjectKind::SpawnPoint) {
    return;
  }
  const auto commit = [&]() {
    if (!cr::isValidCreativePlayerSpawnSettings(draft.playerSpawn)) {
      draft.validation =
          "Profile/group IDs must be non-empty identifiers; clearance must be 0.30-4.00 m";
      return;
    }
    draft.validation.clear();
    commands.enqueue(
        CreativeDesktopCommandId::SetPlayerSpawnSettings,
        CreativeDesktopPlayerSpawnPayload{
            object.id, draft.playerSpawn});
  };

  ImGui::SeparatorText("Player Spawn");
  ImGui::BeginDisabled(fieldsDisabled);
  const bool profileCommitted = creativeDesktopInputTextStdString(
      "Player profile", &draft.playerSpawn.playerProfileId,
      ImGuiInputTextFlags_EnterReturnsTrue);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (profileCommitted || ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  const bool groupCommitted = creativeDesktopInputTextStdString(
      "Spawn group", &draft.playerSpawn.spawnGroup,
      ImGuiInputTextFlags_EnterReturnsTrue);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (groupCommitted || ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  ImGui::InputDouble(
      "Clearance radius (m)",
      &draft.playerSpawn.validationRadiusMeters, 0.05, 0.25, "%.2f");
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  constexpr std::uint16_t kPriorityStep = 1U;
  constexpr std::uint16_t kPriorityFastStep = 10U;
  if (ImGui::InputScalar(
          "Fallback priority", ImGuiDataType_U16,
          &draft.playerSpawn.fallbackPriority, &kPriorityStep,
          &kPriorityFastStep, "%u")) {
    commit();
  }
  ImGui::EndDisabled();

  ImGui::TextDisabled("Lower priority wins; ties use object id");
  ImGui::TextDisabled("Facing follows Rotation Y");
  if (cr::isValidCreativePlayerProfileId(
          draft.playerSpawn.playerProfileId) &&
      !cr::isSupportedCreativePlayerProfileId(
          draft.playerSpawn.playerProfileId)) {
    ImGui::TextColored(
        ImVec4{1.0F, 0.72F, 0.22F, 1.0F},
        "Profile is not available in the current runtime");
  }
}

void appendCreativeDesktopNpcSpawnFields(
    CreativeDesktopInspectorDraft& draft,
    const cr::CreativeDocument& document,
    const cr::CreativeObject& object,
    bool fieldsDisabled,
    CreativeDesktopCommandFrame& commands) {
  const bool npcActor =
      object.kind == cr::CreativeObjectKind::NpcSpawn ||
      object.kind == cr::CreativeObjectKind::EnemySpawn;
  if (!npcActor) {
    return;
  }
  const auto commit = [&]() {
    if (!cr::isValidCreativeNpcSpawnSettings(draft.npcSpawn)) {
      draft.validation =
          "Profile must be a lowercase identifier; health is 0-32767; alert is 0-1";
      return;
    }
    draft.validation.clear();
    commands.enqueue(
        CreativeDesktopCommandId::SetNpcSpawnSettings,
        CreativeDesktopNpcSpawnPayload{object.id, draft.npcSpawn});
  };

  ImGui::SeparatorText("NPC Spawn");
  ImGui::BeginDisabled(fieldsDisabled);
  const bool profileCommitted = creativeDesktopInputTextStdString(
      "Behavior profile", &draft.npcSpawn.behaviorProfileId,
      ImGuiInputTextFlags_EnterReturnsTrue);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (profileCommitted || ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }

  constexpr std::array teams{
      cr::CreativeNpcTeam::ActorDefault,
      cr::CreativeNpcTeam::PlayerAllied,
      cr::CreativeNpcTeam::Hostile,
  };
  const std::string teamLabel =
      std::string(cr::toString(draft.npcSpawn.team));
  if (ImGui::BeginCombo("Team", teamLabel.c_str())) {
    for (const cr::CreativeNpcTeam team : teams) {
      const bool selected = team == draft.npcSpawn.team;
      const std::string label = std::string(cr::toString(team));
      if (ImGui::Selectable(label.c_str(), selected) && !selected) {
        draft.npcSpawn.team = team;
        commit();
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  constexpr std::uint16_t kHealthStep = 1U;
  constexpr std::uint16_t kHealthFastStep = 10U;
  if (ImGui::InputScalar(
          "Health override", ImGuiDataType_U16,
          &draft.npcSpawn.hitPoints, &kHealthStep,
          &kHealthFastStep, "%u")) {
    commit();
  }
  ImGui::TextDisabled("0 uses the actor-kind default");

  constexpr double kMinimumAlert = 0.0;
  constexpr double kMaximumAlert = 1.0;
  ImGui::SliderScalar(
      "Initial alert", ImGuiDataType_Double,
      &draft.npcSpawn.initialAlertLevel, &kMinimumAlert,
      &kMaximumAlert, "%.2f");
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }

  constexpr std::array policies{
      cr::CreativeNpcSpawnPolicy::AtPlayStart,
      cr::CreativeNpcSpawnPolicy::Disabled,
  };
  const std::string policyLabel =
      std::string(cr::toString(draft.npcSpawn.spawnPolicy));
  if (ImGui::BeginCombo("Spawn policy", policyLabel.c_str())) {
    for (const cr::CreativeNpcSpawnPolicy policy : policies) {
      const bool selected = policy == draft.npcSpawn.spawnPolicy;
      const std::string label = std::string(cr::toString(policy));
      if (ImGui::Selectable(label.c_str(), selected) && !selected) {
        draft.npcSpawn.spawnPolicy = policy;
        commit();
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::EndDisabled();

  ImGui::TextDisabled("Facing follows Rotation Y");
  const cr::CreativeObject* patrol =
      object.parentId.has_value()
          ? document.findObject(object.parentId.value())
          : nullptr;
  if (patrol != nullptr &&
      patrol->kind == cr::CreativeObjectKind::PatrolRoute) {
    ImGui::TextDisabled("Patrol: %s", patrol->name.c_str());
  } else {
    ImGui::TextDisabled("Patrol: stationary");
  }
  if (cr::isValidCreativeNpcBehaviorProfileId(
          draft.npcSpawn.behaviorProfileId) &&
      !cr::isSupportedCreativeNpcBehaviorProfileId(
          draft.npcSpawn.behaviorProfileId)) {
    ImGui::TextColored(
        ImVec4{1.0F, 0.72F, 0.22F, 1.0F},
        "Profile is not available in the current runtime");
  }
}

void appendCreativeDesktopLootPointFields(
    CreativeDesktopInspectorDraft& draft,
    const cr::CreativeObject& object,
    bool fieldsDisabled,
    CreativeDesktopCommandFrame& commands) {
  if (object.kind != cr::CreativeObjectKind::LootPoint) {
    return;
  }
  const auto commit = [&]() {
    if (!cr::isValidCreativeLootPointSettings(draft.lootPoint)) {
      draft.validation =
          "Item ID must be empty or a valid identifier; count must be 1-65535";
      return;
    }
    draft.validation.clear();
    commands.enqueue(
        CreativeDesktopCommandId::SetLootPointSettings,
        CreativeDesktopLootPointPayload{object.id, draft.lootPoint});
  };

  ImGui::SeparatorText("Loot");
  ImGui::BeginDisabled(fieldsDisabled);
  const bool itemCommitted = creativeDesktopInputTextStdString(
      "Item ID", &draft.lootPoint.itemId,
      ImGuiInputTextFlags_EnterReturnsTrue);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (itemCommitted || ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  constexpr std::uint32_t kCountStep = 1U;
  constexpr std::uint32_t kCountFastStep = 10U;
  ImGui::InputScalar(
      "Count", ImGuiDataType_U32, &draft.lootPoint.itemCount,
      &kCountStep, &kCountFastStep, "%u");
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  if (ImGui::Checkbox(
          "Remove after collection",
          &draft.lootPoint.deactivateOnCollect)) {
    commit();
  }
  ImGui::EndDisabled();

  const std::string effectiveId =
      draft.lootPoint.itemId.empty()
          ? cr::makeCreativeAutomaticLootItemId(object.id)
          : draft.lootPoint.itemId;
  ImGui::TextDisabled("Effective item: %s", effectiveId.c_str());
}

void appendCreativeDesktopExitPointFields(
    CreativeDesktopInspectorDraft& draft,
    const cr::CreativeDocument& document,
    const cr::CreativeObject& object,
    bool fieldsDisabled,
    CreativeDesktopCommandFrame& commands) {
  if (object.kind != cr::CreativeObjectKind::ExitPoint) {
    return;
  }
  const auto commit = [&]() {
    if (!cr::isValidCreativeExitPointSettings(draft.exitPoint)) {
      draft.validation =
          "Required item and count must both be empty/zero or both be set";
      return;
    }
    draft.validation.clear();
    commands.enqueue(
        CreativeDesktopCommandId::SetExitPointSettings,
        CreativeDesktopExitPointPayload{object.id, draft.exitPoint});
  };

  ImGui::SeparatorText("Exit Objective");
  ImGui::BeginDisabled(fieldsDisabled);
  const char* sourceLabel =
      draft.exitPoint.requiredItemId.empty()
          ? "No item required"
          : draft.exitPoint.requiredItemId.c_str();
  if (ImGui::BeginCombo("Loot requirement", sourceLabel)) {
    const bool noRequirement =
        draft.exitPoint.requiredItemId.empty();
    if (ImGui::Selectable("No item required", noRequirement) &&
        !noRequirement) {
      draft.exitPoint.requiredItemId.clear();
      draft.exitPoint.requiredItemCount = 0U;
      commit();
    }
    for (const cr::CreativeObject& candidate : document.objects()) {
      if (candidate.kind != cr::CreativeObjectKind::LootPoint ||
          !cr::creativeObjectEffectivelyVisible(
              document, candidate.id)) {
        continue;
      }
      const std::string itemId =
          cr::effectiveCreativeLootPointItemId(candidate);
      const bool selected =
          itemId == draft.exitPoint.requiredItemId;
      const std::string label =
          candidate.name.empty()
              ? itemId
              : candidate.name + " (" + itemId + ")";
      if (ImGui::Selectable(label.c_str(), selected) && !selected) {
        draft.exitPoint.requiredItemId = itemId;
        draft.exitPoint.requiredItemCount =
            std::max<std::uint32_t>(
                draft.exitPoint.requiredItemCount, 1U);
        commit();
      }
    }
    ImGui::EndCombo();
  }
  const bool itemCommitted = creativeDesktopInputTextStdString(
      "Required item ID", &draft.exitPoint.requiredItemId,
      ImGuiInputTextFlags_EnterReturnsTrue);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (itemCommitted || ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  constexpr std::uint32_t kCountStep = 1U;
  constexpr std::uint32_t kCountFastStep = 10U;
  ImGui::InputScalar(
      "Required count", ImGuiDataType_U32,
      &draft.exitPoint.requiredItemCount, &kCountStep,
      &kCountFastStep, "%u");
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }
  ImGui::EndDisabled();
  const std::string objectiveId =
      cr::makeCreativeExitObjectiveId(object.id);
  ImGui::TextDisabled("Objective: %s", objectiveId.c_str());
}

void appendCreativeDesktopMovingPlatformFields(
    CreativeDesktopInspectorDraft& draft,
    const cr::CreativeObject& object,
    const CreativeMovingPlatformPreviewState& preview,
    const CreativeMovingPlatformPathEditState& pathEdit,
    bool fieldsDisabled,
    CreativeDesktopCommandFrame& commands) {
  if (object.kind != cr::CreativeObjectKind::MovingPlatform) {
    return;
  }
  const auto commit = [&]() {
    if (!cr::isValidCreativeMovingPlatformSettings(
            draft.movingPlatform)) {
      draft.validation =
          "Speed must be finite and between 0 and 100 m/s";
      return;
    }
    draft.validation.clear();
    commands.enqueue(
        CreativeDesktopCommandId::SetMovingPlatformSettings,
        CreativeDesktopMovingPlatformPayload{
            object.id, draft.movingPlatform});
  };

  ImGui::SeparatorText("Motion");
  ImGui::BeginDisabled(fieldsDisabled);
  ImGui::InputDouble(
      "Speed (m/s)",
      &draft.movingPlatform.speedMetersPerSecond, 0.1, 1.0, "%.2f");
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit();
  }

  const std::string traversalLabel =
      std::string(cr::toString(
          draft.movingPlatform.traversalMode));
  if (ImGui::BeginCombo("Traversal", traversalLabel.c_str())) {
    constexpr std::array modes{
        cr::CreativeMovingPlatformTraversalMode::PingPong,
        cr::CreativeMovingPlatformTraversalMode::Loop,
    };
    for (const cr::CreativeMovingPlatformTraversalMode mode : modes) {
      const bool selected =
          mode == draft.movingPlatform.traversalMode;
      if (ImGui::Selectable(
              std::string(cr::toString(mode)).c_str(), selected) &&
          !selected) {
        draft.movingPlatform.traversalMode = mode;
        commit();
      }
    }
    ImGui::EndCombo();
  }
  if (ImGui::Checkbox(
          "Starts moving",
          &draft.movingPlatform.startsActive)) {
    commit();
  }
  ImGui::EndDisabled();
  ImGui::TextDisabled(
      "Waypoints: %llu",
      static_cast<unsigned long long>(object.pathPoints.size()));

  if (object.pathPoints.empty()) {
    return;
  }

  std::size_t waypointIndex = 0U;
  if (pathEdit.available && pathEdit.pointSelected &&
      pathEdit.objectId == object.id &&
      pathEdit.selectedPointIndex < object.pathPoints.size()) {
    waypointIndex = pathEdit.selectedPointIndex;
  }
  if (!draft.editing &&
      draft.movingPlatformWaypointIndex != waypointIndex) {
    draft.movingPlatformWaypointIndex = waypointIndex;
    draft.movingPlatformWaypointDwellSeconds =
        object.pathPoints[waypointIndex].dwellSeconds;
  }

  const std::string waypointLabel =
      "Point " + std::to_string(waypointIndex + 1U);
  ImGui::BeginDisabled(fieldsDisabled);
  if (ImGui::BeginCombo("Waypoint", waypointLabel.c_str())) {
    for (std::size_t index = 0U;
         index < object.pathPoints.size(); ++index) {
      const std::string label =
          "Point " + std::to_string(index + 1U);
      const bool selected = index == waypointIndex;
      if (ImGui::Selectable(label.c_str(), selected) && !selected) {
        draft.movingPlatformWaypointIndex = index;
        draft.movingPlatformWaypointDwellSeconds =
            object.pathPoints[index].dwellSeconds;
        commands.enqueue(
            CreativeDesktopCommandId::SelectMovingPlatformWaypoint,
            CreativeDesktopMovingPlatformWaypointPayload{
                object.id, index, 0.0});
      }
    }
    ImGui::EndCombo();
  }
  ImGui::InputDouble(
      "Wait (s)", &draft.movingPlatformWaypointDwellSeconds,
      0.25, 1.0, "%.2f");
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    const double dwell =
        draft.movingPlatformWaypointDwellSeconds;
    if (!std::isfinite(dwell) || dwell < 0.0 ||
        dwell > cr::kCreativePathPointMaximumDwellSeconds) {
      draft.validation =
          "Wait must be between 0 and 60 seconds";
    } else {
      draft.validation.clear();
      commands.enqueue(
          CreativeDesktopCommandId::SetMovingPlatformWaypointDwell,
          CreativeDesktopMovingPlatformWaypointPayload{
              object.id, draft.movingPlatformWaypointIndex, dwell});
    }
  }
  ImGui::EndDisabled();

  ImGui::SeparatorText("Route Preview");
  ImGui::Text(
      "%s  |  %.0f%%",
      std::string(
          creativeMovingPlatformPreviewStatusLabel(preview)).c_str(),
      preview.normalizedProgress * 100.0);
  const bool previewDisabled =
      fieldsDisabled || !preview.available ||
      preview.objectId != object.id;
  ImGui::BeginDisabled(previewDisabled);
  if (ImGui::Button(preview.playing ? "Pause" : "Play")) {
    commands.enqueue(
        CreativeDesktopCommandId::ToggleMovingPlatformPreview,
        CreativeDesktopMovingPlatformPreviewPayload{
            object.id, preview.normalizedProgress});
  }
  ImGui::SameLine();
  if (ImGui::Button("Restart")) {
    commands.enqueue(
        CreativeDesktopCommandId::RestartMovingPlatformPreview,
        CreativeDesktopMovingPlatformPreviewPayload{object.id, 0.0});
  }
  float progressPercent =
      static_cast<float>(preview.normalizedProgress * 100.0);
  if (ImGui::SliderFloat(
          "Progress", &progressPercent, 0.0F, 100.0F, "%.0f%%")) {
    commands.enqueue(
        CreativeDesktopCommandId::SeekMovingPlatformPreview,
        CreativeDesktopMovingPlatformPreviewPayload{
            object.id,
            static_cast<double>(progressPercent) / 100.0});
  }
  ImGui::EndDisabled();
}

}  // namespace iggy3d_creative_app
