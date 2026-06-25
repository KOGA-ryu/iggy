#include "runtime/ai/NpcBehaviorDebugSnapshot.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "runtime/combat/CombatState.hpp"
#include "runtime/world/EntityState.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {
namespace {

const CombatantState* findCombatant(const CombatState& combat, EntityId entity) {
  for (const CombatantState& combatant : combat.combatants) {
    if (combatant.entity == entity) {
      return &combatant;
    }
  }
  return nullptr;
}

const AiActorState* findAiActor(const AiState& ai, EntityId actor) {
  for (const AiActorState& candidate : ai.actors) {
    if (candidate.actor == actor) {
      return &candidate;
    }
  }
  return nullptr;
}

bool hasActorRow(const std::vector<NpcBehaviorDebugActorRow>& rows, EntityId actor) {
  for (const NpcBehaviorDebugActorRow& row : rows) {
    if (row.actor == actor) {
      return true;
    }
  }
  return false;
}

float distanceMeters(const EntityState& actor, const EntityState& target) {
  const float dx = target.transform.position.x - actor.transform.position.x;
  const float dy = target.transform.position.y - actor.transform.position.y;
  const float dz = target.transform.position.z - actor.transform.position.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void applyAiState(NpcBehaviorDebugActorRow& row, const AiActorState& state) {
  row.hasAiState = true;
  row.behaviorProfileId = state.behaviorProfileId;
  row.behavior = state.behavior;
  row.lastIntent = state.lastIntent;
  row.target = state.target;
  row.cooldownTicksRemaining = state.cooldownTicksRemaining;
  row.nextDecisionTick = state.nextDecisionTick;
}

void applyWorldState(NpcBehaviorDebugActorRow& row, const EntityState& entity) {
  row.actor = entity.id;
  row.stableName = entity.stableName.empty() ? "none" : entity.stableName;
  row.active = entity.active;
  row.isNpc = entity.kind == EntityKind::Npc;
}

void applyCombatState(NpcBehaviorDebugActorRow& row, const CombatState& combat) {
  if (const CombatantState* combatant = findCombatant(combat, row.actor)) {
    row.hasCombatant = true;
    row.combatantDefeated = combatant->defeated;
  }
}

void applyTargetState(NpcBehaviorDebugActorRow& row,
                      const WorldState& world,
                      const CombatState& combat,
                      const EntityState* actorEntity) {
  if (!isValid(row.target)) {
    return;
  }

  const EntityState* targetEntity = world.findById(row.target);
  if (targetEntity == nullptr) {
    return;
  }

  row.targetResolved = true;
  row.targetStableName =
      targetEntity->stableName.empty() ? "none" : targetEntity->stableName;
  row.targetActive = targetEntity->active;
  if (const CombatantState* targetCombatant = findCombatant(combat, row.target)) {
    row.targetCombatantDefeated = targetCombatant->defeated;
  }
  if (actorEntity != nullptr) {
    row.targetDistanceMeters = distanceMeters(*actorEntity, *targetEntity);
  }
}

void applyProfileState(NpcBehaviorDebugActorRow& row,
                       const NpcBehaviorProfileCatalog& catalog) {
  const NpcBehaviorProfileResolveResult profile =
      resolveNpcBehaviorProfile({&catalog, row.behaviorProfileId});
  row.profileResolved = profile.ok;
  row.profileStatus = std::string(profile.reasonCode);
  if (profile.ok) {
    row.engagementPolicy = profile.config.engagementPolicy;
  }
}

NpcBehaviorDebugActorRow buildRow(EntityId actor,
                                  const WorldState& world,
                                  const AiState& ai,
                                  const CombatState& combat,
                                  const NpcBehaviorProfileCatalog& catalog) {
  NpcBehaviorDebugActorRow row;
  row.actor = actor;

  const EntityState* entity = world.findById(actor);
  if (entity != nullptr) {
    applyWorldState(row, *entity);
  }
  if (const AiActorState* state = findAiActor(ai, actor)) {
    applyAiState(row, *state);
  }
  applyCombatState(row, combat);
  applyTargetState(row, world, combat, entity);
  applyProfileState(row, catalog);
  return row;
}

void accumulateCounts(NpcBehaviorDebugSnapshot& snapshot) {
  for (const NpcBehaviorDebugActorRow& row : snapshot.actors) {
    if (row.profileResolved) {
      ++snapshot.resolvedProfileCount;
      if (row.engagementPolicy == NpcEngagementPolicy::Hostile) {
        ++snapshot.hostileCount;
      } else if (row.engagementPolicy == NpcEngagementPolicy::Passive) {
        ++snapshot.passiveCount;
      }
    } else {
      ++snapshot.failedProfileCount;
    }

    if (row.behavior == AiBehaviorKind::Attacking) {
      ++snapshot.attackingCount;
    }
    if (row.lastIntent == AiIntentKind::Wait) {
      ++snapshot.waitingCount;
    }
  }
}

}  // namespace

std::string_view npcBehaviorDebugSnapshotStatusName(
    NpcBehaviorDebugSnapshotStatus status) {
  switch (status) {
    case NpcBehaviorDebugSnapshotStatus::Ok:
      return "npc_behavior_debug_ok";
    case NpcBehaviorDebugSnapshotStatus::Disabled:
      return "npc_behavior_debug_disabled";
    case NpcBehaviorDebugSnapshotStatus::MissingWorld:
      return "npc_behavior_debug_missing_world";
    case NpcBehaviorDebugSnapshotStatus::MissingAi:
      return "npc_behavior_debug_missing_ai";
    case NpcBehaviorDebugSnapshotStatus::MissingCombat:
      return "npc_behavior_debug_missing_combat";
    case NpcBehaviorDebugSnapshotStatus::MissingProfileCatalog:
      return "npc_behavior_debug_missing_profile_catalog";
  }
  return "npc_behavior_debug_disabled";
}

NpcBehaviorDebugSnapshot buildNpcBehaviorDebugSnapshot(
    const NpcBehaviorDebugSnapshotRequest& request) {
  NpcBehaviorDebugSnapshot snapshot;
  snapshot.sourceTick = request.sourceTick;

  if (!request.enabled) {
    snapshot.status = NpcBehaviorDebugSnapshotStatus::Disabled;
    snapshot.reasonCode = npcBehaviorDebugSnapshotStatusName(snapshot.status);
    return snapshot;
  }
  if (request.world == nullptr) {
    snapshot.status = NpcBehaviorDebugSnapshotStatus::MissingWorld;
    snapshot.reasonCode = npcBehaviorDebugSnapshotStatusName(snapshot.status);
    return snapshot;
  }
  if (request.ai == nullptr) {
    snapshot.status = NpcBehaviorDebugSnapshotStatus::MissingAi;
    snapshot.reasonCode = npcBehaviorDebugSnapshotStatusName(snapshot.status);
    return snapshot;
  }
  if (request.combat == nullptr) {
    snapshot.status = NpcBehaviorDebugSnapshotStatus::MissingCombat;
    snapshot.reasonCode = npcBehaviorDebugSnapshotStatusName(snapshot.status);
    return snapshot;
  }
  if (request.profileCatalog == nullptr || request.profileCatalog->profiles.empty()) {
    snapshot.status = NpcBehaviorDebugSnapshotStatus::MissingProfileCatalog;
    snapshot.reasonCode = npcBehaviorDebugSnapshotStatusName(snapshot.status);
    return snapshot;
  }

  snapshot.status = NpcBehaviorDebugSnapshotStatus::Ok;
  snapshot.reasonCode = npcBehaviorDebugSnapshotStatusName(snapshot.status);
  snapshot.aiActorCount = request.ai->actors.size();

  const std::size_t maxActors = request.maxActors;
  for (const EntityState& entity : request.world->entities()) {
    if (entity.kind != EntityKind::Npc) {
      continue;
    }
    ++snapshot.npcWorldCount;
    if (snapshot.actors.size() < maxActors) {
      snapshot.actors.push_back(buildRow(entity.id,
                                         *request.world,
                                         *request.ai,
                                         *request.combat,
                                         *request.profileCatalog));
    }
  }

  for (const AiActorState& state : request.ai->actors) {
    if (hasActorRow(snapshot.actors, state.actor)) {
      continue;
    }
    if (snapshot.actors.size() >= maxActors) {
      break;
    }
    snapshot.actors.push_back(buildRow(state.actor,
                                       *request.world,
                                       *request.ai,
                                       *request.combat,
                                       *request.profileCatalog));
  }

  std::sort(snapshot.actors.begin(),
            snapshot.actors.end(),
            [](const NpcBehaviorDebugActorRow& lhs,
               const NpcBehaviorDebugActorRow& rhs) {
              return lhs.actor < rhs.actor;
            });
  accumulateCounts(snapshot);
  return snapshot;
}

}  // namespace iggy3d
