#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "runtime/ai/NpcBehaviorProfile.hpp"

namespace iggy3d::creative {
namespace {

constexpr std::uint32_t kPlayerFaction = 1U;
constexpr std::uint32_t kMonsterFaction = 2U;
constexpr Aabb3 kPlayerBounds{{-0.35F, 0.0F, -0.35F},
                              {0.35F, 1.8F, 0.35F}};
constexpr Aabb3 kNpcBounds{{-0.30F, 0.0F, -0.30F},
                           {0.30F, 1.8F, 0.30F}};

bool samePosition(Vec3 lhs, Vec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameAnchor(const RoomAnchorAsset& lhs,
                const RoomAnchorAsset& rhs) noexcept {
  return lhs.id == rhs.id && lhs.kind == rhs.kind &&
         lhs.runtimeStableName == rhs.runtimeStableName &&
         samePosition(lhs.positionMeters, rhs.positionMeters);
}

bool anchorIsValid(const RoomAnchorAsset& anchor) noexcept {
  return !anchor.id.empty() && !anchor.kind.empty() &&
         !anchor.runtimeStableName.empty() && isFinite(anchor.positionMeters);
}

bool anchorsHaveUniqueIdentity(
    const std::vector<RoomAnchorAsset>& anchors) noexcept {
  for (std::size_t index = 0; index < anchors.size(); ++index) {
    for (std::size_t other = index + 1U; other < anchors.size(); ++other) {
      if (anchors[index].id == anchors[other].id ||
          anchors[index].runtimeStableName ==
              anchors[other].runtimeStableName) {
        return false;
      }
    }
  }
  return true;
}

const CreativeNpcPatrolRoutePlan* findPatrolRoute(
    std::span<const CreativeNpcPatrolRoutePlan> routes,
    CreativeObjectId objectId) noexcept {
  const auto found = std::lower_bound(
      routes.begin(), routes.end(), objectId,
      [](const CreativeNpcPatrolRoutePlan& route,
         CreativeObjectId candidateId) {
        return route.objectId < candidateId;
      });
  return found != routes.end() && found->objectId == objectId ? &*found
                                                              : nullptr;
}

bool npcPayloadShapeIsValid(
    const CreativePlayActivationPayload& payload) noexcept {
  CreativeObjectId previousRouteId = kInvalidObjectId;
  for (const CreativeNpcPatrolRoutePlan& route : payload.npcPatrolRoutes) {
    if (!isValidCreativeNpcPatrolRoutePlan(route) ||
        (previousRouteId != kInvalidObjectId &&
         route.objectId <= previousRouteId)) {
      return false;
    }
    previousRouteId = route.objectId;
  }

  CreativeObjectId previousActorId = kInvalidObjectId;
  for (const CreativeNpcSpawnPlan& actor : payload.npcSpawns) {
    if (!isValidCreativeNpcSpawnPlan(actor) ||
        (previousActorId != kInvalidObjectId &&
         actor.objectId <= previousActorId) ||
        (actor.hasPatrol() &&
         findPatrolRoute(payload.npcPatrolRoutes,
                         actor.patrolRouteObjectId) == nullptr)) {
      return false;
    }
    previousActorId = actor.objectId;
    const std::size_t matchingAnchors =
        static_cast<std::size_t>(std::count_if(
            payload.room.anchors.begin(), payload.room.anchors.end(),
            [&actor](const RoomAnchorAsset& anchor) {
              return sameAnchor(anchor, actor.anchor);
            }));
    if (matchingAnchors != 1U) {
      return false;
    }
  }

  for (const CreativeNpcPatrolRoutePlan& route : payload.npcPatrolRoutes) {
    if (std::none_of(
            payload.npcSpawns.begin(), payload.npcSpawns.end(),
            [&route](const CreativeNpcSpawnPlan& actor) {
              return actor.patrolRouteObjectId == route.objectId;
            })) {
      return false;
    }
  }
  const std::size_t roomActorAnchorCount =
      static_cast<std::size_t>(std::count_if(
          payload.room.anchors.begin(), payload.room.anchors.end(),
          [](const RoomAnchorAsset& anchor) {
            return anchor.kind == "npc" || anchor.kind == "monster";
          }));
  return roomActorAnchorCount == payload.npcSpawns.size();
}

bool surfacesHaveUniqueIdentity(
    const std::vector<RoomSpatialSurface>& surfaces) noexcept {
  for (std::size_t index = 0; index < surfaces.size(); ++index) {
    if (surfaces[index].id.empty()) {
      return false;
    }
    for (std::size_t other = index + 1U; other < surfaces.size(); ++other) {
      if (surfaces[index].id == surfaces[other].id) {
        return false;
      }
    }
  }
  return true;
}

bool interactableDefinitionIsValid(
    const CreativeRuntimeInteractableDefinition& definition) noexcept {
  if (definition.objectId == kInvalidObjectId ||
      definition.stableName.empty() || definition.displayName.empty() ||
      !isFinite(definition.transform) || !isValid(definition.localBounds)) {
    return false;
  }
  switch (definition.kind) {
    case CreativeRuntimeInteractableKind::Door:
    case CreativeRuntimeInteractableKind::Platform:
    case CreativeRuntimeInteractableKind::MovingPlatform:
      return definition.logicSourceMode ==
                 CreativeRuntimeLogicSourceMode::None &&
             !definition.roomMeshId.empty() && definition.itemId.empty();
    case CreativeRuntimeInteractableKind::Control:
      if (!definition.itemId.empty()) {
        return false;
      }
      switch (definition.logicSourceMode) {
        case CreativeRuntimeLogicSourceMode::Manual:
        case CreativeRuntimeLogicSourceMode::PulseOnEnter:
          return definition.roomMeshId.empty();
        case CreativeRuntimeLogicSourceMode::HoldWhileOccupied:
          return !definition.roomMeshId.empty();
        case CreativeRuntimeLogicSourceMode::None:
        case CreativeRuntimeLogicSourceMode::Count:
          return false;
      }
      return false;
    case CreativeRuntimeInteractableKind::Pickup:
      return definition.logicSourceMode ==
                 CreativeRuntimeLogicSourceMode::None &&
             definition.roomMeshId.empty() && !definition.itemId.empty() &&
             definition.itemCount > 0U && definition.objectiveId.empty() &&
             definition.requiredItemId.empty() &&
             definition.requiredItemCount == 0U;
    case CreativeRuntimeInteractableKind::Objective:
      return definition.logicSourceMode ==
                 CreativeRuntimeLogicSourceMode::None &&
             definition.roomMeshId.empty() && definition.itemId.empty() &&
             definition.itemCount == 0U && !definition.objectiveId.empty() &&
             ((definition.requiredItemId.empty() &&
               definition.requiredItemCount == 0U) ||
              (!definition.requiredItemId.empty() &&
               definition.requiredItemCount > 0U));
  }
  return false;
}

bool interactablesHaveUniqueIdentity(
    const std::vector<CreativeRuntimeInteractableDefinition>& definitions)
    noexcept {
  for (std::size_t index = 0U; index < definitions.size(); ++index) {
    if (!interactableDefinitionIsValid(definitions[index])) {
      return false;
    }
    for (std::size_t other = index + 1U; other < definitions.size(); ++other) {
      if (definitions[index].objectId == definitions[other].objectId ||
          definitions[index].stableName == definitions[other].stableName) {
        return false;
      }
    }
  }
  return true;
}

bool logicLinksAreValid(
    const std::vector<CreativeRuntimeLogicLink>& links,
    const std::vector<CreativeRuntimeInteractableDefinition>& definitions)
    noexcept {
  for (std::size_t index = 0U; index < links.size(); ++index) {
    const CreativeRuntimeLogicLink& link = links[index];
    const auto source = std::find_if(
        definitions.begin(), definitions.end(),
        [&link](const CreativeRuntimeInteractableDefinition& definition) {
          return definition.objectId == link.sourceObjectId;
        });
    const auto target = std::find_if(
        definitions.begin(), definitions.end(),
        [&link](const CreativeRuntimeInteractableDefinition& definition) {
          return definition.objectId == link.targetObjectId;
        });
    if (source == definitions.end() || target == definitions.end() ||
        source->kind != CreativeRuntimeInteractableKind::Control ||
        !creativeRuntimeInteractableIsLogicTarget(target->kind) ||
        !creativeRuntimeLogicActionSupported(target->kind, link.action) ||
        (link.compatibilityFallback &&
         (link.action != CreativeLogicLinkAction::Toggle ||
          target->kind != CreativeRuntimeInteractableKind::Door ||
          source->circuitId == kInvalidObjectId ||
          target->circuitId != source->circuitId))) {
      return false;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      if (links[prior].sourceObjectId == link.sourceObjectId &&
          links[prior].targetObjectId == link.targetObjectId) {
        return false;
      }
      if (links[prior].sourceObjectId == link.sourceObjectId &&
          links[prior].compatibilityFallback != link.compatibilityFallback) {
        return false;
      }
    }
  }
  return true;
}

bool payloadShapeIsValid(
    const CreativePlayActivationPayload& payload) noexcept {
  if (payload.documentId == kInvalidDocumentId || payload.roomId.empty() ||
      payload.room.id != payload.roomId ||
      payload.playerSpawnObjectId == kInvalidObjectId ||
      payload.playerSpawn.kind != "spawn" ||
      !anchorIsValid(payload.playerSpawn) || payload.room.anchors.empty() ||
      !isValidCreativePlayerSpawnSettings(payload.playerSpawnSettings) ||
      !isSupportedCreativePlayerProfileId(
          payload.playerSpawnSettings.playerProfileId) ||
      !std::isfinite(payload.playerSpawnYawRadians) ||
      !isFinite(payload.playerSpawnCameraPositionMeters)) {
    return false;
  }

  std::size_t spawnCount = 0U;
  bool matchedPreparedSpawn = false;
  for (const RoomAnchorAsset& anchor : payload.room.anchors) {
    if (!anchorIsValid(anchor)) {
      return false;
    }
    if (anchor.kind == "spawn") {
      ++spawnCount;
      matchedPreparedSpawn = matchedPreparedSpawn ||
                             sameAnchor(anchor, payload.playerSpawn);
    }
  }
  return spawnCount == 1U && matchedPreparedSpawn &&
         anchorsHaveUniqueIdentity(payload.room.anchors) &&
         npcPayloadShapeIsValid(payload) &&
         interactablesHaveUniqueIdentity(payload.interactables) &&
         logicLinksAreValid(payload.logicLinks, payload.interactables);
}

bool sandboxConfigIsValid(const CreativeRuntimeSandboxConfig& config) {
  if (validateRuntimeConfig(config.runtimeConfig) != RuntimeConfigStatus::Ok ||
      config.packageId.empty() || config.playerHitPoints <= 0 ||
      config.npcHitPoints <= 0 || config.monsterHitPoints <= 0) {
    return false;
  }

  const NpcBehaviorProfileCatalog catalog =
      makeBuiltInNpcBehaviorProfileCatalog();
  return resolveNpcBehaviorProfile(
             {&catalog, config.npcBehaviorProfileId})
             .ok &&
         resolveNpcBehaviorProfile(
             {&catalog, config.monsterBehaviorProfileId})
             .ok;
}

ScenarioEntitySeed makeCombatantEntity(const RoomAnchorAsset& anchor,
                                       ScenarioEntityKind kind,
                                       Aabb3 bounds,
                                       std::uint32_t factionId,
                                       std::int32_t hitPoints) {
  ScenarioEntitySeed entity;
  entity.stableName = anchor.runtimeStableName;
  entity.kind = kind;
  entity.transform = identityTransform3();
  entity.transform.position = anchor.positionMeters;
  entity.localBounds = bounds;
  entity.active = true;
  entity.persistent = true;
  entity.targeting.targetable = true;
  entity.targeting.actions = {ScenarioTargetAction::Attack,
                              ScenarioTargetAction::Interact,
                              ScenarioTargetAction::Inspect};
  entity.interaction.kind = ScenarioInteractionKind::Inspect;
  entity.interaction.repeatable = true;
  entity.combatantEnabled = true;
  entity.combatant.factionId = factionId;
  entity.combatant.hitPoints = hitPoints;
  entity.combatant.maxHitPoints = hitPoints;
  return entity;
}

std::uint32_t resolvedNpcFaction(CreativeNpcTeam team,
                                 bool monster) noexcept {
  switch (team) {
    case CreativeNpcTeam::ActorDefault:
      return monster ? kMonsterFaction : kPlayerFaction;
    case CreativeNpcTeam::PlayerAllied:
      return kPlayerFaction;
    case CreativeNpcTeam::Hostile:
      return kMonsterFaction;
    case CreativeNpcTeam::Count:
      break;
  }
  return 0U;
}

Vec3 runtimePatrolPoint(const CreativePathPoint& point) noexcept {
  return {
      static_cast<float>(point.position.x),
      static_cast<float>(point.position.y),
      static_cast<float>(point.position.z),
  };
}

float scenarioFacingDegrees(Vec3 facingDirection) noexcept {
  constexpr float kRadiansToDegrees = 57.2957795131F;
  return std::atan2(facingDirection.x, facingDirection.z) *
         kRadiansToDegrees;
}

std::vector<Vec3> collectPatrolWaypoints(
    std::span<const CreativeNpcPatrolRoutePlan> routes) {
  std::size_t waypointCount = 0U;
  for (const CreativeNpcPatrolRoutePlan& route : routes) {
    waypointCount += route.path.size();
  }
  std::vector<Vec3> waypoints;
  waypoints.reserve(waypointCount);
  for (const CreativeNpcPatrolRoutePlan& route : routes) {
    for (const CreativePathPoint& point : route.path) {
      waypoints.push_back(runtimePatrolPoint(point));
    }
  }
  return waypoints;
}

ScenarioEntitySeed makeInteractableEntity(
    const CreativeRuntimeInteractableDefinition& definition) {
  ScenarioEntitySeed entity;
  entity.stableName = definition.stableName;
  entity.transform = definition.transform;
  entity.localBounds = definition.localBounds;
  entity.active = true;
  entity.persistent = true;
  switch (definition.kind) {
    case CreativeRuntimeInteractableKind::Door:
      entity.kind = ScenarioEntityKind::Door;
      entity.targeting.targetable = true;
      entity.targeting.actions = {ScenarioTargetAction::Interact,
                                  ScenarioTargetAction::Inspect};
      entity.interaction.kind = ScenarioInteractionKind::Activate;
      entity.interaction.primaryEffect =
          ScenarioInteractionEffectKind::EmitEventOnly;
      entity.interaction.repeatable = true;
      break;
    case CreativeRuntimeInteractableKind::Platform:
    case CreativeRuntimeInteractableKind::MovingPlatform:
      entity.kind = ScenarioEntityKind::Marker;
      entity.active = false;
      break;
    case CreativeRuntimeInteractableKind::Control:
      entity.kind = ScenarioEntityKind::Marker;
      if (definition.logicSourceMode ==
          CreativeRuntimeLogicSourceMode::Manual) {
        entity.targeting.targetable = true;
        entity.targeting.actions = {ScenarioTargetAction::Interact,
                                    ScenarioTargetAction::Inspect};
        entity.interaction.kind = ScenarioInteractionKind::Activate;
        entity.interaction.primaryEffect =
            ScenarioInteractionEffectKind::EmitEventOnly;
        entity.interaction.repeatable = true;
      }
      break;
    case CreativeRuntimeInteractableKind::Pickup:
      entity.kind = ScenarioEntityKind::Pickup;
      entity.targeting.targetable = true;
      entity.targeting.actions = {ScenarioTargetAction::Interact,
                                  ScenarioTargetAction::Inspect};
      entity.interaction.kind = ScenarioInteractionKind::Pickup;
      entity.interaction.primaryEffect =
          ScenarioInteractionEffectKind::AddItemToInventory;
      entity.interaction.itemId = definition.itemId;
      entity.interaction.itemCount = definition.itemCount;
      entity.interaction.deactivateTargetOnSuccess =
          definition.deactivateOnSuccess;
      break;
    case CreativeRuntimeInteractableKind::Objective:
      entity.kind = ScenarioEntityKind::Marker;
      entity.targeting.targetable = true;
      entity.targeting.actions = {ScenarioTargetAction::Interact,
                                  ScenarioTargetAction::Inspect};
      entity.interaction.kind = ScenarioInteractionKind::ObjectiveTrigger;
      entity.interaction.primaryEffect =
          ScenarioInteractionEffectKind::CompleteObjective;
      entity.interaction.objectiveId = definition.objectiveId;
      entity.interaction.requiredItemId = definition.requiredItemId;
      entity.interaction.requiredItemCount = definition.requiredItemCount;
      entity.interaction.deactivateTargetOnSuccess =
          definition.deactivateOnSuccess;
      break;
  }
  return entity;
}

bool anchorBacksInteractable(
    const RoomAnchorAsset& anchor,
    const std::vector<CreativeRuntimeInteractableDefinition>& definitions)
    noexcept {
  return std::any_of(
      definitions.begin(), definitions.end(),
      [&anchor](const CreativeRuntimeInteractableDefinition& definition) {
        return definition.stableName == anchor.runtimeStableName;
      });
}

ScenarioObjectiveSeed makeSandboxObjective() {
  ScenarioObjectiveSeed objective;
  objective.id = "creative_sandbox_active";
  objective.initialStatus = ObjectiveStatusSeed::Active;
  objective.condition = "None";
  objective.playerSlot = 0U;
  objective.completeStatus = ObjectiveStatusSeed::Complete;
  return objective;
}

ScenarioObjectiveSeed makeInteractableObjective(
    const CreativeRuntimeInteractableDefinition& definition) {
  ScenarioObjectiveSeed objective;
  objective.id = definition.objectiveId;
  objective.initialStatus = ObjectiveStatusSeed::Active;
  objective.condition = "None";
  objective.playerSlot = 0U;
  objective.completeStatus = ObjectiveStatusSeed::Complete;
  return objective;
}

void setActivationStatus(CreativeRuntimeSandboxActivationReceipt& receipt,
                         CreativeRuntimeSandboxActivationStatus status,
                         std::string reasonCode,
                         bool accepted = false) {
  receipt.status = status;
  receipt.reasonCode = std::move(reasonCode);
  receipt.accepted = accepted;
}

}  // namespace

std::string_view toString(CreativeRuntimeScenarioSeedStatus status) noexcept {
  switch (status) {
    case CreativeRuntimeScenarioSeedStatus::NotRequested:
      return "not_requested";
    case CreativeRuntimeScenarioSeedStatus::InvalidConfig:
      return "invalid_config";
    case CreativeRuntimeScenarioSeedStatus::InvalidPayload:
      return "invalid_payload";
    case CreativeRuntimeScenarioSeedStatus::Built:
      return "built";
  }
  return "not_requested";
}

CreativeRuntimeScenarioSeedResult buildCreativeRuntimeScenarioSeed(
    const CreativePlayActivationPayload& payload,
    const CreativeRuntimeSandboxConfig& config) {
  CreativeRuntimeScenarioSeedResult result;
  result.summary.sourceAnchorCount = payload.room.anchors.size();
  result.summary.sourceInteractableCount = payload.interactables.size();
  if (!sandboxConfigIsValid(config)) {
    result.status = CreativeRuntimeScenarioSeedStatus::InvalidConfig;
    result.reasonCode = "creative_runtime_seed_config_invalid";
    return result;
  }
  if (!payloadShapeIsValid(payload)) {
    result.status = CreativeRuntimeScenarioSeedStatus::InvalidPayload;
    result.reasonCode = "creative_runtime_seed_payload_invalid";
    return result;
  }

  FixtureScenarioSeed seed;
  seed.scenarioId = payload.roomId + ".creative_play";
  seed.config = config.runtimeConfig;
  seed.initialClockMode = ScenarioClockMode::Normal;
  seed.defaultRealtimeCamera = ScenarioCameraMode::FirstPerson;
  seed.defaultTacticalCamera = ScenarioCameraMode::TacticalOverhead;
  seed.players.push_back(
      {0U, ScenarioPlayerSlotKind::Local,
       payload.playerSpawn.runtimeStableName});
  seed.entities.push_back(makeCombatantEntity(
      payload.playerSpawn, ScenarioEntityKind::Player, kPlayerBounds,
      kPlayerFaction, config.playerHitPoints));
  seed.entities.back().transform.rotationEulerRadians.y =
      payload.playerSpawnYawRadians;
  seed.objectives.push_back(makeSandboxObjective());
  result.summary.playerEntityCount = 1U;

  // Raw anchors remain useful to room and interactable owners, but actor
  // meaning comes only from the normalized plans below. Legacy patrol_post
  // markers are inert rather than being assigned by incidental bake order.
  for (const RoomAnchorAsset& anchor : payload.room.anchors) {
    if (anchor.kind == "spawn" || anchor.kind == "npc" ||
        anchor.kind == "monster") {
      continue;
    }
    if (!anchorBacksInteractable(anchor, payload.interactables)) {
      ++result.summary.ignoredAnchorCount;
    }
  }

  result.summary.patrolRouteCount = payload.npcPatrolRoutes.size();
  for (const CreativeNpcPatrolRoutePlan& route : payload.npcPatrolRoutes) {
    result.summary.patrolWaypointCount += route.path.size();
  }
  for (const CreativeNpcSpawnPlan& actorPlan : payload.npcSpawns) {
    const bool monster =
        actorPlan.objectKind == CreativeObjectKind::EnemySpawn;
    if (actorPlan.settings.spawnPolicy ==
        CreativeNpcSpawnPolicy::Disabled) {
      ++result.summary.disabledNpcSpawnCount;
      continue;
    }
    const std::string& profileId =
        actorPlan.settings.behaviorProfileId.empty()
            ? (monster ? config.monsterBehaviorProfileId
                       : config.npcBehaviorProfileId)
            : actorPlan.settings.behaviorProfileId;
    const std::uint32_t factionId =
        resolvedNpcFaction(actorPlan.settings.team, monster);
    const std::int32_t hitPoints =
        actorPlan.settings.hitPoints == 0U
            ? (monster ? config.monsterHitPoints : config.npcHitPoints)
            : static_cast<std::int32_t>(actorPlan.settings.hitPoints);
    seed.entities.push_back(makeCombatantEntity(
        actorPlan.anchor, ScenarioEntityKind::Npc, kNpcBounds, factionId,
        hitPoints));
    seed.entities.back().transform.rotationEulerRadians.y =
        actorPlan.yawRadians;

    ScenarioAiActorSeed actor;
    actor.actorStableName = actorPlan.anchor.runtimeStableName;
    actor.behaviorProfileId = profileId;
    actor.hasFacing = true;
    actor.facingDegrees = scenarioFacingDegrees(actorPlan.facingDirection);
    actor.initialAlertLevel =
        static_cast<float>(actorPlan.settings.initialAlertLevel);
    if (actorPlan.hasPatrol()) {
      const CreativeNpcPatrolRoutePlan* route =
          findPatrolRoute(payload.npcPatrolRoutes,
                          actorPlan.patrolRouteObjectId);
      actor.patrolWaypoints.reserve(route->path.size());
      for (const CreativePathPoint& point : route->path) {
        actor.patrolWaypoints.push_back(runtimePatrolPoint(point));
      }
      actor.patrolMode = ScenarioPatrolMode::Loop;
    }
    seed.aiActors.push_back(std::move(actor));
    if (monster) {
      ++result.summary.monsterEntityCount;
    } else {
      ++result.summary.npcEntityCount;
    }
  }

  for (const CreativeRuntimeInteractableDefinition& definition :
       payload.interactables) {
    if (definition.kind == CreativeRuntimeInteractableKind::Objective) {
      seed.objectives.push_back(makeInteractableObjective(definition));
    }
    seed.entities.push_back(makeInteractableEntity(definition));
    switch (definition.kind) {
      case CreativeRuntimeInteractableKind::Door:
        ++result.summary.doorEntityCount;
        break;
      case CreativeRuntimeInteractableKind::Platform:
      case CreativeRuntimeInteractableKind::MovingPlatform:
        ++result.summary.platformEntityCount;
        break;
      case CreativeRuntimeInteractableKind::Control:
        ++result.summary.controlEntityCount;
        result.summary.automaticControlEntityCount +=
            definition.logicSourceMode ==
                    CreativeRuntimeLogicSourceMode::Manual
                ? 0U
                : 1U;
        break;
      case CreativeRuntimeInteractableKind::Pickup:
        ++result.summary.pickupEntityCount;
        break;
      case CreativeRuntimeInteractableKind::Objective:
        ++result.summary.objectiveEntityCount;
        break;
    }
  }

  result.accepted = true;
  result.status = CreativeRuntimeScenarioSeedStatus::Built;
  result.reasonCode = "creative_runtime_seed_built";
  result.seed = std::move(seed);
  return result;
}

std::string_view toString(
    CreativeRuntimeSandboxActivationStatus status) noexcept {
  switch (status) {
    case CreativeRuntimeSandboxActivationStatus::NotRequested:
      return "not_requested";
    case CreativeRuntimeSandboxActivationStatus::MissingDocument:
      return "missing_document";
    case CreativeRuntimeSandboxActivationStatus::StalePayload:
      return "stale_payload";
    case CreativeRuntimeSandboxActivationStatus::InvalidConfig:
      return "invalid_config";
    case CreativeRuntimeSandboxActivationStatus::InvalidPayload:
      return "invalid_payload";
    case CreativeRuntimeSandboxActivationStatus::InvalidInteractables:
      return "invalid_interactables";
    case CreativeRuntimeSandboxActivationStatus::InvalidCollisionSurfaces:
      return "invalid_collision_surfaces";
    case CreativeRuntimeSandboxActivationStatus::SessionCreationFailed:
      return "session_creation_failed";
    case CreativeRuntimeSandboxActivationStatus::Activated:
      return "activated";
  }
  return "not_requested";
}

CreativeRuntimeSandboxActivationResult activateCreativeRuntimeSandbox(
    CreativeRuntimeSandboxActivationRequest request) {
  CreativeRuntimeSandboxActivationResult result;
  CreativeRuntimeSandboxActivationReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.documentId = request.payload.documentId;
  receipt.documentRevision = request.payload.documentRevision;
  receipt.roomId = request.payload.roomId;
  receipt.playerProfileId = request.payload.playerSpawnSettings.playerProfileId;

  if (request.sourceDocument == nullptr) {
    setActivationStatus(
        receipt, CreativeRuntimeSandboxActivationStatus::MissingDocument,
        "creative_runtime_sandbox_document_missing");
    return result;
  }
  if (!creativePlayActivationIsCurrent(request.payload,
                                       *request.sourceDocument)) {
    setActivationStatus(receipt,
                        CreativeRuntimeSandboxActivationStatus::StalePayload,
                        "creative_runtime_sandbox_payload_stale");
    return result;
  }

  CreativeRuntimeScenarioSeedResult seed =
      buildCreativeRuntimeScenarioSeed(request.payload, request.config);
  receipt.scenario = seed.summary;
  if (!seed.accepted) {
    const CreativeRuntimeSandboxActivationStatus status =
        seed.status == CreativeRuntimeScenarioSeedStatus::InvalidConfig
            ? CreativeRuntimeSandboxActivationStatus::InvalidConfig
            : CreativeRuntimeSandboxActivationStatus::InvalidPayload;
    setActivationStatus(receipt, status, std::string(seed.reasonCode));
    return result;
  }

  SpatialSurfaceSet collisionSurfaces =
      buildSpatialSurfaceSet(request.payload.room);
  receipt.collisionSurfaceCount = collisionSurfaces.size();
  if (request.payload.room.spatialSurfaces.empty() ||
      collisionSurfaces.size() !=
          request.payload.room.spatialSurfaces.size() ||
      !surfacesHaveUniqueIdentity(request.payload.room.spatialSurfaces)) {
    setActivationStatus(
        receipt,
        CreativeRuntimeSandboxActivationStatus::InvalidCollisionSurfaces,
        "creative_runtime_sandbox_collision_surfaces_invalid");
    return result;
  }

  const std::vector<Vec3> patrolWaypoints =
      collectPatrolWaypoints(request.payload.npcPatrolRoutes);
  ReasoningGraph reasoningGraph =
      buildReasoningGraph(request.payload.room, patrolWaypoints);
  receipt.reasoningGraph = summarizeReasoningGraph(reasoningGraph);

  CreativeRuntimeInteractableStateBuildResult interactableStates =
      buildCreativeRuntimeInteractableStates(
          request.payload.room, std::move(request.payload.interactables));
  if (!interactableStates.ok) {
    setActivationStatus(
        receipt,
        CreativeRuntimeSandboxActivationStatus::InvalidInteractables,
        std::string(interactableStates.reasonCode));
    return result;
  }

  SessionCreateRequest sessionRequest;
  sessionRequest.packageId = request.config.packageId;
  sessionRequest.config = request.config.runtimeConfig;
  sessionRequest.seed = std::move(seed.seed);
  Result<Session> created = Session::create(sessionRequest);
  if (created.status != ResultStatus::Ok) {
    setActivationStatus(
        receipt,
        CreativeRuntimeSandboxActivationStatus::SessionCreationFailed,
        created.error.code);
    return result;
  }
  created.value.setReasoningGraph(std::move(reasoningGraph));
  for (CreativeRuntimeInteractableState& state : interactableStates.states) {
    const EntityState* entity = created.value.state().world.findByStableName(
        state.definition.stableName);
    if (entity == nullptr) {
      setActivationStatus(
          receipt,
          CreativeRuntimeSandboxActivationStatus::InvalidInteractables,
          "creative_runtime_interactable_entity_missing");
      return result;
    }
    state.entity = entity->id;
  }
  CreativeRuntimeSandbox sandbox;
  sandbox.sourceDocumentId = request.payload.documentId;
  sandbox.sourceDocumentRevision = request.payload.documentRevision;
  sandbox.roomId = request.payload.roomId;
  sandbox.playerSpawnObjectId = request.payload.playerSpawnObjectId;
  sandbox.playerProfileId = request.payload.playerSpawnSettings.playerProfileId;
  sandbox.playerSpawnYawRadians = request.payload.playerSpawnYawRadians;
  sandbox.scenario = receipt.scenario;
  sandbox.reasoningGraph = receipt.reasoningGraph;
  sandbox.room = std::move(request.payload.room);
  sandbox.roomStaticMeshOrder.reserve(sandbox.room.staticMeshes.size());
  for (const RoomStaticMeshAsset& mesh : sandbox.room.staticMeshes) {
    sandbox.roomStaticMeshOrder.push_back(mesh.id);
  }
  sandbox.roomSpatialSurfaceOrder.reserve(
      sandbox.room.spatialSurfaces.size());
  for (const RoomSpatialSurface& surface : sandbox.room.spatialSurfaces) {
    sandbox.roomSpatialSurfaceOrder.push_back(surface.id);
  }
  sandbox.collisionSurfaces = std::move(collisionSurfaces);
  sandbox.interactables = std::move(interactableStates.states);
  sandbox.logicLinks = std::move(request.payload.logicLinks);
  sandbox.session = std::move(created.value);
  const CreativeRuntimeDoorUpdateReceipt initializedDoors =
      initializeCreativeRuntimeDoors(sandbox);
  if (!initializedDoors.accepted) {
    setActivationStatus(
        receipt,
        CreativeRuntimeSandboxActivationStatus::InvalidInteractables,
        std::string(initializedDoors.reasonCode));
    return result;
  }
  const CreativeRuntimeAutomaticLogicReceipt initializedHoldLogic =
      initializeCreativeRuntimeHoldLogic(sandbox);
  if (!initializedHoldLogic.accepted) {
    setActivationStatus(
        receipt,
        CreativeRuntimeSandboxActivationStatus::InvalidInteractables,
        std::string(initializedHoldLogic.reasonCode));
    return result;
  }
  receipt.collisionSurfaceCount = sandbox.collisionSurfaces.size();
  receipt.reasoningGraph = sandbox.reasoningGraph;
  receipt.initialStateHash = sandbox.session.stateHash();
  result.sandbox.emplace(std::move(sandbox));
  setActivationStatus(receipt,
                      CreativeRuntimeSandboxActivationStatus::Activated,
                      "creative_runtime_sandbox_activated", true);
  return result;
}

bool creativeRuntimeSandboxIsCurrent(
    const CreativeRuntimeSandbox& sandbox,
    const CreativeDocument& document) noexcept {
  return document.isValid() &&
         sandbox.sourceDocumentId != kInvalidDocumentId &&
         sandbox.sourceDocumentId == document.id() &&
         sandbox.sourceDocumentRevision == document.revision();
}

std::string_view toString(CreativeRuntimeSandboxStopStatus status) noexcept {
  switch (status) {
    case CreativeRuntimeSandboxStopStatus::NotRequested:
      return "not_requested";
    case CreativeRuntimeSandboxStopStatus::NoActiveSandbox:
      return "no_active_sandbox";
    case CreativeRuntimeSandboxStopStatus::Stopped:
      return "stopped";
  }
  return "not_requested";
}

CreativeRuntimeSandboxStopReceipt stopCreativeRuntimeSandbox(
    std::optional<CreativeRuntimeSandbox>& sandbox) noexcept {
  CreativeRuntimeSandboxStopReceipt receipt;
  receipt.requested = true;
  if (!sandbox.has_value()) {
    receipt.status = CreativeRuntimeSandboxStopStatus::NoActiveSandbox;
    receipt.reasonCode = "creative_runtime_sandbox_not_active";
    return receipt;
  }

  receipt.documentId = sandbox->sourceDocumentId;
  receipt.documentRevision = sandbox->sourceDocumentRevision;
  receipt.finalStateHash = sandbox->session.stateHash();
  sandbox.reset();
  receipt.stopped = true;
  receipt.status = CreativeRuntimeSandboxStopStatus::Stopped;
  receipt.reasonCode = "creative_runtime_sandbox_stopped";
  return receipt;
}

}  // namespace iggy3d::creative
