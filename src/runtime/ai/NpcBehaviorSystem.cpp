#include "runtime/ai/NpcBehaviorSystem.hpp"

#include <algorithm>
#include <cmath>

#include "runtime/combat/CombatState.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {
namespace {

float horizontalDistanceMeters(Vec3 lhs, Vec3 rhs) {
  const float dx = rhs.x - lhs.x;
  const float dz = rhs.z - lhs.z;
  return std::sqrt(dx * dx + dz * dz);
}

// Horizontal vision-cone test. Returns true when the target lies within
// +/- config half-angle of the facing direction. A degenerate facing vector
// (never configured) or a target sharing the actor's column disables the gate.
bool targetWithinVisionCone(Vec3 facing,
                            Vec3 actorPosition,
                            Vec3 targetPosition,
                            float halfAngleDegrees) {
  const Vec3 flatFacing{facing.x, 0.0F, facing.z};
  const float facingLenSq = lengthSquared(flatFacing);
  if (!std::isfinite(facingLenSq) || facingLenSq < 1.0e-8F) {
    return true;  // no facing configured -> omnidirectional
  }
  const Vec3 toTarget{targetPosition.x - actorPosition.x, 0.0F,
                      targetPosition.z - actorPosition.z};
  const float toTargetLenSq = lengthSquared(toTarget);
  if (!std::isfinite(toTargetLenSq) || toTargetLenSq < 1.0e-8F) {
    return true;  // target on top of actor -> treat as visible
  }
  const float cosBetween =
      dot(flatFacing, toTarget) / std::sqrt(facingLenSq * toTargetLenSq);
  constexpr float kDegreesToRadians = 0.01745329252F;
  const float cosThreshold = std::cos(halfAngleDegrees * kDegreesToRadians);
  constexpr float kBoundaryEpsilon = 1.0e-4F;
  return cosBetween >= cosThreshold - kBoundaryEpsilon;
}

const CombatantState* findCombatant(const CombatState& combat, EntityId entity) {
  for (const CombatantState& combatant : combat.combatants) {
    if (combatant.entity == entity) {
      return &combatant;
    }
  }
  return nullptr;
}

bool isDefeated(const CombatState& combat, EntityId entity) {
  const CombatantState* combatant = findCombatant(combat, entity);
  return combatant != nullptr && combatant->defeated;
}

NpcBehaviorDecision makeDecision(const NpcBehaviorDecisionRequest& request) {
  NpcBehaviorDecision decision;
  if (request.actorState != nullptr) {
    decision.actor = request.actorState->actor;
    decision.target = request.actorState->target;
    decision.nextDecisionTick = request.actorState->nextDecisionTick;
    decision.cooldownTicksRemaining = request.actorState->cooldownTicksRemaining;
  }
  return decision;
}

bool perceptionReadyForCommand(const NpcPerceptionResult& perception) {
  return perception.status == NpcPerceptionStatus::Ready &&
         isValid(perception.actor) && isValid(perception.target);
}

bool perceptionHasActorPosition(const NpcPerceptionResult& perception) {
  return isValid(perception.actor) && isFinite(perception.actorPosition);
}

bool validGuardState(const AiActorState& actorState) {
  if (!actorState.hasHomePosition) {
    return true;
  }
  return isFinite(actorState.homePosition) &&
         std::isfinite(actorState.leashRadiusMeters) &&
         std::isfinite(actorState.returnRadiusMeters) &&
         std::isfinite(actorState.homeToleranceMeters) &&
         actorState.leashRadiusMeters > 0.0F &&
         actorState.returnRadiusMeters > 0.0F &&
         actorState.homeToleranceMeters >= 0.0F &&
         actorState.returnRadiusMeters <= actorState.leashRadiusMeters;
}

float guardReturnStopDistance(const AiActorState& actorState) {
  return std::max(actorState.returnRadiusMeters, actorState.homeToleranceMeters);
}

bool actorIsHome(const AiActorState& actorState, const NpcPerceptionResult& perception) {
  return horizontalDistanceMeters(perception.actorPosition, actorState.homePosition) <=
         guardReturnStopDistance(actorState);
}

bool actorOutsideLeash(const AiActorState& actorState,
                       const NpcPerceptionResult& perception) {
  return horizontalDistanceMeters(perception.actorPosition, actorState.homePosition) >
         actorState.leashRadiusMeters + actorState.homeToleranceMeters;
}

bool targetOutsideLeash(const AiActorState& actorState,
                        const NpcPerceptionResult& perception) {
  return horizontalDistanceMeters(perception.targetPosition, actorState.homePosition) >
         actorState.leashRadiusMeters;
}

bool buildHorizontalMoveDestination(Vec3 start,
                                    Vec3 target,
                                    float stepMeters,
                                    float stopDistanceMeters,
                                    Vec3& out) {
  const float dx = target.x - start.x;
  const float dz = target.z - start.z;
  const float distance = std::sqrt(dx * dx + dz * dz);
  if (!std::isfinite(distance) || distance <= 0.0001F) {
    return false;
  }
  const float moveDistance =
      std::min(stepMeters, std::max(0.0F, distance - stopDistanceMeters));
  if (moveDistance <= 0.0F) {
    return false;
  }
  out = start;
  out.x += (dx / distance) * moveDistance;
  out.z += (dz / distance) * moveDistance;
  return isFinite(out);
}

bool chaseDestinationWouldExceedLeash(const AiActorState& actorState,
                                      const NpcPerceptionResult& perception,
                                      const NpcBehaviorConfig& config) {
  Vec3 destination;
  if (!buildHorizontalMoveDestination(perception.actorPosition,
                                      perception.targetPosition,
                                      config.chaseStepMeters,
                                      config.chaseStopDistanceMeters,
                                      destination)) {
    return false;
  }
  return horizontalDistanceMeters(destination, actorState.homePosition) >
         actorState.leashRadiusMeters;
}

void setReturnDecision(NpcBehaviorDecisionRequest request,
                       NpcBehaviorDecision& decision) {
  decision.status = NpcBehaviorDecisionStatus::Decided;
  decision.behavior = AiBehaviorKind::Returning;
  decision.intent = AiIntentKind::ReturnToAnchor;
  decision.homePosition = request.actorState->homePosition;
  decision.returnStopDistanceMeters = guardReturnStopDistance(*request.actorState);
  decision.nextDecisionTick =
      request.currentTick + request.config.decisionIntervalTicks;
}

}  // namespace

std::string_view npcPerceptionStatusName(NpcPerceptionStatus status) {
  switch (status) {
    case NpcPerceptionStatus::Ready:
      return "ready";
    case NpcPerceptionStatus::InvalidWorld:
      return "invalid_world";
    case NpcPerceptionStatus::InvalidCombat:
      return "invalid_combat";
    case NpcPerceptionStatus::InvalidActor:
      return "invalid_actor";
    case NpcPerceptionStatus::InvalidTarget:
      return "invalid_target";
    case NpcPerceptionStatus::InvalidConfig:
      return "invalid_config";
    case NpcPerceptionStatus::ActorInactive:
      return "actor_inactive";
    case NpcPerceptionStatus::TargetInactive:
      return "target_inactive";
    case NpcPerceptionStatus::ActorDefeated:
      return "actor_defeated";
    case NpcPerceptionStatus::TargetDefeated:
      return "target_defeated";
    case NpcPerceptionStatus::TargetOutOfRange:
      return "target_out_of_range";
    case NpcPerceptionStatus::TargetOutOfCone:
      return "target_out_of_cone";
    case NpcPerceptionStatus::TargetOccluded:
      return "target_occluded";
  }
  return "invalid_perception";
}

std::string_view npcEngagementPolicyName(NpcEngagementPolicy policy) {
  switch (policy) {
    case NpcEngagementPolicy::Hostile:
      return "hostile";
    case NpcEngagementPolicy::Passive:
      return "passive";
  }
  return "invalid_policy";
}

bool isValidNpcBehaviorConfig(const NpcBehaviorConfig& config) {
  const bool validPolicy = config.engagementPolicy == NpcEngagementPolicy::Hostile ||
                           config.engagementPolicy == NpcEngagementPolicy::Passive;
  return validPolicy &&
         std::isfinite(config.perceptionRadiusMeters) &&
         config.perceptionRadiusMeters > 0.0F &&
         std::isfinite(config.chaseStopDistanceMeters) &&
         config.chaseStopDistanceMeters > 0.0F &&
         std::isfinite(config.attackRangeMeters) &&
         config.attackRangeMeters > 0.0F &&
         std::isfinite(config.chaseStepMeters) &&
         config.chaseStepMeters > 0.0F &&
         std::isfinite(config.visionHalfAngleDegrees) &&
         config.visionHalfAngleDegrees > 0.0F &&
         config.visionHalfAngleDegrees <= 180.0F &&
         config.attackDamage > 0 &&
         config.decisionIntervalTicks > 0U;
}

NpcPerceptionResult queryNpcPerception(const NpcPerceptionRequest& request) {
  NpcPerceptionResult result;
  result.actor = request.actor;
  result.target = request.target;

  if (!isValidNpcBehaviorConfig(request.config)) {
    result.status = NpcPerceptionStatus::InvalidConfig;
    return result;
  }
  if (request.world == nullptr) {
    result.status = NpcPerceptionStatus::InvalidWorld;
    return result;
  }
  if (request.combat == nullptr) {
    result.status = NpcPerceptionStatus::InvalidCombat;
    return result;
  }

  const EntityState* actor = request.world->findById(request.actor);
  if (actor == nullptr) {
    result.status = NpcPerceptionStatus::InvalidActor;
    return result;
  }
  const EntityState* target = request.world->findById(request.target);
  if (target == nullptr) {
    result.status = NpcPerceptionStatus::InvalidTarget;
    return result;
  }

  result.actorActive = actor->active;
  result.targetActive = target->active;
  result.actorPosition = actor->transform.position;
  result.targetPosition = target->transform.position;
  if (!actor->active) {
    result.status = NpcPerceptionStatus::ActorInactive;
    return result;
  }
  if (!target->active) {
    result.status = NpcPerceptionStatus::TargetInactive;
    return result;
  }

  result.actorDefeated = isDefeated(*request.combat, request.actor);
  result.targetDefeated = isDefeated(*request.combat, request.target);
  if (result.actorDefeated) {
    result.status = NpcPerceptionStatus::ActorDefeated;
    return result;
  }
  if (result.targetDefeated) {
    result.status = NpcPerceptionStatus::TargetDefeated;
    return result;
  }

  result.distanceMeters = horizontalDistanceMeters(result.actorPosition,
                                                  result.targetPosition);
  if (!std::isfinite(result.distanceMeters)) {
    result.status = NpcPerceptionStatus::InvalidTarget;
    return result;
  }
  result.targetInPerceptionRadius =
      result.distanceMeters <= request.config.perceptionRadiusMeters;
  result.targetInAttackRange = result.distanceMeters <= request.config.attackRangeMeters;
  if (!result.targetInPerceptionRadius) {
    result.status = NpcPerceptionStatus::TargetOutOfRange;
    return result;
  }

  result.targetInVisionCone =
      targetWithinVisionCone(request.actorFacingDirection,
                             result.actorPosition,
                             result.targetPosition,
                             request.config.visionHalfAngleDegrees);
  if (!result.targetInVisionCone) {
    result.status = NpcPerceptionStatus::TargetOutOfCone;
    return result;
  }

  result.hasLineOfSight = request.targetHasLineOfSight;
  if (!result.hasLineOfSight) {
    result.status = NpcPerceptionStatus::TargetOccluded;
    return result;
  }

  result.status = NpcPerceptionStatus::Ready;
  return result;
}

std::string_view npcBehaviorDecisionStatusName(NpcBehaviorDecisionStatus status) {
  switch (status) {
    case NpcBehaviorDecisionStatus::Decided:
      return "decided";
    case NpcBehaviorDecisionStatus::Disabled:
      return "disabled";
    case NpcBehaviorDecisionStatus::WaitingForDecisionTick:
      return "waiting_for_decision_tick";
    case NpcBehaviorDecisionStatus::ActorDefeated:
      return "actor_defeated";
    case NpcBehaviorDecisionStatus::NoTarget:
      return "no_target";
    case NpcBehaviorDecisionStatus::OnCooldown:
      return "on_cooldown";
    case NpcBehaviorDecisionStatus::InvalidConfig:
      return "invalid_config";
    case NpcBehaviorDecisionStatus::InvalidGuard:
      return "invalid_guard";
    case NpcBehaviorDecisionStatus::InvalidPerception:
      return "invalid_perception";
  }
  return "invalid_perception";
}

NpcBehaviorDecision chooseNpcBehaviorIntent(
    const NpcBehaviorDecisionRequest& request) {
  NpcBehaviorDecision decision = makeDecision(request);

  if (!isValidNpcBehaviorConfig(request.config)) {
    decision.status = NpcBehaviorDecisionStatus::InvalidConfig;
    return decision;
  }
  if (request.actorState == nullptr || !isValid(request.actorState->actor)) {
    decision.status = NpcBehaviorDecisionStatus::InvalidPerception;
    return decision;
  }
  if (!request.actorState->enabled) {
    decision.status = NpcBehaviorDecisionStatus::Disabled;
    decision.behavior = AiBehaviorKind::Idle;
    decision.intent = AiIntentKind::None;
    return decision;
  }
  if (request.currentTick < request.actorState->nextDecisionTick) {
    decision.status = NpcBehaviorDecisionStatus::WaitingForDecisionTick;
    decision.behavior = request.actorState->behavior;
    decision.intent = AiIntentKind::None;
    return decision;
  }
  if (request.perception.status == NpcPerceptionStatus::ActorDefeated) {
    decision.status = NpcBehaviorDecisionStatus::ActorDefeated;
    decision.behavior = AiBehaviorKind::Defeated;
    decision.intent = AiIntentKind::None;
    decision.nextDecisionTick =
        request.currentTick + request.config.decisionIntervalTicks;
    return decision;
  }
  if (!validGuardState(*request.actorState)) {
    decision.status = NpcBehaviorDecisionStatus::InvalidGuard;
    decision.behavior = AiBehaviorKind::Idle;
    decision.intent = AiIntentKind::None;
    decision.nextDecisionTick =
        request.currentTick + request.config.decisionIntervalTicks;
    return decision;
  }
  if (request.actorState->hasHomePosition && perceptionHasActorPosition(request.perception) &&
      actorOutsideLeash(*request.actorState, request.perception)) {
    setReturnDecision(request, decision);
    return decision;
  }
  if (request.perception.status != NpcPerceptionStatus::Ready) {
    decision.status = NpcBehaviorDecisionStatus::NoTarget;
    decision.behavior = AiBehaviorKind::Idle;
    decision.intent = AiIntentKind::Wait;
    decision.target = {};
    decision.nextDecisionTick =
        request.currentTick + request.config.decisionIntervalTicks;
    return decision;
  }

  decision.target = request.perception.target;
  decision.nextDecisionTick =
      request.currentTick + request.config.decisionIntervalTicks;
  if (request.actorState->hasHomePosition) {
    if (targetOutsideLeash(*request.actorState, request.perception)) {
      if (actorIsHome(*request.actorState, request.perception)) {
        decision.status = NpcBehaviorDecisionStatus::Decided;
        decision.behavior = AiBehaviorKind::Alert;
        decision.intent = AiIntentKind::Wait;
        return decision;
      }
      setReturnDecision(request, decision);
      return decision;
    }
  }
  if (request.config.engagementPolicy == NpcEngagementPolicy::Passive) {
    decision.status = NpcBehaviorDecisionStatus::Decided;
    decision.behavior = AiBehaviorKind::Alert;
    decision.intent = AiIntentKind::Wait;
    return decision;
  }
  if (!request.perception.targetInAttackRange) {
    if (request.actorState->hasHomePosition &&
        chaseDestinationWouldExceedLeash(*request.actorState,
                                         request.perception,
                                         request.config)) {
      setReturnDecision(request, decision);
      return decision;
    }
    decision.status = NpcBehaviorDecisionStatus::Decided;
    decision.behavior = AiBehaviorKind::Chasing;
    decision.intent = AiIntentKind::MoveTowardTarget;
    return decision;
  }

  if (request.actorState->cooldownTicksRemaining > 0U) {
    decision.status = NpcBehaviorDecisionStatus::OnCooldown;
    decision.behavior = AiBehaviorKind::Alert;
    decision.intent = AiIntentKind::Wait;
    decision.cooldownTicksRemaining =
        request.actorState->cooldownTicksRemaining - 1U;
    return decision;
  }

  decision.status = NpcBehaviorDecisionStatus::Decided;
  decision.behavior = AiBehaviorKind::Attacking;
  decision.intent = AiIntentKind::AttackTarget;
  decision.cooldownTicksRemaining = request.config.attackCooldownTicks;
  return decision;
}

std::string_view npcBehaviorCommandStatusName(NpcBehaviorCommandStatus status) {
  switch (status) {
    case NpcBehaviorCommandStatus::Built:
      return "built";
    case NpcBehaviorCommandStatus::NoCommand:
      return "no_command";
    case NpcBehaviorCommandStatus::InvalidConfig:
      return "invalid_config";
    case NpcBehaviorCommandStatus::InvalidDecision:
      return "invalid_decision";
    case NpcBehaviorCommandStatus::InvalidPerception:
      return "invalid_perception";
    case NpcBehaviorCommandStatus::InvalidDestination:
      return "invalid_destination";
  }
  return "invalid_decision";
}

NpcBehaviorCommandResult buildNpcBehaviorCommand(
    const NpcBehaviorCommandRequest& request) {
  NpcBehaviorCommandResult result;
  if (!isValidNpcBehaviorConfig(request.config)) {
    result.status = NpcBehaviorCommandStatus::InvalidConfig;
    return result;
  }
  if (!isValid(request.decision.actor)) {
    result.status = NpcBehaviorCommandStatus::InvalidDecision;
    return result;
  }

  CommandRecord command;
  command.actor = request.decision.actor;
  command.source = CommandSource::Ai;

  if (request.decision.intent == AiIntentKind::None) {
    result.status = NpcBehaviorCommandStatus::NoCommand;
    return result;
  }
  if (request.decision.intent == AiIntentKind::Wait) {
    command.kind = CommandKind::Wait;
    result.status = NpcBehaviorCommandStatus::Built;
    result.hasCommand = true;
    result.command = command;
    return result;
  }
  if (request.decision.intent == AiIntentKind::AttackTarget) {
    if (!perceptionReadyForCommand(request.perception)) {
      result.status = NpcBehaviorCommandStatus::InvalidPerception;
      return result;
    }
    command.kind = CommandKind::Attack;
    command.payload.target.hasEntity = true;
    command.payload.target.entity = request.decision.target;
    command.payload.attackDamage = request.config.attackDamage;
    result.status = NpcBehaviorCommandStatus::Built;
    result.hasCommand = true;
    result.command = command;
    return result;
  }
  // ReturnToAnchor and Patrol are both point-moves toward decision.homePosition (the
  // anchor, resp. the current waypoint); they share this branch exactly.
  if (request.decision.intent == AiIntentKind::ReturnToAnchor ||
      request.decision.intent == AiIntentKind::Patrol) {
    if (!perceptionHasActorPosition(request.perception) ||
        !isFinite(request.decision.homePosition) ||
        !std::isfinite(request.decision.returnStopDistanceMeters) ||
        request.decision.returnStopDistanceMeters < 0.0F) {
      result.status = NpcBehaviorCommandStatus::InvalidDestination;
      return result;
    }
    Vec3 destination;
    if (!buildHorizontalMoveDestination(request.perception.actorPosition,
                                        request.decision.homePosition,
                                        request.config.chaseStepMeters,
                                        request.decision.returnStopDistanceMeters,
                                        destination)) {
      result.status = NpcBehaviorCommandStatus::NoCommand;
      return result;
    }

    command.kind = CommandKind::Move;
    command.payload.target.hasPoint = true;
    command.payload.target.point = destination;
    result.status = NpcBehaviorCommandStatus::Built;
    result.hasCommand = true;
    result.command = command;
    return result;
  }
  if (request.decision.intent == AiIntentKind::MoveTowardTarget) {
    if (!perceptionReadyForCommand(request.perception)) {
      result.status = NpcBehaviorCommandStatus::InvalidPerception;
      return result;
    }
    const float dx = request.perception.targetPosition.x -
                     request.perception.actorPosition.x;
    const float dz = request.perception.targetPosition.z -
                     request.perception.actorPosition.z;
    const float distance = std::sqrt(dx * dx + dz * dz);
    if (!std::isfinite(distance) || distance <= 0.0001F) {
      result.status = NpcBehaviorCommandStatus::InvalidDestination;
      return result;
    }
    const float moveDistance =
        std::min(request.config.chaseStepMeters,
                 std::max(0.0F, distance - request.config.chaseStopDistanceMeters));
    if (moveDistance <= 0.0F) {
      result.status = NpcBehaviorCommandStatus::NoCommand;
      return result;
    }
    Vec3 destination = request.perception.actorPosition;
    destination.x += (dx / distance) * moveDistance;
    destination.z += (dz / distance) * moveDistance;
    if (!isFinite(destination)) {
      result.status = NpcBehaviorCommandStatus::InvalidDestination;
      return result;
    }

    command.kind = CommandKind::Move;
    command.payload.target.hasPoint = true;
    command.payload.target.point = destination;
    result.status = NpcBehaviorCommandStatus::Built;
    result.hasCommand = true;
    result.command = command;
    return result;
  }

  result.status = NpcBehaviorCommandStatus::InvalidDecision;
  return result;
}

}  // namespace iggy3d
