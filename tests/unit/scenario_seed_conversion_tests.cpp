#include "core/result/Result.hpp"
#include "runtime/session/ScenarioSeedConversion.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool validValuesConvert() {
  const auto player = iggy3d::playerSlotKindFromScenario(
      iggy3d::ScenarioPlayerSlotKind::Local);
  const auto clock = iggy3d::clockModeFromScenario(iggy3d::ScenarioClockMode::Slow);
  const auto camera = iggy3d::cameraModeFromScenario(
      iggy3d::ScenarioCameraMode::TacticalOverhead);
  const auto patrol = iggy3d::patrolModeFromScenario(
      iggy3d::ScenarioPatrolMode::PingPong);
  return expect(player.status == iggy3d::ResultStatus::Ok &&
                    player.value == iggy3d::PlayerSlotKind::Local,
                "player slot conversion") &&
         expect(clock.status == iggy3d::ResultStatus::Ok &&
                    clock.value == iggy3d::ClockMode::Slow,
                "clock conversion") &&
         expect(camera.status == iggy3d::ResultStatus::Ok &&
                    camera.value == iggy3d::CameraMode::TacticalOverhead,
                "camera conversion") &&
         expect(patrol.status == iggy3d::ResultStatus::Ok &&
                    patrol.value == iggy3d::PatrolMode::PingPong,
                "patrol conversion");
}

bool invalidValuesFailClosed() {
  const auto player = iggy3d::playerSlotKindFromScenario(
      static_cast<iggy3d::ScenarioPlayerSlotKind>(99U));
  const auto clock = iggy3d::clockModeFromScenario(
      static_cast<iggy3d::ScenarioClockMode>(99U));
  const auto camera = iggy3d::cameraModeFromScenario(
      static_cast<iggy3d::ScenarioCameraMode>(99U));
  const auto patrol = iggy3d::patrolModeFromScenario(
      static_cast<iggy3d::ScenarioPatrolMode>(99U));
  return expect(player.status == iggy3d::ResultStatus::Error &&
                    player.error.code == "scenario_seed.invalid_player_slot_kind",
                "invalid player slot fails closed") &&
         expect(clock.status == iggy3d::ResultStatus::Error &&
                    clock.error.code == "scenario_seed.invalid_clock_mode",
                "invalid clock fails closed") &&
         expect(camera.status == iggy3d::ResultStatus::Error &&
                    camera.error.code == "scenario_seed.invalid_camera_mode",
                "invalid camera fails closed") &&
         expect(patrol.status == iggy3d::ResultStatus::Error &&
                    patrol.error.code == "scenario_seed.invalid_patrol_mode",
                "invalid patrol fails closed");
}

bool entityValuesConvertAndInvalidValuesFailClosed() {
  iggy3d::ScenarioEntitySeed entity;
  entity.stableName = "seed_entity";
  entity.kind = iggy3d::ScenarioEntityKind::Npc;
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::ScenarioTargetAction::Attack};
  entity.interaction.kind = iggy3d::ScenarioInteractionKind::Inspect;
  entity.interaction.primaryEffect = iggy3d::ScenarioInteractionEffectKind::EmitEventOnly;
  const auto converted = iggy3d::entityFromScenario(entity, {42U});

  iggy3d::ScenarioCombatantSeed combatSeed;
  combatSeed.factionId = 2U;
  combatSeed.hitPoints = 3;
  combatSeed.maxHitPoints = 5;
  const auto combatant = iggy3d::combatantFromScenario(&combatSeed, {42U});
  const auto noCombatant = iggy3d::combatantFromScenario(nullptr, {42U});

  iggy3d::ScenarioObjectiveSeed validObjective;
  validObjective.id = "seed_objective";
  validObjective.initialStatus = iggy3d::ObjectiveStatusSeed::Complete;
  validObjective.condition = "InventoryContains";
  validObjective.playerSlot = 7U;
  validObjective.itemId = "seed_item";
  validObjective.itemCount = 2U;
  const auto convertedObjective = iggy3d::objectiveFromScenario(validObjective);

  iggy3d::ScenarioEntitySeed invalidKind = entity;
  invalidKind.kind = static_cast<iggy3d::ScenarioEntityKind>(99U);
  iggy3d::ScenarioEntitySeed invalidAction = entity;
  invalidAction.targeting.actions = {
      static_cast<iggy3d::ScenarioTargetAction>(99U)};
  iggy3d::ScenarioEntitySeed invalidInteraction = entity;
  invalidInteraction.interaction.kind =
      static_cast<iggy3d::ScenarioInteractionKind>(99U);
  iggy3d::ScenarioEntitySeed invalidEffect = entity;
  invalidEffect.interaction.primaryEffect =
      static_cast<iggy3d::ScenarioInteractionEffectKind>(99U);
  iggy3d::ScenarioObjectiveSeed invalidObjective;
  invalidObjective.initialStatus = static_cast<iggy3d::ObjectiveStatusSeed>(99U);

  const auto kind = iggy3d::entityFromScenario(invalidKind, {42U});
  const auto action = iggy3d::entityFromScenario(invalidAction, {42U});
  const auto interaction = iggy3d::entityFromScenario(invalidInteraction, {42U});
  const auto effect = iggy3d::entityFromScenario(invalidEffect, {42U});
  const auto objective = iggy3d::objectiveFromScenario(invalidObjective);
  return expect(converted.status == iggy3d::ResultStatus::Ok &&
                    converted.value.id.value == 42U &&
                    converted.value.kind == iggy3d::EntityKind::Npc &&
                    converted.value.targeting.actions.size() == 1U &&
                    converted.value.targeting.actions.front() == iggy3d::TargetAction::Attack &&
                    converted.value.interaction.kind == iggy3d::InteractionKind::Inspect &&
                    converted.value.interaction.primaryEffect ==
                        iggy3d::InteractionEffectKind::EmitEventOnly,
                "entity conversion") &&
         expect(combatant.status == iggy3d::ResultStatus::Ok &&
                    combatant.value.has_value() &&
                    combatant.value->entity.value == 42U &&
                    combatant.value->factionId == 2U &&
                    combatant.value->hitPoints == 3 &&
                    combatant.value->maxHitPoints == 5 && !combatant.value->defeated,
                "combatant conversion") &&
         expect(noCombatant.status == iggy3d::ResultStatus::Ok &&
                    !noCombatant.value.has_value(),
                "absent combatant conversion") &&
         expect(convertedObjective.status == iggy3d::ResultStatus::Ok &&
                    convertedObjective.value.objectiveId == "seed_objective" &&
                    convertedObjective.value.status == iggy3d::ObjectiveStatus::Complete &&
                    convertedObjective.value.condition.kind ==
                        iggy3d::ObjectiveConditionKind::PlayerHasItem &&
                    convertedObjective.value.condition.playerSlot == 7U &&
                    convertedObjective.value.condition.itemId == "seed_item" &&
                    convertedObjective.value.condition.itemCount == 2U,
                "objective conversion") &&
         expect(kind.status == iggy3d::ResultStatus::Error &&
                    kind.error.code == "scenario_seed.invalid_entity_kind",
                "invalid entity kind fails closed") &&
         expect(action.status == iggy3d::ResultStatus::Error &&
                    action.error.code == "scenario_seed.invalid_target_action",
                "invalid target action fails closed") &&
         expect(interaction.status == iggy3d::ResultStatus::Error &&
                    interaction.error.code == "scenario_seed.invalid_interaction_kind",
                "invalid interaction kind fails closed") &&
         expect(effect.status == iggy3d::ResultStatus::Error &&
                    effect.error.code == "scenario_seed.invalid_interaction_effect",
                "invalid interaction effect fails closed") &&
         expect(objective.status == iggy3d::ResultStatus::Error &&
                    objective.error.code == "scenario_seed.invalid_objective_status",
                "invalid objective status fails closed");
}

}  // namespace

int main() {
  return validValuesConvert() && invalidValuesFailClosed() &&
                 entityValuesConvertAndInvalidValuesFailClosed()
             ? 0
             : 1;
}
