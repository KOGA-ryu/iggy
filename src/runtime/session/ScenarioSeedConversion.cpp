#include "runtime/session/ScenarioSeedConversion.hpp"

#include <utility>

#include "runtime/session/Session.hpp"

namespace iggy3d {
namespace {

template <typename T>
Result<T> invalid(std::string code, std::string message) {
  Result<T> result;
  result.error = {std::move(code), std::move(message)};
  return result;
}

template <typename T>
Result<T> converted(T value) {
  Result<T> result;
  result.status = ResultStatus::Ok;
  result.value = value;
  return result;
}

}  // namespace

Result<PlayerSlotKind> playerSlotKindFromScenario(ScenarioPlayerSlotKind kind) {
  switch (kind) {
    case ScenarioPlayerSlotKind::Unknown:
      return converted(PlayerSlotKind::Unknown);
    case ScenarioPlayerSlotKind::Local:
      return converted(PlayerSlotKind::Local);
    case ScenarioPlayerSlotKind::Remote:
      return converted(PlayerSlotKind::Remote);
    case ScenarioPlayerSlotKind::Ai:
      return converted(PlayerSlotKind::Ai);
    case ScenarioPlayerSlotKind::Observer:
      return converted(PlayerSlotKind::Observer);
  }
  return invalid<PlayerSlotKind>("scenario_seed.invalid_player_slot_kind",
                                 "scenario player slot kind is out of range");
}

Result<ClockMode> clockModeFromScenario(ScenarioClockMode mode) {
  switch (mode) {
    case ScenarioClockMode::Normal:
      return converted(ClockMode::Normal);
    case ScenarioClockMode::Slow:
      return converted(ClockMode::Slow);
    case ScenarioClockMode::Paused:
      return converted(ClockMode::Paused);
  }
  return invalid<ClockMode>("scenario_seed.invalid_clock_mode",
                            "scenario clock mode is out of range");
}

Result<CameraMode> cameraModeFromScenario(ScenarioCameraMode mode) {
  switch (mode) {
    case ScenarioCameraMode::FirstPerson:
      return converted(CameraMode::FirstPerson);
    case ScenarioCameraMode::ThirdPerson:
      return converted(CameraMode::ThirdPerson);
    case ScenarioCameraMode::TacticalOverhead:
      return converted(CameraMode::TacticalOverhead);
  }
  return invalid<CameraMode>("scenario_seed.invalid_camera_mode",
                             "scenario camera mode is out of range");
}

Result<PatrolMode> patrolModeFromScenario(ScenarioPatrolMode mode) {
  switch (mode) {
    case ScenarioPatrolMode::Loop:
      return converted(PatrolMode::Loop);
    case ScenarioPatrolMode::PingPong:
      return converted(PatrolMode::PingPong);
  }
  return invalid<PatrolMode>("scenario_seed.invalid_patrol_mode",
                             "scenario patrol mode is out of range");
}

namespace {

Result<EntityKind> entityKindFromScenario(ScenarioEntityKind kind) {
  switch (kind) {
    case ScenarioEntityKind::Unknown:
      return converted(EntityKind::Unknown);
    case ScenarioEntityKind::Player:
      return converted(EntityKind::Player);
    case ScenarioEntityKind::Pickup:
      return converted(EntityKind::Pickup);
    case ScenarioEntityKind::Door:
      return converted(EntityKind::Door);
    case ScenarioEntityKind::Marker:
      return converted(EntityKind::Marker);
    case ScenarioEntityKind::Npc:
      return converted(EntityKind::Npc);
  }
  return invalid<EntityKind>("scenario_seed.invalid_entity_kind",
                             "scenario entity kind is out of range");
}

Result<TargetAction> targetActionFromScenario(ScenarioTargetAction action) {
  switch (action) {
    case ScenarioTargetAction::Interact:
      return converted(TargetAction::Interact);
    case ScenarioTargetAction::Inspect:
      return converted(TargetAction::Inspect);
    case ScenarioTargetAction::Attack:
      return converted(TargetAction::Attack);
    case ScenarioTargetAction::Move:
      return converted(TargetAction::Move);
  }
  return invalid<TargetAction>("scenario_seed.invalid_target_action",
                               "scenario target action is out of range");
}

Result<InteractionKind> interactionKindFromScenario(ScenarioInteractionKind kind) {
  switch (kind) {
    case ScenarioInteractionKind::None:
      return converted(InteractionKind::None);
    case ScenarioInteractionKind::Pickup:
      return converted(InteractionKind::Pickup);
    case ScenarioInteractionKind::Activate:
      return converted(InteractionKind::Activate);
    case ScenarioInteractionKind::OpenDoor:
      return converted(InteractionKind::OpenDoor);
    case ScenarioInteractionKind::Inspect:
      return converted(InteractionKind::Inspect);
    case ScenarioInteractionKind::ObjectiveTrigger:
      return converted(InteractionKind::ObjectiveTrigger);
  }
  return invalid<InteractionKind>("scenario_seed.invalid_interaction_kind",
                                 "scenario interaction kind is out of range");
}

Result<InteractionEffectKind> interactionEffectFromScenario(
    ScenarioInteractionEffectKind effect) {
  switch (effect) {
    case ScenarioInteractionEffectKind::None:
      return converted(InteractionEffectKind::None);
    case ScenarioInteractionEffectKind::AddItemToInventory:
      return converted(InteractionEffectKind::AddItemToInventory);
    case ScenarioInteractionEffectKind::DeactivateTarget:
      return converted(InteractionEffectKind::DeactivateTarget);
    case ScenarioInteractionEffectKind::CompleteObjective:
      return converted(InteractionEffectKind::CompleteObjective);
    case ScenarioInteractionEffectKind::EmitEventOnly:
      return converted(InteractionEffectKind::EmitEventOnly);
  }
  return invalid<InteractionEffectKind>("scenario_seed.invalid_interaction_effect",
                                         "scenario interaction effect is out of range");
}

Result<ObjectiveStatus> objectiveStatusFromScenario(ObjectiveStatusSeed status) {
  switch (status) {
    case ObjectiveStatusSeed::Active:
      return converted(ObjectiveStatus::Active);
    case ObjectiveStatusSeed::Complete:
      return converted(ObjectiveStatus::Complete);
    case ObjectiveStatusSeed::Failed:
      return converted(ObjectiveStatus::Failed);
  }
  return invalid<ObjectiveStatus>("scenario_seed.invalid_objective_status",
                                  "scenario objective status is out of range");
}

}  // namespace

Result<EntityState> entityFromScenario(const ScenarioEntitySeed& seed, EntityId id) {
  const Result<EntityKind> kind = entityKindFromScenario(seed.kind);
  if (kind.status != ResultStatus::Ok) {
    return invalid<EntityState>(kind.error.code, kind.error.message);
  }
  const Result<InteractionKind> interactionKind =
      interactionKindFromScenario(seed.interaction.kind);
  if (interactionKind.status != ResultStatus::Ok) {
    return invalid<EntityState>(interactionKind.error.code, interactionKind.error.message);
  }
  const Result<InteractionEffectKind> interactionEffect =
      interactionEffectFromScenario(seed.interaction.primaryEffect);
  if (interactionEffect.status != ResultStatus::Ok) {
    return invalid<EntityState>(interactionEffect.error.code, interactionEffect.error.message);
  }

  EntityState entity;
  entity.id = id;
  entity.stableName = seed.stableName;
  entity.kind = kind.value;
  entity.transform = seed.transform;
  entity.localBounds = seed.localBounds;
  entity.active = seed.active;
  entity.persistent = seed.persistent;
  entity.targeting.targetable = seed.targeting.targetable;
  for (const ScenarioTargetAction action : seed.targeting.actions) {
    const Result<TargetAction> convertedAction = targetActionFromScenario(action);
    if (convertedAction.status != ResultStatus::Ok) {
      return invalid<EntityState>(convertedAction.error.code, convertedAction.error.message);
    }
    entity.targeting.actions.push_back(convertedAction.value);
  }
  entity.interaction.kind = interactionKind.value;
  entity.interaction.primaryEffect = interactionEffect.value;
  entity.interaction.itemId = seed.interaction.itemId;
  entity.interaction.itemCount = seed.interaction.itemCount;
  entity.interaction.objectiveId = seed.interaction.objectiveId;
  entity.interaction.requiredItemId = seed.interaction.requiredItemId;
  entity.interaction.requiredItemCount = seed.interaction.requiredItemCount;
  entity.interaction.repeatable = seed.interaction.repeatable;
  entity.interaction.deactivateTargetOnSuccess =
      seed.interaction.deactivateTargetOnSuccess;
  return converted(entity);
}

Result<std::optional<CombatantState>> combatantFromScenario(
    const ScenarioCombatantSeed* seed, EntityId id) {
  if (seed == nullptr) {
    return converted(std::optional<CombatantState>{});
  }
  CombatantState combatant;
  combatant.entity = id;
  combatant.factionId = seed->factionId;
  combatant.hitPoints = seed->hitPoints;
  combatant.maxHitPoints = seed->maxHitPoints;
  combatant.defeated = combatant.hitPoints == 0;
  return converted(std::optional<CombatantState>{combatant});
}

Result<ObjectiveRecord> objectiveFromScenario(const ScenarioObjectiveSeed& seed) {
  const Result<ObjectiveStatus> status = objectiveStatusFromScenario(seed.initialStatus);
  if (status.status != ResultStatus::Ok) {
    return invalid<ObjectiveRecord>(status.error.code, status.error.message);
  }
  ObjectiveRecord objective;
  objective.objectiveId = seed.id;
  objective.status = status.value;
  objective.condition.kind = seed.condition == "InventoryContains"
                                 ? ObjectiveConditionKind::PlayerHasItem
                                 : ObjectiveConditionKind::None;
  objective.condition.playerSlot = seed.playerSlot;
  objective.condition.itemId = seed.itemId;
  objective.condition.itemCount = seed.itemCount;
  return converted(objective);
}

}  // namespace iggy3d
